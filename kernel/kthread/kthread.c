#include "kthread/kthread.h"

#include "cpu/irq_guard.h"
#include "log/kprint.h"
#include "mem/kmem.h"
#include "timer/tick.h"
#include "util/min_heap.h"

extern void thread_switch(unsigned int *save_sp, unsigned int new_sp);

enum kthread_state
{
    KTHREAD_RUNNABLE,
    KTHREAD_RUNNING,
    KTHREAD_BLOCKED,
    KTHREAD_DEAD,
};

enum kthread_block_kind
{
    KTHREAD_BLOCK_KIND_NONE,
    KTHREAD_BLOCK_KIND_INTERRUPTIBLE,
    KTHREAD_BLOCK_KIND_UNINTERRUPTIBLE,
};

enum kthread_block_reason
{
    KTHREAD_BLOCK_REASON_NONE,
    KTHREAD_BLOCK_REASON_SLEEP,
};

struct kthread
{
    enum kthread_state state;
    enum kthread_block_kind block_kind;
    enum kthread_block_reason block_reason;
    tick_t wake_at_ticks;
    int sleep_heap_idx;
    void *stack_base;
    unsigned long stack_size;
    unsigned int saved_sp;
    void (*fn)(void *arg);
    void *arg;
};

static struct kthread kthread_table[KTHREAD_MAX];
static int kthread_current_id;
static unsigned int scheduler_saved_sp;
static int kthread_reap_after_switch;
static unsigned int kthread_discard_sp;

static min_heap_t kthread_sleep_heap;

static void kthread_trampoline(void);

static void kthread_block_reset(struct kthread *t)
{
    t->block_kind = KTHREAD_BLOCK_KIND_NONE;
    t->block_reason = KTHREAD_BLOCK_REASON_NONE;
    t->wake_at_ticks = 0;
    t->sleep_heap_idx = MIN_HEAP_IDX_NONE;
}

static void kthread_sleep_heap_set_index(void *item, int idx)
{
    struct kthread *t = (struct kthread *)item;

    t->sleep_heap_idx = idx;
}

static int kthread_sleep_cmp(const void *a, const void *b)
{
    const struct kthread *ta = (const struct kthread *)a;
    const struct kthread *tb = (const struct kthread *)b;

    if (ta->wake_at_ticks < tb->wake_at_ticks)
        return -1;

    if (ta->wake_at_ticks > tb->wake_at_ticks)
        return 1;

    return 0;
}

static void kthread_leave_sleep_heap(struct kthread *t)
{
    if (t->sleep_heap_idx == MIN_HEAP_IDX_NONE)
        return;

    cpu_irq_disable();
    min_heap_remove(&kthread_sleep_heap, t->sleep_heap_idx);
    cpu_irq_enable();
}

static int kthread_id_valid(int id)
{
    return id >= 0 && (unsigned int)id < KTHREAD_MAX;
}

static struct kthread *kthread_get(int id)
{
    if (!kthread_id_valid(id))
        return 0;

    return &kthread_table[id];
}

static int kthread_alloc_slot(void)
{
    for (unsigned int i = 0; i < KTHREAD_MAX; i++)
    {
        if (kthread_table[i].state != KTHREAD_DEAD)
            continue;

        return (int)i;
    }

    return KTHREAD_ID_NONE;
}

static unsigned int kthread_stack_prepare(void *stack_base, unsigned long stack_size,
                                          void (*entry)(void))
{
    if (stack_base == 0 || stack_size < 5u * sizeof(unsigned int) || entry == 0)
        return 0;

    unsigned long bottom = (unsigned long)stack_base;
    unsigned long top = bottom + stack_size;

    top &= ~(unsigned long)(sizeof(unsigned int) - 1u);

    unsigned int *slot = (unsigned int *)(top - 5u * sizeof(unsigned int));

    if ((unsigned long)slot < bottom)
        return 0;

    /*
     * Layout must match kthread/switch.asm: esp points here, then pop edi, esi,
     * ebx, ebp and ret. slot[4] is the return address (entry trampoline) ret uses.
     */
    slot[0] = 0;
    slot[1] = 0;
    slot[2] = 0;
    slot[3] = 0;
    slot[4] = (unsigned int)(unsigned long)entry;

    return (unsigned int)(unsigned long)slot;
}

/*
 * Next RUNNABLE slot after after_id: scan ids (after_id + 1) … KTHREAD_MAX - 1,
 * then 0 … after_id (wrap). after_id KTHREAD_ID_NONE starts at 0.
 */
static int kthread_find_next_runnable(int after_id)
{
    unsigned int start = 0;

    if (kthread_id_valid(after_id))
        start = ((unsigned int)after_id + 1u) % KTHREAD_MAX;

    for (unsigned int n = 0; n < KTHREAD_MAX; n++)
    {
        unsigned int i = (start + n) % KTHREAD_MAX;

        if (kthread_table[i].state == KTHREAD_RUNNABLE)
            return (int)i;
    }

    return KTHREAD_ID_NONE;
}

static void kthread_reap_if_needed(void)
{
    int id = kthread_reap_after_switch;

    kthread_reap_after_switch = KTHREAD_ID_NONE;
    if (!kthread_id_valid(id))
        return;

    struct kthread *t = kthread_get(id);

    if (t->stack_base != 0)
    {
        kfree(t->stack_base);
        t->stack_base = 0;
    }

    t->stack_size = 0;
    t->saved_sp = 0;
    t->fn = 0;
    t->arg = 0;
    kthread_block_reset(t);
    t->state = KTHREAD_DEAD;
}

/*
 * Boot-stack scheduler: reap, pick next RUNNABLE, run it. Returns after a kthread
 * switches back via scheduler_saved_sp (e.g. exit when no other peer to run).
 */
static void kthread_schedule(void)
{
    kthread_reap_if_needed();

    int next = kthread_find_next_runnable(KTHREAD_ID_NONE);

    if (next == KTHREAD_ID_NONE)
    {
        for (;;)
        {
            cpu_irq_enable();
            cpu_halt();

            kthread_reap_if_needed();
            next = kthread_find_next_runnable(KTHREAD_ID_NONE);

            if (next != KTHREAD_ID_NONE)
                break;
        }
    }

    struct kthread *nt = kthread_get(next);

    kthread_current_id = next;
    nt->state = KTHREAD_RUNNING;
    thread_switch(&scheduler_saved_sp, nt->saved_sp);
}

/*
 * Exit path: run another RUNNABLE kthread, or return to kthread_schedule() on
 * the boot stack. Does not return on the exiting kthread's stack.
 */
static void kthread_switch_away(unsigned int *save_sp)
{
    int next = kthread_find_next_runnable(KTHREAD_ID_NONE);

    if (next != KTHREAD_ID_NONE)
    {
        struct kthread *nt = kthread_get(next);

        kthread_current_id = next;
        nt->state = KTHREAD_RUNNING;
        thread_switch(save_sp, nt->saved_sp);
        return;
    }

    if (scheduler_saved_sp == 0)
    {
        kprint("kthread: nowhere to switch\n");
        cpu_halt();
    }

    thread_switch(save_sp, scheduler_saved_sp);
}

static void kthread_trampoline(void)
{
    if (!kthread_id_valid(kthread_current_id))
        kthread_exit();

    struct kthread *t = kthread_get(kthread_current_id);
    void (*fn)(void *arg) = t->fn;
    void *arg = t->arg;

    fn(arg);
    kthread_exit();
}

void kthread_init(void)
{
    kthread_current_id = KTHREAD_ID_NONE;
    scheduler_saved_sp = 0;
    kthread_reap_after_switch = KTHREAD_ID_NONE;

    min_heap_init(&kthread_sleep_heap, kthread_sleep_cmp, kthread_sleep_heap_set_index);

    for (unsigned int i = 0; i < KTHREAD_MAX; i++)
    {
        kthread_table[i].state = KTHREAD_DEAD;
        kthread_table[i].stack_base = 0;
        kthread_table[i].stack_size = 0;
        kthread_table[i].saved_sp = 0;
        kthread_table[i].fn = 0;
        kthread_table[i].arg = 0;
        kthread_block_reset(&kthread_table[i]);
    }
}

int kthread_create(void (*fn)(void *arg), void *arg)
{
    if (fn == 0)
        return KTHREAD_ID_NONE;

    int id = kthread_alloc_slot();

    if (id == KTHREAD_ID_NONE)
        return KTHREAD_ID_NONE;

    void *stack = kzalloc(KTHREAD_STACK_SIZE);

    if (stack == 0)
        return KTHREAD_ID_NONE;

    unsigned int sp = kthread_stack_prepare(stack, KTHREAD_STACK_SIZE, kthread_trampoline);

    if (sp == 0)
    {
        kfree(stack);
        return KTHREAD_ID_NONE;
    }

    struct kthread *t = kthread_get(id);

    t->state = KTHREAD_RUNNABLE;
    kthread_block_reset(t);
    t->stack_base = stack;
    t->stack_size = KTHREAD_STACK_SIZE;
    t->saved_sp = sp;
    t->fn = fn;
    t->arg = arg;

    return id;
}

void kthread_exit(void)
{
    if (!kthread_id_valid(kthread_current_id))
    {
        kprint("kthread: exit with no current kthread\n");
        kthread_switch_away(&kthread_discard_sp);
        __builtin_unreachable();
    }

    struct kthread *t = kthread_get(kthread_current_id);

    if (t->state == KTHREAD_BLOCKED && t->block_reason == KTHREAD_BLOCK_REASON_SLEEP)
        kthread_leave_sleep_heap(t);

    t->state = KTHREAD_DEAD;
    kthread_reap_after_switch = kthread_current_id;
    kthread_current_id = KTHREAD_ID_NONE;

    kthread_switch_away(&t->saved_sp);
    __builtin_unreachable();
}

/*
 * Running kthread gives up the CPU to another RUNNABLE peer if one exists;
 * otherwise returns immediately (still RUNNING, same stack).
 */
void kthread_yield(void)
{
    kthread_reap_if_needed();

    if (!kthread_id_valid(kthread_current_id))
    {
        kprint("kthread: yield with no current kthread\n");
        return;
    }

    int self = kthread_current_id;
    struct kthread *t = kthread_get(self);
    int next = kthread_find_next_runnable(self);

    if (next == KTHREAD_ID_NONE)
        return;

    t->state = KTHREAD_RUNNABLE;

    struct kthread *nt = kthread_get(next);

    kthread_current_id = next;
    nt->state = KTHREAD_RUNNING;
    thread_switch(&t->saved_sp, nt->saved_sp);
    kthread_reap_if_needed();
}

void kthread_sleep_interruptible_until(tick_t wake_at_ticks)
{
    kthread_reap_if_needed();

    if (!kthread_id_valid(kthread_current_id))
        return;

    struct kthread *t = kthread_get(kthread_current_id);

    t->wake_at_ticks = wake_at_ticks;
    t->block_kind = KTHREAD_BLOCK_KIND_INTERRUPTIBLE;
    t->block_reason = KTHREAD_BLOCK_REASON_SLEEP;
    t->state = KTHREAD_BLOCKED;

    cpu_irq_disable();

    if (min_heap_push(&kthread_sleep_heap, t) != 0)
    {
        cpu_irq_enable();
        kprint("kthread: sleep heap push failed\n");
        t->state = KTHREAD_RUNNING;
        kthread_block_reset(t);
        return;
    }

    cpu_irq_enable();

    kthread_switch_away(&t->saved_sp);
    kthread_reap_if_needed();
}

void kthread_on_tick(tick_t now)
{
    for (;;)
    {
        struct kthread *t = (struct kthread *)min_heap_peek(&kthread_sleep_heap);

        if (t == 0 || t->wake_at_ticks > now)
            break;

        min_heap_pop(&kthread_sleep_heap);
        if (t->state != KTHREAD_BLOCKED)
            continue;

        kthread_block_reset(t);
        t->state = KTHREAD_RUNNABLE;
    }
}

void kthread_run(void)
{
    for (;;)
        kthread_schedule();
}

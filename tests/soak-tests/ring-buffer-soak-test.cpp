#include "util/ring_buffer.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <thread>

// Standalone SPSC soak: one producer pushes an ascending sequence, one consumer
// drains it and checks every element is the next expected value. Runs for a
// number of seconds (argv[1], else 10; 0 = until Ctrl-C) or until correctness
// breaks. Prints a liveness/metrics line every second.
namespace {

constexpr unsigned int Cap = 16u;
using elem_t = unsigned long long;

std::atomic<bool> stop{false};
std::atomic<unsigned long long> transferred{0};
std::atomic<unsigned long long> mismatches{0};

// Ctrl-C ends a run cleanly instead of killing it mid-transfer.
void on_signal(int)
{
    stop.store(true, std::memory_order_relaxed);
}

}  // namespace

int main(int argc, char **argv)
{
    unsigned long long limit_seconds = 10;   // default 10s; 0 = run until Ctrl-C

    if (argc > 1) {
        errno = 0;
        char *end = nullptr;

        limit_seconds = std::strtoull(argv[1], &end, 10);

        if (errno == ERANGE || end == argv[1] || *end != '\0') {
            std::fprintf(stderr, "usage: %s [seconds]  (0 = forever)\n", argv[0]);
            return 2;
        }
    }

    std::signal(SIGINT, on_signal);

    alignas(elem_t) unsigned char storage[Cap * sizeof(elem_t)];
    ring_buffer_t rb;
    ring_buffer_init(&rb, storage, Cap, sizeof(elem_t));

    std::thread producer([&] {
        for (elem_t i = 0; !stop.load(std::memory_order_relaxed); ++i)
            while (ring_buffer_put(&rb, &i) == 0)
                if (stop.load(std::memory_order_relaxed))
                    return;
    });

    std::thread consumer([&] {
        for (elem_t expected = 0; !stop.load(std::memory_order_relaxed); ++expected) {
            elem_t out = ~0ull;
            while (ring_buffer_get(&rb, &out) == 0)
                if (stop.load(std::memory_order_relaxed))
                    return;

            if (out != expected) {
                std::fprintf(stderr,
                             "MISMATCH after %llu transfers: got %llu, expected %llu\n",
                             transferred.load(std::memory_order_relaxed), out, expected);
                mismatches.fetch_add(1, std::memory_order_relaxed);
                expected = out;   // resync so one slip does not cascade
            }

            transferred.fetch_add(1, std::memory_order_relaxed);
        }
    });

    const auto started = std::chrono::steady_clock::now();
    unsigned long long last_count = 0;

    while (!stop.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        const auto now = std::chrono::steady_clock::now();
        const unsigned long long elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(now - started).count();
        const unsigned long long count = transferred.load(std::memory_order_relaxed);

        std::fprintf(stderr, "[%4llus] %18llu transfers  %7.2f M/s  mismatches: %llu\n",
                     elapsed, count, (double)(count - last_count) / 1e6,
                     mismatches.load(std::memory_order_relaxed));
        last_count = count;

        if (mismatches.load(std::memory_order_relaxed) != 0) {
            std::fprintf(stderr, "stopping: correctness violated\n");
            stop.store(true, std::memory_order_relaxed);
            break;
        }

        if (limit_seconds != 0 && elapsed >= limit_seconds)
            stop.store(true, std::memory_order_relaxed);
    }

    producer.join();
    consumer.join();

    const unsigned long long bad = mismatches.load(std::memory_order_relaxed);
    std::fprintf(stderr, "finished: %llu transfers, %llu mismatches\n",
                 transferred.load(std::memory_order_relaxed), bad);
    return bad == 0 ? 0 : 1;
}

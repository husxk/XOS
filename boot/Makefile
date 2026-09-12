
all: os.img

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

kernel.bin: kernel.asm
	nasm -f bin kernel.asm -o kernel.bin

os.img: boot.bin kernel.bin
	cat boot.bin kernel.bin > os.img

clean:
	rm -f boot.bin kernel.bin os.img

qemu: all
	qemu-system-x86_64 -display curses -drive format=raw,file=os.img

.PHONY: all clean qemu

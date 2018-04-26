build/timer.o:device/timer.c
	gcc -m32 -I lib/kernel -I lib/ -I thread/ -I kernel/ -c -fno-builtin -o build/timer.o device/timer.c
build/main.o:kernel/main.c
	gcc -m32 -I lib/kernel -I lib/ -I thread/ -I kernel/ -I device/ -c -fno-builtin -o build/main.o kernel/main.c
build/init.o:kernel/init.c
	gcc -m32 -I lib/kernel -I lib/ -I thread/ -I device/ -I kernel/ -c -fno-builtin -o build/init.o kernel/init.c
build/interrupt.o:kernel/interrupt.c
	gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -fno-stack-protector -o build/interrupt.o kernel/interrupt.c
build/debug.o:kernel/debug.c
	gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/debug.o kernel/debug.c
build/string.o:lib/string.c
	gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/string.o lib/string.c
build/bitmap.o:lib/kernel/bitmap.c
	gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/bitmap.o lib/kernel/bitmap.c
build/memory.o:kernel/memory.c
	gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/memory.o kernel/memory.c
build/thread.o:thread/thread.c
	gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/thread.o thread/thread.c
build/list.o:lib/kernel/list.c
	gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/list.o lib/kernel/list.c
build/sync.o:thread/sync.c
	gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/sync.o thread/sync.c
build/console.o:device/console.c
	gcc -m32 -I lib/kernel -I lib/ -I thread/ -I kernel/ -c -fno-builtin -o build/console.o device/console.c
build/keyboard.o:device/keyboard.c
	gcc -m32 -I lib/kernel -I lib/ -I thread/ -I kernel/ -c -fno-builtin -o build/keyboard.o device/keyboard.c

build/kernel.o:kernel/kernel.S
	nasm -f elf -o build/kernel.o kernel/kernel.S
build/print.o:lib/kernel/print.S
	nasm -f elf -o build/print.o lib/kernel/print.S
build/switch.o:thread/switch.S
	nasm -f elf -o build/switch.o thread/switch.S

build/kernel.bin:build/timer.o build/main.o build/init.o build/interrupt.o build/print.o build/kernel.o build/debug.o build/string.o build/bitmap.o build/memory.o build/thread.o build/list.o build/switch.o build/sync.o build/console.o build/keyboard.o
	ld -m elf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/main.o build/init.o build/interrupt.o build/print.o build/kernel.o build/timer.o build/debug.o build/string.o build/bitmap.o build/memory.o build/thread.o build/list.o build/switch.o build/sync.o build/console.o build/keyboard.o

all:build/kernel.bin
	@echo "compile done"

.PHONY:clean
clean:
	@rm build/*.o
	@echo "object files removed"

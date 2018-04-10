build/timer.o:device/timer.c
	gcc -m32 -I lib/kernel -c -o build/timer.o device/timer.c
build/main.o:kernel/main.c
	gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/main.o kernel/main.c
build/init.o:kernel/init.c
	gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -o build/init.o kernel/init.c
build/interrupt.o:kernel/interrupt.c
	gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-builtin -fno-stack-protector -o build/interrupt.o kernel/interrupt.c

build/kernel.o:kernel/kernel.S
	nasm -f elf -o build/kernel.o kernel/kernel.S
build/print.o:lib/kernel/print.S
	nasm -f elf -o build/print.o lib/kernel/print.S

build/kernel.bin:build/timer.o build/main.o build/init.o build/interrupt.o build/print.o build/kernel.o
	ld -m elf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/main.o build/init.o build/interrupt.o build/print.o build/kernel.o build/timer.o

all:build/kernel.bin
	@echo "compile done"

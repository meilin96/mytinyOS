CC = gcc
AS = nasm
LD = ld
LIB = -I lib/ -I lib/kernel -I kernel/ -I device/ -I thread/
ASFLAGS = -f elf -o
CFLAGS = -m32 -W -Wall $(LIB) -c -fno-builtin -o
ENTRY_POINT = 0xc0001500
BUILD_DIR = ./build
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -o
OBJS =  $(BUILD_DIR)/main.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/init.o \
        $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/string.o \
        $(BUILD_DIR)/bitmap.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/thread.o \
        $(BUILD_DIR)/list.o $(BUILD_DIR)/sync.o $(BUILD_DIR)/console.o \
        $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/ioqueue.o $(BUILD_DIR)/print.o $(BUILD_DIR)/switch.o $(BUILD_DIR)/kernel.o

#--------------------------------C代码
$(BUILD_DIR)/main.o: kernel/main.c
	$(CC) $(CFLAGS) $@ $<

$(BUILD_DIR)/timer.o: device/timer.c
	$(CC) $(CFLAGS) $@ $<
 
$(BUILD_DIR)/init.o: kernel/init.c 
	$(CC) $(CFLAGS) $@ $<

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c 
	$(CC) -m32 -W -Wall $(LIB) -c -fno-builtin -fno-stack-protector -o $@ $<

ENTRY_POINT = 0xc0001500

$(BUILD_DIR)/debug.o: kernel/debug.c 
	$(CC) $(CFLAGS) $@ $<

$(BUILD_DIR)/string.o: lib/string.c 
	$(CC) $(CFLAGS) $@ $<

$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c 
	$(CC) $(CFLAGS) $@ $<

$(BUILD_DIR)/memory.o: kernel/memory.c 
	$(CC) $(CFLAGS) $@ $<

$(BUILD_DIR)/thread.o: thread/thread.c 
	$(CC) $(CFLAGS) $@ $<

$(BUILD_DIR)/list.o: lib/kernel/list.c 
	$(CC) $(CFLAGS) $@ $<

$(BUILD_DIR)/sync.o: thread/sync.c 
	$(CC) $(CFLAGS) $@ $<

$(BUILD_DIR)/console.o: device/console.c 
	$(CC) $(CFLAGS) $@ $<

$(BUILD_DIR)/keyboard.o: device/keyboard.c 
	$(CC) $(CFLAGS) $@ $<

$(BUILD_DIR)/ioqueue.o: device/ioqueue.c 
	$(CC) $(CFLAGS) $@ $<
    
#--------------------------------汇编代码
$(BUILD_DIR)/kernel.o: kernel/kernel.S
	$(AS) $(ASFLAGS) $@ $<

$(BUILD_DIR)/print.o: lib/kernel/print.S
	$(AS) $(ASFLAGS) $@ $<

$(BUILD_DIR)/switch.o: thread/switch.S
	$(AS) $(ASFLAGS) $@ $<

#--------------------------------链接
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $@ $^


.PHONY:clean all
all: clean build/kernel.bin

clean:
	@rm build/*.o
	@echo "object files removed"

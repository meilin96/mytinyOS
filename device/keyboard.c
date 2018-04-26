#include "keyboard.h"
#include "print.h"
#include "interrupt.h"
#include "io.h"
#include "global.h"

#define KBD_BUF_PORT 0x60

void intr_keyboard_handler(){
    put_char('k');
    inb(KBD_BUF_PORT);
    return;
}
void keyboard_init(){
    put_str("kerboard init start\n");
    regist_handler(0x21, intr_keyboard_handler);
    put_str("kerboard init done\n");
}

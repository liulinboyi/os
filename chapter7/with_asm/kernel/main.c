# include "kernel/print.h"
# include "init.h"

void main(void) {
    put_str("I am kernel.\n");
    init_all();
    put_str("Init finished.\n");
    asm volatile ("sti"); //为演示中断处理,在此临时开中断
    put_str("Turn on the interrupt.\n");
    while (1);
}

// mem.c的作用是变量in_b用in_a的值替换。in_b最终变成1。
#include<stdio.h>
void main() {
    int in_a = 1, in_b = 2;
    printf("in_b is %d\n", in_b);
    // 内联汇编，把in_a施加寄存器约束a，告诉gcc把变量in_a放到寄存器eax中，对in_b施加内存约束m，告诉gcc把变量in_b的指针作为内联代码的操作数。
    // 行对寄存器eax的引用：%b0，这是用的32位数据的低8位，在这里就是指al寄存器。如果不显式加字符'b'，编译器也会按照低8位来处理，但它会发出警告。
    // 新的符号—“%1”，它是序号占位符，在这里大家认为它代表in_b的内存地址（指针）就行了。
    // asm("movb %b0, %1;"::"a"(in_a),"m"(in_b));
    asm("movb %%al, %1;"::"a"(in_a),"m"(in_b));
    printf("in_b now is %d\n", in_b);
}



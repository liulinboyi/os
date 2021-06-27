#include<stdio.h>
void main() {
/*
我们的目的是用18除以3，最后打印结果是6。
在第6行，被除数in_a通过寄存器约束a存入寄存器eax，除数in_b通过内存约束m被gcc将自己的内存地址传给汇编做操作数，咱们不用关心此内存地址是哪里。为了引用除数所在的内存，我们用名称占位符标识它，名字是divisor。这样汇编代码中，第4行除法指令divb可以通过%[divisor]引用除数所在的内存，进行除法运算。divb是8位除法指令，商存放在寄存器al中，余数存放在寄存器ah中。所以第4行中用movb指令将寄存器al的值写入用于存储结果的c变量out的地址中。
*/
    int in_a = 18, in_b = 3, out = 0;
    asm("divb %[divisor];movb %%al,%[result]"\
           :[result]"=m"(out)\
           :"a"(in_a),[divisor]"m"(in_b)\
           );
    printf("result is %d\n",out);
}



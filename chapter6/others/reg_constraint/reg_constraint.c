#include<stdio.h>
 void main() {
    int in_a = 1, in_b = 2, out_sum;
    int a = 500,b = 300;
    long result;
    /* 
    同样是为加法指令提供参数，in_a和in_b是在input部分中输入的，用约束名a为c变量in_a指定了用寄存器eax，用约束名b为c变量in_b指定了用寄存器ebx。 
    addl指令的结果存放到了寄存器eax中，在output中用约束名a指定了把寄存器eax的值存储到c变量out_sum中。
    output中的'='号是操作数类型修饰符，表示只写，其实就是out_sum=eax的意思。
    */
    asm("addl %%ebx, %%eax":"=a"(out_sum):"a"(in_a),"b"(in_b));
    asm("mull %%ebx":"=a"(result):"a"(a),"b"(b));
    printf("sum is %d\n",out_sum);
    printf("result is %ld\n",result);
}



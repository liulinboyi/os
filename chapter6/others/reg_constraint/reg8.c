#include <stdio.h>
void main() {
    int ret_cnt = 0, test = 0;
    char* fmt = "hello,world\n";// 共12个字符
    asm("pushl %1;\
      call printf;/* 调用printf。将第4行定义的字符串指针fmt通过压栈传参给printf函数，在第6行执行会屏幕会打印hello，world换行。 在此，eax寄存器中值是printf的返回值，应该为12。*/\
      addl $4, %%esp;/* 回收参数所占的栈空间。 */\
      movl $6, %2"/* 第8行把立即数6传入了gcc为变量test分配的寄存器。 */\
      :"=a&"(ret_cnt)/* 在第9行，output部分中的c变量ret_cnt获得了寄存器的值。 */\
      :"m"(fmt),"r"(test)/*通过内存约束m把字符串指针fmt传给了汇编代码，把变量test用寄存器约束r声明，由gcc自由分配寄存器。*/\
      );
     printf("the number of bytes written is %d\n", ret_cnt);
}

/* 我们原本想着打印返回值，应该是12，而不是6，并未按照我们的预期打印the number of bytes written is 12。 */
/* 说明在文件reg7.c的第8行，%2被gcc分配为寄存器eax了。 */
/* 修饰符'&'用来表示此寄存器只能分配给output中的某个C变量使用，不能再分给input中某变量了。 */
/* 这时候修饰符'&'就派上用场了，只要为test约束的寄存器不要和ret_cnt相同就行，可以在reg7.c的第9行加个'&'，修改后的文件为reg8.c。 */



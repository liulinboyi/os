section .data
str: db "asm_print says hello world!", 0xa, 0
;0xa是换行符,0是手工加上的字符串结束符\0的ASCII码 定义待打印的字符串时，在结尾人为地加了个0，它就是空字符'\0'的ASCII码。
str_len equ $-str

section .text
extern c_print ; 通知编译器这个函数并不在当前文件中定义
global _start ; 将_start导出为全局符号，为的是给链接器用的 在汇编语言中导出符号名用global关键字，这在之前说_start时大伙已有所耳闻，global将符号导出为全局属性，对程序中的所有文件可见，这样其他外部文件中也可以引用被global导出的符号啦，无论该符号是函数，还是变量。
_start:
;;;;;;;;;;;; 调用c代码中的函数c_print ;;;;;;;;;;;
     push str ; 传入参数
     call c_print ; 调用c函数
     add esp,4 ; 回收栈空间

 ;;;;;;;;;;;;;;;;;;; 退出程序 ;;;;;;;;;;;;;;;;;;;;
     mov eax,1 ; 第1号子功能是exit系统调用
     int 0x80 ; 发起中断，通知Linux完成请求的功能

 global asm_print ; 相当于asm_print（str,size）
 asm_print:
     push ebp ; 备份ebp
     mov ebp,esp
     mov eax,4 ; 第4号子功能是write系统调用
     mov ebx, 1 ; 此项固定为文件描述符1，标准输出（stdout）指向屏幕
     mov ecx, [ebp+8] ; 第1个参数
     mov edx, [ebp+12] ; 第2个参数
     int 0x80 ; 发起中断，通知Linux完成请求的功能
     pop ebp ; 恢复ebp
     ret

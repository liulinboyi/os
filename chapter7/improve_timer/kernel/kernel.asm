[bits 32]

; 对于CPU会自动压入错误码的中断类型，无需额外的操作
%define ERROR_CODE nop
; 如果CPU没有压入错误码，为了保持处理逻辑的一致性，我们需要手动压入一个0
%define ZERO push 0

extern put_str
; 中断处理函数数组
extern idt_table ; idt_table是C中注册的中断处理程序数组

section .data
intr_str db "interrupt occur!", 0xa, 0
global intr_entry_table
intr_entry_table:

; 中断处理程序宏定义
%macro VECTOR 2
section .text
intr%1entry: ; 每个中断处理程序都要压入中断向量号
             ; 所以一个中断类型一个中断处理程序
             ; 自己知道自己的中断向量号是多少    
    %2 ; 自己知道自己的中断向量号是多少
    ; 保存上下文 以下是保存上下文环境
    push ds
    push es
    push fs
    push gs
    pushad ; PUSHAD指令压入32位寄存器，其入栈顺序是：
           ; EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI,EAX最先入栈

    ;如果是从片上进入的中断
    ; 除了往从片上发送EOI外，还要往主片上发送EOI
    mov al, 0x20 ; 中断结束命令EOI
    out 0xa0, al ; 向从片发送
    out 0x20, al ; 向主片发送

    push %1 ; 不管idt_table中的目标程序是否需要参数
            ; 都一律压入中断向量号，调试时很方便

    ; 调用C的中断处理函数
    call [idt_table + 4 * %1]; 调用idt_table中的C版本中断处理函数
    jmp intr_exit

section .data
    dd intr%1entry ; 存储各个中断入口程序的地址
                   ; 形成intr_entry_table数组

%endmacro

section .text
global intr_exit
intr_exit:
    ; 以下是恢复上下文环境
    add esp, 4 ; 跳过中断号
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4 ; 跳过error_code
    iretd

VECTOR 0x00,ZERO
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO
VECTOR 0x03,ZERO 
VECTOR 0x04,ZERO
VECTOR 0x05,ZERO
VECTOR 0x06,ZERO
VECTOR 0x07,ZERO 
VECTOR 0x08,ERROR_CODE
VECTOR 0x09,ZERO
VECTOR 0x0a,ERROR_CODE
VECTOR 0x0b,ERROR_CODE 
VECTOR 0x0c,ZERO
VECTOR 0x0d,ERROR_CODE
VECTOR 0x0e,ERROR_CODE
VECTOR 0x0f,ZERO 
VECTOR 0x10,ZERO
VECTOR 0x11,ERROR_CODE
VECTOR 0x12,ZERO
VECTOR 0x13,ZERO 
VECTOR 0x14,ZERO
VECTOR 0x15,ZERO
VECTOR 0x16,ZERO
VECTOR 0x17,ZERO 
VECTOR 0x18,ERROR_CODE
VECTOR 0x19,ZERO
VECTOR 0x1a,ERROR_CODE
VECTOR 0x1b,ERROR_CODE 
VECTOR 0x1c,ZERO
VECTOR 0x1d,ERROR_CODE
VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO 
VECTOR 0x20,ZERO

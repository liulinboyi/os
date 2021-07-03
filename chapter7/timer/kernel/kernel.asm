[bits 32]

; 对于CPU会自动压入错误码的中断类型，无需额外的操作
%define ERROR_CODE nop
; 如果CPU没有压入错误码，为了保持处理逻辑的一致性，我们需要手动压入一个0
%define ZERO push 0

extern put_str
; 中断处理函数数组
extern idt_table

section .data
intr_str db "interrupt occur!", 0xa, 0
global intr_entry_table
intr_entry_table:

; 中断处理程序宏定义
%macro VECTOR 2
section .text
intr%1entry:
    
    %2
    ; 保存上下文
    push ds
    push es
    push fs
    push gs
    pushad

    mov al, 0x20
    out 0xa0, al
    out 0x20, al

    push %1

    ; 调用C的中断处理函数
    call [idt_table + 4 * %1]
    jmp intr_exit

section .data
    dd intr%1entry

%endmacro

section .text
global intr_exit
intr_exit:
    add esp, 4
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4
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
VECTOR 0x1e,ERROR_CODE; 调用宏VECTOR的两个地方 第1个参数0x1e是中断向量号，用来表示：本宏是为了此中断向量号而定义的中断处理程序，或者说这是本宏实现的中断处理程序对应的中断向量号，总之将来咱们要把它装载到中断描述符表中以该中断向量号为索引的中断门描述符位置。 第2个参数ERROR_CODE也是个宏
VECTOR 0x1f,ZERO ; 调用宏VECTOR的两个地方 第1个参数0x1f是中断向量号，第2个参数是ZERO，它也是个宏 ZERO的值是push 0，是“把0压入栈”这个操作。
VECTOR 0x20,ZERO

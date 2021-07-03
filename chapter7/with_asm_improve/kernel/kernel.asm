[bits 32]
; 定义了33个中断处理程序。每个中断处理程序都一样，就是调用字符串打印函数put_str来打印字符串“interrupt occur!”，之后直接退出中断。
; 原因是中断向量0～19为处理器内部固定的异常类型，20～31是Intel保留的
; 所以咱们可用的中断向量号最低是32，将来咱们在设置8259A的时候，会把IR0的中断向量号设置为32（这是后话）。
; 目前只为演示中断机制的原理，咱们就拿连接在主片IR0接口上外部设备—时钟，小试身手。

; 对于CPU会自动压入错误码的中断类型，无需额外的操作
%define ERROR_CODE nop  ; 宏它的值为nop nop是汇编指令，它表示no operation，不操作，什么都不干。在此完全是占位充数用的，一会儿说占位是怎么回事。 若在相关的异常中CPU已经自动压入了
; 错误码，为保持栈中格式统一，这里不做操作
; 如果CPU没有压入错误码，为了保持处理逻辑的一致性，我们需要手动压入一个0
%define ZERO push 0 ; 若在相关的异常中CPU没有压入错误码 ;为了统一栈中格式，就手工压入一个0

extern put_str ;声明外部函数
global inter_rupt_call_in_asm
; inter_rupt_call times 0x21 db 0 ; 第一种方法，直接开辟所有的中断处理程序的地址，然后在interrupt.c里面修改中断处理程序的地址
inter_rupt_call_in_asm db 0 ; 第二种，先声明一个32位全0数据占位，，使用inter_rupt_call_in_asm引用地址，作为中断处理程序数组指针的存放地址，然后在interrupt.c里面通过inter_rupt_call_in_asm引用，修改存放地址里面的值为真正的数组指针

global intr_entry_table ; 为了引用所有中断处理程序的地址，我们得事先把它们都记下来。为此，我们在kernel.S中定义了一个数组，数组名为intr_entry_table

section .data ; 编译器会将属性相同的section合并到同一个大的segment中，而且，为了显得更靠谱一点，我们在kernel.S中对所有的数据section都用了同一个名字.data，这显得更万无一失啦
intr_str db "interrupt occur!", 0xa, 0 ; 地址 字符串“interrupt occur!换行0”
intr_entry_table: ; 最后会在一个大的segment中编译结果如下
                ; dd intr0x00entry ; 存储各个中断入口程序的地址 ; 形成intr_entry_table数组
                ; dd intr0x01entry ; 存储各个中断入口程序的地址 ; 形成intr_entry_table数组
                ; dd intr0x02entry ; 存储各个中断入口程序的地址 ; 形成intr_entry_table数组
                ; ...

; 中断处理程序宏定义
; 用宏定义中断处理程序
%macro VECTOR 2 ; 用macro定义了名为VECTOR的宏，其接受2个参数。
section .text
intr%1entry: ; 标号，也就是地址，这是中断处理程序的起始处，所以此标号是为了获取中断处理程序的地址
    ; 每个中断处理程序都要压入中断向量号 ;所以一个中断类型一个中断处理程序 自己知道自己的中断向量号是多少
    %2 ; 手工压入的32位数也应该放在压入EIP之后操作，即它必须是中断处理程序中的第一个指令
    ; push intr_str ; 传入参数
    ; call put_str ; 调用c函数
    ; add esp, 4 ; 跳过参数 ; 回收栈空间 调用者处理回收栈空间
    push %1
    push eax
    mov eax, [inter_rupt_call_in_asm] ;取地址里面的值
    call [eax + 4 * %1] ; 地址为4个字节32位
    pop eax
    add esp, 4
    
    ;如果是从片上进入的中断，除了往从片上发送EOI外，还要往主片上发送EOI
    ; 往主片和从片中写入0x20，也就是写入EOI。这是8259A的操作控制字OCW2，其中第5位是EOI位，此位为1，其余位全为0，所以是0x20。
    ; 由于将来咱们在设置8259A时设置了手动结束，所以咱们得在中断处理程序中手动向8259A发送中断结束标记，否则8259A并不知道中断处理完成了，它会一直等下去，从而不再接受新的中断。也就是说，为了让8259A接受新的中断，必须要让8259A知道当前中断处理程序已经执行完成。
    mov al, 0x20 ; 中断结束命令EOI
    out 0xa0, al ; 向从片发送
    out 0x20, al ; 向主片发送

    add esp, 4 ; 跨过error_code
    iret ; 从中断返回，32位下等同指令iretd

section .data ; 编译器会将属性相同的section合并到同一个大的segment中，而且，为了显得更靠谱一点，我们在kernel.S中对所有的数据section都用了同一个名字.data，这显得更万无一失啦
    dd intr%1entry ; 存储各个中断入口程序的地址 ; 形成intr_entry_table数组

%endmacro


; 有的中断不会压入错误码，有的会压入错误码入栈，所以将不会压入错误码的中断压入一个32位数，在iret指令执行前我们再将栈顶的数据（错误码或手工压入的32位数）跨过就万事大吉了。
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

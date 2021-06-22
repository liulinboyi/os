%include "boot.inc"

section loader vstart=LOADER_BASE_ADDR
;LOADER_BASE_ADDR = 0x900
;此处虚拟头结点是0x900
LOADER_STACK_TOP equ LOADER_BASE_ADDR

; 这里其实就是GDT的起始地址，第一个描述符为空
GDT_BASE: dd 0x00000000
          dd 0x00000000

; 代码段描述符，一个dd为4字节，段描述符为8字节，上面为低4字节
CODE_DESC: dd 0x0000FFFF
           dd DESC_CODE_HIGH4

; 栈段描述符，和数据段共用
DATA_STACK_DESC: dd 0x0000FFFF
                 dd DESC_DATA_HIGH4

; 显卡段，非平坦
VIDEO_DESC: dd 0x80000007
            dd DESC_VIDEO_HIGH4

GDT_SIZE equ $ - GDT_BASE
GDT_LIMIT equ GDT_SIZE - 1
times 120 dd 0 ; 预留60个段描述槽位
SELECTOR_CODE equ (0x0001 << 3) + TI_GDT + RPL0
SELECTOR_DATA equ (0x0002 << 3) + TI_GDT + RPL0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

; 内存大小，单位字节，此处的内存地址是0xb00
; total_memory_bytes用于保存内存容量，以字节为单位，此位置比较好记
; 当前偏移loader.bin文件头0x200字节
; loader.bin的加载地址是0x900
; 故total_memory_bytes内存中的地址是0xb00
; 将来在内核中咱们会引用此地址

; 段描述符大小是8字节，dq也是8字节，所以偏移量是 (4+60)*8=512=0x200字节
; 本程序的加载地址是0x900，0x900+0x200=0xb00，所以0xb00便是变量total_mem_bytes加载到内存中的地址。

total_memory_bytes dd 0 ; 4字节

gdt_ptr dw GDT_LIMIT
        dd GDT_BASE
; 6字节 dw 字两个字节 dd 双字 4个字节

ards_buf times 244 db 0 ; 244字节
ards_nr dw 0 ; 2字节

; total_mem_bytes是4字节，gdt_ptr是6字节，ards_buf是244字节，ards_nr是2字节，加起来的和是256字节，即0x100。
; 加上total_mem_bytes在文件内偏移地址是0x200，所以loader_start在文件内的偏移地址是0x100+0x200=0x300。
loader_start: 

    xor ebx, ebx ; ;第一次调用时，ebx值要为0
    mov edx, 0x534d4150 ; edx只赋值一次，循环体中不会改变
    mov di, ards_buf ; ards结构缓冲区

.e820_mem_get_loop: ; 循环获取每个ARDS内存范围描述结构
    ;执行int 0x15后，eax值变为0x534d4150，
    ;所以每次执行int前都要更新为子功能号
    mov eax, 0x0000e820 
    mov ecx, 20 ; ARDS地址范围描述符结构大小是20字节
    int 0x15 ; 调用中断
    
    jc .e820_mem_get_failed ; 条件转移指令jc,若cf位为1则有错误发生，尝试0xe801子功能
    
    add di, cx ; 使di增加20字节指向缓冲区中新的ARDS结构位置
    inc word [ards_nr] ; 这个地址处，一个字加1并赋值给这个地址 ;记录ARDS数量

    cmp ebx, 0 ; ;若ebx为0且cf不为1，这说明ards全部返回,否则表示没有全部返回，执行条件转移
    ; 当前已是最后一个
    jnz .e820_mem_get_loop

    ; 在所有ards结构中
    ; 找出(base_add_low + length_low)的最大值，即内存的容量
    
    
    ; 遍历每一个ARDS结构体,循环次数是ARDS的数量
    mov cx, [ards_nr]
    
    mov ebx, ards_buf ; 将ards内容的起始地址（标号）传送到ebx
    xor edx, edx ; edx为最大的内存容量，在此先清0

; 无需判断type是否为1,最大的内存块一定是可被使用的
.find_max_mem_area:
    mov eax, [ebx] ; base_add_low的低32位 eax是32位寄存器
    add eax, [ebx + 8] ; length_low的低32位 eax是32位寄存器
    add ebx, 20 ; 指向缓冲区中下一个ARDS结构
    
    ; 冒泡排序,找出最大,edx寄存器始终是最大的内存容量
    cmp edx, eax
    jge .next_ards ; edx >= eax 跳转,edx < eax 不跳转
    mov edx, eax

.next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok

.e820_mem_get_failed:
    mov byte [gs:0], 'f'
    mov byte [gs:2], 'a'
    mov byte [gs:4], 'i'
    mov byte [gs:6], 'l'
    mov byte [gs:8], 'e'
    mov byte [gs:10], 'd'
    jmp .done

.mem_get_ok:
    mov [total_memory_bytes], edx

.done:
    mov byte [gs:160], 'd'
    mov byte [gs:162], 'o'
    mov byte [gs:164], 'n'
    mov byte [gs:166], 'e'
    jmp $

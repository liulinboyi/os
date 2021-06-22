%include "boot.inc"

section loader vstart=LOADER_BASE_ADDR
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
times 120 dd 0
SELECTOR_CODE equ (0x0001 << 3) + TI_GDT + RPL0
SELECTOR_DATA equ (0x0002 << 3) + TI_GDT + RPL0
SELECTOR_VIDEO equ (0x0003 << 3) + TI_GDT + RPL0

; 内存大小，单位字节，此处的内存地址是0xb00
total_memory_bytes dd 0

gdt_ptr dw GDT_LIMIT
        dd GDT_BASE

ards_buf times 244 db 0
ards_nr dw 0

loader_start: 

    xor ebx, ebx
    mov edx, 0x534d4150
    mov di, ards_buf

.e820_mem_get_loop:
    mov eax, 0x0000e820
    mov ecx, 20
    int 0x15
    
    jc .e820_mem_get_failed
    
    add di, cx
    inc word [ards_nr]
    cmp ebx, 0
    jnz .e820_mem_get_loop

    mov cx, [ards_nr]
    mov ebx, ards_buf
    xor edx, edx

.find_max_mem_area:
    mov eax, [ebx]
    add eax, [ebx + 8]
    add ebx, 20
    cmp edx, eax
    jge .next_ards
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
    ; 内存检测失败，不再继续向下执行
    jmp $

.mem_get_ok:
    mov [total_memory_bytes], edx

    ; 开始进入保护模式
    ; 打开A20地址线
    in al, 0x92
    or al, 00000010B
    out 0x92, al

    ; 加载gdt
    lgdt [gdt_ptr]

    ; cr0第0位置1
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax

    ; 刷新流水线
    jmp dword SELECTOR_CODE:p_mode_start

[bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax

    mov es, ax
    mov ss, ax

    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax
    
    call setup_page

    ; 保存gdt表
    ; 要将描述符表地址及偏移量写入内存gdt_ptr，一会儿用新地址重新加载
    sgdt [gdt_ptr] ;  存储到原来gdt所有的位置

    ; 重新设置gdt描述符， 使虚拟地址指向内核的第一个页表
    ; 将gdt描述符中视频段描述符中的段基址+0xc0000000
    mov ebx, [gdt_ptr + 2] ; 将GDT基地址取出，传送给ebx
    or dword [ebx + 0x18 + 4], 0xc0000000
    ;视频段是第3个段描述符，每个描述符是8字节，故0x18 = 24 = 3 * 8
    ; 段描述符的高4字节的最高位是段基址的第31～24位
    ; 将gdt的基址加上0xc0000000使其成为内核所在的高地址
    add dword [gdt_ptr + 2], 0xc0000000
    
    ; 将栈指针同样映射到内核地址
    add esp, 0xc0000000

    ; 页目录基地址寄存器
    ; 把页目录地址赋给cr3
    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax

    ; 打开分页
    ; 打开cr0的pg位（第31位）
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ;在开启分页后，用gdt新的地址重新加载
    lgdt [gdt_ptr]
    ; 视频段段基址已经被更新，用字符v表示virtual addr
    mov ax,00000000000_11_000B ; 第三个全局描述符表中的描述符
    mov gs,ax
    mov byte [gs:160], 'V'
    mov byte [gs:160+2], ' '
    mov byte [gs:160+4], 'O'
    mov byte [gs:160+6], 'K'

    jmp $

; 创建页目录以及页表
setup_page:
    ; 页目录表占据4KB空间，清零之
    ; 是利用PAGE_DIR_TABLE_POS作为基址，esi作为变址，然后通过188行的inc esi，每次使esi自增1，逐步完成4096字节的清0工作。
    mov ecx, 4096
    mov esi, 0
.clear_page_dir:   
    mov byte [PAGE_DIR_TABLE_POS + esi], 0
    inc esi
    loop .clear_page_dir

; 创建页目录表(PDE)
.create_pde:
    mov eax, PAGE_DIR_TABLE_POS
    ; 0x1000为4KB，加上页目录表起始地址便是第一个页表的地址
    add eax, 0x1000
    mov ebx, eax ;  此处为ebx赋值，是为.create_pte做准备，ebx为基址

    ; 设置页目录项属性
    ; 寄存器eax是页目录项的内容
    ; 下面将页目录项0和0xc00都存为第一个页表的地址，每个页表表示4MB内存
    ; 这样0xc03fffff以下的地址和0x003fffff以下的地址都指向相同的页表
    ; 这是为将地址映射为内核地址做准备

    or eax, PG_US_U | PG_RW_W | PG_P ; PG_US_U | PG_RW_W | PG_P逻辑或的结果是0x7,整体的结果是0x101007
    ; 页目录项的属性RW和P位为1，US为1，表示用户属性，所有特权级别都可以访问
    ; 设置第一个页目录项
    mov [PAGE_DIR_TABLE_POS], eax
    ; 第768(内核空间的第一个)个页目录项，与第一个相同，这样第一个和768个都指向低端4MB空间
    ; 在页目录表中的第1个目录项写入第一个页表的位置(0x101000)及属性(7)
    mov [PAGE_DIR_TABLE_POS + 0xc00], eax
    ;它将来要指向的物理地址范围是0～0xfffff，只是1MB的空间，其余3MB并未分配。这是我们设计好的。
    
    
    ; 一个页表项占用4字节
    ; 0xc00表示第768个页表占用的目录项，0xc00以上的目录项用于内核空间
    ; 也就是页表的0xc0000000～0xffffffff共计1G属于内核
    ; 0x0～0xbfffffff共计3G属于用户进程
    ; 最后一个表项指向自己，用于访问页目录本身
    sub eax, 0x1000
    mov [PAGE_DIR_TABLE_POS + 4096 - 4], eax ; 最后表项的地址

; 创建页表
    mov ecx, 256 ; 1M低端内存 / 每页大小4k = 256
    mov esi, 0
    mov edx, PG_US_U | PG_RW_W | PG_P ; 属性为7，US=1，RW=1，P=1 前20位是物理地址，后面都是其他属性
.create_pte: ;  创建Page Table Entry
    mov [ebx + esi * 4], edx
    ; 此时的ebx已经在上面通过eax赋值为0x101000，也就是第一个页表的地址
    add edx, 4096
    inc esi
    loop .create_pte

; 创建内核的其它PDE
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x2000 ;  此时eax为第二个页表的位置
    or eax, PG_US_U | PG_RW_W | PG_P ;  页目录项的属性US､RW和P位都为1
    mov ebx, PAGE_DIR_TABLE_POS
    mov ecx, 254 ; 范围为第769～1022的所有目录项数量
    mov esi, 769
.create_kernel_pde:
    mov [ebx + esi * 4], eax
    inc esi
    add eax, 0x1000 ; 0x1000 = 4096
    loop .create_kernel_pde
    ret

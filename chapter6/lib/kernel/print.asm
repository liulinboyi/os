%include "lib/include/config.inc"
; 内核打印功能实现

section .data
put_int_buffer dd 0, 0 ; 定义8字节缓冲区用于数字到字符的转换

[bits 32]
section .text
; put_char，将栈中的一个字符写入光标所在处
global put_char ; 通过关键字global把函数put_char导出为全局符号，这样对外部文件便可见了，外部文件通过声明便可以调用。
global put_str
global put_int

; ----------将小端字节序的数字变成对应的ASCII后，倒置---
; 输入:栈中参数为待打印的数字
; 输出:在屏幕上打印十六进制数字，并不会打印前缀0x
; 如打印十进制15时，只会直接打印f，不会是0xf

put_int:
    pushad ; 备份32位寄存器环境
    mov ebp, esp ; 借鉴C调用约定，先把栈顶esp赋值给ebp，再通过ebp来获取参数
    mov eax, [ebp + 4 * 9] ; call的返回地址占4字节+pushad的8个4字节
    mov edx, eax
    mov edi, 7 ; 指定在put_int_buffer中初始的偏移量
    mov ecx, 8 ; 32位数字中，十六进制数字的位数是8个
    mov ebx, put_int_buffer
; 将32位数字按照十六进制的形式从低位到高位逐个处理
; 共处理8个十六进制数字
.16based_4bits: ; 每4位二进制是十六进制数字的1位
                ; 遍历每一位十六进制数字
    and edx, 0x0000000F ; 解析十六进制数字的每一位
                        ; and与操作后，edx只有低4位有效
    cmp edx, 9 ; 数字0～9和a～f需要分别处理成对应的字符
    jg .is_A2F
    add edx, '0' ; ASCII码是8位大小｡add求和操作后，edx低8位有效
    jmp .store

.is_A2F:
    sub edx, 10  ; A～F 减去10 所得到的差，再加上字符A的
                 ; ASCII码，便是A～F对应的ASCII码
    add edx, 'A' 
    ; 将每一位数字转换成对应的字符后，按照类似“大端”的顺序
    ; 存储到缓冲区put_int_buffer
    ; 高位字符放在低地址，低位字符要放在高地址，这样和大端字节序
    ; 类似,只不过咱们这里是字符序
.store:
; 此时dl中应该是数字对应的字符的ASCII码
    mov [ebx + edi], dl
    dec edi
    shr eax, 4
    mov edx, eax
    loop .16based_4bits
; 现在put_int_buffer中已全是字符，打印之前
; 把高位连续的字符去掉，比如把字符000123变成123
.ready_print:
    inc edi ; 此时edi退减为-1(0xffffffff)，加1使其为0

.skip_prefix_0:
    cmp edi, 8 ; 若已经比较第9个字符了
               ; 表示待打印的字符串为全0
    je .full0
; 找出连续的0字符, edi作为非0的最高位字符的偏移
.go_on_skip:
    mov cl, [put_int_buffer + edi]
    inc edi
    cmp cl, '0'
    je .skip_prefix_0 ; 继续判断下一位字符是否为字符0（不是数字0）
    dec edi ; edi在上面的inc操作中指向了下一个字符
            ; 若当前字符不为'0',要使edi减1恢复指向当前字符
    jmp .put_each_num

.full0:
    mov cl, '0' ; 输入的数字为全0时，则只打印0
.put_each_num:
    push ecx ; 此时cl中为可打印的字符
    call put_char
    add esp, 4
    inc edi ; 使edi指向下一个字符
    mov cl, [put_int_buffer + edi] ;  获取下一个字符到cl寄存器
    cmp edi, 8
    jl .put_each_num
    popad
    ret

; 字符串打印函数，基于put_char封装
; put_str 通过put_char来打印以0字符结尾的字符串
; 输入:栈中参数为打印的字符串
; 输出:无

put_str:
    ; 由于本函数中只用到了ebx和ecx，只备份这两个寄存器
    push ebx
    push ecx
    xor ecx, ecx ; 准备用ecx存储参数，清空
    mov ebx, [esp + 12] ; 从栈中得到待打印的字符串地址

.go_on:
    mov cl, [ebx]
    cmp cl, 0 ; 如果处理到了字符串尾，跳到结束处返回
    jz .str_over
    push ecx ; 为put_char函数传递参数
    call put_char
    add esp, 4 ; 回收参数所占的栈空间
    inc ebx; 使ebx指向下一个字符
    jmp .go_on

.str_over:
    pop ecx
    pop ebx
    ret

put_char:
    pushad ;备份32位寄存器环境 pushad指令备份32位寄存器的环境，按理说用到哪些寄存器就要备份哪些，我这里是偷懒行为，将8个32位全部备份了。
    ; PUSHAD是push all double，该指令压入所有双字长的寄存器，这里的“所有”一共是8个，它们的入栈先后顺序是：EAX->ECX->EDX->EBX-> ESP-> EBP->ESI->EDI，EAX是最先入栈。
    ;需要保证gs中为正确的视频段选择子
    ;为保险起见，每次打印时都为gs赋值
    mov ax, SELECTOR_VIDEO ; ; 不能直接把立即数送入段寄存器
    mov gs, ax

    ; 获取当前光标位置
    ; 先获得高8位
    mov dx, 0x03d4 ; 索引寄存器 端口地址为0x3D4的Address Register寄存器
    mov al, 0x0e ;用于提供光标位置的高8位 寄存器的索引
    out dx, al ; 向外围设备发送数据 写入索引 往端口地址为0x3D4的Address Register寄存器中写入寄存器的索引
    mov dx, 0x03d5 ; 通过读写数据端口0x3d5来获得或设置光标位置 

    in al, dx ; 得到了光标位置的高8位 从端口地址为0x3D5的Data Register寄存器读、写数据。 ，对于in指令，如果源操作是8位寄存器，目的操作数必须是al，如果源操作数是16位寄存器，目的操作数必须是ax。
    mov ah, al ; 存起来

    ; 再获取低8位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    in al, dx ; ，对于in指令，如果源操作是8位寄存器，目的操作数必须是al，如果源操作数是16位寄存器，目的操作数必须是ax。

    ; 将光标存入bx ah+al = ax
    mov bx, ax
    ; 下面这行是在栈中获取待打印的字符
    ; pushad压入4×8＝32字节
    ; 加上主调函数4字节的返回地址，故esp+36字节
    mov ecx, [esp + 36] 

    ; 寄存器ecx已经是待打印的参数了,由于字符的ASCII码只是1字节，所以只用寄存器cl就够了。
    cmp cl, 0xd ; CR是0x0d，LF是0x0a
    jz .is_carriage_return
    cmp cl, 0xa
    jz .is_line_feed

    cmp cl, 0x8 ; BS(backspace)的asc码是8
    jz .is_backspace
    jmp .put_other

;;;;;;;;;;;;       backspace的一点说明       ;;;;;;;;;;
; 当为backspace时，本质上只要将光标移向前一个显存位置即可.后面再输入的字符自然会覆盖此处的字符
; 但有可能在键入backspace后并不再键入新的字符，这时光标已经向前移动到待删除的字符位置，但字符还在原处
; 这就显得好怪异，所以此处添加了空格或空字符0
.is_backspace:
    dec bx ; 用dec指令先将bx减1，这样光标坐标便指向前一个字符了
    shl bx, 1 ; 光标左移1位等于乘2 SHL（左移）指令使目的操作数逻辑左移一位，最低位用0 填充
              ; 表示光标对应显存中的偏移字节
              ; 用shl指令将bx左移1位，相当于乘以2
              ; 此时bx为字符在显存中偏移量 已经乘以2了

    mov byte [gs:bx], 0x20 ; 将待删除的字节补为0或空格皆可 字符在显存中占2字节，低字节是ASCII码，高字符是属性 低字节处先把空格的ASCII码0x20写入
    ; inc bx
    mov byte [gs:bx+1], 0x07
    shr bx, 1 ; 目的操作数逻辑右移一位，最高位用0填充
    jmp .set_cursor ; 经过它的处理，光标才会显示在新位置

.put_other: 
    shl bx, 1 ; 光标位置用2字节表示，将光标值乘2
              ; 表示对应显存中的偏移字节
    mov [gs:bx], cl ; ASCII字符本身
    ; inc bx
    mov byte [gs:bx+1], 0x07 ; 字符属性
    shr bx, 1 ; 恢复老的光标值
    inc bx ; 下一个光标值
    cmp bx, 2000 ; cmp 指令把下次打印字符的坐标和2000比较，若小于2000则表示下次打印时，字符还会在当前屏幕的范围之内
    jl .set_cursor   ; 若光标值小于2000，表示未写到
                     ; 显存的最后，则去设置新的光标值
                     ; 若超出屏幕字符数大小（2000）
                     ; 则换行处理
.is_line_feed:       ; 是换行符LF(\n)
.is_carriage_return: ; 是回车符CR(\r)
    ; 如果是CR(\r)，只要把光标移到行首就行了
    xor dx, dx ; dx是被除数的高16位，清0
    mov ax, bx ;  ax是被除数的低16位
    mov si, 80 ;  由于是效仿Linux，Linux中\n表示
    ; 下一行的行首，所以本系统中
    ; 把\n和\r都处理为Linux中\n的意思
    ; 也就是下一行的行首
    div si 
    sub bx, dx ; 光标值减去除80的余数便是取整 行首字符的位置
    ; 以上4行处理\r的代码

.is_carriage_return_end: ;  回车符CR处理结束
    add bx, 80 ; 下一行的行首字符的位置
    cmp bx, 2000 ; 回车换行符处理流程中自己的滚屏判断，它和前面所说的滚屏流程无关，不要误以为是接上面的滚屏说的。
.is_line_feed_end: ; 若是LF(\n),将光标移+80便可 
    jl .set_cursor

;屏幕行范围是0～24，滚屏的原理是将屏幕的第1～24行搬运到第0～23行
; 再将第24行用空格填充
; 若超出屏幕大小，开始滚屏
.roll_screeen:
    cld 
    mov ecx, 960 ; 2000-80=1920个字符要搬运，共1920*2=3840字节
                 ; 一次搬4字节，共3840/4=960次

    mov esi, 0xc00b80a0 ; 第1行行首 虚拟地址
    mov edi, 0xc00b8000 ; 第0行行首 虚拟地址
    rep movsd

    ;;;;;;;将最后一行填充为空白 滚屏操作还差一步，需要将最后一行用空格填充
    mov ebx, 3840 ; 最后一行首字符的第一个字节偏移= 1920 * 2  
    mov ecx, 80 ; 一行是80字符（160字节），每次清空1字符
                ; （2字节），一行需要移动80次

.cls:
    mov word [gs:ebx], 0x0720 ; 0x0720是黑底白字的空格键
    add ebx, 2
    loop .cls
    mov bx, 1920 ; 将光标值重置为1920，最后一行的首字符

.set_cursor: ; 经过它的处理，光标才会显示在新位置
    ; 将光标设为bx值
    ;;;;;;; 1 先设置高8位 ;;;;;;;;
    mov dx, 0x03d4 ; 索引寄存器
    mov al, 0x0e ; 用于提供光标位置的高8位
    out dx, al
    mov dx, 0x03d5 ; 通过读写数据端口0x3d5来获得或设置光标位置
    mov al, bh
    out dx, al
    ;;;;;;; 2 再设置低8位 ;;;;;;;;;
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    mov al, bl
    out dx, al

.put_char_done:
    popad
    ret

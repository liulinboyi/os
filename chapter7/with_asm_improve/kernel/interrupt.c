/*
有关中断的内容都在此文件中实现。
*/

/*
8259A的编程就是写入ICW和OCW，其中ICW是初始化控制字，共4个，ICW1～ICW4，用于初始化8259A的各个功能。OCW是操作控制字，用于同初始化后的8259A进行操作命令交互。所以，对8259A的操作是在其初始化之后，对于8259A的初始化必须最先完成。
因为硬盘是接在了从片的引脚上，可参见图7-12，将来实现文件系统是离不开硬盘的，所以我们这里使用的8259A要采用主、从片级联的方式。在x86系统中，对于初始化级联8259A，4个ICW都需要，必须严格按照ICW1～4顺序写入。
ICW1和OCW2、OCW3是用偶地址端口0x20（主片）或0xA0（从片）写入。
ICW2～ICW4和OCW1是用奇地址端口0x21（主片）或0xA1（从片）写入。
*/

# include "stdint.h"
# include "global.h"
# include "io.h"
# include "interrupt.h"

# define IDT_DESC_CNT 0x21 // 目前总共支持的中断数33
# define PIC_M_CTRL 0x20 // 主片的控制端口是0x20
# define PIC_M_DATA 0x21 // 主片的数据端口是0x21
# define PIC_S_CTRL 0xa0 // 从片的控制端口是0xa0
# define PIC_S_DATA 0xa1 // 从片的数据端口是0xa1

/*中断门描述符结构体*/
/*
结构体中位置越偏下的成员，其地址越高。
struct gate_desc结构中成员的定义是参照中断门描述符来定义的，大伙参照中断门描述符的图自己对比一下，各成员名也一目了然
* 描述符都是8字节，不信您自己算算struct gate_desc结构体中成员的大小，总和便是8字节。
*/
struct gate_desc {
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t dcount; //此项为双字计数字段，是门描述符中的第4字节 //此项固定值，不用考虑

    uint8_t attribute;
    uint16_t func_offset_high_word;
};

// 静态函数声明,非必须
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
// 由于IDT属于全局数据结构，所以我们声明它为static类型
static struct gate_desc idt[IDT_DESC_CNT]; // idt是中断描述符表 //本质上就是个中断门描述符数组 它的数据类型正是struct gate_desc，数组大小是IDT_DESC_CNT。
void* inter_rupt_calls[IDT_DESC_CNT];
extern void* inter_rupt_call_in_asm;
// extern void* inter_rupt_call[IDT_DESC_CNT];
extern intr_handler intr_entry_table[IDT_DESC_CNT]; // 声明引用定义在kernel.S // 中的中断处理函数入口数组 // intr_entry_table中的元素都是普通地址

/**
 * 创建中断门描述符.
 用来创建中断门描述符的，它接受3个参数：中断门描述符的指针、中断描述符内的属性及中断描述符内对应的中断处理函数。
 
 原理是将后两个参数写入第1个参数所指向的中断门描述符中，实际上就是用后面的两个参数构建第1个参数指向的中断门描述符。
 */ 
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function) {
    p_gdesc->func_offset_low_word = (uint32_t) function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE; // 定义在global.h中
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t) function & 0xFFFF0000) >> 16;
}


void process(char type) {
    put_str("interrupt occur!\n");
    put_int(type);
    put_str("\n");
    put_int(20);
    put_str("\n");
    put_int(0x20);
    put_str("\n");
}

void inter_rupt_call_init() {
    int j;
    for(j = 0;j < IDT_DESC_CNT;j++) {
    	inter_rupt_calls[j] = process;
    }
    inter_rupt_call_in_asm = inter_rupt_calls;
}

/**
 * 初始化中断描述符表.
 idt_desc_init用来填充中断描述符表，这是重中之重。
 函数体中用了一个for循环，通过调用make_idt_desc函数在中断描述符表中创建了IDT_DESC_CNT个中断门描述符。
 */ 
static void idt_desc_init(void) {

    inter_rupt_call_init();
    
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++) {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]); // 第1个参数便是中断描述符表idt的数组成员指针  第2个参数IDT_DESC_ATTR_DPL0是描述符的属性  第3个参数是在kernel.asm中定义的中断描述符地址数组intr_entry_table中的元素值，即中断处理程序的地址。
    }
    put_str("idt_desc_init done.\n");
}

// 初始化8259A
/* 初始化可编程中断控制器8259A */
static void pic_init(void) {
    // 初始化主片
    outb(PIC_M_CTRL, 0x11); // 0x11: 00010001b ICW1: 边沿触发,级联8259, 需要ICW4 *第0位是IC4位，表示是否需要指定ICW4，我们需要在ICW4中设置EOI为手动方式，所以需要ICW4。由于我们要级联从片，所以将ICW1中的第1位SNGL置为0，表示级联。设置第3位的LTIM为0，表示边沿触发。ICW1中第4位是固定为1。其他位不设置。
    outb(PIC_M_DATA, 0x20); // ICW2: 起始中断向量号为0x20 // 也就是IR[0-7] 为 0x20 ～ 0x27 *ICW2专用于设置8259A的起始中断向量号，由于中断向量号0～31已经被占用或保留，从32起才可用，所以我们往主片PIC_M_DATA端口（主片的数据端口0x21）写入的ICW2值为0x20，即32。这说明主片的起始中断向量号为0x20，即IR0对应的中断向量号为0x20，这是我们的时钟所接入的引脚。IR1～IR7对应的中断向量号依次往下排。

    outb(PIC_M_DATA, 0x04); // ICW3: IR2接从片 ICW3专用于设置主从级联时用到的引脚。  *在设置级联8259A时，都是用IR2引脚来级联从片的 第2个引脚，即IR2，在ICW3中将其置为1，故ICW3值为0x04:00000100b，写入主片奇地址端口0x21，即PIC_M_DATA。
    outb(PIC_M_DATA, 0x01); // ICW4: 8086模式, 正常EOI *8259A的很多工作模式都在ICW4中设置，我们只要设置其中的第0位：μPM位，它设置当前处理器的类型，咱们所在的开发平台是x86，所以要将其置为1。此外还要设置ICW4的第1位：EOI的工作模式位。还记得EOI的作用吧，即End OF Interrupt，就是告诉8259A中断处理程序执行完了，8259A现在可以接受下一个中断信号啦。EOI的工作模式位就是设置发送EOI的方式。如果为1，8259A会自动结束中断，这里我们需要手动向8259A发送中断，所以将此位设置为0。其他位按默认就行了，所以ICW4的值为0x01。写入主片奇地址端口0x21，即PIC_M_DATA。00000001b

    /*初始化从片 */
    outb(PIC_S_CTRL, 0x11); // ICW1: 边沿触发,级联8259, 需要ICW4
    outb(PIC_S_DATA, 0x28); // ICW2: 起始中断向量号为0x28 // 也就是IR[8-15]为0x28 ～ 0x2F *由于主片的中断向量号是0x20～0x27，故从片的中断向量号顺着它延续下来，从0x28开始，即ICW2值为0x28，也就是IR[8-15] 为 0x28 ～ 0x2F。

    outb(PIC_S_DATA, 0x02); // ICW3: 设置从片连接到主片的IR2引脚 *ICW3专用于设置级联的引脚，这里设置从片连接在主片的哪个IRQ引脚上。刚才在设置主片的时候是设置用IR2引脚来级联从片，所以此处要告诉从片连接到主片的IR2上，即ICW2值为0x02
    outb(PIC_S_DATA, 0x01); // ICW4: 8086模式, 正常EOI

    /*打开主片上IR0,也就是目前只接受时钟产生的中断 */
    // 设置中断屏蔽寄存器IMR，只放行主片上IR0的时钟中断，屏蔽其他外部设备的中断。
    outb(PIC_M_DATA, 0xfe); // 往IMR寄存器中发送的命令控制字称为OCW1，主片上的OCW1为0xfe，即第0位为0，表示不屏蔽IR0的时钟中断。其他位都是1，表示都屏蔽。
    outb(PIC_S_DATA, 0xff); // 从片上的所有外设都屏蔽，所以发送的OCW1值为0xff。

    put_str("pic_init done.\n");
}

/*
完成有关中断的所有初始化工作
idt_init函数负责所有和中断相关的初始化工作，是中断初始化工作的主函数
中断描述符IDT本质上就是中断门描述符的数组，门描述符的结构咱们也定义好啦，它是由文件顶端的struct gate_desc来描述的。
*/
void idt_init() {
    put_str("idt_init start.\n");
    idt_desc_init(); // 初始化中断描述符表
    pic_init(); // 初始化8259A

    // 加载idt
    // 先用sizeof(idt) – 1得到idt的段界限limit，这用作低16位的段界限。
    // 接下来再将idt的地址挪到高32位即可，这可以通过把idt地址左移16位的形式实现。
    uint64_t base = (uint64_t) (uint32_t) idt;
    /*
    `000000000000000000000000000000 000000000000000000000000000000 0000
     | 与
                                                      000001000000 0100
     000000000000000000000000000000 000000000000000000000001000000 0100
     `
    */
    uint64_t idt_operand = ((sizeof(idt) - 1) | ( base << 16));
    asm volatile ("lidt %0" : : "m" (idt_operand));
    put_str("idt_init done.\n");
}

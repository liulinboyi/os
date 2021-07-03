# include "stdint.h"
# include "global.h"
# include "io.h"
# include "interrupt.h"

# define IDT_DESC_CNT 0x21
# define PIC_M_CTRL 0x20
# define PIC_M_DATA 0x21
# define PIC_S_CTRL 0xa0
# define PIC_S_DATA 0xa1

struct gate_desc {
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t dcount;

    uint8_t attribute;
    uint16_t func_offset_high_word;
};

/**
 * 中断的名称.
 */ 
char* intr_name[IDT_DESC_CNT]; // / 用于保存异常的名字
//定义中断处理程序数组，在kernel.S中定义的intrXXentry
//只是中断处理程序的入口，最终调用的是ide_table中的处理程序
intr_handler idt_table[IDT_DESC_CNT];
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static void init_custom_handler_name();
static struct gate_desc idt[IDT_DESC_CNT];

extern intr_handler intr_entry_table[IDT_DESC_CNT];

/**
 * 通用的中断处理函数.
 */ 
/* 通用的中断处理函数，一般用在异常出现时的处理 */
static void general_intr_handler(uint8_t vec_nr) {
    if (vec_nr == 0x27 || vec_nr == 0x2f) {
        // IRQ7和IRQ15会产生伪中断（spurious interrupt），无需处理
        // 0x2f是从片8259A上的最后一个IRQ引脚，保留项
        // 伪中断，无需处理
        return;
    }

    put_str("int vector: 0x");
    put_int(vec_nr);
    put_char('\n');
}

/**
 * 通用(默认)的异常/中断处理器注册.
 */
 /* 完成一般中断处理函数注册及异常名称注册 */
static void exception_handler_init(void) {
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++) {
    	/* idt_table数组中的函数是在进入中断后根据中断向量号调用的
    	* 见kernel/kernel.S的call [idt_table + %1*4] */
        idt_table[i] = general_intr_handler;
        // 默认为general_intr_handler
        // 以后会由register_handler来注册具体处理函数
        intr_name[i] = "unknown"; // 先统一赋值为unknown
    }

    init_custom_handler_name();
}

/**
 * 设置需要自定义的中断名称.
 */ 
static void init_custom_handler_name() {
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device Not Available Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    // intr_name[15] 第15项是intel保留项,未使用
    intr_name[16] = "#MF 0x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}

/**
 * 创建中断门描述符.
 */ 
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function) {
    p_gdesc->func_offset_low_word = (uint32_t) function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t) function & 0xFFFF0000) >> 16;
}

/**
 * 初始化中断描述符表.
 */ 
static void idt_desc_init(void) {
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++) {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    put_str("idt_desc_init done.\n");
}

static void pic_init(void) {
    // 初始化主片
    outb(PIC_M_CTRL, 0x11);
    outb(PIC_M_DATA, 0x20);

    outb(PIC_M_DATA, 0x04);
    outb(PIC_M_DATA, 0x01);

    outb(PIC_S_CTRL, 0x11);
    outb(PIC_S_DATA, 0x28);

    outb(PIC_S_DATA, 0x02);
    outb(PIC_S_DATA, 0x01);

    outb(PIC_M_DATA, 0xfe);
    outb(PIC_S_DATA, 0xff);

    put_str("pic_init done.\n");
}

/*完成有关中断的所有初始化工作*/
void idt_init() {
    put_str("idt_init start.\n");
    idt_desc_init(); // 初始化中断描述符表
    exception_handler_init(); // 异常名初始化并注册通常的中断处理函数
    pic_init(); // 初始化8259A

    // 加载idt
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t) ((uint32_t) idt << 16)));
    asm volatile ("lidt %0" : : "m" (idt_operand));
    put_str("idt_init done.\n");
}

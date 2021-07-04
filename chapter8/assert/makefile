# 定义了目录变量，即BUILD_DIR，它用来存储编译生成的所有目标文件
BUILD_DIR = build
# 第2行定义的变量是ENTRY_POINT，其值为0xc0001500，就是之前ld命令中-Ttext参数的值。
ENTRY_POINT = 0xc0001500
# 6～13行定义了编译器及编译参数，其中在第8行的参数变量CFLAGS中定义了-fno-builtin，它是告诉编译器不要采用内部函数，因为咱们在以后实现中会自定义与内部函数同名的函数，如果不添加此选项的话，编译时gcc会提示与内部函数冲突。-Wstrict-prototypes选项要求函数声明中必须有参数类型，否则编译时发出警告。-Wmissing-prototypes选项要求函数必须有声明，否则编译时发出警告。
AS = nasm
CC = gcc
LD = ld
LIB = -I lib/ -I lib/kernel/ -I lib/user/ -I kernel/ -I device/
ASIB = -I boot/include/
ASFLAGS = -f elf
CFLAGS = -Wall -m32 $(LIB) -c -fno-builtin -W -Wstrict-prototypes \
         -Wmissing-prototypes
# 第11行定义的变量OBJS用来存储所有的目标文件名，以后每增加一个目标文件，直接在此变量中增加就行了，此变量用在链接阶段。注意，最好不要用模式规则%.o来匹配，这样不能保证链接顺序，链接时的目标文件，位置顺序上最好还是调用在前，实现在后。
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
      $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o \
      $(BUILD_DIR)/debug.o

##############     c代码编译     ###############
$(BUILD_DIR)/main.o: kernel/main.c lib/kernel/print.h \
        lib/stdint.h kernel/init.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: kernel/init.c kernel/init.h lib/kernel/print.h \
        lib/stdint.h kernel/interrupt.h device/timer.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c kernel/interrupt.h \
        lib/stdint.h kernel/global.h lib/kernel/io.h lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: device/timer.c device/timer.h lib/stdint.h\
         lib/kernel/io.h lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o: kernel/debug.c kernel/debug.h \
        lib/kernel/print.h lib/stdint.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

##############    汇编代码编译    ###############

# 编译loader和mbr
$(BUILD_DIR)/mbr.bin: boot/mbr.S
	$(AS) $(ASIB) $< -o $@

$(BUILD_DIR)/loader.bin: boot/loader.S
	$(AS) $(ASIB) $< -o $@


$(BUILD_DIR)/kernel.o: kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/print.o: lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@

##############    链接所有目标文件    #############
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@
# 伪目标
.PHONY : mk_dir hd clean all

mk_dir:
	if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
	bximage -mode=create -hd=10M -q disk.img
# 伪目标hd 将build/kernel.bin写入硬盘，执行make hd是将文件写入硬盘。
hd:
	dd if=$(BUILD_DIR)/mbr.bin of=disk.img bs=512 count=1 conv=notrunc
	dd if=$(BUILD_DIR)/loader.bin of=disk.img bs=512 count=4 seek=2 conv=notrunc
	dd if=$(BUILD_DIR)/kernel.bin of=disk.img bs=512 count=200 seek=9 conv=notrunc

clean:
	rm -rf disk.img
	cd $(BUILD_DIR) && rm -f ./*

build: $(BUILD_DIR)/mbr.bin $(BUILD_DIR)/loader.bin $(BUILD_DIR)/kernel.bin

all: mk_dir build hd

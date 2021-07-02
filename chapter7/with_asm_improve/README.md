```
nasm -f elf -E -o kernel/kernel.S kernel/kernel.asm
```

```
b 0x000000000d13
n

info idt

```

```
gcc -m32 -o base_asm.bin base_asm.c


gcc -m32 -o reg_constraint.bin reg_constraint.c


gcc -m32 -o mem.bin mem.c

gcc -m32 -S -o mem.S ./mem.c

gcc -m32 -o reg4.bin ./reg4.c

gcc -m32 -o reg5.bin ./reg5.c

gcc -m32 -o reg6.bin ./reg6.c

gcc -m32 -o reg7.bin ./reg7.c

gcc -m32 -o reg8.bin ./reg8.c

gcc -m32 -o reg9.bin ./reg9.c

gcc -m32 -o mach_mode_warn.bin ./mach_mode_warn.c

gcc -m32 -o mach_mode.bin ./mach_mode.c
```

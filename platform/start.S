    .section .text.init
    .globl _start
    .type _start, @function

_start:
    // set stack
    la sp, __stack_top

    // set trap
    la      t0, trap_entry
    csrw    mtvec, t0

    // clear .bss
    la t0, __bss_start
    la t1, __bss_end

1:
    bgeu t0, t1, 2f
    sb zero, 0(t0)
    addi t0, t0, 1
    j 1b

2:
    call main

3:
    wfi
    j 3b

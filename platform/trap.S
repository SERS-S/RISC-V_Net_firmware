    .section .text.trap
    .globl trap_entry
    .type trap_entry, @function

trap_entry:
    addi    sp, sp, -272

    sd      ra,   0(sp)
    sd      gp,   8(sp)
    sd      tp,  16(sp)
    sd      t0,  24(sp)
    sd      t1,  32(sp)
    sd      t2,  40(sp)
    sd      s0,  48(sp)
    sd      s1,  56(sp)
    sd      a0,  64(sp)
    sd      a1,  72(sp)
    sd      a2,  80(sp)
    sd      a3,  88(sp)
    sd      a4,  96(sp)
    sd      a5, 104(sp)
    sd      a6, 112(sp)
    sd      a7, 120(sp)
    sd      s2, 128(sp)
    sd      s3, 136(sp)
    sd      s4, 144(sp)
    sd      s5, 152(sp)
    sd      s6, 160(sp)
    sd      s7, 168(sp)
    sd      s8, 176(sp)
    sd      s9, 184(sp)
    sd      s10, 192(sp)
    sd      s11, 200(sp)
    sd      t3, 208(sp)
    sd      t4, 216(sp)
    sd      t5, 224(sp)
    sd      t6, 232(sp)

    addi    t0, sp, 272
    sd      t0, 240(sp)

    csrr    t0, mepc
    sd      t0, 248(sp)

    csrr    t0, mcause
    sd      t0, 256(sp)

    csrr    t0, mtval
    sd      t0, 264(sp)

    mv      a0, sp
    call    trap_handler

1:
    wfi
    j       1b
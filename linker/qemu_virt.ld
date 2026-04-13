OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS
{
    . = ORIGIN(RAM);

    .text : ALIGN(16)
    {
        KEEP(*(.text.init))
        *(.text*)
        *(.rodata*)
    } > RAM

    .data : ALIGN(16)
    {
        *(.data*)
    } > RAM

    .bss (NOLOAD) : ALIGN(16)
    {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    } > RAM

    . = ALIGN(16);
    . += 16K;
    __stack_top = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
    }
}
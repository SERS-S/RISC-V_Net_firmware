#include "trap.h"
#include "uart.h"

static const char *trap_name(uint64_t mcause) 
{
    const uint64_t interrupt_bit = (1ULL << 63);
    const uint64_t is_interrupt = (mcause & interrupt_bit) != 0;
    const uint64_t code = mcause & ~interrupt_bit;

    if (is_interrupt) 
    {
        switch (code) 
        {
            case 3:  return "machine software interrupt";
            case 7:  return "machine timer interrupt";
            case 11: return "machine external interrupt";
            default: return "unknown interrupt";
        }
    }

    switch (code) 
    {
        case 0:  return "instruction address misaligned";
        case 1:  return "instruction access fault";
        case 2:  return "illegal instruction";
        case 3:  return "breakpoint";
        case 4:  return "load address misaligned";
        case 5:  return "load access fault";
        case 6:  return "store/AMO address misaligned";
        case 7:  return "store/AMO access fault";
        case 8:  return "environment call from U-mode";
        case 11: return "environment call from M-mode";
        default: return "unknown exception";
    }
}

__attribute__((noreturn)) void trap_handler(struct trap_frame *tf)
{
    uart_puts("\n");
    uart_puts("=== TRAP ===\n");
    uart_puts("name   : ");
    uart_puts(trap_name(tf->mcause));
    uart_puts("\n");

    uart_puts("mcause : ");
    uart_puthex64(tf->mcause);
    uart_puts("\n");

    uart_puts("mepc   : ");
    uart_puthex64(tf->mepc);
    uart_puts("\n");

    uart_puts("mtval  : ");
    uart_puthex64(tf->mtval);
    uart_puts("\n");

    uart_puts("ra     : ");
    uart_puthex64(tf->ra);
    uart_puts("\n");

    uart_puts("sp     : ");
    uart_puthex64(tf->sp);
    uart_puts("\n");

    uart_puts("a0     : ");
    uart_puthex64(tf->a0);
    uart_puts("\n");

    uart_puts("a1     : ");
    uart_puthex64(tf->a1);
    uart_puts("\n");

    for (;;) 
    {
        __asm__ volatile("wfi");
    }
}
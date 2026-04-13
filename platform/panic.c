#include "panic.h"
#include "uart.h"

__attribute__((noreturn)) void panic(const char *msg) 
{
    uart_puts("\n");
    uart_puts("### PANIC ###\n");

    if (msg) 
    {
        uart_puts(msg);
        uart_puts("\n");
    }

    for (;;) __asm__ volatile("wfi");
}
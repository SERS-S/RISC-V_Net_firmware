#include "uart.h"
#include "qemu_virt.h"

#include <stdint.h>

#define UART_RBR 0   // receive buffer register (read)
#define UART_THR 0   // transmit holding register (write)
#define UART_DLL 0   // divisor latch low (when DLAB=1)
#define UART_IER 1   // interrupt enable register
#define UART_DLM 1   // divisor latch high (when DLAB=1)
#define UART_FCR 2   // FIFO control register
#define UART_LCR 3   // line control register
#define UART_LSR 5   // line status register

#define UART_LCR_DLAB 0x80
#define UART_LCR_8N1  0x03

#define UART_LSR_THRE 0x20


static inline void uart_write(uintptr_t reg, uint8_t value) 
{
    *(volatile uint8_t*)(UART0_BASE + reg) = value;
}

static inline uint8_t uart_read(uintptr_t reg)
{
    return *(volatile uint8_t*)(UART0_BASE + reg);
}

void uart_init(void)
{
    uart_write(UART_IER, 0x00);

    uart_write(UART_LCR, UART_LCR_DLAB);
    uart_write(UART_DLL, 0x02);
    uart_write(UART_DLM, 0x00);

    uart_write(UART_LCR, UART_LCR_8N1);

    uart_write(UART_FCR, 0x07);
}

void uart_putc(char c) 
{
    while ((uart_read(UART_LSR) & UART_LSR_THRE) == 0) 
    {
        // wait
    }

    uart_write(UART_THR, (uint8_t)c);
}

void uart_puts(const char *s)
{
    while (*s) 
    {
        if (*s == '\n') 
        {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}

void uart_puthex64(uint64_t value) 
{
    static const char hex[] = "0123456789abcdef";

    uart_puts("0x");
    for (int shift = 60; shift >= 0; shift -= 4) 
    {
        uart_putc(hex[(value >> shift) & 0xFULL]);
    }
}

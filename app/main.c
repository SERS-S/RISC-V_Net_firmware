#include <stdint.h>

#include "virtio_mmio.h"
#include "virtio_net.h"
#include "panic.h"
#include "uart.h"

#ifndef TEST_EBREAK
#define TEST_EBREAK 0
#endif

#ifndef TEST_PANIC
#define TEST_PANIC 0
#endif

static volatile uint64_t g_bss_probe;
static volatile uint64_t g_data_probe = 0x1122334455667788ULL;

int main(void) 
{
    uart_init();

    uart_puts("\n");
    uart_puts("hello-uart: RISC-V bare-metal on QEMU virt\n");

    uart_puts("g_data_probe = ");
    uart_puthex64(g_data_probe);
    uart_puts("\n");

    uart_puts("g_bss_probe  = ");
    uart_puthex64(g_bss_probe);
    uart_puts("\n");

    if (g_bss_probe != 0) 
    {
        panic("BSS is not zero");
    }
    uart_puts("BSS clear OK\n");
    uart_puts("mtvec installed\n");

    virtio_mmio_scan();
    VIRTIO_MMIO_DEVICE net_device;
    if (!virtio_mmio_find_device(VIRTIO_DEVICE_ID_NET, &net_device))
    {
        panic("virtio-net device not found");
    }

    if (!virtio_net_init(&net_device))
    {
        panic("virtio-net init failed");
    }

    if (!virtio_net_send_test_frame())
    {
        uart_puts("virtio-net tx test frame pending or failed\n");
    }

    #if TEST_PANIC
        panic("manual panic requested");
    #endif

    #if TEST_EBREAK
        uart_puts("triggering EBREAK...\n");
        __asm__ volatile("ebreak");
    #endif

    for (;;)
    {
        virtio_net_poll_rx();
        virtio_net_poll_tx();
    }

    return 0;
}

#include "virtio_net.h"

#include "eth.h"
#include "stats.h"
#include "uart.h"
#include "virtqueue.h"

#define VIRTIO_NET_RX_QUEUE 0U
#define VIRTIO_NET_TX_QUEUE 1U

#define VIRTIO_NET_RX_BUFFERS 4U
#define VIRTIO_NET_BUFFER_SIZE 2048U
#define VIRTIO_NET_HDR_SIZE 10U

typedef struct virtio_net_hdr
{
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t csum_start;
    uint16_t csum_offset;
} __attribute__((packed)) VIRTIO_NET_HDR;

static VIRTIO_MMIO_DEVICE g_device;
static VIRTQUEUE g_rx_vq;
static VIRTQUEUE g_tx_vq;
static NETIF g_netif;
static uint8_t g_mac[6];
static int g_ready;
static int g_tx_busy;

static uint8_t g_rx_queue_mem[VIRTQUEUE_MEM_SIZE] __attribute__((aligned(VIRTQUEUE_ALIGN)));
static uint8_t g_tx_queue_mem[VIRTQUEUE_MEM_SIZE] __attribute__((aligned(VIRTQUEUE_ALIGN)));
static uint8_t g_rx_buffers[VIRTIO_NET_RX_BUFFERS][VIRTIO_NET_BUFFER_SIZE] __attribute__((aligned(16)));
static uint8_t g_tx_buffer[VIRTIO_NET_BUFFER_SIZE] __attribute__((aligned(16)));

static void memzero(void *ptr, uintptr_t len)
{
    uint8_t *p = (uint8_t *)ptr;

    for (uintptr_t i = 0; i < len; ++i)
    {
        p[i] = 0;
    }
}

static void copy_bytes(uint8_t *dst, const uint8_t *src, uint16_t len)
{
    for (uint16_t i = 0; i < len; ++i)
    {
        dst[i] = src[i];
    }
}

static void print_status(const char *label)
{
    uart_puts(label);
    uart_puthex64(virtio_mmio_read_status(&g_device));
    uart_puts("\n");
}

static void print_mac(void)
{
    static const char hex[] = "0123456789abcdef";

    for (uintptr_t i = 0; i < 6; ++i)
    {
        if (i != 0)
        {
            uart_putc(':');
        }

        uart_putc(hex[(g_mac[i] >> 4) & 0x0fU]);
        uart_putc(hex[g_mac[i] & 0x0fU]);
    }
}

static void read_mac(uint32_t features0)
{
    if ((features0 & (1U << VIRTIO_NET_F_MAC)) != 0)
    {
        for (uintptr_t i = 0; i < 6; ++i)
        {
            g_mac[i] = virtio_mmio_config_read8(&g_device, i);
        }
        return;
    }

    g_mac[0] = 0x02;
    g_mac[1] = 0x00;
    g_mac[2] = 0x00;
    g_mac[3] = 0x00;
    g_mac[4] = 0x00;
    g_mac[5] = 0x01;
}

static void setup_netif(void)
{
    copy_bytes(g_netif.mac, g_mac, ETH_ADDR_LEN);
    g_netif.ipv4_addr = NETIF_IPV4_ADDR(192, 168, 100, 2);
    g_netif.ipv4_mask = NETIF_IPV4_ADDR(255, 255, 255, 0);
    g_netif.tx = virtio_net_tx;
}

static int post_rx_buffer(uint16_t id)
{
    return virtqueue_add_buffer(&g_rx_vq,
                                id,
                                (uintptr_t)g_rx_buffers[id],
                                VIRTIO_NET_BUFFER_SIZE,
                                VIRTQ_DESC_F_WRITE);
}

static int post_initial_rx_buffers(void)
{
    for (uint16_t i = 0; i < VIRTIO_NET_RX_BUFFERS; ++i)
    {
        if (!post_rx_buffer(i))
        {
            return 0;
        }
    }

    virtio_mmio_notify_queue(&g_device, VIRTIO_NET_RX_QUEUE);
    return 1;
}

int virtio_net_init(VIRTIO_MMIO_DEVICE *device)
{
    const uint32_t features0 = virtio_mmio_read_device_features(device, 0);
    const uint32_t features1 = virtio_mmio_read_device_features(device, 1);
    uint32_t driver_features0 = 0;
    uint32_t driver_features1 = 0;

    if (device->device_id != VIRTIO_DEVICE_ID_NET)
    {
        uart_puts("virtio-net: wrong device id\n");
        return 0;
    }

    g_device = *device;
    g_ready = 0;
    g_tx_busy = 0;

    uart_puts("virtio-net init\n");

    virtio_mmio_write_status(&g_device, 0);
    print_status("  reset    : ");

    virtio_mmio_set_status(&g_device, VIRTIO_STATUS_ACKNOWLEDGE);
    print_status("  ack      : ");

    virtio_mmio_set_status(&g_device, VIRTIO_STATUS_DRIVER);
    print_status("  driver   : ");

    if ((features0 & (1U << VIRTIO_NET_F_MAC)) != 0)
    {
        driver_features0 |= (1U << VIRTIO_NET_F_MAC);
    }

    if ((g_device.version == VIRTIO_MMIO_VERSION_MODERN) &&
        ((features1 & (1U << (VIRTIO_F_VERSION_1 - 32U))) != 0))
    {
        driver_features1 |= (1U << (VIRTIO_F_VERSION_1 - 32U));
    }

    virtio_mmio_write_driver_features(&g_device, 0, driver_features0);
    virtio_mmio_write_driver_features(&g_device, 1, driver_features1);

    uart_puts("  driver features0: ");
    uart_puthex64(driver_features0);
    uart_puts("\n");

    uart_puts("  driver features1: ");
    uart_puthex64(driver_features1);
    uart_puts("\n");

    if (g_device.version == VIRTIO_MMIO_VERSION_MODERN)
    {
        virtio_mmio_set_status(&g_device, VIRTIO_STATUS_FEATURES_OK);
        print_status("  features : ");

        if ((virtio_mmio_read_status(&g_device) & VIRTIO_STATUS_FEATURES_OK) == 0)
        {
            virtio_mmio_set_status(&g_device, VIRTIO_STATUS_FAILED);
            uart_puts("  features : rejected\n");
            return 0;
        }
    }
    else
    {
        uart_puts("  features : legacy device, FEATURES_OK skipped\n");
    }

    read_mac(features0);
    setup_netif();

    uart_puts("  mac      : ");
    print_mac();
    uart_puts("\n");

    virtqueue_init(&g_rx_vq, VIRTIO_NET_RX_QUEUE, g_rx_queue_mem);
    virtqueue_init(&g_tx_vq, VIRTIO_NET_TX_QUEUE, g_tx_queue_mem);

    if (!virtio_mmio_setup_queue(&g_device,
                                 VIRTIO_NET_RX_QUEUE,
                                 VIRTQUEUE_SIZE,
                                 virtqueue_mem_addr(&g_rx_vq)))
    {
        return 0;
    }
    uart_puts("  rx queue : ready\n");

    if (!virtio_mmio_setup_queue(&g_device,
                                 VIRTIO_NET_TX_QUEUE,
                                 VIRTQUEUE_SIZE,
                                 virtqueue_mem_addr(&g_tx_vq)))
    {
        return 0;
    }
    uart_puts("  tx queue : ready\n");

    if (!post_initial_rx_buffers())
    {
        uart_puts("virtio-net: failed to post rx buffers\n");
        return 0;
    }
    uart_puts("  rx bufs  : posted\n");

    virtio_mmio_set_status(&g_device, VIRTIO_STATUS_DRIVER_OK);
    print_status("  ready    : ");

    if ((virtio_mmio_read_status(&g_device) & VIRTIO_STATUS_FAILED) != 0)
    {
        uart_puts("virtio-net init failed\n");
        return 0;
    }

    g_ready = 1;
    uart_puts("virtio-net init OK\n");
    return 1;
}

NETIF *virtio_net_netif(void)
{
    return &g_netif;
}

void virtio_net_poll_tx(void)
{
    uint32_t id;
    uint32_t len;

    while (virtqueue_get_used(&g_tx_vq, &id, &len))
    {
        g_tx_busy = 0;
    }
}

int virtio_net_tx(const void *frame, uint16_t len)
{
    uint32_t total_len;

    if (!g_ready || !frame || (len > ETH_FRAME_MAX))
    {
        return 0;
    }

    for (uint32_t i = 0; g_tx_busy && (i < 100000U); ++i)
    {
        virtio_net_poll_tx();
    }

    if (g_tx_busy)
    {
        uart_puts("virtio-net tx busy\n");
        return 0;
    }

    memzero(g_tx_buffer, VIRTIO_NET_HDR_SIZE);
    copy_bytes(g_tx_buffer + VIRTIO_NET_HDR_SIZE, (const uint8_t *)frame, len);
    total_len = VIRTIO_NET_HDR_SIZE + len;

    if (!virtqueue_add_buffer(&g_tx_vq, 0, (uintptr_t)g_tx_buffer, total_len, 0))
    {
        return 0;
    }

    g_tx_busy = 1;
    virtio_mmio_notify_queue(&g_device, VIRTIO_NET_TX_QUEUE);

    for (uint32_t i = 0; g_tx_busy && (i < 100000U); ++i)
    {
        virtio_net_poll_tx();
    }

    return !g_tx_busy;
}

void virtio_net_poll_rx(void)
{
    uint32_t id;
    uint32_t len;

    while (virtqueue_get_used(&g_rx_vq, &id, &len))
    {
        if (id < VIRTIO_NET_RX_BUFFERS)
        {
            if (len > VIRTIO_NET_HDR_SIZE)
            {
                const uint8_t *frame = g_rx_buffers[id] + VIRTIO_NET_HDR_SIZE;
                const uint16_t frame_len = (uint16_t)(len - VIRTIO_NET_HDR_SIZE);
                ++g_net_stats.rx_frames;
                eth_input(&g_netif, frame, frame_len);
            }
            else
            {
                ++g_net_stats.drop_bad_len;
            }

            post_rx_buffer((uint16_t)id);
            virtio_mmio_notify_queue(&g_device, VIRTIO_NET_RX_QUEUE);
        }
    }
}

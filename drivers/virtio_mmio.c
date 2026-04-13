#include "virtio_mmio.h"

#include "qemu_virt.h"
#include "uart.h"

#define VIRTIO_MMIO_MAGIC_VALUE_OFF      0x000
#define VIRTIO_MMIO_VERSION_OFF          0x004
#define VIRTIO_MMIO_DEVICE_ID_OFF        0x008
#define VIRTIO_MMIO_VENDOR_ID_OFF        0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES_OFF  0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL  0x014
#define VIRTIO_MMIO_DRIVER_FEATURES_OFF  0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL  0x024
#define VIRTIO_MMIO_GUEST_PAGE_SIZE_OFF  0x028
#define VIRTIO_MMIO_QUEUE_SEL_OFF        0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX_OFF    0x034
#define VIRTIO_MMIO_QUEUE_NUM_OFF        0x038
#define VIRTIO_MMIO_QUEUE_ALIGN_OFF      0x03c
#define VIRTIO_MMIO_QUEUE_PFN_OFF        0x040
#define VIRTIO_MMIO_STATUS_OFF           0x070
#define VIRTIO_MMIO_QUEUE_NOTIFY_OFF     0x050
#define VIRTIO_MMIO_CONFIG_OFF           0x100

uint8_t mmio_read8(uintptr_t base, uintptr_t offset)
{
    return *(volatile uint8_t *)(base + offset);
}

uint32_t mmio_read32(uintptr_t base, uintptr_t offset)
{
    return *(volatile uint32_t *)(base + offset);
}

void mmio_write32(uintptr_t base, uintptr_t offset, uint32_t value)
{
    *(volatile uint32_t *)(base + offset) = value;
}

int virtio_mmio_probe(uintptr_t base, VIRTIO_MMIO_DEVICE *device)
{
    const uint32_t magic = mmio_read32(base, VIRTIO_MMIO_MAGIC_VALUE_OFF);
    const uint32_t version = mmio_read32(base, VIRTIO_MMIO_VERSION_OFF);
    const uint32_t device_id = mmio_read32(base, VIRTIO_MMIO_DEVICE_ID_OFF);

    if (magic != VIRTIO_MMIO_MAGIC_VALUE)
    {
        return 0;
    }

    if ((version != VIRTIO_MMIO_VERSION_LEGACY) &&
        (version != VIRTIO_MMIO_VERSION_MODERN))
    {
        return 0;
    }

    if (device_id == 0)
    {
        return 0;
    }

    if (device)
    {
        device->base = base;
        device->magic = magic;
        device->version = version;
        device->device_id = device_id;
        device->vendor_id = mmio_read32(base, VIRTIO_MMIO_VENDOR_ID_OFF);
        device->status = mmio_read32(base, VIRTIO_MMIO_STATUS_OFF);
    }

    return 1;
}

static const char *virtio_device_name(uint32_t device_id)
{
    switch (device_id)
    {
        case VIRTIO_DEVICE_ID_NET:
            return "net";
        default:
            return "unknown";
    }
}

static void uart_puthex8(uint8_t value)
{
    static const char hex[] = "0123456789abcdef";

    uart_putc(hex[(value >> 4) & 0x0fU]);
    uart_putc(hex[value & 0x0fU]);
}

uint8_t virtio_mmio_config_read8(const VIRTIO_MMIO_DEVICE *device, uintptr_t offset)
{
    return mmio_read8(device->base, VIRTIO_MMIO_CONFIG_OFF + offset);
}

uint32_t virtio_mmio_read_device_features(const VIRTIO_MMIO_DEVICE *device, uint32_t sel)
{
    mmio_write32(device->base, VIRTIO_MMIO_DEVICE_FEATURES_SEL, sel);
    return mmio_read32(device->base, VIRTIO_MMIO_DEVICE_FEATURES_OFF);
}

void virtio_mmio_write_driver_features(const VIRTIO_MMIO_DEVICE *device, uint32_t sel, uint32_t value)
{
    mmio_write32(device->base, VIRTIO_MMIO_DRIVER_FEATURES_SEL, sel);
    mmio_write32(device->base, VIRTIO_MMIO_DRIVER_FEATURES_OFF, value);
}

uint32_t virtio_mmio_read_status(const VIRTIO_MMIO_DEVICE *device)
{
    return mmio_read32(device->base, VIRTIO_MMIO_STATUS_OFF);
}

void virtio_mmio_write_status(const VIRTIO_MMIO_DEVICE *device, uint32_t status)
{
    mmio_write32(device->base, VIRTIO_MMIO_STATUS_OFF, status);
}

void virtio_mmio_set_status(const VIRTIO_MMIO_DEVICE *device, uint32_t bit)
{
    virtio_mmio_write_status(device, virtio_mmio_read_status(device) | bit);
}

static void virtio_mmio_dump_mac(uintptr_t base, uint32_t features0)
{
    if ((features0 & (1U << VIRTIO_NET_F_MAC)) == 0)
    {
        uart_puts("  mac      : feature not offered\n");
        return;
    }

    uart_puts("  mac      : ");
    for (uintptr_t i = 0; i < 6; ++i)
    {
        if (i != 0)
        {
            uart_putc(':');
        }
        uart_puthex8(mmio_read8(base, VIRTIO_MMIO_CONFIG_OFF + i));
    }
    uart_puts("\n");
}

void virtio_mmio_dump(const VIRTIO_MMIO_DEVICE *device)
{
    const uintptr_t base = device->base;
    const uint32_t features0 = virtio_mmio_read_device_features(device, 0);
    const uint32_t features1 = virtio_mmio_read_device_features(device, 1);

    mmio_write32(base, VIRTIO_MMIO_QUEUE_SEL_OFF, 0);

    uart_puts("virtio-mmio device\n");

    uart_puts("  base     : ");
    uart_puthex64(base);
    uart_puts("\n");

    uart_puts("  magic    : ");
    uart_puthex64(device->magic);
    uart_puts("\n");

    uart_puts("  version  : ");
    uart_puthex64(device->version);
    uart_puts("\n");

    uart_puts("  device   : ");
    uart_puthex64(device->device_id);
    uart_puts(" (");
    uart_puts(virtio_device_name(device->device_id));
    uart_puts(")\n");

    uart_puts("  vendor   : ");
    uart_puthex64(device->vendor_id);
    uart_puts("\n");

    uart_puts("  status   : ");
    uart_puthex64(device->status);
    uart_puts("\n");

    uart_puts("  features0: ");
    uart_puthex64(features0);
    uart_puts("\n");

    uart_puts("  features1: ");
    uart_puthex64(features1);
    uart_puts("\n");

    uart_puts("  q0 max   : ");
    uart_puthex64(mmio_read32(base, VIRTIO_MMIO_QUEUE_NUM_MAX_OFF));
    uart_puts("\n");

    if (device->device_id == VIRTIO_DEVICE_ID_NET)
    {
        virtio_mmio_dump_mac(base, features0);
    }
}

int virtio_mmio_setup_queue(const VIRTIO_MMIO_DEVICE *device,
                            uint32_t queue_index,
                            uint32_t queue_size,
                            uintptr_t queue_addr)
{
    const uintptr_t base = device->base;
    uint32_t max_size;

    if (device->version != VIRTIO_MMIO_VERSION_LEGACY)
    {
        uart_puts("virtio-mmio: modern queue setup is not implemented yet\n");
        return 0;
    }

    if ((queue_addr & (VIRTIO_MMIO_QUEUE_ALIGN - 1U)) != 0)
    {
        uart_puts("virtio-mmio: queue memory is not page-aligned\n");
        return 0;
    }

    mmio_write32(base, VIRTIO_MMIO_GUEST_PAGE_SIZE_OFF, VIRTIO_MMIO_QUEUE_ALIGN);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_SEL_OFF, queue_index);

    if (mmio_read32(base, VIRTIO_MMIO_QUEUE_PFN_OFF) != 0)
    {
        uart_puts("virtio-mmio: queue already in use\n");
        return 0;
    }

    max_size = mmio_read32(base, VIRTIO_MMIO_QUEUE_NUM_MAX_OFF);
    if ((max_size == 0) || (queue_size == 0) || (queue_size > max_size))
    {
        uart_puts("virtio-mmio: invalid queue size\n");
        return 0;
    }

    mmio_write32(base, VIRTIO_MMIO_QUEUE_NUM_OFF, queue_size);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_ALIGN_OFF, VIRTIO_MMIO_QUEUE_ALIGN);
    mmio_write32(base, VIRTIO_MMIO_QUEUE_PFN_OFF, (uint32_t)(queue_addr / VIRTIO_MMIO_QUEUE_ALIGN));

    return 1;
}

void virtio_mmio_notify_queue(const VIRTIO_MMIO_DEVICE *device, uint32_t queue_index)
{
    mmio_write32(device->base, VIRTIO_MMIO_QUEUE_NOTIFY_OFF, queue_index);
}

int virtio_mmio_find_device(uint32_t device_id, VIRTIO_MMIO_DEVICE *device)
{
    for (uintptr_t i = 0; i < VIRTIO_MMIO_COUNT; ++i)
    {
        VIRTIO_MMIO_DEVICE candidate;
        const uintptr_t base = VIRTIO_MMIO_BASE + (i * VIRTIO_MMIO_STRIDE);

        if (virtio_mmio_probe(base, &candidate) && (candidate.device_id == device_id))
        {
            if (device)
            {
                *device = candidate;
            }
            return 1;
        }
    }

    return 0;
}

void virtio_mmio_scan(void)
{
    int found = 0;

    uart_puts("virtio-mmio scan\n");

    for (uintptr_t i = 0; i < VIRTIO_MMIO_COUNT; ++i)
    {
        VIRTIO_MMIO_DEVICE device;
        const uintptr_t base = VIRTIO_MMIO_BASE + (i * VIRTIO_MMIO_STRIDE);

        if (virtio_mmio_probe(base, &device))
        {
            found = 1;
            virtio_mmio_dump(&device);
        }
    }

    if (!found)
    {
        uart_puts("virtio-mmio: no devices found\n");
    }
}

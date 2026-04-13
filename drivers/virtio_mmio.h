#ifndef VIRTIO_MMIO_H
#define VIRTIO_MMIO_H

#include <stdint.h>

#define VIRTIO_MMIO_MAGIC_VALUE 0x74726976U
#define VIRTIO_MMIO_VERSION_LEGACY 1U
#define VIRTIO_MMIO_VERSION_MODERN 2U

#define VIRTIO_DEVICE_ID_NET 1U

#define VIRTIO_NET_F_MAC 5U
#define VIRTIO_F_VERSION_1 32U

#define VIRTIO_STATUS_ACKNOWLEDGE 0x01U
#define VIRTIO_STATUS_DRIVER      0x02U
#define VIRTIO_STATUS_DRIVER_OK   0x04U
#define VIRTIO_STATUS_FEATURES_OK 0x08U
#define VIRTIO_STATUS_FAILED      0x80U

#define VIRTIO_MMIO_QUEUE_ALIGN 4096U

typedef struct virtio_mmio_device
{
    uintptr_t base;
    uint32_t magic;
    uint32_t version;
    uint32_t device_id;
    uint32_t vendor_id;
    uint32_t status;
} VIRTIO_MMIO_DEVICE;

uint8_t mmio_read8(uintptr_t base, uintptr_t offset);
uint32_t mmio_read32(uintptr_t base, uintptr_t offset);
void mmio_write32(uintptr_t base, uintptr_t offset, uint32_t value);

int virtio_mmio_probe(uintptr_t base, VIRTIO_MMIO_DEVICE *device);
int virtio_mmio_find_device(uint32_t device_id, VIRTIO_MMIO_DEVICE *device);
void virtio_mmio_dump(const VIRTIO_MMIO_DEVICE *device);
void virtio_mmio_scan(void);

uint8_t virtio_mmio_config_read8(const VIRTIO_MMIO_DEVICE *device, uintptr_t offset);
uint32_t virtio_mmio_read_device_features(const VIRTIO_MMIO_DEVICE *device, uint32_t sel);
void virtio_mmio_write_driver_features(const VIRTIO_MMIO_DEVICE *device, uint32_t sel, uint32_t value);
uint32_t virtio_mmio_read_status(const VIRTIO_MMIO_DEVICE *device);
void virtio_mmio_write_status(const VIRTIO_MMIO_DEVICE *device, uint32_t status);
void virtio_mmio_set_status(const VIRTIO_MMIO_DEVICE *device, uint32_t bit);
int virtio_mmio_setup_queue(const VIRTIO_MMIO_DEVICE *device,
                            uint32_t queue_index,
                            uint32_t queue_size,
                            uintptr_t queue_addr);
void virtio_mmio_notify_queue(const VIRTIO_MMIO_DEVICE *device, uint32_t queue_index);

#endif

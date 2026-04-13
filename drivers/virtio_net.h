#ifndef VIRTIO_NET_H
#define VIRTIO_NET_H

#include "virtio_mmio.h"

#include <stdint.h>

int virtio_net_init(VIRTIO_MMIO_DEVICE *device);
int virtio_net_send_test_frame(void);
void virtio_net_poll_rx(void);
void virtio_net_poll_tx(void);

#endif

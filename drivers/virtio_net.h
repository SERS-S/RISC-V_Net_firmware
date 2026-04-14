#ifndef VIRTIO_NET_H
#define VIRTIO_NET_H

#include "virtio_mmio.h"
#include "netif.h"

#include <stdint.h>

int virtio_net_init(VIRTIO_MMIO_DEVICE *device);
NETIF *virtio_net_netif(void);
int virtio_net_tx(const void *frame, uint16_t len);
void virtio_net_poll_rx(void);
void virtio_net_poll_tx(void);

#endif

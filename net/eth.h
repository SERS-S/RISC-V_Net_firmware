#ifndef ETH_H
#define ETH_H

#include "netif.h"

#include <stdint.h>

#define ETH_ADDR_LEN 6U
#define ETH_HDR_LEN  14U
#define ETH_MTU      1500U
#define ETH_FRAME_MIN 60U
#define ETH_FRAME_MAX (ETH_HDR_LEN + ETH_MTU)

#define ETH_TYPE_IPV4 0x0800U
#define ETH_TYPE_ARP  0x0806U

typedef struct eth_hdr
{
    uint8_t dst[ETH_ADDR_LEN];
    uint8_t src[ETH_ADDR_LEN];
    uint16_t type;
} __attribute__((packed)) ETH_HDR;

void eth_input(NETIF *nif, const uint8_t *frame, uint16_t len);
int eth_output(NETIF *nif,
               const uint8_t dst_mac[ETH_ADDR_LEN],
               uint16_t eth_type,
               const void *payload,
               uint16_t payload_len);

#endif

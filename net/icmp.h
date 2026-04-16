#ifndef ICMP_H
#define ICMP_H

#include "eth.h"
#include "netif.h"

#include <stdint.h>

#define ICMP_ECHO_REPLY   0U
#define ICMP_ECHO_REQUEST 8U

#define ICMP_ECHO_HDR_LEN 8U
#define ICMP_MAX_PACKET   (ETH_MTU - 20U)

typedef struct icmp_echo_hdr
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} __attribute__((packed)) ICMP_ECHO_HDR;

void icmp_input(NETIF *nif,
                const uint8_t src_mac[ETH_ADDR_LEN],
                uint32_t src_ip,
                uint32_t dst_ip,
                const uint8_t *packet,
                uint16_t len);

#endif

#ifndef UDP_H
#define UDP_H

#include "eth.h"
#include "netif.h"

#include <stdint.h>

#define UDP_HDR_LEN   8U
#define UDP_ECHO_PORT 12345U
#define UDP_MAX_PACKET (ETH_MTU - 20U)

typedef struct udp_hdr
{
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) UDP_HDR;

uint16_t udp_checksum_ipv4(uint32_t src_ip,
                           uint32_t dst_ip,
                           const void *udp_packet,
                           uint16_t udp_len);

void udp_input(NETIF *nif,
               const uint8_t src_mac[ETH_ADDR_LEN],
               uint32_t src_ip,
               uint32_t dst_ip,
               const uint8_t *packet,
               uint16_t len);

#endif

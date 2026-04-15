#ifndef IPV4_H
#define IPV4_H

#include "eth.h"
#include "netif.h"

#include <stdint.h>

#define IPV4_HDR_MIN_LEN 20U
#define IPV4_VERSION     4U
#define IPV4_IHL_NO_OPT  5U
#define IPV4_TTL_DEFAULT 64U

#define IPV4_PROTO_ICMP 1U
#define IPV4_PROTO_UDP  17U

#define IPV4_FLAG_MORE_FRAGMENTS 0x2000U
#define IPV4_FRAGMENT_OFFSET_MASK 0x1fffU
#define IPV4_MAX_PACKET ETH_MTU
#define IPV4_MAX_PAYLOAD (IPV4_MAX_PACKET - IPV4_HDR_MIN_LEN)

typedef struct ipv4_hdr
{
    uint8_t version_ihl;
    uint8_t dscp_ecn;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t header_checksum;
    uint32_t src_addr;
    uint32_t dst_addr;
} __attribute__((packed)) IPV4_HDR;

void ipv4_input(NETIF *nif,
                const uint8_t src_mac[ETH_ADDR_LEN],
                const uint8_t *packet,
                uint16_t len);

int ipv4_output(NETIF *nif,
                const uint8_t dst_mac[ETH_ADDR_LEN],
                uint32_t src_ip,
                uint32_t dst_ip,
                uint8_t proto,
                const void *payload,
                uint16_t payload_len);

#endif

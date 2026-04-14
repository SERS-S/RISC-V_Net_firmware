#ifndef ARP_H
#define ARP_H

#include "eth.h"
#include "netif.h"

#include <stdint.h>

#define ARP_HTYPE_ETHERNET 1U
#define ARP_PTYPE_IPV4    0x0800U

#define ARP_OPER_REQUEST 1U
#define ARP_OPER_REPLY   2U

#define ARP_IPV4_ADDR_LEN 4U
#define ARP_CACHE_SIZE    8U

typedef struct arp_packet
{
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t oper;
    uint8_t sha[ETH_ADDR_LEN];
    uint8_t spa[ARP_IPV4_ADDR_LEN];
    uint8_t tha[ETH_ADDR_LEN];
    uint8_t tpa[ARP_IPV4_ADDR_LEN];
} __attribute__((packed)) ARP_PACKET;

typedef struct arp_entry
{
    uint32_t ip;
    uint8_t mac[ETH_ADDR_LEN];
    uint8_t valid;
} ARP_ENTRY;

const uint8_t *arp_cache_lookup(uint32_t ip);
void arp_cache_insert(uint32_t ip, const uint8_t mac[ETH_ADDR_LEN]);

void arp_input(NETIF *nif, const uint8_t *payload, uint16_t len);
int arp_request(NETIF *nif, uint32_t target_ip);

#endif

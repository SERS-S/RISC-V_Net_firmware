#ifndef NETIF_H
#define NETIF_H

#include <stdint.h>

#define NETIF_IPV4_ADDR(a, b, c, d) ((((uint32_t)(a)) << 24) | (((uint32_t)(b)) << 16) | (((uint32_t)(c)) << 8) | ((uint32_t)(d)))

typedef struct netif
{
    uint8_t mac[6];
    uint32_t ipv4_addr;
    uint32_t ipv4_mask;
    int (*tx)(const void *frame, uint16_t len);
} NETIF;

#endif

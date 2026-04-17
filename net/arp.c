#include "arp.h"

#include "endian.h"
#include "stats.h"

static ARP_ENTRY g_arp_cache[ARP_CACHE_SIZE];
static uint8_t g_arp_next;

static void copy_bytes(uint8_t *dst, const uint8_t *src, uint16_t len)
{
    for (uint16_t i = 0; i < len; ++i)
    {
        dst[i] = src[i];
    }
}

static void zero_bytes(uint8_t *dst, uint16_t len)
{
    for (uint16_t i = 0; i < len; ++i)
    {
        dst[i] = 0;
    }
}

static uint32_t ipv4_from_bytes(const uint8_t ip[ARP_IPV4_ADDR_LEN])
{
    return (((uint32_t)ip[0]) << 24) |
           (((uint32_t)ip[1]) << 16) |
           (((uint32_t)ip[2]) << 8) |
           ((uint32_t)ip[3]);
}

static void ipv4_to_bytes(uint8_t out[ARP_IPV4_ADDR_LEN], uint32_t ip)
{
    out[0] = (uint8_t)(ip >> 24);
    out[1] = (uint8_t)(ip >> 16);
    out[2] = (uint8_t)(ip >> 8);
    out[3] = (uint8_t)ip;
}

const uint8_t *arp_cache_lookup(uint32_t ip)
{
    for (uint8_t i = 0; i < ARP_CACHE_SIZE; ++i)
    {
        if (g_arp_cache[i].valid && (g_arp_cache[i].ip == ip))
        {
            return g_arp_cache[i].mac;
        }
    }

    return 0;
}

void arp_cache_insert(uint32_t ip, const uint8_t mac[ETH_ADDR_LEN])
{
    uint8_t slot = ARP_CACHE_SIZE;

    for (uint8_t i = 0; i < ARP_CACHE_SIZE; ++i)
    {
        if (g_arp_cache[i].valid && (g_arp_cache[i].ip == ip))
        {
            slot = i;
            break;
        }
    }

    if (slot == ARP_CACHE_SIZE)
    {
        slot = g_arp_next;
        g_arp_next = (uint8_t)((g_arp_next + 1U) % ARP_CACHE_SIZE);
    }

    g_arp_cache[slot].ip = ip;
    copy_bytes(g_arp_cache[slot].mac, mac, ETH_ADDR_LEN);
    g_arp_cache[slot].valid = 1;
}

void arp_input(NETIF *nif, const uint8_t *payload, uint16_t len)
{
    if (!nif || !payload || (len < sizeof(ARP_PACKET)))
    {
        ++g_net_stats.drop_bad_len;
        return;
    }

    const ARP_PACKET *arp = (const ARP_PACKET *)payload;
    const uint16_t htype = ntohs(arp->htype);
    const uint16_t ptype = ntohs(arp->ptype);
    const uint16_t oper = ntohs(arp->oper);
    const uint32_t spa = ipv4_from_bytes(arp->spa);
    const uint32_t tpa = ipv4_from_bytes(arp->tpa);

    if ((htype != ARP_HTYPE_ETHERNET) ||
        (ptype != ARP_PTYPE_IPV4) ||
        (arp->hlen != ETH_ADDR_LEN) ||
        (arp->plen != ARP_IPV4_ADDR_LEN))
    {
        ++g_net_stats.drop_unsupported;
        return;
    }

    if (oper == ARP_OPER_REPLY)
    {
        arp_cache_insert(spa, arp->sha);
        return;
    }

    if (oper != ARP_OPER_REQUEST)
    {
        ++g_net_stats.drop_unsupported;
        return;
    }

    if (tpa != nif->ipv4_addr)
    {
        ++g_net_stats.drop_wrong_dst;
        return;
    }

    arp_cache_insert(spa, arp->sha);

    ARP_PACKET reply;
    reply.htype = htons(ARP_HTYPE_ETHERNET);
    reply.ptype = htons(ARP_PTYPE_IPV4);
    reply.hlen = ETH_ADDR_LEN;
    reply.plen = ARP_IPV4_ADDR_LEN;
    reply.oper = htons(ARP_OPER_REPLY);
    copy_bytes(reply.sha, nif->mac, ETH_ADDR_LEN);
    ipv4_to_bytes(reply.spa, nif->ipv4_addr);
    copy_bytes(reply.tha, arp->sha, ETH_ADDR_LEN);
    ipv4_to_bytes(reply.tpa, spa);

    (void)eth_output(nif, arp->sha, ETH_TYPE_ARP, &reply, (uint16_t)sizeof(reply));
}

int arp_request(NETIF *nif, uint32_t target_ip)
{
    static const uint8_t broadcast_mac[ETH_ADDR_LEN] =
    {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };

    if (!nif)
    {
        return 0;
    }

    ARP_PACKET request;
    request.htype = htons(ARP_HTYPE_ETHERNET);
    request.ptype = htons(ARP_PTYPE_IPV4);
    request.hlen = ETH_ADDR_LEN;
    request.plen = ARP_IPV4_ADDR_LEN;
    request.oper = htons(ARP_OPER_REQUEST);
    copy_bytes(request.sha, nif->mac, ETH_ADDR_LEN);
    ipv4_to_bytes(request.spa, nif->ipv4_addr);
    zero_bytes(request.tha, ETH_ADDR_LEN);
    ipv4_to_bytes(request.tpa, target_ip);

    return eth_output(nif,
                      broadcast_mac,
                      ETH_TYPE_ARP,
                      &request,
                      (uint16_t)sizeof(request));
}

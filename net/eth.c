#include "eth.h"

#include "arp.h"
#include "endian.h"
#include "ipv4.h"
#include "stats.h"

static void copy_bytes(uint8_t *dst, const uint8_t *src, uint16_t len)
{
    for (uint16_t i = 0; i < len; ++i)
    {
        dst[i] = src[i];
    }
}

static int mac_equal(const uint8_t a[ETH_ADDR_LEN], const uint8_t b[ETH_ADDR_LEN])
{
    for (uintptr_t i = 0; i < ETH_ADDR_LEN; ++i)
    {
        if (a[i] != b[i])
        {
            return 0;
        }
    }

    return 1;
}

static int mac_is_broadcast(const uint8_t mac[ETH_ADDR_LEN])
{
    for (uintptr_t i = 0; i < ETH_ADDR_LEN; ++i)
    {
        if (mac[i] != 0xffU)
        {
            return 0;
        }
    }

    return 1;
}

void eth_input(NETIF *nif, const uint8_t *frame, uint16_t len)
{
    if (len < sizeof(ETH_HDR))
    {
        ++g_net_stats.drop_bad_len;
        return;
    }

    const ETH_HDR *hdr = (const ETH_HDR *)frame;
    if (!mac_equal(hdr->dst, nif->mac) && !mac_is_broadcast(hdr->dst))
    {
        ++g_net_stats.drop_wrong_dst;
        return;
    }

    const uint16_t eth_type = ntohs(hdr->type);

    switch (eth_type)
    {
        case ETH_TYPE_ARP:
            ++g_net_stats.rx_arp;
            arp_input(nif,
                      frame + sizeof(ETH_HDR),
                      (uint16_t)(len - sizeof(ETH_HDR)));
            break;
        case ETH_TYPE_IPV4:
            ++g_net_stats.rx_ipv4;
            ipv4_input(nif,
                       hdr->src,
                       frame + sizeof(ETH_HDR),
                       (uint16_t)(len - sizeof(ETH_HDR)));
            break;
        default:
            ++g_net_stats.drop_unsupported;
            break;
    }
}

int eth_output(NETIF *nif,
               const uint8_t dst_mac[ETH_ADDR_LEN],
               uint16_t eth_type,
               const void *payload,
               uint16_t payload_len)
{
    static uint8_t frame[ETH_FRAME_MAX];
    uint16_t frame_len = (uint16_t)(sizeof(ETH_HDR) + payload_len);

    if (!nif || !nif->tx || (payload_len > ETH_MTU))
    {
        ++g_net_stats.tx_errors;
        return 0;
    }

    ETH_HDR *hdr = (ETH_HDR *)frame;
    copy_bytes(hdr->dst, dst_mac, ETH_ADDR_LEN);
    copy_bytes(hdr->src, nif->mac, ETH_ADDR_LEN);
    hdr->type = htons(eth_type);

    copy_bytes(frame + sizeof(ETH_HDR), (const uint8_t *)payload, payload_len);

    while (frame_len < ETH_FRAME_MIN)
    {
        frame[frame_len++] = 0;
    }

    if (!nif->tx(frame, frame_len))
    {
        ++g_net_stats.tx_errors;
        return 0;
    }

    ++g_net_stats.tx_frames;
    return 1;
}

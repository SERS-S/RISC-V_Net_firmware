#include "ipv4.h"

#include "checksum.h"
#include "endian.h"
#include "eth.h"
#include "icmp.h"
#include "stats.h"
#include "udp.h"

static uint16_t g_ipv4_identification;

static void copy_bytes(uint8_t *dst, const uint8_t *src, uint16_t len)
{
    for (uint16_t i = 0; i < len; ++i)
    {
        dst[i] = src[i];
    }
}

void ipv4_input(NETIF *nif,
                const uint8_t src_mac[ETH_ADDR_LEN],
                const uint8_t *packet,
                uint16_t len)
{
    if (!nif || !src_mac || !packet || (len < IPV4_HDR_MIN_LEN))
    {
        ++g_net_stats.drop_bad_len;
        return;
    }

    const IPV4_HDR *hdr = (const IPV4_HDR *)packet;
    const uint8_t version = (uint8_t)(hdr->version_ihl >> 4);
    const uint8_t ihl = (uint8_t)(hdr->version_ihl & 0x0fU);
    const uint16_t header_len = (uint16_t)(ihl * 4U);

    if (version != IPV4_VERSION)
    {
        ++g_net_stats.drop_unsupported;
        return;
    }

    if ((ihl < IPV4_IHL_NO_OPT) || (header_len > len))
    {
        ++g_net_stats.drop_bad_len;
        return;
    }

    const uint16_t total_len = ntohs(hdr->total_length);
    if ((total_len < header_len) || (total_len > len))
    {
        ++g_net_stats.drop_bad_len;
        return;
    }

    if (ip_checksum(packet, header_len) != 0U)
    {
        ++g_net_stats.drop_bad_checksum;
        return;
    }

    const uint16_t flags_fragment = ntohs(hdr->flags_fragment);
    if ((flags_fragment & (IPV4_FLAG_MORE_FRAGMENTS | IPV4_FRAGMENT_OFFSET_MASK)) != 0U)
    {
        ++g_net_stats.drop_unsupported;
        return;
    }

    const uint32_t src_ip = ntohl(hdr->src_addr);
    const uint32_t dst_ip = ntohl(hdr->dst_addr);
    if (dst_ip != nif->ipv4_addr)
    {
        ++g_net_stats.drop_wrong_dst;
        return;
    }

    const uint8_t *payload = packet + header_len;
    const uint16_t payload_len = (uint16_t)(total_len - header_len);

    switch (hdr->protocol)
    {
        case IPV4_PROTO_UDP:
            ++g_net_stats.rx_udp;
            udp_input(nif, src_mac, src_ip, dst_ip, payload, payload_len);
            break;
        case IPV4_PROTO_ICMP:
            ++g_net_stats.rx_icmp;
            icmp_input(nif, src_mac, src_ip, dst_ip, payload, payload_len);
            break;
        default:
            ++g_net_stats.drop_unsupported;
            break;
    }
}

int ipv4_output(NETIF *nif,
                const uint8_t dst_mac[ETH_ADDR_LEN],
                uint32_t src_ip,
                uint32_t dst_ip,
                uint8_t proto,
                const void *payload,
                uint16_t payload_len)
{
    static uint8_t packet[IPV4_MAX_PACKET];

    if (!nif || !dst_mac || (!payload && (payload_len != 0U)) ||
        (payload_len > IPV4_MAX_PAYLOAD))
    {
        return 0;
    }

    const uint16_t total_len = (uint16_t)(sizeof(IPV4_HDR) + payload_len);
    IPV4_HDR *hdr = (IPV4_HDR *)packet;

    hdr->version_ihl = (uint8_t)((IPV4_VERSION << 4) | IPV4_IHL_NO_OPT);
    hdr->dscp_ecn = 0;
    hdr->total_length = htons(total_len);
    hdr->identification = htons(g_ipv4_identification++);
    hdr->flags_fragment = htons(0);
    hdr->ttl = IPV4_TTL_DEFAULT;
    hdr->protocol = proto;
    hdr->header_checksum = 0;
    hdr->src_addr = htonl(src_ip);
    hdr->dst_addr = htonl(dst_ip);
    hdr->header_checksum = htons(ip_checksum(packet, (uint16_t)sizeof(IPV4_HDR)));

    copy_bytes(packet + sizeof(IPV4_HDR), (const uint8_t *)payload, payload_len);

    return eth_output(nif, dst_mac, ETH_TYPE_IPV4, packet, total_len);
}

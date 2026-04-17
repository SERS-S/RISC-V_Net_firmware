#include "icmp.h"

#include "checksum.h"
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

void icmp_input(NETIF *nif,
                const uint8_t src_mac[ETH_ADDR_LEN],
                uint32_t src_ip,
                uint32_t dst_ip,
                const uint8_t *packet,
                uint16_t len)
{
    static uint8_t reply[ICMP_MAX_PACKET];

    if (!nif || !src_mac || !packet || (len < ICMP_ECHO_HDR_LEN))
    {
        ++g_net_stats.drop_bad_len;
        return;
    }

    if (len > ICMP_MAX_PACKET)
    {
        ++g_net_stats.drop_bad_len;
        return;
    }

    if (ip_checksum(packet, len) != 0U)
    {
        ++g_net_stats.drop_bad_checksum;
        return;
    }

    const ICMP_ECHO_HDR *hdr = (const ICMP_ECHO_HDR *)packet;
    if ((hdr->type != ICMP_ECHO_REQUEST) || (hdr->code != 0U))
    {
        ++g_net_stats.drop_unsupported;
        return;
    }

    copy_bytes(reply, packet, len);

    ICMP_ECHO_HDR *reply_hdr = (ICMP_ECHO_HDR *)reply;
    reply_hdr->type = ICMP_ECHO_REPLY;
    reply_hdr->code = 0;
    reply_hdr->checksum = 0;
    reply_hdr->checksum = htons(ip_checksum(reply, len));

    (void)ipv4_output(nif,
                      src_mac,
                      dst_ip,
                      src_ip,
                      IPV4_PROTO_ICMP,
                      reply,
                      len);
}

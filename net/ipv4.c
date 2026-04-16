#include "ipv4.h"

#include "checksum.h"
#include "endian.h"
#include "eth.h"
#include "icmp.h"
#include "uart.h"
#include "udp.h"

static uint16_t g_ipv4_identification;

static void copy_bytes(uint8_t *dst, const uint8_t *src, uint16_t len)
{
    for (uint16_t i = 0; i < len; ++i)
    {
        dst[i] = src[i];
    }
}

static void print_ipv4(uint32_t ip)
{
    uart_puthex64((ip >> 24) & 0xffU);
    uart_putc('.');
    uart_puthex64((ip >> 16) & 0xffU);
    uart_putc('.');
    uart_puthex64((ip >> 8) & 0xffU);
    uart_putc('.');
    uart_puthex64(ip & 0xffU);
}

void ipv4_input(NETIF *nif,
                const uint8_t src_mac[ETH_ADDR_LEN],
                const uint8_t *packet,
                uint16_t len)
{
    if (!nif || !src_mac || !packet || (len < IPV4_HDR_MIN_LEN))
    {
        uart_puts("ipv4: drop short packet\n");
        return;
    }

    const IPV4_HDR *hdr = (const IPV4_HDR *)packet;
    const uint8_t version = (uint8_t)(hdr->version_ihl >> 4);
    const uint8_t ihl = (uint8_t)(hdr->version_ihl & 0x0fU);
    const uint16_t header_len = (uint16_t)(ihl * 4U);

    if (version != IPV4_VERSION)
    {
        uart_puts("ipv4: drop bad version\n");
        return;
    }

    if ((ihl < IPV4_IHL_NO_OPT) || (header_len > len))
    {
        uart_puts("ipv4: drop bad ihl\n");
        return;
    }

    const uint16_t total_len = ntohs(hdr->total_length);
    if ((total_len < header_len) || (total_len > len))
    {
        uart_puts("ipv4: drop bad total length\n");
        return;
    }

    if (ip_checksum(packet, header_len) != 0U)
    {
        uart_puts("ipv4: drop bad checksum\n");
        return;
    }

    const uint16_t flags_fragment = ntohs(hdr->flags_fragment);
    if ((flags_fragment & (IPV4_FLAG_MORE_FRAGMENTS | IPV4_FRAGMENT_OFFSET_MASK)) != 0U)
    {
        uart_puts("ipv4: drop fragmented packet\n");
        return;
    }

    const uint32_t src_ip = ntohl(hdr->src_addr);
    const uint32_t dst_ip = ntohl(hdr->dst_addr);
    if (dst_ip != nif->ipv4_addr)
    {
        uart_puts("ipv4: drop wrong dst=");
        print_ipv4(dst_ip);
        uart_puts("\n");
        return;
    }

    const uint8_t *payload = packet + header_len;
    const uint16_t payload_len = (uint16_t)(total_len - header_len);

    uart_puts("ipv4: input proto=");
    uart_puthex64(hdr->protocol);
    uart_puts(" src=");
    print_ipv4(src_ip);
    uart_puts(" len=");
    uart_puthex64(total_len);
    uart_puts("\n");

    switch (hdr->protocol)
    {
        case IPV4_PROTO_UDP:
            udp_input(nif, src_mac, src_ip, dst_ip, payload, payload_len);
            break;
        case IPV4_PROTO_ICMP:
            icmp_input(nif, src_mac, src_ip, dst_ip, payload, payload_len);
            break;
        default:
            uart_puts("ipv4: drop unsupported protocol\n");
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

    uart_puts("ipv4: output proto=");
    uart_puthex64(proto);
    uart_puts(" dst=");
    print_ipv4(dst_ip);
    uart_puts(" len=");
    uart_puthex64(total_len);
    uart_puts("\n");

    return eth_output(nif, dst_mac, ETH_TYPE_IPV4, packet, total_len);
}

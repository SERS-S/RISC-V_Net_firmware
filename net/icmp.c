#include "icmp.h"

#include "checksum.h"
#include "endian.h"
#include "ipv4.h"
#include "uart.h"

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
        uart_puts("icmp: drop short packet\n");
        return;
    }

    if (len > ICMP_MAX_PACKET)
    {
        uart_puts("icmp: drop oversized packet\n");
        return;
    }

    if (ip_checksum(packet, len) != 0U)
    {
        uart_puts("icmp: drop bad checksum\n");
        return;
    }

    const ICMP_ECHO_HDR *hdr = (const ICMP_ECHO_HDR *)packet;
    if ((hdr->type != ICMP_ECHO_REQUEST) || (hdr->code != 0U))
    {
        uart_puts("icmp: drop unsupported type/code\n");
        return;
    }

    const uint16_t payload_len = (uint16_t)(len - ICMP_ECHO_HDR_LEN);

    uart_puts("icmp: echo request src=");
    print_ipv4(src_ip);
    uart_puts(" payload_len=");
    uart_puthex64(payload_len);
    uart_puts("\n");

    copy_bytes(reply, packet, len);

    ICMP_ECHO_HDR *reply_hdr = (ICMP_ECHO_HDR *)reply;
    reply_hdr->type = ICMP_ECHO_REPLY;
    reply_hdr->code = 0;
    reply_hdr->checksum = 0;
    reply_hdr->checksum = htons(ip_checksum(reply, len));

    if (ipv4_output(nif,
                    src_mac,
                    dst_ip,
                    src_ip,
                    IPV4_PROTO_ICMP,
                    reply,
                    len))
    {
        uart_puts("icmp: echo reply sent\n");
    }
    else
    {
        uart_puts("icmp: echo reply tx failed\n");
    }
}

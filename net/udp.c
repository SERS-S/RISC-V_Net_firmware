#include "udp.h"

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

uint16_t udp_checksum_ipv4(uint32_t src_ip,
                           uint32_t dst_ip,
                           const void *udp_packet,
                           uint16_t udp_len)
{
    uint8_t pseudo[12];

    pseudo[0] = (uint8_t)(src_ip >> 24);
    pseudo[1] = (uint8_t)(src_ip >> 16);
    pseudo[2] = (uint8_t)(src_ip >> 8);
    pseudo[3] = (uint8_t)src_ip;
    pseudo[4] = (uint8_t)(dst_ip >> 24);
    pseudo[5] = (uint8_t)(dst_ip >> 16);
    pseudo[6] = (uint8_t)(dst_ip >> 8);
    pseudo[7] = (uint8_t)dst_ip;
    pseudo[8] = 0;
    pseudo[9] = IPV4_PROTO_UDP;
    pseudo[10] = (uint8_t)(udp_len >> 8);
    pseudo[11] = (uint8_t)udp_len;

    uint32_t sum = checksum_add(pseudo, (uint16_t)sizeof(pseudo), 0);
    sum = checksum_add(udp_packet, udp_len, sum);

    return checksum_finalize(sum);
}

void udp_input(NETIF *nif,
               const uint8_t src_mac[ETH_ADDR_LEN],
               uint32_t src_ip,
               uint32_t dst_ip,
               const uint8_t *packet,
               uint16_t len)
{
    static uint8_t reply[UDP_MAX_PACKET];

    if (!nif || !src_mac || !packet || (len < UDP_HDR_LEN))
    {
        uart_puts("udp: drop short packet\n");
        return;
    }

    const UDP_HDR *hdr = (const UDP_HDR *)packet;
    const uint16_t src_port = ntohs(hdr->src_port);
    const uint16_t dst_port = ntohs(hdr->dst_port);
    const uint16_t udp_len = ntohs(hdr->length);

    if ((udp_len < UDP_HDR_LEN) || (udp_len > len))
    {
        uart_puts("udp: drop bad length\n");
        return;
    }

    if ((hdr->checksum != 0U) &&
        (udp_checksum_ipv4(src_ip, dst_ip, packet, udp_len) != 0U))
    {
        uart_puts("udp: drop bad checksum\n");
        return;
    }

    if (dst_port != UDP_ECHO_PORT)
    {
        uart_puts("udp: drop dst port=");
        uart_puthex64(dst_port);
        uart_puts("\n");
        return;
    }

    const uint16_t payload_len = (uint16_t)(udp_len - UDP_HDR_LEN);

    uart_puts("udp: echo request src=");
    print_ipv4(src_ip);
    uart_puts(" sport=");
    uart_puthex64(src_port);
    uart_puts(" payload_len=");
    uart_puthex64(payload_len);
    uart_puts("\n");

    UDP_HDR *reply_hdr = (UDP_HDR *)reply;
    reply_hdr->src_port = htons(dst_port);
    reply_hdr->dst_port = htons(src_port);
    reply_hdr->length = htons(udp_len);
    reply_hdr->checksum = 0;
    copy_bytes(reply + UDP_HDR_LEN, packet + UDP_HDR_LEN, payload_len);

    uint16_t checksum = udp_checksum_ipv4(nif->ipv4_addr, src_ip, reply, udp_len);
    if (checksum == 0U)
    {
        checksum = 0xffffU;
    }
    reply_hdr->checksum = htons(checksum);

    if (ipv4_output(nif,
                    src_mac,
                    nif->ipv4_addr,
                    src_ip,
                    IPV4_PROTO_UDP,
                    reply,
                    udp_len))
    {
        uart_puts("udp: echo reply sent\n");
    }
    else
    {
        uart_puts("udp: echo reply tx failed\n");
    }
}

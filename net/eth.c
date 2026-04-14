#include "eth.h"

#include "endian.h"
#include "uart.h"

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
        uart_puts("eth: drop short frame\n");
        return;
    }

    const ETH_HDR *hdr = (const ETH_HDR *)frame;
    if (!mac_equal(hdr->dst, nif->mac) && !mac_is_broadcast(hdr->dst))
    {
        uart_puts("eth: drop foreign dst mac\n");
        return;
    }

    const uint16_t eth_type = ntohs(hdr->type);
    uart_puts("eth: input type=");
    uart_puthex64(eth_type);
    uart_puts(" len=");
    uart_puthex64(len);
    uart_puts("\n");

    switch (eth_type)
    {
        case ETH_TYPE_ARP:
            uart_puts("eth: ARP handler not implemented yet\n");
            break;
        case ETH_TYPE_IPV4:
            uart_puts("eth: IPv4 handler not implemented yet\n");
            break;
        default:
            uart_puts("eth: drop unsupported ethertype\n");
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

    uart_puts("eth: output type=");
    uart_puthex64(eth_type);
    uart_puts(" len=");
    uart_puthex64(frame_len);
    uart_puts("\n");

    return nif->tx(frame, frame_len);
}

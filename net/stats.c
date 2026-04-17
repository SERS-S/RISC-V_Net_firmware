#include "stats.h"

#include "uart.h"

NET_STATS g_net_stats;

static NET_STATS g_last_printed_stats;

static int stats_equal(const NET_STATS *a, const NET_STATS *b)
{
    return (a->rx_frames == b->rx_frames) &&
           (a->tx_frames == b->tx_frames) &&
           (a->rx_arp == b->rx_arp) &&
           (a->rx_ipv4 == b->rx_ipv4) &&
           (a->rx_icmp == b->rx_icmp) &&
           (a->rx_udp == b->rx_udp) &&
           (a->drop_bad_len == b->drop_bad_len) &&
           (a->drop_bad_checksum == b->drop_bad_checksum) &&
           (a->drop_wrong_dst == b->drop_wrong_dst) &&
           (a->drop_unsupported == b->drop_unsupported) &&
           (a->tx_errors == b->tx_errors);
}

static void stats_copy(NET_STATS *dst, const NET_STATS *src)
{
    *dst = *src;
}

static void print_counter(const char *name, uint64_t value)
{
    uart_putc(' ');
    uart_puts(name);
    uart_putc('=');
    uart_puthex64(value);
}

void net_stats_print(void)
{
    uart_puts("net-stats:");
    print_counter("rx", g_net_stats.rx_frames);
    print_counter("tx", g_net_stats.tx_frames);
    print_counter("arp", g_net_stats.rx_arp);
    print_counter("ip", g_net_stats.rx_ipv4);
    print_counter("icmp", g_net_stats.rx_icmp);
    print_counter("udp", g_net_stats.rx_udp);
    print_counter("bad_len", g_net_stats.drop_bad_len);
    print_counter("bad_sum", g_net_stats.drop_bad_checksum);
    print_counter("wrong_dst", g_net_stats.drop_wrong_dst);
    print_counter("unsup", g_net_stats.drop_unsupported);
    print_counter("tx_err", g_net_stats.tx_errors);
    uart_puts("\n");
}

void net_stats_print_if_changed(void)
{
    if (stats_equal(&g_net_stats, &g_last_printed_stats))
    {
        return;
    }

    net_stats_print();
    stats_copy(&g_last_printed_stats, &g_net_stats);
}

#ifndef NET_STATS_H
#define NET_STATS_H

#include <stdint.h>

typedef struct net_stats
{
    uint64_t rx_frames;
    uint64_t tx_frames;
    uint64_t rx_arp;
    uint64_t rx_ipv4;
    uint64_t rx_icmp;
    uint64_t rx_udp;
    uint64_t drop_bad_len;
    uint64_t drop_bad_checksum;
    uint64_t drop_wrong_dst;
    uint64_t drop_unsupported;
    uint64_t tx_errors;
} NET_STATS;

extern NET_STATS g_net_stats;

void net_stats_print(void);
void net_stats_print_if_changed(void);

#endif

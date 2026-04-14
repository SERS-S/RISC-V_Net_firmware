#ifndef NET_ENDIAN_H
#define NET_ENDIAN_H

#include <stdint.h>

static inline uint16_t bswap16(uint16_t value)
{
    return (uint16_t)((value >> 8) | (value << 8));
}

static inline uint32_t bswap32(uint32_t value)
{
    return ((value & 0x000000ffUL) << 24) |
           ((value & 0x0000ff00UL) << 8) |
           ((value & 0x00ff0000UL) >> 8) |
           ((value & 0xff000000UL) >> 24);
}

static inline uint16_t htons(uint16_t value)
{
    return bswap16(value);
}

static inline uint16_t ntohs(uint16_t value)
{
    return bswap16(value);
}

static inline uint32_t htonl(uint32_t value)
{
    return bswap32(value);
}

static inline uint32_t ntohl(uint32_t value)
{
    return bswap32(value);
}

#endif

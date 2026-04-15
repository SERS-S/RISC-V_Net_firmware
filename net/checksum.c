#include "checksum.h"

uint32_t checksum_add(const void *data, uint16_t len, uint32_t sum)
{
    const uint8_t *bytes = (const uint8_t *)data;

    while (len > 1U)
    {
        sum += (((uint32_t)bytes[0]) << 8) | ((uint32_t)bytes[1]);
        bytes += 2;
        len = (uint16_t)(len - 2U);
    }

    if (len != 0U)
    {
        sum += ((uint32_t)bytes[0]) << 8;
    }

    return sum;
}

uint16_t checksum_finalize(uint32_t sum)
{
    while ((sum >> 16) != 0U)
    {
        sum = (sum & 0xffffU) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}

uint16_t ip_checksum(const void *data, uint16_t len)
{
    return checksum_finalize(checksum_add(data, len, 0));
}

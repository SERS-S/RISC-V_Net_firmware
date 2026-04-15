#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>

uint32_t checksum_add(const void *data, uint16_t len, uint32_t sum);
uint16_t checksum_finalize(uint32_t sum);
uint16_t ip_checksum(const void *data, uint16_t len);

#endif

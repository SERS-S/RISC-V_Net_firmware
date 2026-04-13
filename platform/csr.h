#ifndef CSR_H
#define CSR_H

#include <stdint.h>

static inline uint64_t csr_read_mstatus(void)
{
    uint64_t value;
    __asm__ volatile("csrr %0, mstatus" : "=r"(value));
    return value;
}

static inline uint64_t csr_read_mtvec(void)
{
    uint64_t value;
    __asm__ volatile("csrr %0, mtvec" : "=r"(value));
    return value;
}

static inline void csr_write_mtvec(uint64_t value)
{
    __asm__ volatile("csrw mtvec, %0" :: "r"(value));
}

static inline uint64_t csr_read_mcause(void)
{
    uint64_t value;
    __asm__ volatile("csrr %0, mcause" : "=r"(value));
    return value;
}

static inline uint64_t csr_read_mepc(void)
{
    uint64_t value;
    __asm__ volatile("csrr %0, mepc" : "=r"(value));
    return value;
}

static inline uint64_t csr_read_mtval(void)
{
    uint64_t value;
    __asm__ volatile("csrr %0, mtval" : "=r"(value));
    return value;
}

#endif

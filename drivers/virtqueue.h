#ifndef VIRTQUEUE_H
#define VIRTQUEUE_H

#include <stdint.h>

#define VIRTQUEUE_SIZE       8U
#define VIRTQUEUE_ALIGN      4096U
#define VIRTQUEUE_USED_OFF   4096U
#define VIRTQUEUE_MEM_SIZE   8192U

#define VIRTQ_DESC_F_NEXT  1U
#define VIRTQ_DESC_F_WRITE 2U

typedef struct virtq_desc
{
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} VIRTQ_DESC;

typedef struct virtq_avail
{
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTQUEUE_SIZE];
} VIRTQ_AVAIL;

typedef struct virtq_used_elem
{
    uint32_t id;
    uint32_t len;
} VIRTQ_USED_ELEM;

typedef struct virtq_used
{
    uint16_t flags;
    uint16_t idx;
    VIRTQ_USED_ELEM ring[VIRTQUEUE_SIZE];
} VIRTQ_USED;

typedef struct virtqueue
{
    uint32_t index;
    uint32_t size;
    uint16_t last_used_idx;
    uint8_t *mem;
    VIRTQ_DESC *desc;
    volatile VIRTQ_AVAIL *avail;
    volatile VIRTQ_USED *used;
} VIRTQUEUE;

void virtqueue_init(VIRTQUEUE *vq, uint32_t index, void *mem);
int virtqueue_add_buffer(VIRTQUEUE *vq,
                         uint16_t desc_id,
                         uintptr_t addr,
                         uint32_t len,
                         uint16_t flags);
int virtqueue_get_used(VIRTQUEUE *vq, uint32_t *id, uint32_t *len);
void virtqueue_memory_barrier(void);
uintptr_t virtqueue_mem_addr(const VIRTQUEUE *vq);

#endif

#include "virtqueue.h"

static void memzero(void *ptr, uintptr_t len)
{
    uint8_t *p = (uint8_t *)ptr;

    for (uintptr_t i = 0; i < len; ++i)
    {
        p[i] = 0;
    }
}

void virtqueue_memory_barrier(void)
{
    __asm__ volatile("fence rw, rw" ::: "memory");
}

void virtqueue_init(VIRTQUEUE *vq, uint32_t index, void *mem)
{
    memzero(mem, VIRTQUEUE_MEM_SIZE);

    vq->index = index;
    vq->size = VIRTQUEUE_SIZE;
    vq->last_used_idx = 0;
    vq->mem = (uint8_t *)mem;
    vq->desc = (VIRTQ_DESC *)vq->mem;
    vq->avail = (volatile VIRTQ_AVAIL *)(vq->mem + (sizeof(VIRTQ_DESC) * VIRTQUEUE_SIZE));
    vq->used = (volatile VIRTQ_USED *)(vq->mem + VIRTQUEUE_USED_OFF);
}

int virtqueue_add_buffer(VIRTQUEUE *vq,
                         uint16_t desc_id,
                         uintptr_t addr,
                         uint32_t len,
                         uint16_t flags)
{
    const uint16_t slot = vq->avail->idx % (uint16_t)vq->size;

    if (desc_id >= vq->size)
    {
        return 0;
    }

    vq->desc[desc_id].addr = (uint64_t)addr;
    vq->desc[desc_id].len = len;
    vq->desc[desc_id].flags = flags;
    vq->desc[desc_id].next = 0;

    virtqueue_memory_barrier();
    vq->avail->ring[slot] = desc_id;
    virtqueue_memory_barrier();
    vq->avail->idx++;
    virtqueue_memory_barrier();

    return 1;
}

int virtqueue_get_used(VIRTQUEUE *vq, uint32_t *id, uint32_t *len)
{
    if (vq->last_used_idx == vq->used->idx)
    {
        return 0;
    }

    virtqueue_memory_barrier();

    const uint16_t slot = vq->last_used_idx % (uint16_t)vq->size;
    *id = vq->used->ring[slot].id;
    *len = vq->used->ring[slot].len;
    vq->last_used_idx++;

    virtqueue_memory_barrier();
    return 1;
}

uintptr_t virtqueue_mem_addr(const VIRTQUEUE *vq)
{
    return (uintptr_t)vq->mem;
}

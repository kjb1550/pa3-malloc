#include "vm.h"
#include "vmlib.h"
#include <stddef.h>

static struct block_header *find_best_fit(size_t needed_size) {
    struct block_header *curr = heapstart;
    struct block_header *best = NULL;

    while (curr->size_status != VM_ENDMARK) {
        size_t curr_sz = BLKSZ(curr);
        int is_busy = curr->size_status & VM_BUSY;

        if (!is_busy && curr_sz >= needed_size) {
            if (best == NULL || curr_sz < BLKSZ(best)) {
                best = curr;
            }
        }
        curr = (struct block_header *)((char *)curr + curr_sz);
    }
    return best;
}

/**
 * Allocate a memory block with the given minimum size on the simulated heap.
 * If allocation succeeds, return a pointer to the start of the new block's
 * payload. If allocation fails due to insufficient free space, return NULL.
 *
 * Read the section for vmalloc in the PA writeup for how to implement this
 * function.
 */
void *vmalloc(size_t size) {
    if (size == 0) return NULL;

    size_t needed = ROUND_UP(size + sizeof(struct block_header), BLKALIGN);

    struct block_header *blk = find_best_fit(needed);
    if (blk == NULL) return NULL;

    size_t blk_sz = BLKSZ(blk);
    size_t leftover = blk_sz - needed;

    if (leftover >= BLKALIGN) {
        struct block_header *new_free =
            (struct block_header *)((char *)blk + needed);

        new_free->size_status = leftover | VM_PREVBUSY;

        struct block_footer *new_footer =
            (struct block_footer *)((char *)new_free + leftover - sizeof(struct block_footer));
        new_footer->size = leftover;

        struct block_header *after_new =
            (struct block_header *)((char *)new_free + leftover);
        if (after_new->size_status != VM_ENDMARK) {
            after_new->size_status &= ~VM_PREVBUSY;
        }

        size_t prev_bit = blk->size_status & VM_PREVBUSY;
        blk->size_status = needed | prev_bit | VM_BUSY;

    } else {
        size_t prev_bit = blk->size_status & VM_PREVBUSY;
        blk->size_status = blk_sz | prev_bit | VM_BUSY;

        struct block_header *nxt =
            (struct block_header *)((char *)blk + blk_sz);
        if (nxt->size_status != VM_ENDMARK) {
            nxt->size_status |= VM_PREVBUSY;
        }
    }

    return (void *)((char *)blk + sizeof(struct block_header));
}

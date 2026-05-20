#include "vm.h"
#include "vmlib.h"
#include <stddef.h>

/**
 * The vmfree() function frees the memory space pointed to by ptr,
 * which must have been returned by a previous call to vmalloc().
 * Otherwise, or if vmfree(ptr) has already been called before,
 * undefined behavior occurs.
 *
 * If ptr is NULL, no operation is performed.
 */
void vmfree(void *ptr) {
    if (ptr == NULL) return;

    struct block_header *hdr =
        (struct block_header *)((char *)ptr - sizeof(struct block_header));

    if (!(hdr->size_status & VM_BUSY)) return;

    size_t sz = BLKSZ(hdr);
    size_t prev_bit = hdr->size_status & VM_PREVBUSY;

    // --- Try to coalesce with the NEXT block ---
    struct block_header *nxt =
        (struct block_header *)((char *)hdr + sz);

    if (nxt->size_status != VM_ENDMARK && !(nxt->size_status & VM_BUSY)) {
        sz += BLKSZ(nxt);
    }

    // --- Try to coalesce with the PREVIOUS block ---
    if (!(prev_bit)) {
        struct block_footer *prev_footer =
            (struct block_footer *)((char *)hdr - sizeof(struct block_footer));
        size_t prev_sz = prev_footer->size;
        struct block_header *prev_hdr =
            (struct block_header *)((char *)hdr - prev_sz);

        sz += prev_sz;
        prev_bit = prev_hdr->size_status & VM_PREVBUSY;
        hdr = prev_hdr; // merged block now starts here
    }

    hdr->size_status = sz | prev_bit;

    struct block_footer *footer =
        (struct block_footer *)((char *)hdr + sz - sizeof(struct block_footer));
    footer->size = sz;

    struct block_header *after =
        (struct block_header *)((char *)hdr + sz);
    if (after->size_status != VM_ENDMARK) {
        after->size_status &= ~VM_PREVBUSY;
    }
}

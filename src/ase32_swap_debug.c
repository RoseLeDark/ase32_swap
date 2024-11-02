#include "ase32_swap_debug.h"

void ase32_swap_dump(const ase32_swap_t* swap) {
    if (swap == NULL) return;

    printf("Swap System Dump:\n");
    printf("Total Pages: %d\n", swap->page_count);
    printf("Max Pages: %d\n", swap->config.max_pages);
    printf("Block Size: %d\n", swap->config.block_size);
    printf("Cached Pages: %d\n\n", swap->config.cached_pages);

    for (uint32_t i = 0; i < swap->page_count; i++) {
        const swap_page_t* page = &swap->pages[i];
        printf("Page %d:\n", i);
        printf("  Virt Addr: 0x%08x\n", page->virt_addr);
        printf("  Phys Addr: 0x%08x\n", page->phys_addr);
        printf("  Partition Offset: %d\n", page->partition_offset);
        printf("  Dirty: %s\n", page->is_dirty ? "Yes" : "No");
        printf("  In Memory: %s\n", page->is_in_memory ? "Yes" : "No");
        printf("  Last Access: %d\n", page->last_access);
        printf("  Access Count: %d\n", page->access_count);
        printf("  Used: %s\n\n", page->is_used ? "Yes" : "No");
    }
}
#ifndef __ASE32_SWAP_DEBUG H__
#define __ASE32_SWAP_DEBUG_H__

#include <stdint.h>
#include <stddef.h>
#include <esp_err.h>

#include "ase32_swap_caps.h"
#include "ase32_swap_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void        ase32_swap_dump(const ase32_swap_t* swap);


inline size_t ase32_swap_get_free_pages(const ase32_swap_t* swap) {
    size_t free_pages = 0;
    for (uint32_t i = 0; i < swap->page_count; i++) {
        if (!swap->pages[i].is_used) {
            free_pages++;
        }
    }
    return free_pages;
}

inline size_t ase32_swap_get_num_pages(const ase32_swap_t* swap) {
    size_t num_pages = 0;
    for (uint32_t i = 0; i < swap->page_count; i++) {
        if (swap->pages[i].is_used) {
            num_pages++;
        }
    }
    return num_pages;
}

// Debugging und Tracing-Funktionen
void ASE32_SWAP_WEAK ase32_swap_debug_trace(int level, const char* message);
void ASE32_SWAP_WEAK ase32_swap_debug_memory_usage(size_t used, size_t total);
void ASE32_SWAP_WEAK ase32_swap_debug_page_fault(uint32_t addr);


#ifdef __cplusplus
}
#endif

#endif
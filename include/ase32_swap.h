#ifndef __ASE_32_SWAP_H__
#define __ASE_32_SWAP_H__

#include <stdint.h>
#include <stddef.h>
#include <esp_err.h>

#include "ase32_swap_caps.h"
#include "ase32_swap_config.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifndef ESP_PARTITION_SUBTYPE_SWAP
#define ESP_PARTITION_SUBTYPE_SWAP 0xFA
#endif // ESP_PARTITION_SUBTYPE_SWAP



// Funktionen für die Bibliothek
esp_err_t   ase32_swap_init(const ase32_swap_cfg_t cfg, ase32_swap_t* out);
void        ase32_swap_deinit(ase32_swap_t* swap);
void*       ase32_swap_malloc(ase32_swap_t* swap, size_t size);
void        ase32_swap_free(ase32_swap_t* swap, void* ptr);
bool        ase32_swap_is_mem(const ase32_swap_t* swap, const void* ptr, swap_page_t* out);

void        ase32_swap_mark_dirty(ase32_swap_t* swap, void* ptr, size_t size);
esp_err_t   ase32_swap_flush(ase32_swap_t* swap);
esp_err_t   ase32_swap_set_page_fault_handler(void (*handler)(ase32_swap_t* swap, void* arg, uint32_t addr), void* arg);


void ase32_swap_defragment(ase32_swap_t* swap, ase32_swap_defrag_callback callback);



void* ase32_swap_memcpy(ase32_swap_t* swap, void* dest, const void* src, size_t s );

#ifdef __cplusplus
}
#endif

#endif // __ASE_32_SWAP_H__

#ifndef __ASE32_SWAP_MMU_DEV_H__
#define __ASE32_SWAP_MMU_DEV_H__

#include <stdint.h>
#include <stddef.h>
#include <esp_err.h>

#include "ase32_swap_caps.h"
#include "ase32_swap_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
    void* safe_memcpy(void* dest, const void* src, size_t n, void* buffer_start, size_t buffer_size, fault_handler_t handler) { if ((uint32_t)dest < (uint32_t)buffer_start || (uint32_t)dest + n > (uint32_t)buffer_start + buffer_size) { handler(dest); return NULL; } return memcpy(dest, src, n); }
*/
typedef void (*ase32_swap_fault_handler_t)(const swap_page_t* swap, void* address);

void ase32_swap_register_page_fault_handler(const swap_page_t* swap, ase32_swap_fault_handler_t hanlder );

esp_err_t ase32_swap_register_page(swap_page_t* swap, swap_page_t* page);
esp_err_t ase32_swap_unregist_page(swap_page_t* page);

void ase32_swap_page_fault(const swap_page_t* swap, void* address);

#ifdef __cplusplus
}
#endif

#endif // __ASE32_SWAP_MMU_DEV_H__
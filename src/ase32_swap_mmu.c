#include "ase32_swap_mmu.h"
#include "esp_mmu_map.h"

#include "esp_mmu_map.h"

ase32_swap_fault_handler_t __global_handler;

void ase32_swap_default_fault_handler(const swap_page_t* swap, void* address) { 
    ESP_LOGE("PageFault", "Invalid memory access at %p", address); // Mögliche zusätzliche Fehlerbehandlung 
}

esp_err_t ase32_swap_register_page(ase32_swap_t* swap, swap_page_t* page) {
    esp_err_t ret = esp_mmu_map(page->phys_addr, swap->config.block_size, 
                        MMU_TARGET_PSRAM0, 
                        MMU_MEM_CAP_8BIT | MMU_MEM_CAP_READ | MMU_MEM_CAP_WRITE, 
                        0, (void**)&(page->virt_addr));
    if(ret != ESP_OK) return ret;

    page->is_in_memory = true;
    page->last_access = swap->access_counter++;
    page->access_count++;

    return ret;
}
esp_err_t ase32_swap_unregist_page(swap_page_t* page) {

    page->is_dirty = 0;
    heap_caps_free((void*)page->phys_addr);
    page->phys_addr = 0;
    page->is_in_memory = 0;

    return esp_mmu_unmap(page->virt_addr);
}

void ase32_swap_register_page_fault_handler(const swap_page_t* swap, ase32_swap_fault_handler_t hanlder ) {
    if(hanlder)
    __global_handler = hanlder;
    else
        __global_handler = ase32_swap_default_fault_handler; 
}

void ase32_swap_page_fault(const swap_page_t* swap, void* address) {
    if(__global_handler) __global_handler(swap, address);
    else ESP_LOGE("swap", "PAGE_FAULT!!! %d", address);
}



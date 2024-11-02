#include "ase32_swap_devices.h"
#include "ase32_swap_debug.h"

#include "esp_partition.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_attr.h"
#include <string.h>

#include "esp_private/esp_mmu_map_private.h"
#include "esp_mmu_map.h"

#include <esp_private/mmu_psram_flash.h>
#include <esp_heap_caps.h>

esp_err_t ase32_swap_device_open_partition(void* hndl, const char* name, size_t *size) {
    hndl = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_SWAP, name);
    if (hndl== NULL) {
        if(ase32_swap_debug_trace) 
            ase32_swap_debug_trace(ESP_LOG_ERROR, "Keine Swap-Partition gefunden");
        return ESP_ERR_NOT_FOUND;
    }

     // Initialize partition header if needed
    uint32_t magic;
    esp_err_t err = esp_partition_read(hndl, 0, &magic, sizeof(magic));
    if (err != ESP_OK)  return err;
    

     if (magic != ASE32_SWAP_PARTITION_MAGIC) {
        // Initialize partition
        magic = ASE32_SWAP_PARTITION_MAGIC;
        err = esp_partition_erase_range(hndl, 0, ((esp_partition_t*)hndl)->size);
        if (err != ESP_OK)  return err;
       
        err = esp_partition_write(hndl, 0, &magic, sizeof(magic));
        if (err != ESP_OK)  return err;
    }
    return err;
}
esp_err_t ase32_swap_device_close_partition(void* hndl) {
    return ESP_OK;
}
size_t ase32_swap_device_write_partition(const void* hndl, void* data, size_t offset, size_t size) {
    esp_err_t err = esp_partition_write(hndl, offset, data, size);
    if (err != ESP_OK) { return 0; }

    return size;
}
size_t ase32_swap_device_read_partition(const void* hndl, void* data, size_t offset, size_t size) {
    esp_err_t err = esp_partition_read(hndl, offset, data, size);
    if (err != ESP_OK) { return 0; }
    
    return size;
}
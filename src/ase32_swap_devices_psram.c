#include "ase32_swap_devices.h"
#include "ase32_swap_debug.h"

#include "soc/efuse_reg.h"
#include "esp_heap_caps.h"

#include "esp_system.h"
#include "esp_psram.h"
#include "esp_log.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

typedef struct {
    size_t size;
    void* buffer;
} ase32_swap_psram_t;

#define AS_PSRAM_STRUCT(HND) ((ase32_swap_psram_t*)HND)
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

esp_err_t ase32_swap_device_open_psram(void* hndl, const char* name, size_t *size) {
    hndl = heap_caps_malloc( sizeof(ase32_swap_psram_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);;
    if( (hndl) == NULL) return ESP_ERR_NO_MEM;

    if(esp_psram_is_initialized() != false) {
        esp_err_t err =  esp_psram_init();
        if(err != ESP_OK) return err;
    }
    portENTER_CRITICAL(&mux);

    AS_PSRAM_STRUCT(hndl)->size = esp_psram_get_size() / 4;
    AS_PSRAM_STRUCT(hndl)->buffer = heap_caps_malloc( AS_PSRAM_STRUCT(hndl)->size / 4, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);


    *size = esp_psram_get_size() / 4;

    if(AS_PSRAM_STRUCT(hndl)->buffer != NULL) {

        uint32_t magic = ASE32_SWAP_PARTITION_MAGIC;
        memcpy(AS_PSRAM_STRUCT(hndl)->buffer, &magic, sizeof(magic));
    
        portEXIT_CRITICAL(&mux);

        return ESP_OK;
    }
    portEXIT_CRITICAL(&mux);
    return ESP_ERR_NO_MEM;
}
esp_err_t ase32_swap_device_close_psram(void* hndl) {
    ase32_swap_psram_t* buffer = (ase32_swap_psram_t*)hndl;
    if(buffer == NULL) return ESP_ERR_INVALID_ARG;

    free(buffer->buffer);
    free(buffer);

    return ESP_OK;
}
size_t ase32_swap_device_write_psram(const void* hndl, void* data, size_t offset, size_t size) {
    ase32_swap_psram_t* buffer = (ase32_swap_psram_t*)hndl;
    if(buffer == NULL) return 0;

    portENTER_CRITICAL(&mux);
    if (offset + size > buffer->size) { 
        ESP_LOGE("ase32_swap", "Write exceeds psram size"); 
        return 0; } 
    
    memcpy((void*)((uint32_t)buffer->buffer + offset), data, size); 
    portEXIT_CRITICAL(&mux);

    return size;
}
size_t ase32_swap_device_read_psram(const void* hndl, void* data, size_t offset, size_t size) {
    ase32_swap_psram_t* buffer = (ase32_swap_psram_t*)hndl;
    if(buffer == NULL) return 0;

    portENTER_CRITICAL(&mux);
    if (offset + size > buffer->size) { 
        ESP_LOGE("ase32_swap", "Read exceeds psram size"); 
        return 0; 
    }
    memcpy(data, (void*)((uint32_t)buffer->buffer + offset), size);
    portEXIT_CRITICAL(&mux);

    return size;
}
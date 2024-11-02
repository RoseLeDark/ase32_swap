#include "esp_partition.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_attr.h"
#include <string.h>

#include "esp_private/esp_mmu_map_private.h"
#include "esp_mmu_map.h"

#include <esp_private/mmu_psram_flash.h>
#include <esp_heap_caps.h>

#include "ase32_swap_caps.h"
#include "ase32_swap_compress.h"
#include "ase32_swap_debug.h"

// Hilfsfunktion für die Kompression einer Seite
size_t ase32_swap_compress_page(ase32_swap_t* swap, const void* input, size_t input_size, void* output) {
    size_t compressed_size = 0;
    
    switch (swap->config.compression_type) {
        case ASE32_SWAP_COMPRESSION_LZ4:
            compressed_size = ase32_swap_compress_lz4(input, swap->config.block_size, output);
            break;
            
        case ASE32_SWAP_COMPRESSION_RLE:
            compressed_size = ase32_swap_compress_rle(input, swap->config.block_size, output);
            break;
        case ASE32_SWAP_COMPRESSION_USER:
            if(ase32_swap_compress_usr) {
                compressed_size = ase32_swap_compress_usr(input, swap->config.block_size, output);
                break;
            }
        case ASE32_SWAP_COMPRESSION_NONE:
            memcpy(output, input, swap->config.block_size);
            compressed_size = swap->config.block_size;
            break;
    }
    
    if (compressed_size == 0) {
        if (ase32_swap_debug_trace)
            ase32_swap_debug_trace(ESP_LOG_ERROR, "Kompression fehlgeschlagen");

        return 0;
    }
    
    return compressed_size;
}

// Hilfsfunktion für die Dekompression einer Seite
size_t ase32_swap_decompress_page(ase32_swap_t* swap, const void* input, size_t input_size, void* output, size_t output_size)  {
    size_t decompressed_size = 0;
    
    switch (swap->config.compression_type) {
        case ASE32_SWAP_COMPRESSION_LZ4:
            decompressed_size = ase32_swap_decompress_lz4(input, input_size, output, swap->config.block_size);
            break;
            
        case ASE32_SWAP_COMPRESSION_RLE:
            decompressed_size = ase32_swap_decompress_rle(input, input_size, output, swap->config.block_size);
            break;
        case ASE32_SWAP_COMPRESSION_USER:
            if(ase32_swap_decompress_usr) {
                decompressed_size = ase32_swap_decompress_usr(input, input_size, output, swap->config.block_size);
                break;
            } 
        case ASE32_SWAP_COMPRESSION_NONE:
            memcpy(output, input, input_size);
            decompressed_size = input_size;
            break;
        
    }
    
    if (decompressed_size != swap->config.block_size) {
        if (ase32_swap_debug_trace)
            ase32_swap_debug_trace(ESP_LOG_ERROR, "Dekompression fehlgeschlagen");
        return 0;
    }
    
    return decompressed_size;
}

size_t ase32_swap_compress_rle(const uint8_t* input, size_t input_size, uint8_t* output) {
    size_t out_size = 0;
    for (size_t i = 0; i < input_size;) {
        uint8_t count = 1;
        uint8_t current = input[i];
        
        while (i + count < input_size && input[i+ count] == current && count < 255) {
            count++;
        }
        
        output[out_size++] = count;
        output[out_size++] = current;
        i += count;
    }
    return out_size;
}
size_t ase32_swap_decompress_rle(const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size) {
    size_t in_pos = 0;
    size_t out_pos = 0;
    
    while (in_pos < input_size && out_pos < output_size) {
        uint8_t count = input[in_pos++];
        uint8_t value = input[in_pos++];
        
        for (uint8_t i = 0; i < count && out_pos < output_size; i++) {
            output[out_pos++] = value;
        }
    }
    return out_pos;
}

size_t ase32_swap_compress_lz4(const uint8_t* input, size_t input_size, uint8_t* output) {
    
}
size_t ase32_swap_decompress_lz4(const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size) {

}
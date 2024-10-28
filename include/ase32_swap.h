#ifndef __ASE_32_SWAP_H__
#define __ASE_32_SWAP_H__

#include "ase32_config.h"
#include <stdint.h>
#include <stddef.h>
#include <esp_err.h>

#ifndef ASE32_SWAP_WEAK
#define ASE32_SWAP_WEAK __attribute__((weak))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_PARTITION_SUBTYPE_SWAP 0xFA

// Konfigurationsstruktur
typedef enum {
    ASE32_SWAP_COMPRESSION_NONE,
    ASE32_SWAP_COMPRESSION_LZ4,
    ASE32_SWAP_COMPRESSION_HEATSHRINK,
} ase32_swap_compression_t;

typedef enum {
    ASE32_SWAP_STORAGE_PARTITION,
    ASE32_SWAP_STORAGE_FILE,
} ase32_swap_storage_t;

typedef enum {
    ASE32_SWAP_SECURE_NOSE,
    ASE32_SWAP_SECURE_XOR,
    ASE32_SWAP_SECURE_USER, // Implementiert schwache Hooks
} ase32_swap_secure_t;

typedef enum {
    ASE32_SWAP_BLOCK_SIZE_4096 = 4096,
    ASE32_SWAP_BLOCK_SIZE_2048 = 2048,
    ASE32_SWAP_BLOCK_SIZE_1024 = 1024,
    ASE32_SWAP_BLOCK_SIZE_512 = 512, // Kritisch
} ase32_swap_block_size_t;

typedef struct {
    uint16_t cached_pages;           // Standard: 32 CACHE_PAGES
    uint16_t max_pages;              // Standard: 256 MAX_SWAP_PAGES
    ase32_swap_block_size_t block_size; // Standard: 4096
    ase32_swap_storage_t storage_type;
#if ASE32_SWAP_CFG_USE_COMPRESS == 1
    ase32_swap_compression_t compression_type;
#endif
#if ASE32_SWAP_CFG_USE_SECURE == 1
    ase32_swap_secure_t secure_type;
#endif
    const char* storage_name; // Partition Label oder Dateiname
    size_t swap_address; // Startadresse im Speicher oder Offset in FILE
    size_t swap_size; // Größe des Swap-Speichers
} ase32_swap_cfg_t;

typedef struct {
    uint32_t virt_addr;
    uint32_t phys_addr;
    uint32_t partition_offset;
    bool is_dirty;
    bool is_in_memory;
    uint32_t last_access;
    uint8_t access_count;
    bool is_used;
} swap_page_t;

// Swap-Struktur
typedef struct {
    swap_page_t *pages; // [MAX_SWAP_PAGES];
    uint32_t page_count;
    const esp_partition_t* partition;
    FILE* file;           // Wenn File, dann bitte vorher öffnen!!
    uint32_t physic_size;
    uint8_t* cache_buffer;
    uint32_t cache_size;
    uint32_t access_counter;
    ase32_swap_cfg_t config;
} ase32_swap_t;

// Funktionen für die Bibliothek
esp_err_t ase32_swap_init(const ase32_swap_cfg_t cfg, ase32_swap_t* out);
void ase32_swap_deinit(ase32_swap_t* swap);
void* ase32_swap_malloc(ase32_swap_t* swap, size_t size);
void ase32_swap_free(ase32_swap_t* swap, void* ptr);
void ase32_swap_mark_dirty(ase32_swap_t* swap, void* ptr, size_t size);
esp_err_t ase32_swap_flush(ase32_swap_t* swap);
void ase32_swap_set_page_fault_handler(void (*handler)(ase32_swap_t* swap, void* arg, uint32_t addr), void* arg);
void ase32_swap_dump(const ase32_swap_t* swap);

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

#endif // __ASE_32_SWAP_H__

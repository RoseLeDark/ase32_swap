#ifndef __ASE32_SWAP_PCAPS_H__
#define __ASE32_SWAP_PCAPS_H__

#include <stdint.h>
#include <stddef.h>
#include <esp_err.h>
#include <esp_partition.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ESP_PARTITION_SUBTYPE_SWAP
#define ESP_PARTITION_SUBTYPE_SWAP 0xFA
#endif

// Konfigurationsstruktur
typedef enum {
    ASE32_SWAP_COMPRESSION_NONE = 1,
    ASE32_SWAP_COMPRESSION_LZ4 = 2,
    ASE32_SWAP_COMPRESSION_RLE = 4,
    ASE32_SWAP_COMPRESSION_USER = 250
} ase32_swap_compression_t;

typedef enum {
    ASE32_SWAP_STORAGE_PARTITION = 1,
    ASE32_SWAP_STORAGE_FILE = 2,
    ASE32_SWAP_STORAGE_PSRAM = 4,
    ASE32_SPAP_STORAGE_USR = 250,
} ase32_swap_storage_t;

typedef enum {
    ASE32_SWAP_SECURE_NONE = 1,
    ASE32_SWAP_SECURE_XOR = 2,
    ASE32_SWAP_SECURE_USER = 250, // Implementiert user Hooks
} ase32_swap_encryption_t;

typedef enum {
    ASE32_SWAP_BLOCK_SIZE_4096 = 4096,
    ASE32_SWAP_BLOCK_SIZE_2048 = 2048,
    ASE32_SWAP_BLOCK_SIZE_1024 = 1024,
    ASE32_SWAP_BLOCK_SIZE_512 = 512, 
    ASE32_SWAP_BLOCK_SIZE_256 = 256,
    ASE32_SWAP_BLOCK_SIZE_128 = 128,
    ASE32_SWAP_BLOCK_SIZE_64 = 64,
    ASE32_SWAP_BLOCK_SIZE_32 = 32,
} ase32_swap_block_size_t;

typedef enum {
    ASE32_SWAP_PRIORITY_LOW,
    ASE32_SWAP_PRIORITY_MIDDLE_LOW,
    ASE32_SWAP_PRIORITY_MIDDLE,
    ASE32_SWAP_PRIORITY_MIDDLE_HIGH,
    ASE32_SWAP_PRIORITY_HIGH,
}  ase32_swap_priority_t;


typedef struct {
    uint16_t cached_pages;           // Standard: 32 CACHE_PAGES
    uint16_t max_pages;              // Standard: 256 MAX_SWAP_PAGES
    ase32_swap_block_size_t block_size; // Standard: 4096
    ase32_swap_storage_t storage_type;
    ase32_swap_compression_t compression_type;
    ase32_swap_encryption_t secure_type;
    ase32_swap_priority_t start_priority;
    uint32_t xor_key;
    const char* storage_name; // Partition Label oder Dateiname
    size_t swap_address; // Startadresse im Speicher oder Offset in FILE
    size_t swap_size; // Größe des Swap-Speichers
} ase32_swap_cfg_t;

typedef struct {
    uint32_t    virt_addr;
    uint32_t    phys_addr;
    uint32_t    partition_offset;
    uint64_t    timestamp;
    bool        is_dirty;
    bool        is_in_memory;
    uint32_t    last_access;
    uint8_t     access_count;
    bool        is_used;
    uint16_t    priority;  
    uint32_t    hash; 
} swap_page_t;

// Swap-Struktur
typedef struct {
    swap_page_t         *pages; 
    uint32_t            page_count;
    union {
        esp_partition_t*    partition;
        FILE*               file;  
        void*               pointer;
    };      
    uint32_t            physic_size;
    uint8_t*            cache_buffer;
    uint32_t            cache_size;
    uint32_t            access_counter;
    ase32_swap_cfg_t    config;
} ase32_swap_t;

typedef struct {
    uint32_t hash;
    ase32_swap_t swap;
    
    ase32_swap_list_t *next;
    ase32_swap_list_t *prev;
} ase32_swap_list_t;  

typedef void (*ase32_swap_defrag_callback)(float progress);


#ifdef __cplusplus
}
#endif



#endif
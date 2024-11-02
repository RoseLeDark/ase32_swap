#include "ase32_swap_config.h"

ase32_swap_cfg_t ase32_swap_get_default_config() {
     ase32_swap_cfg_t config = {
        .cached_pages = 32,
        .max_pages = 4096,
        .block_size = ASE32_SWAP_BLOCK_SIZE_4096,
        .compression_type = ASE32_SWAP_COMPRESSION_LZ4,
        .secure_type = ASE32_SWAP_SECURE_NONE,
        .start_priority = ASE32_SWAP_PRIORITY_MIDDLE,
        .xor_key = 0xA34F,
        .storage_name = "swap"
    };
    return config;
}
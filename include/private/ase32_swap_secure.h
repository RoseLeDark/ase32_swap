#ifndef __ASE_32_SWAP_HEATSHRINK_H__
#define __ASE_32_SWAP_HEATSHRINK_H__

#include "ase32_config.h"
#include <stdint.h>
#include <stddef.h>

#ifndef ASE32_SWAP_WEAK
#define ASE32_SWAP_WEAK __attribute__((weak))
#endif

#if ASE32_SWAP_CFG_USE_SECURE == 1

size_t ase32_swap_heats_com(const void* src, size_t src_size, void* dst, size_t dst_size);
size_t ase32_swap_heats_decom(const void* src, size_t src_size, void* dst, size_t dst_size);
size_t ase32_swap_lz4_com(const void* src, size_t src_size, void* dst, size_t max_dst_size);
size_t ase32_swap_lz4_decom(const void* src, size_t src_size, void* dst, size_t dst_size);

// User secure - encrypt and decrypt functions
size_t ASE32_SWAP_WEAK ase32_swap_user_com(const void* src, size_t src_size, void* dst, size_t max_dst_size);
size_t ASE32_SWAP_WEAK ase32_swap_user_decom(const void* src, size_t src_size, void* dst, size_t max_dst_size);

#endif // ASE32_SWAP_CFG_USE_SECURE

#endif // __ASE_32_SWAP_HEATSHRINK_H__

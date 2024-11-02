#ifndef __ASE_32_SWAP_COMPRESS_H__
#define __ASE_32_SWAP_COMPRESS_H__

#include "ase32_swap_config.h"
#include "ase32_swap_caps.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t ase32_swap_compress_page(ase32_swap_t* swap, const void* input, size_t input_size, void* output); //
size_t ase32_swap_decompress_page(ase32_swap_t* swap, const void* input, size_t input_size, void* output, size_t output_size) ; //

size_t ase32_swap_compress_rle(const uint8_t* input, size_t input_size, uint8_t* output); //
size_t ase32_swap_decompress_rle(const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size); //

size_t ase32_swap_compress_lz4(const uint8_t* input, size_t input_size, uint8_t* output);
size_t ase32_swap_decompress_lz4(const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size);

size_t ASE32_SWAP_WEAK ase32_swap_compress_usr(const uint8_t* input, size_t input_size, uint8_t* output); //
size_t ASE32_SWAP_WEAK ase32_swap_decompress_usr(const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size); //

#ifdef __cplusplus
}
#endif

#endif
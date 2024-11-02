#ifndef __ASE_32_SWAP_CRYPT_H__
#define __ASE_32_SWAP_CRYPT_H__

#include "ase32_swap_config.h"
#include "ase32_swap_caps.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// User secure - encrypt and decrypt functions

esp_err_t ase32_swap_encrypt(ase32_swap_t* swap, void* data, size_t size); //
esp_err_t ase32_swap_decrypt(ase32_swap_t* swap, void* data, size_t size); //

size_t ase32_swap_encrypt_xor(uint8_t* data, size_t size, uint32_t key); //
size_t ase32_swap_decrypt_xor(uint8_t* data, size_t size, uint32_t key); //

size_t ASE32_SWAP_WEAK ase32_swap_encrypt_usr(uint8_t* data, size_t size, uint32_t key); //
size_t ASE32_SWAP_WEAK ase32_swap_decrypt_usr(uint8_t* data, size_t size, uint32_t key); //

#ifdef __cplusplus
}
#endif

#endif // __ASE_32_SWAP_CRYPT_H__

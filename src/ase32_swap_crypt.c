#include "ase32_swap_crypt.h"


esp_err_t ase32_swap_encrypt_xor(uint8_t* data, size_t size, uint32_t key) {
    uint8_t* bytes = (uint8_t*)data;
    uint8_t key = (key & 0xFF) ^ 0x55;

    for (size_t i = 0; i < size; i++) {
        bytes[i] ^= key;
        key = ((key << 1) | (key >> 7)); // Rotiere Schlüssel
    }
    return ESP_OK;
}
esp_err_t ase32_swap_decrypt_xor(uint8_t* data, size_t size, uint32_t key) {
    return ase32_swap_encrypt_xor(data, size, key);
}
// Sicherheitsfunktionen
esp_err_t ase32_swap_encrypt(ase32_swap_t* swap, void* data, size_t size) {
    size_t encrypted_size = 0;

    switch (swap->config.secure_type) {
        case ASE32_SWAP_SECURE_XOR: {
            // Einfache XOR-Verschlüsselung mit rotierendem Schlüssel
            encrypted_size = ase32_swap_encrypt_xor(data, size, swap->config.xor_key);
            break;
        }
        case ASE32_SWAP_SECURE_USER:
            // Nutze benutzerdefinierte Verschlüsselung
            if (ase32_swap_encrypt_usr) {
                encrypted_size = ase32_swap_encrypt_usr(data, size, swap->config.xor_key);
            }
            break;
        case ASE32_SWAP_SECURE_NONE:
        default:
            encrypted_size = size;
            break;
    }
    if (encrypted_size != size) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}
// ase32_unsecure_page_data
esp_err_t ase32_swap_decrypt(ase32_swap_t* swap, void* data, size_t size) {
    size_t decrypted_size = 0;

    switch (swap->config.secure_type) {
        case ASE32_SWAP_SECURE_XOR: {
            // Inverse XOR-Operation
            decrypted_size = ase32_swap_encrypt_xor(data, size, swap->config.xor_key);
            break;
        }
        case ASE32_SWAP_SECURE_USER:
            if (ase32_swap_decrypt_usr) {
                decrypted_size = ase32_swap_decrypt_usr(data, size, swap->config.xor_key);
            }
            break;
        case ASE32_SWAP_SECURE_NONE:
        default:
            decrypted_size = size;
            break;
    }
    if (decrypted_size != size) {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}


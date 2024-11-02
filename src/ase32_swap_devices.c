#include "ase32_swap_devices.h"


esp_err_t ase32_swap_device_open(const ase32_swap_cfg_t cfg, ase32_swap_t* out) {
    esp_err_t _ret = ESP_FAIL;
    void* pHandle;
    size_t physic_size = 0;

    switch (cfg.storage_type) {
    case ASE32_SWAP_STORAGE_PARTITION:
        _ret = ase32_swap_device_open_partition(pHandle, cfg.storage_name, &physic_size);
        break;
    case ASE32_SWAP_STORAGE_FILE:
        _ret = ase32_swap_device_open_file(pHandle, cfg.storage_name, &physic_size);
        break;
    case ASE32_SWAP_STORAGE_PSRAM:
        _ret = ase32_swap_device_open_psram(pHandle, cfg.storage_name, &physic_size);
        break;
    case ASE32_SPAP_STORAGE_USR:
        if(ase32_swap_device_open_usr) {
            _ret = ase32_swap_device_open_usr(pHandle, cfg.storage_name, &physic_size);
        }
        break;
    }

    if(_ret == ESP_OK) {
        out->pointer = pHandle;
        out->physic_size = physic_size;
    }

    return _ret;
}

esp_err_t ase32_swap_device_close(const ase32_swap_t* swap) {
    esp_err_t _ret = ESP_FAIL;

    switch (swap->config.storage_type) {
    case ASE32_SWAP_STORAGE_PARTITION:
        _ret = ase32_swap_device_close_partition(swap->pointer);
        break;
    case ASE32_SWAP_STORAGE_FILE:
        _ret = ase32_swap_device_close_file(swap->pointer);
        break;
    case ASE32_SWAP_STORAGE_PSRAM:
        _ret = ase32_swap_device_close_psram(swap->pointer);
        break;
    case ASE32_SPAP_STORAGE_USR:
        if(ase32_swap_device_close_usr) {
            _ret = ase32_swap_device_close_usr(swap->pointer);
        }
        break;
    }

    return _ret;
}

size_t ase32_swap_device_write(const ase32_swap_t* swap, void* data, size_t offset, size_t size) {
    size_t _ret = 0;

    switch (swap->config.storage_type) {
    case ASE32_SWAP_STORAGE_PARTITION:
        _ret = ase32_swap_device_write_partition(swap->pointer, data, offset, size);
        break;
    case ASE32_SWAP_STORAGE_FILE:
        _ret = ase32_swap_device_write_file(swap->pointer, data, offset, size);
        break;
    case ASE32_SWAP_STORAGE_PSRAM:
        _ret = ase32_swap_device_write_psram(swap->pointer, data, offset, size);
        break;
    case ASE32_SPAP_STORAGE_USR:
        if(ase32_swap_device_write_usr) {
            _ret = ase32_swap_device_write_usr(swap->pointer, data, offset, size);
        }
        break;
    }

    return _ret;
}

size_t ase32_swap_device_read(const ase32_swap_t* swap, void* data, size_t offset, size_t size) {
    size_t _ret = 0;

    switch (swap->config.storage_type) {
    case ASE32_SWAP_STORAGE_PARTITION:
        _ret = ase32_swap_device_read_partition(swap->pointer, data, offset, size);
        break;
    case ASE32_SWAP_STORAGE_FILE:
        _ret = ase32_swap_device_read_file(swap->pointer, data, offset, size);
        break;
    case ASE32_SWAP_STORAGE_PSRAM:
        _ret = ase32_swap_device_read_psram(swap->pointer, data, offset, size);
        break;
    case ASE32_SPAP_STORAGE_USR:
        if(ase32_swap_device_write_usr) {
            _ret = ase32_swap_device_read_usr(swap->pointer, data, offset, size);
        }
        break;
    }

    return _ret;
}
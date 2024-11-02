#ifndef __ASE32_SWAP_TARGET_H__
#define __ASE32_SWAP_TARGET_H__

#include <stdint.h>
#include <stddef.h>
#include <esp_err.h>

#include "ase32_swap_caps.h"
#include "ase32_swap_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ASE32_SWAP_PARTITION_MAGIC
#define ASE32_SWAP_PARTITION_MAGIC 0xFEFE
#endif // ASE32_SWAP_PARTITION_MAGIC

esp_err_t ase32_swap_device_open(const ase32_swap_cfg_t cfg, ase32_swap_t* out); //
esp_err_t ase32_swap_device_close(const ase32_swap_t* swap);//
size_t ase32_swap_device_write(const ase32_swap_t* swap, void* data, size_t offset, size_t size);//
size_t ase32_swap_device_read(const ase32_swap_t* swap, void* data, size_t offset, size_t size);//

esp_err_t ase32_swap_device_open_partition(void* hndl, const char* name, size_t *size);//
esp_err_t ase32_swap_device_close_partition(void* hndl);//
size_t ase32_swap_device_write_partition(const void* hndl, void* data, size_t offset, size_t size);//
size_t ase32_swap_device_read_partition(const void* hndl, void* data, size_t offset, size_t size);//

esp_err_t ase32_swap_device_open_file(void* hndl, const char* name, size_t *size);//
esp_err_t ase32_swap_device_close_file(void* hndl);//
size_t ase32_swap_device_write_file(const void* hndl, void* data, size_t offset, size_t size);//
size_t ase32_swap_device_read_file(const void* hndl, void* data, size_t offset, size_t size);//

esp_err_t ase32_swap_device_open_psram(void* hndl, const char* name, size_t *size);
esp_err_t ase32_swap_device_close_psram(void* hndl);
size_t ase32_swap_device_write_psram(const void* hndl, void* data, size_t offset, size_t size);
size_t ase32_swap_device_read_psram(const void* hndl, void* data, size_t offset, size_t size);

esp_err_t ASE32_SWAP_WEAK ase32_swap_device_open_usr(void* hndl, const char* name, size_t *size);//
esp_err_t ASE32_SWAP_WEAK ase32_swap_device_close_usr(void* hndl);//
size_t ASE32_SWAP_WEAK ase32_swap_device_write_usr(const void* hndl, void* data, size_t offset, size_t size);//
size_t ASE32_SWAP_WEAK ase32_swap_device_read_usr(const void* hndl, void* data, size_t offset, size_t size);//

#ifdef __cplusplus
}
#endif

#endif
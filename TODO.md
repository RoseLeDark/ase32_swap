# TODO

## ase32_swap/include/ase32_swap_compress.h
- size_t ase32_swap_compress_lz4(const uint8_t* input, size_t input_size, uint8_t* output);
- size_t ase32_swap_decompress_lz4(const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size);

## ase32_swap/include/ase32_swap_device.h
- esp_err_t ase32_swap_device_open_psram(void* hndl, const char* name, size_t *size);
- esp_err_t ase32_swap_device_close_psram(void* hndl);
- size_t ase32_swap_device_write_psram(const void* hndl, void* data, size_t offset, size_t size);
- size_t ase32_swap_device_read_psram(const void* hndl, void* data, size_t offset, size_t size);

## ase32_swap/ase32_swap.h
- ALL to new system


## Futures
- swaptabe
- debug trace
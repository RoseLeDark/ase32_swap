#ifndef __ASE32_SWAP_CONFIG_H__
#define __ASE32_SWAP_CONFIG_H__

#ifndef ASE32_SWAP_WEAK
#define ASE32_SWAP_WEAK __attribute__((weak))
#endif

#include "ase32_swap_caps.h"

#ifdef __cplusplus
extern "C" {
#endif

ase32_swap_cfg_t ase32_swap_get_default_config();

#ifdef __cplusplus
}
#endif

#endif // ifndef __ASE32_SWAP_CONFIG_H__
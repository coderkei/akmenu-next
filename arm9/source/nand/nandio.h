/*
 * Source: DS-Homebrew/TWiLightMenu, title/arm9/source/nand/nandio.h
 * https://github.com/DS-Homebrew/TWiLightMenu
 * Adapted for AKMenu-Next.
 */

#pragma once

#include <nds.h>
#include <nds/disc_io.h>

#ifdef __cplusplus
extern "C" {
#endif

void nandio_set_fat_sig_fix(u32 offset);
const char* nandio_get_error(void);

extern const DISC_INTERFACE io_dsi_nand;

#ifdef __cplusplus
}
#endif

/*
 * Source: DS-Homebrew/TWiLightMenu, title/arm9/source/nand/f_xy.h
 * https://github.com/DS-Homebrew/TWiLightMenu
 * Adapted for AKMenu-Next.
 */

#ifndef _H_F_XY
#define _H_F_XY

#ifdef __cplusplus
extern "C" {
#endif

void F_XY(uint32_t *key, uint32_t *key_x, uint32_t *key_y);
void F_XY_reverse(uint32_t *key, uint32_t *key_xy);

#ifdef __cplusplus
}
#endif

#endif


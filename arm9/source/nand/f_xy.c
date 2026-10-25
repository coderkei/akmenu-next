/*
 * Source: DS-Homebrew/TWiLightMenu, title/arm9/source/nand/f_xy.c
 * https://github.com/DS-Homebrew/TWiLightMenu
 * Adapted for AKMenu-Next.
 */

#include <string.h>
#include <stdint.h>
#include "f_xy.h"

static const uint64_t key_magic[2] = {
	UINT64_C(0x2a680f5f1a4f3e79), UINT64_C(0xfffefb4e29590258)
};

static void n128_lrot(uint64_t *num, uint32_t shift)
{
	uint64_t tmp[2];

	tmp[0] = num[0]<<shift;
	tmp[1] = num[1]<<shift;
	tmp[0] |= (num[1]>>(64-shift));
	tmp[1] |= (num[0]>>(64-shift));

	num[0] = tmp[0];
	num[1] = tmp[1];
}
static void n128_rrot(uint64_t *num, uint32_t shift)
{
	uint64_t tmp[2];

	tmp[0] = num[0]>>shift;
	tmp[1] = num[1]>>shift;
	tmp[0] |= (num[1]<<(64-shift));
	tmp[1] |= (num[0]<<(64-shift));

	num[0] = tmp[0];
	num[1] = tmp[1];
}

static void n128_add(uint64_t *a, const uint64_t *b)
{
	const uint64_t low = a[0];
	a[0] += b[0];
	a[1] += b[1] + (a[0] < low);
}

static void n128_sub(uint64_t *a, const uint64_t *b)
{
	const uint64_t borrow = a[0] < b[0];
	a[0] -= b[0];
	a[1] -= b[1] + borrow;
}

// Use uint64_t storage to avoid strict-aliasing violations under -O3.
void F_XY(uint32_t *key, uint32_t *key_x, uint32_t *key_y)
{
	uint64_t value[2];
	uint64_t other[2];
	memcpy(value, key_x, sizeof(value));
	memcpy(other, key_y, sizeof(other));
	value[0] ^= other[0];
	value[1] ^= other[1];
	n128_add(value, key_magic);
	n128_lrot(value, 42);
	memcpy(key, value, sizeof(value));
}

//F_XY_reverse does the reverse of F(X^Y): takes (normal)key, and does F in reverse to generate the original X^Y key_xy.
void F_XY_reverse(uint32_t *key, uint32_t *key_xy)
{
	uint64_t value[2];
	memcpy(value, key, sizeof(value));
	n128_rrot(value, 42);
	n128_sub(value, key_magic);
	memcpy(key_xy, value, sizeof(value));
}


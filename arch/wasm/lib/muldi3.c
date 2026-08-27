// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/lib/muldi3.c: 64x64 -> 128 multiply.
 *
 * wasm32 has no 128-bit integer instructions; clang lowers 128-bit
 * multiply to a call to __multi3 (the compiler-rt runtime function).
 * Implement it with 32-bit limbs so the emulation itself does not
 * recurse into __multi3.
 */

typedef unsigned int u32;
typedef unsigned long long u64;
typedef unsigned __int128 u128;

static u128 mul_64x64(u64 a, u64 b)
{
	u32 a0 = a, a1 = a >> 32;
	u32 b0 = b, b1 = b >> 32;
	u64 r0, r1, r2;

	r0 = (u64)a0 * b0;
	r1 = (u64)a0 * b1 + (u64)a1 * b0 + (r0 >> 32);
	r2 = (u64)a1 * b1 + (r1 >> 32);

	return ((u128)(u32)r2 << 64) | ((u64)(u32)r1 << 32) | (u32)r0;
}

__int128 __multi3(__int128 a, __int128 b)
{
	u128 ua = (u128)a, ub = (u128)b;
	u64 a0 = (u64)ua, b0 = (u64)ub;
	u64 ah = (u64)(ua >> 64), bh = (u64)(ub >> 64);
	u128 res;

	res  = mul_64x64(a0, b0);
	res += (u128)mul_64x64(a0, bh) << 64;
	res += (u128)mul_64x64(ah, b0) << 64;
	return (__int128)res;
}

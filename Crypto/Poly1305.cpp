/*
 * Based on:
 * Public Domain poly1305 from Andrew Moon
 * Based on poly1305-donna.c, poly1305-donna-32.h and poly1305-donna.h from:
 *   https://github.com/floodyberry/poly1305-donna
 */

#include "Crypto/Crypto.h"
#include <stddef.h>
#include <stdint.h>

namespace Crypto
{
	/*
	 * poly1305 implementation using 32 bit * 32 bit = 64 bit multiplication
	 * and 64 bit addition.
	 */

	namespace _Poly1305
	{
		/* interpret four 8 bit unsigned integers as a 32 bit unsigned integer in little endian */
		static uint32_t inline U8TO32(const uint8_t *p)
		{
			return (((uint32_t)(p[0] & 0xff)) |
				((uint32_t)(p[1] & 0xff) << 8) |
				((uint32_t)(p[2] & 0xff) << 16) |
				((uint32_t)(p[3] & 0xff) << 24));
		}

		/* store a 32 bit unsigned integer as four 8 bit unsigned integers in little endian */
		static void	inline U32TO8(uint8_t *p, uint32_t v)
		{
			p[0] = (v) & 0xff;
			p[1] = (v >> 8) & 0xff;
			p[2] = (v >> 16) & 0xff;
			p[3] = (v >> 24) & 0xff;
		}
	}
	
	void Poly1305::init(const uint8_t key[KEY_SIZE_BYTES])
	{
		/* r &= 0xffffffc0ffffffc0ffffffc0fffffff */
		r[0] = (_Poly1305::U8TO32(&key[0])) & 0x3ffffff;
		r[1] = (_Poly1305::U8TO32(&key[3]) >> 2) & 0x3ffff03;
		r[2] = (_Poly1305::U8TO32(&key[6]) >> 4) & 0x3ffc0ff;
		r[3] = (_Poly1305::U8TO32(&key[9]) >> 6) & 0x3f03fff;
		r[4] = (_Poly1305::U8TO32(&key[12]) >> 8) & 0x00fffff;

		/* h = 0 */
		h[0] = 0;
		h[1] = 0;
		h[2] = 0;
		h[3] = 0;
		h[4] = 0;

		/* save pad for later */
		pad[0] = _Poly1305::U8TO32(&key[16]);
		pad[1] = _Poly1305::U8TO32(&key[20]);
		pad[2] = _Poly1305::U8TO32(&key[24]);
		pad[3] = _Poly1305::U8TO32(&key[28]);

		leftover = 0;
		final = 0;
	}

	void Poly1305::blocks(const uint8_t *m, uint32_t bytes)
	{
		const uint32_t hibit = (final) ? 0 : (1 << 24); /* 1 << 128 */
		uint32_t r0, r1, r2, r3, r4;
		uint32_t s1, s2, s3, s4;
		uint32_t h0, h1, h2, h3, h4;
		uint64_t d0, d1, d2, d3, d4;
		uint32_t c;

		r0 = r[0];
		r1 = r[1];
		r2 = r[2];
		r3 = r[3];
		r4 = r[4];

		s1 = r1 * 5;
		s2 = r2 * 5;
		s3 = r3 * 5;
		s4 = r4 * 5;

		h0 = h[0];
		h1 = h[1];
		h2 = h[2];
		h3 = h[3];
		h4 = h[4];

		while (bytes >= TAG_SIZE_BYTES) 
		{
			/* h += m[i] */
			h0 += (_Poly1305::U8TO32(m + 0)) & 0x3ffffff;
			h1 += (_Poly1305::U8TO32(m + 3) >> 2) & 0x3ffffff;
			h2 += (_Poly1305::U8TO32(m + 6) >> 4) & 0x3ffffff;
			h3 += (_Poly1305::U8TO32(m + 9) >> 6) & 0x3ffffff;
			h4 += (_Poly1305::U8TO32(m + 12) >> 8) | hibit;

			/* h *= r */
			d0 = ((uint64_t)h0 * r0) +
				((uint64_t)h1 * s4) +
				((uint64_t)h2 * s3) +
				((uint64_t)h3 * s2) +
				((uint64_t)h4 * s1);
			d1 = ((uint64_t)h0 * r1) +
				((uint64_t)h1 * r0) +
				((uint64_t)h2 * s4) +
				((uint64_t)h3 * s3) +
				((uint64_t)h4 * s2);
			d2 = ((uint64_t)h0 * r2) +
				((uint64_t)h1 * r1) +
				((uint64_t)h2 * r0) +
				((uint64_t)h3 * s4) +
				((uint64_t)h4 * s3);
			d3 = ((uint64_t)h0 * r3) +
				((uint64_t)h1 * r2) +
				((uint64_t)h2 * r1) +
				((uint64_t)h3 * r0) +
				((uint64_t)h4 * s4);
			d4 = ((uint64_t)h0 * r4) +
				((uint64_t)h1 * r3) +
				((uint64_t)h2 * r2) +
				((uint64_t)h3 * r1) +
				((uint64_t)h4 * r0);

			/* (partial) h %= p */
			c = (uint32_t)(d0 >> 26);
			h0 = (uint32_t)d0 & 0x3ffffff;
			d1 += c;
			c = (uint32_t)(d1 >> 26);
			h1 = (uint32_t)d1 & 0x3ffffff;
			d2 += c;
			c = (uint32_t)(d2 >> 26);
			h2 = (uint32_t)d2 & 0x3ffffff;
			d3 += c;
			c = (uint32_t)(d3 >> 26);
			h3 = (uint32_t)d3 & 0x3ffffff;
			d4 += c;
			c = (uint32_t)(d4 >> 26);
			h4 = (uint32_t)d4 & 0x3ffffff;
			h0 += c * 5;
			c = (h0 >> 26);
			h0 = h0 & 0x3ffffff;
			h1 += c;

			m += TAG_SIZE_BYTES;
			bytes -= TAG_SIZE_BYTES;
		}

		h[0] = h0;
		h[1] = h1;
		h[2] = h2;
		h[3] = h3;
		h[4] = h4;
	}

	void Poly1305::update(const uint8_t *m, uint32_t bytes)
	{
		uint32_t i;

		/* handle leftover */
		if (leftover) {
			uint32_t want = (TAG_SIZE_BYTES - leftover);
			if (want > bytes)
				want = bytes;
			for (i = 0; i < want; i++)
				buffer[leftover + i] = m[i];
			bytes -= want;
			m += want;
			leftover += want;
			if (leftover < TAG_SIZE_BYTES)
				return;
			blocks(buffer, TAG_SIZE_BYTES);
			leftover = 0;
		}

		/* process full blocks */
		if (bytes >= TAG_SIZE_BYTES)
		{
			uint32_t want = (bytes & ~(TAG_SIZE_BYTES - 1));
			blocks(m, want);
			m += want;
			bytes -= want;
		}

		/* store leftover */
		if (bytes) {
			for (i = 0; i < bytes; i++)
				buffer[leftover + i] = m[i];
			leftover += bytes;
		}
	}

	void Poly1305::finish(uint8_t tag[TAG_SIZE_BYTES])
	{
		uint32_t h0, h1, h2, h3, h4, c;
		uint32_t g0, g1, g2, g3, g4;
		uint64_t f;
		uint32_t mask;

		/* process the remaining block */
		if (leftover) {
			uint32_t i = leftover;
			buffer[i++] = 1;
			for (; i < TAG_SIZE_BYTES; i++)
				buffer[i] = 0;
			final = 1;
			blocks(buffer, TAG_SIZE_BYTES);
		}

		/* fully carry h */
		h0 = h[0];
		h1 = h[1];
		h2 = h[2];
		h3 = h[3];
		h4 = h[4];

		c = h1 >> 26;
		h1 = h1 & 0x3ffffff;
		h2 += c;
		c = h2 >> 26;
		h2 = h2 & 0x3ffffff;
		h3 += c;
		c = h3 >> 26;
		h3 = h3 & 0x3ffffff;
		h4 += c;
		c = h4 >> 26;
		h4 = h4 & 0x3ffffff;
		h0 += c * 5;
		c = h0 >> 26;
		h0 = h0 & 0x3ffffff;
		h1 += c;

		/* compute h + -p */
		g0 = h0 + 5;
		c = g0 >> 26;
		g0 &= 0x3ffffff;
		g1 = h1 + c;
		c = g1 >> 26;
		g1 &= 0x3ffffff;
		g2 = h2 + c;
		c = g2 >> 26;
		g2 &= 0x3ffffff;
		g3 = h3 + c;
		c = g3 >> 26;
		g3 &= 0x3ffffff;
		g4 = h4 + c - (1 << 26);

		/* select h if h < p, or h + -p if h >= p */
		mask = (g4 >> ((sizeof(uint32_t) * 8) - 1)) - 1;
		g0 &= mask;
		g1 &= mask;
		g2 &= mask;
		g3 &= mask;
		g4 &= mask;
		mask = ~mask;
		h0 = (h0 & mask) | g0;
		h1 = (h1 & mask) | g1;
		h2 = (h2 & mask) | g2;
		h3 = (h3 & mask) | g3;
		h4 = (h4 & mask) | g4;

		/* h = h % (2^128) */
		h0 = ((h0) | (h1 << 26)) & 0xffffffff;
		h1 = ((h1 >> 6) | (h2 << 20)) & 0xffffffff;
		h2 = ((h2 >> 12) | (h3 << 14)) & 0xffffffff;
		h3 = ((h3 >> 18) | (h4 << 8)) & 0xffffffff;

		/* mac = (h + pad) % (2^128) */
		f = (uint64_t)h0 + pad[0];
		h0 = (uint32_t)f;
		f = (uint64_t)h1 + pad[1] + (f >> 32);
		h1 = (uint32_t)f;
		f = (uint64_t)h2 + pad[2] + (f >> 32);
		h2 = (uint32_t)f;
		f = (uint64_t)h3 + pad[3] + (f >> 32);
		h3 = (uint32_t)f;

		_Poly1305::U32TO8(tag + 0, h0);
		_Poly1305::U32TO8(tag + 4, h1);
		_Poly1305::U32TO8(tag + 8, h2);
		_Poly1305::U32TO8(tag + 12, h3);

		/* zero out the state */
		h[0] = 0;
		h[1] = 0;
		h[2] = 0;
		h[3] = 0;
		h[4] = 0;
		r[0] = 0;
		r[1] = 0;
		r[2] = 0;
		r[3] = 0;
		r[4] = 0;
		pad[0] = 0;
		pad[1] = 0;
		pad[2] = 0;
		pad[3] = 0;
	}
}

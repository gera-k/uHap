/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#include "Crypto/Crypto.h"

namespace Crypto
{
	namespace _Sha512
	{
		static inline void set(uint8_t* d, int i, uint8_t v)
		{
			d[(i & ~7) + 7 - (i & 7)] = v;
		}

		static inline uint8_t get(uint8_t* d, int i)
		{
			return d[(i & ~7) + 7 - (i & 7)];
		}
	}

	Sha512::Sha512(const uint8_t* msg, uint32_t size, uint8_t* hash)
	{
		init();
		update(msg, size);
		fini(hash);
	}

	Sha512::Sha512()	// init/reset
	{
		init();
	}

	void Sha512::init() // reset context
	{
		h[0] = 0x6a09e667f3bcc908;
		h[1] = 0xbb67ae8584caa73b;
		h[2] = 0x3c6ef372fe94f82b;
		h[3] = 0xa54ff53a5f1d36f1;
		h[4] = 0x510e527fade682d1;
		h[5] = 0x9b05688c2b3e6c1f;
		h[6] = 0x1f83d9abfb41bd6b;
		h[7] = 0x5be0cd19137e2179;

		L = 0;
		p = 0;
	}

	void Sha512::update(const uint8_t* msg, uint32_t size)
	{
		L += size * 8;
		uint8_t* b = (uint8_t*)m;
		while (size > 0)
		{
			// copy up to BLOCK_SIZE_BYTES into current position of m
			uint32_t l = size;
			if (l > BLOCK_SIZE_BYTES)
				l = BLOCK_SIZE_BYTES;

			// p0 - buffer position before copy
			uint32_t p0 = p;

			for (uint32_t i = 0; i < l; i++)
			{
				_Sha512::set(b, p, msg[i]);
				p = (p + 1) % (BLOCK_SIZE_BYTES * 2);
			}

			msg += l;
			size -= l;

			// block boundary cross - calculate hash of complete block
			if (p0 < BLOCK_SIZE_BYTES && p >= BLOCK_SIZE_BYTES)
			{
				Sha512blk(h, m);
			}
			else if (p0 >= BLOCK_SIZE_BYTES && p < BLOCK_SIZE_BYTES)
			{
				Sha512blk(h, m + BLOCK_SIZE_WORDS);
			}
		}
	}
		
	void Sha512::fini(uint8_t* hash)
	{
		uint32_t ps;
		bool blk0 = false;
		bool blk1 = false;

		if (p < BLOCK_SIZE_BYTES - (128 / 8) - 1)	// padding fits into first block
		{
			ps = BLOCK_SIZE_BYTES - p;
			blk0 = true;
		}
		else if ((p >= BLOCK_SIZE_BYTES) && (p < BLOCK_SIZE_BYTES * 2 - (128 / 8) - 1)) 	// padding fits into second block
		{
			ps = BLOCK_SIZE_BYTES * 2 - p;
			blk1 = true;
		}
		else     // padding crosses block boundary
		{
			ps = BLOCK_SIZE_BYTES * 2 - p % BLOCK_SIZE_BYTES;
			blk0 = true;
			blk1 = true;
		}

		uint8_t* b = (uint8_t*)m;
		_Sha512::set(b, p, 0x80);
		p = (p + 1) % (BLOCK_SIZE_BYTES * 2);
		ps--;

		while (ps > 8)
		{
			_Sha512::set(b, p, 0);
			p = (p + 1) % (BLOCK_SIZE_BYTES * 2);
			ps--;
		}

		b[p + 0] = (L >> 0) & 0xFF;
		b[p + 1] = (L >> 8) & 0xFF;
		b[p + 2] = (L >> 16) & 0xFF;
		b[p + 3] = (L >> 24) & 0xFF;
		b[p + 4] = 0;
		b[p + 5] = 0;
		b[p + 6] = 0;
		b[p + 7] = 0;

		if (blk0)
			Sha512blk(h, m);
		if (blk1)
			Sha512blk(h, m + BLOCK_SIZE_WORDS);

		if (hash)
		{
			for (uint32_t i = 0; i < HASH_SIZE_BYTES; i++)
				hash[i] = _Sha512::get((uint8_t*)h, i);
		}
	}

	void Sha512::get(uint8_t* hash)
	{
		if (hash)
		{
			for (uint32_t i = 0; i < HASH_SIZE_BYTES; i++)
				hash[i] = _Sha512::get((uint8_t*)h, i);
		}
	}

	void Sha512::calc(const uint8_t* msg, uint32_t size, uint8_t* hash)
	{
		init();
		update(msg, size);
		fini(hash);
	}
}

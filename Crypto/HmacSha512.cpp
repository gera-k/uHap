/*
MIT License

Copyright(c) 2019 Gera Kazakov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Crypto/Crypto.h"

#include "string.h"

namespace Crypto
{
	HmacSha512::HmacSha512(const uint8_t *key, uint32_t key_size,
		const uint8_t *msg, uint32_t msg_len,
		uint8_t *mac, uint32_t mac_size)
	{
		start(key, key_size);
		next(msg, msg_len);
		end(mac, mac_size);
	}

	HmacSha512::HmacSha512(const uint8_t *key, uint32_t key_size)
	{
		start(key, key_size);
	}

	void HmacSha512::start(const uint8_t *key, uint32_t key_size)
	{
		const uint8_t *k = key;
		uint32_t s = key_size;
		uint8_t key_hash[Sha512::HASH_SIZE_BYTES];

		// if key_size > Sha512::BLOCK_SIZE_BYTES
		//	use K' = hash(K);
		if (key_size > Sha512::BLOCK_SIZE_BYTES)
		{
			Sha512 b(key, key_size, key_hash);
			k = key_hash;
			s = Sha512::HASH_SIZE_BYTES;
		}

		// i_blk = K' ^ 0x36
		// o_blk = K' ^ 0x5c
		for (uint32_t i = 0; i < Sha512::BLOCK_SIZE_BYTES; i++)
		{
			if (i < s)
			{
				i_blk[i] = k[i] ^ 0x36;
				o_blk[i] = k[i] ^ 0x5c;
			}
			else
			{
				i_blk[i] = 0x36;
				o_blk[i] = 0x5c;
			}
		}

		// i_sha += H(i_blk)
		// o_sha += H(o_blk)
		i_sha.update(i_blk, Sha512::BLOCK_SIZE_BYTES);
		o_sha.update(o_blk, Sha512::BLOCK_SIZE_BYTES);
	}

	void HmacSha512::next(const uint8_t *msg, uint32_t msg_len)
	{
		// i_sha += H(msg)
		i_sha.update(msg, msg_len);
	}

	void HmacSha512::end(uint8_t *mac, uint32_t mac_size)
	{
		uint8_t i_hash[Sha512::HASH_SIZE_BYTES];
		uint8_t o_hash[Sha512::HASH_SIZE_BYTES];

		// i_hash = i_sha
		i_sha.fini(i_hash);

		// o_sha += H(i_hash)
		o_sha.update(i_hash, Sha512::HASH_SIZE_BYTES);
		// o_hash = o_sha
		o_sha.fini(o_hash);

		// copy only requested number of bytes
		if (mac_size > Sha512::HASH_SIZE_BYTES)
			mac_size = Sha512::HASH_SIZE_BYTES;
		memcpy(mac, o_hash, mac_size);
	}
}

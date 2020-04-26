/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _CRYPTO_SHA512_BLOCK_H_
#define _CRYPTO_SHA512_BLOCK_H_

#include <stdint.h>

namespace Crypto
{
	typedef uint64_t sha512_word;

	static constexpr unsigned int SHA512_BLOCK_SIZE_BITS = 1024;
	static constexpr unsigned int SHA512_HASH_SIZE_BITS = 512;
	static constexpr unsigned int SHA512_BLOCK_SIZE_BYTES = SHA512_BLOCK_SIZE_BITS / 8;
	static constexpr unsigned int SHA512_HASH_SIZE_BYTES = SHA512_HASH_SIZE_BITS / 8;
	static constexpr unsigned int SHA512_BLOCK_SIZE_WORDS = SHA512_BLOCK_SIZE_BYTES / sizeof(sha512_word);
	static constexpr unsigned int SHA512_HASH_SIZE_WORDS = SHA512_HASH_SIZE_BYTES / sizeof(sha512_word);

	// calculates hash of 1024-bit block
	void Sha512blk(sha512_word h[SHA512_HASH_SIZE_WORDS], sha512_word m[SHA512_BLOCK_SIZE_WORDS]);
}

#endif /*_CRYPTO_SHA512_BLOCK_H_*/
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
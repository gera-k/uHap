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

namespace Crypto
{
	namespace _Chacha20
	{
		static uint32_t rotl(uint32_t a, int n)
		{
			return (a << n) | (a >> (32 - n));
		}

		static void inline qrnd(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d)
		{
			a += b; d = rotl(d ^ a, 16);
			c += d; b = rotl(b ^ c, 12);
			a += b; d = rotl(d ^ a, 8);
			c += d; b = rotl(b ^ c, 7);
		}
		static void inline qrnd(uint32_t* w, int a, int b, int c, int d)
		{
			qrnd(w[a], w[b], w[c], w[d]);
		}
	}

	// The inputs to ChaCha20 block function are :
	//	- A 256-bit key, treated as a concatenation of eight 32-bit little-endian integers.
	//	- A 32-bit block count parameter, treated as a 32-bit little-endian integer.
	//	- A 96-bit nonce, treated as a concatenation of three 32-bit little-Endian integers.
	// The output is 64 random-looking bytes.
	void Chacha20::block(
		const uint8_t key[KEY_SIZE_BYTES],
		uint32_t count,
		const uint8_t nonce[NONCE_SIZE_BYTES],
		uint8_t out[BLK_SIZE_BYTES]
	)
	{
		//  The ChaCha20 state is initialized as follows:
		//	- The first four words(0 - 3) are constants : 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574.
		blk[0] = 0x61707865;	// expa
		blk[1] = 0x3320646e;	// nd 3
		blk[2] = 0x79622d32;	// 2-by
		blk[3] = 0x6b206574;	// te k

		// The next eight words (4-11) are taken from the 256-bit key by
		//	reading the bytes in little - endian order, in 4 - byte chunks.
		for (unsigned i = 0; i < KEY_SIZE_WORDS; i++)
			blk[i + 4] = *(W*)(key + i * 4);

		// Word 12 is a block counter.
		blk[12] = count;

		// Words 13-15 are a nonce, which should not be repeated for the same
		//	key. The 13th word is the first 32 bits of the input nonce taken
		//	as a little - endian integer, while the 15th word is the last 32 bits.
		blk[13] = *(W*)(nonce + 0);
		blk[14] = *(W*)(nonce + 4);
		blk[15] = *(W*)(nonce + 8);

		W w[BLK_SIZE_WORDS];
		memcpy(w, blk, sizeof(w));

		for (int i = 0; i < 10; i++)
		{
			_Chacha20::qrnd(w, 0, 4, 8, 12);
			_Chacha20::qrnd(w, 1, 5, 9, 13);
			_Chacha20::qrnd(w, 2, 6, 10, 14);
			_Chacha20::qrnd(w, 3, 7, 11, 15);
			_Chacha20::qrnd(w, 0, 5, 10, 15);
			_Chacha20::qrnd(w, 1, 6, 11, 12);
			_Chacha20::qrnd(w, 2, 7, 8, 13);
			_Chacha20::qrnd(w, 3, 4, 9, 14);
		}

		for (unsigned i = 0; i < BLK_SIZE_WORDS; i++)
			blk[i] += w[i];

		if (out != NULL)
			for (unsigned i = 0; i < BLK_SIZE_WORDS; i++)
				*(W*)(out + i * 4) = blk[i];
	}

	// The inputs to ChaCha20 encrypt function are :
	//	-  A 256-bit key
	//  -  A 32-bit initial counter.This can be set to any number, but will
	//			usually be zero or one.It makes sense to use one if we use the
	//			zero block for something else, such as generating a one-time
	//			authenticator key as part of an AEAD algorithm.
	//  -  A 96-bit nonce.In some protocols, this is known as the
	//			Initialization Vector.
	//	-  An arbitrary-length plaintext
	// The output is an encrypted message, or "ciphertext", of the same	length.		
	void Chacha20::encrypt(
		const uint8_t key[KEY_SIZE_BYTES],
		uint32_t count,
		const uint8_t nonce[NONCE_SIZE_BYTES],
		const uint8_t* msg, uint32_t msg_len,
		uint8_t* out
	)
	{
		// algorythm in pseudocode
		//	chacha20_encrypt(key, counter, nonce, plaintext) :
		//	for j = 0 upto floor(len(plaintext) / 64) - 1
		//		key_stream = chacha20_block(key, counter + j, nonce)
		//		block = plaintext[(j * 64)..(j * 64 + 63)]
		//		encrypted_message += block ^ key_stream
		//	end
		//	if ((len(plaintext) % 64) != 0)
		//		j = floor(len(plaintext) / 64)
		//		key_stream = chacha20_block(key, counter + j, nonce)
		//		block = plaintext[(j * 64)..len(plaintext) - 1]
		//		encrypted_message += (block^key_stream)[0..len(plaintext) % 64]
		//	end
		//	return encrypted_message

		uint32_t len = msg_len;
		while (len > 0)
		{
			uint32_t l = len;
			if (l > BLK_SIZE_BYTES)
				l = BLK_SIZE_BYTES;

			// key_stream = chacha20_block(key, counter + j, nonce)
			block(key, count, nonce);

			// key_stream is in blk, xor with the message
			// TODO: XOR by machine words, arbitrary alignment
			uint32_t cb = l;
			uint8_t* k = (uint8_t*)blk;
			while (cb-- > 0)
				*out++ = *msg++ ^ *k++;

			len -= l;
			count++;
		}
	}

	Chacha20::Chacha20(
		const uint8_t key[KEY_SIZE_BYTES],
		uint32_t count,
		const uint8_t nonce[NONCE_SIZE_BYTES],
		const uint8_t* msg, uint32_t msg_len,
		uint8_t* out
	)
	{
		encrypt(key, count, nonce, msg, msg_len, out);
	}
}

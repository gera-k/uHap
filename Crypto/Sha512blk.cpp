/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#include "Crypto/Sha512blk.h"

namespace Crypto
{
	// assert sha512_word is 8 bytes
	static_assert(sizeof(sha512_word) == sizeof(uint64_t), "Unsupported sha512_word");

	static inline sha512_word rrt(sha512_word d, int n)
	{
		return (d >> n) | (d << (64 - n));
	}

	static inline sha512_word rsh(sha512_word d, int n)
	{
		return (d >> n);
	}

	static inline sha512_word maj(sha512_word a, sha512_word b, sha512_word c)
	{
		return (a & b) ^ (a & c) ^ (b & c);
	}

	static inline sha512_word ch(sha512_word e, sha512_word f, sha512_word g)
	{
		return (e & f) ^ (~e & g);
	}

	static inline sha512_word s0(sha512_word d)
	{
		return rrt(d, 1) ^ rrt(d, 8) ^ rsh(d, 7);
	}

	static inline sha512_word s1(sha512_word d)
	{
		return rrt(d, 19) ^ rrt(d, 61) ^ rsh(d, 6);
	}

	static inline sha512_word S0(sha512_word d)
	{
		return rrt(d, 28) ^ rrt(d, 34) ^ rrt(d, 39);
	}

	static inline sha512_word S1(sha512_word d)
	{
		return rrt(d, 14) ^ rrt(d, 18) ^ rrt(d, 41);
	}

	// calculates hash of 1024-bit block
	void Sha512blk(sha512_word h[SHA512_HASH_SIZE_WORDS], sha512_word m[SHA512_BLOCK_SIZE_WORDS])
	{
		static const sha512_word sha512_k[80] =
		{
			0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc, 0x3956c25bf348b538,
			0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118, 0xd807aa98a3030242, 0x12835b0145706fbe,
			0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2, 0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235,
			0xc19bf174cf692694, 0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
			0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5, 0x983e5152ee66dfab,
			0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4, 0xc6e00bf33da88fc2, 0xd5a79147930aa725,
			0x06ca6351e003826f, 0x142929670a0e6e70, 0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed,
			0x53380d139d95b3df, 0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
			0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30, 0xd192e819d6ef5218,
			0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8, 0x19a4c116b8d2d0c8, 0x1e376c085141ab53,
			0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8, 0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373,
			0x682e6ff3d6b2b8a3, 0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
			0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b, 0xca273eceea26619c,
			0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178, 0x06f067aa72176fba, 0x0a637dc5a2c898a6,
			0x113f9804bef90dae, 0x1b710b35131c471b, 0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc,
			0x431d67c49c100d4c, 0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817
		};

		static sha512_word w[80];

		// copy chunk into first 16 words w[0..15] of the message schedule array
		// Extend the first 16 words into the remaining words w[16..79] of the message schedule array
		for (int i = 0; i < 80; i++)
			if (i < 16)
				w[i] = m[i];
			else
				w[i] = s1(w[i - 2]) + w[i - 7] + s0(w[i - 15]) + w[i - 16];

		// Initialize working variables to current hash value
		sha512_word
			ta = h[0],
			tb = h[1],
			tc = h[2],
			td = h[3],
			te = h[4],
			tf = h[5],
			tg = h[6],
			th = h[7];

		// Compression function main loop :
		for (int l = 0; l < 80; l++)
		{
			sha512_word t1 = th + S1(te) + ch(te, tf, tg) + sha512_k[l] + w[l];
			sha512_word t2 = t1 + S0(ta) + maj(ta, tb, tc);

			th = tg;
			tg = tf;
			tf = te;
			te = td + t1;
			td = tc;
			tc = tb;
			tb = ta;
			ta = t2;
		}

		// Add the compressed chunk to the current hash value
		h[0] += ta;
		h[1] += tb;
		h[2] += tc;
		h[3] += td;
		h[4] += te;
		h[5] += tf;
		h[6] += tg;
		h[7] += th;
	}

}
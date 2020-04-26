/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#include "CryptoTest/CryptoTest.h"

namespace CryptoTest
{

	int sha512_test()
	{
		// Sha512 test vectors
		static struct _sha512_tst
		{
			const char* m;
			Crypto::sha512_word v[8];
		} sha512_tst[] =
		{
		#if 1
			{
				"",
				{ 0xcf83e1357eefb8bd, 0xf1542850d66d8007, 0xd620e4050b5715dc, 0x83f4a921d36ce9ce, 0x47d0d13c5d85f2b0, 0xff8318d2877eec2f, 0x63b931bd47417a81, 0xa538327af927da3e }
			},
		#endif
		#if 1
			{
				"abc",
				{ 0xddaf35a193617aba, 0xcc417349ae204131, 0x12e6fa4e89a97ea2, 0x0a9eeee64b55d39a, 0x2192992a274fc1a8, 0x36ba3c23a3feebbd, 0x454d4423643ce80e, 0x2a9ac94fa54ca49f }
			},
		#endif
		#if 1
			{
				"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
				{ 0x204a8fc6dda82f0a, 0x0ced7beb8e08a416, 0x57c16ef468b228a8, 0x279be331a703c335, 0x96fd15c13b1b07f9, 0xaa1d3bea57789ca0, 0x31ad85c7a71dd703, 0x54ec631238ca3445 }
			},
		#endif
		#if 1
			{
				"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
				{ 0x8e959b75dae313da, 0x8cf4f72814fc143f, 0x8f7779c6eb9f7fa1, 0x7299aeadb6889018, 0x501d289e4900f7e4, 0x331b99dec4b5433a, 0xc7d329eeb6dd2654, 0x5e96e55b874be909 }
			},
		#endif
		#if 0
			{
				"LONG",
				{ 0xe718483d0ce76964, 0x4e2e42c7bc15b463, 0x8e1f98b13b204428, 0x5632a803afa973eb, 0xde0ff244877ea60a, 0x4cb0432ce577c31b, 0xeb009c5c2c49aa2e, 0x4eadb217ad8cc09b }
			},
		#endif
		};

		int r = 0;

		LOG_MSG("SHA512 test\n");

		for (unsigned t = 0; t < sizeofarr(sha512_tst); t++)
		{
			// test values
			char* msg = (char*)sha512_tst[t].m;
			uint32_t len = (uint32_t)strlen(msg);

			LOG_MSG("Test %d: '%s'\n", t, sha512_tst[t].m);

			if (strcmp(msg, "LONG") == 0)
			{
				len = 1000000;
				msg = (char*)malloc(len);
				memset(msg, 'a', len);
			}

			{	// do whole message at once
				Crypto::Sha512 sha((uint8_t*)msg, len);
				for (int i = 0; i < 8; i++)
				{
					if (sha.h[i] != sha512_tst[t].v[i])
					{
						LOG_MSG("a. Word %d error: %016llX  %016llX\n", i, sha.h[i], sha512_tst[t].v[i]);
						r++;
					}
				}
			}

			{	// do it in 100 byte increments
				Crypto::Sha512 sha;
				uint32_t MAX = 100;
				while (len > 0)
				{
					uint32_t l = len;
					if (l > MAX)
						l = MAX;
					sha.update((uint8_t*)msg, l);
					msg += l;
					len -= l;
				}
				sha.fini();
				for (int i = 0; i < 8; i++)
				{
					if (sha.h[i] != sha512_tst[t].v[i])
					{
						LOG_MSG("b. Word %d error: %016llX  %016llX\n", i, sha.h[i], sha512_tst[t].v[i]);
						r++;
					}
				}
			}
		}

		return r;
	}
}
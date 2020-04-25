/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#define HAP_LOG_DECLARE hapCryptoTest
#include "Platform.h"

#include "Crypto/Test/CryptoTest.h"

namespace CryptoTest
{
	int poly1305_test()
	{
		int r = 0;

		uint8_t key[Crypto::Poly1305::KEY_SIZE_BYTES] =
		{
			0x85, 0xd6, 0xbe, 0x78, 0x57, 0x55, 0x6d, 0x33, 0x7f, 0x44, 0x52, 0xfe, 0x42, 0xd5, 0x06, 0xa8,
			0x01, 0x03, 0x80, 0x8a, 0xfb, 0x0d, 0xb2, 0xfd, 0x4a, 0xbf, 0xf6, 0xaf, 0x41, 0x49, 0xf5, 0x1b
		};

		char msg[] = "Cryptographic Forum Research Group";

		uint8_t v[Crypto::Poly1305::TAG_SIZE_BYTES] = 
		{
			0xa8, 0x06, 0x1d, 0xc1, 0x30, 0x51, 0x36, 0xc6, 0xc2, 0x2b, 0x8b, 0xaf, 0x0c, 0x01, 0x27, 0xa9
		};

		uint8_t t[Crypto::Poly1305::TAG_SIZE_BYTES];

		HAP_PRINTF("Poly1305 test");

		Crypto::Poly1305 p(key, (const uint8_t*)msg, (uint32_t)strlen(msg), t);

		if (memcmp(v, t, sizeof(v)) != 0)
		{
			r++;
		}

		return r;
	}
}
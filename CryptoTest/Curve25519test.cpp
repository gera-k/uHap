/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#include "CryptoTest/CryptoTest.h"

namespace CryptoTest
{
	int curve25519_test()
	{
		unsigned char e1k[Crypto::Curve25519::KEY_SIZE_BYTES];
		unsigned char e2k[Crypto::Curve25519::KEY_SIZE_BYTES];
		unsigned char e1e2k[Crypto::Curve25519::KEY_SIZE_BYTES];
		unsigned char e2e1k[Crypto::Curve25519::KEY_SIZE_BYTES];
		unsigned char e1[Crypto::Curve25519::KEY_SIZE_BYTES] = { 3 };
		unsigned char e2[Crypto::Curve25519::KEY_SIZE_BYTES] = { 5 };
		unsigned char k[Crypto::Curve25519::KEY_SIZE_BYTES] = { 9 };

		int r = 0;

		LOG_MSG("Curve25519 test\n");

		for (int loop = 0; loop < 10; loop++)
		{
			unsigned i;

			Crypto::Curve25519::calculate(e1k, e1, k);			// pubkey e1k  = F(prvkey e1, basepoint k)
			Crypto::Curve25519::calculate(e2e1k, e2, e1k);		// secret e2e1 = F(prvkey e2, pubkey e1k)
			Crypto::Curve25519::calculate(e2k, e2, k);			// pubkey e2k  = F(prvkey e2, basepoint k)
			Crypto::Curve25519::calculate(e1e2k, e1, e2k);		// secret e1e2 = F(prvkey e1, pubkey e2k)
			
			// compare shared secret
			for (i = 0; i < Crypto::Curve25519::KEY_SIZE_BYTES; ++i)
			{
				if (e1e2k[i] != e2e1k[i])	
				{
					LOG_MSG("Loop %d: fail\n", loop);
					r++;
					break;
				}
			}

			for (i = 0; i < Crypto::Curve25519::KEY_SIZE_BYTES; ++i) e1[i] ^= e2k[i];		// genererate new prvkey e1
			for (i = 0; i < Crypto::Curve25519::KEY_SIZE_BYTES; ++i) e2[i] ^= e1k[i];		// genererate new prvkey e2
			for (i = 0; i < Crypto::Curve25519::KEY_SIZE_BYTES; ++i) k[i] ^= e1e2k[i];		// genererate new basepoint k
		}

		return r;
	}
}
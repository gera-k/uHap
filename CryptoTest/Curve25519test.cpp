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
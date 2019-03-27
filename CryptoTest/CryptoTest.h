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

#ifndef _CRYPTO_TEST_H_
#define _CRYPTO_TEST_H_

#include "Platform.h"

#include "Crypto/Crypto.h"
#include "Crypto/MD.h"
#include "Crypto/Srp.h"

namespace CryptoTest
{
	int sha512_test();
	int hmac_test();
	int hkdf_test();
	int chacha20_test();
	int poly1305_test();
	int aead_test();
	int curve25519_test();
	int ed25519_test();
	int md_test();
	int srp_test();
}

int cryptoTest();

#endif /*_CRYPTO_TEST_H_*/
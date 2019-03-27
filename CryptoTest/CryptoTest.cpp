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

#include <functional>

static int runTest(std::function<int()> t)
{
	int r;
	Timer::Point d1, d2;

	d1 = Timer::now();
	r = t();
	d2 = Timer::now();
	LOG_MSG("Result: %s    duration: %lld  ms\n", r ? "FAIL" : "PASS", Timer::ms(d1, d2));

	return r;
}


int cryptoTest()
{
	int r = 0;

	MD::cntMul = MD::cntMulS = MD::cntMulK = 0;

#if 1
	r += runTest(CryptoTest::sha512_test);

	r += runTest(CryptoTest::hmac_test);

	r += runTest(CryptoTest::hkdf_test);

	r += runTest(CryptoTest::chacha20_test);

	r += runTest(CryptoTest::poly1305_test);

	r += runTest(CryptoTest::aead_test);

	r += runTest(CryptoTest::curve25519_test);

	r += runTest(CryptoTest::ed25519_test);
#endif
#if 1
	r += runTest(CryptoTest::srp_test);
#endif
#if 0
	r += runTest(CryptoTest::md_test);
#endif

	LOG_MSG("\nCrypto Test: %s  Mul %d  MulS %d  MulK %d\n", r ? "FAIL" : "PASS", MD::cntMul, MD::cntMulS, MD::cntMulK);

	return r;
}


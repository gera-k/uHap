/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
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


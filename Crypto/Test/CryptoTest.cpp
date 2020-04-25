/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#define HAP_LOG_REGISTER hapCryptoTest
#include "Platform.h"

#include "Crypto/Test/CryptoTest.h"

using Test = int();
static int runTest(Test t)
{
	int r;
	Timer::Point d1, d2;

	d1 = Timer::now();
	r = t();
	d2 = Timer::now();
	HAP_PRINTF("Result: %s    duration: " TIMER_FORMAT "  ms", r ? "FAIL" : "PASS", Timer::ms(d1, d2));

	return r;
}


int cryptoTest()
{
	int r = 0;

	MD::cntMul = MD::cntMulS = MD::cntMulK = 0;

#if 0
	r += runTest(CryptoTest::sha512_test);
#endif
#if 0
	r += runTest(CryptoTest::hmac_test);
#endif
#if 0
	r += runTest(CryptoTest::hkdf_test);
#endif
#if 0
	r += runTest(CryptoTest::chacha20_test);
#endif
#if 0
	r += runTest(CryptoTest::poly1305_test);
#endif
#if 0
	r += runTest(CryptoTest::aead_test);
#endif
#if 0
	r += runTest(CryptoTest::curve25519_test);
#endif
#if 1
	r += runTest(CryptoTest::ed25519_test);
#endif
#if 0
	r += runTest(CryptoTest::srp_test);
#endif
#if 0
	r += runTest(CryptoTest::md_test);
#endif

	HAP_PRINTF("Crypto Test: %s  Mul %d  MulS %d  MulK %d", r ? "FAIL" : "PASS", MD::cntMul, MD::cntMulS, MD::cntMulK);

	return r;
}


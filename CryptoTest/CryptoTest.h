/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
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
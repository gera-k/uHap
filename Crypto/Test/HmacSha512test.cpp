/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#define HAP_LOG_DECLARE hapCryptoTest
#include "Platform.h"

#include "stdio.h"

#include "Crypto/Test/CryptoTest.h"

namespace CryptoTest
{
	int hmac_test(void)
	{
		int r = 0;

		static struct _hmac_tst
		{
			const char* vector;
			unsigned int vect_len;
			char* msg;
			unsigned int key_len;
			uint8_t key[131];
			uint8_t msg2_3[51];
		} hmac_tst[] =
		{
			{
				"87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cde"
				"daa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854",
				Crypto::Sha512::HASH_SIZE_BYTES,
				(char*)"Hi There",
				20
			},
			{
				"164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea250554"
				"9758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737",
				Crypto::Sha512::HASH_SIZE_BYTES,
				(char*)"what do ya want for nothing?",
				4
			},
			{
				"fa73b0089d56a284efb0f0756c890be9b1b5dbdd8ee81a3655f83e33b2279d39"
				"bf3e848279a722c806b485a47e67c807b946a337bee8942674278859e13292fb",
				Crypto::Sha512::HASH_SIZE_BYTES,
				NULL,
				20
			},
			{
				"b0ba465637458c6990e5a8c5f61d4af7e576d97ff94b872de76f8050361ee3db"
				"a91ca5c11aa25eb4d679275cc5788063a5f19741120c4f2de2adebeb10a298dd",
				Crypto::Sha512::HASH_SIZE_BYTES,
				NULL,
				25
			},
			{
				"415fad6271580a531d4179bc891d87a6",
				Crypto::Sha512::HASH_SIZE_BYTES / 4,
				(char*)"Test With Truncation",
				20
			},
			{
				"80b24263c7c1a3ebb71493c1dd7be8b49b46d1f41b4aeec1121b013783f8f352"
				"6b56d037e05f2598bd0fd2215d6a1e5295e64f73f63f0aec8b915a985d786598",
				Crypto::Sha512::HASH_SIZE_BYTES,
				(char*)"Test Using Larger Than Block-Size Key - Hash Key First",
				131
			},
			{
				"e37b6a775dc87dbaa4dfa9f96e5e3ffddebd71f8867289865df5a32d20cdc944"
				"b6022cac3c4982b10d5eeb55c3e4de15134676fb6de0446065c97440fa8c6a58",
				Crypto::Sha512::HASH_SIZE_BYTES,
				(char*)"This is a test using a larger than block-size key "
				"and a larger than block-size data. The key needs"
				" to be hashed before being used by the HMAC algorithm.",
				131
			},
		};
		uint8_t mac[Crypto::Sha512::HASH_SIZE_BYTES];
		unsigned int messages2and3_len = 50;

		HAP_PRINTF("HMAC-SHA-2 IETF Validation tests");

		for (unsigned tst = 0; tst < sizeofarr(hmac_tst); tst++)
		{
			struct _hmac_tst* t = &hmac_tst[tst];

			// allocate and fill the key
			unsigned int key_len = t->key_len;
			uint8_t *key = t->key; //(uint8_t *)malloc(key_len);

			switch (tst)
			{
			case 0:
				memset(key, 0x0b, key_len);
				break;
			case 1:
				memcpy((char *)key, "Jefe", 4);
				break;
			case 2:
				t->msg = (char*)t->msg2_3; //(char *)malloc(messages2and3_len + 1);
				memset(t->msg, 0xdd, messages2and3_len);
				t->msg[messages2and3_len] = '\0';
				memset(key, 0xaa, key_len);
				break;
			case 3:
				t->msg = (char*)t->msg2_3; //(char *)malloc(messages2and3_len + 1);
				memset(t->msg, 0xcd, messages2and3_len);
				t->msg[messages2and3_len] = '\0';
				for (unsigned int i = 0; i < key_len; i++)
					key[i] = (uint8_t)(i + 1);
				break;
			case 4:
				memset(key, 0x0c, key_len);
				break;
			case 5:
				memset(key, 0xaa, key_len);
				break;
			case 6:
				memset(key, 0xaa, key_len);
				break;
			}

			HAP_PRINTF("Test %d:", tst + 1);

			Crypto::HmacSha512(key, key_len, (uint8_t *)t->msg, (uint32_t)strlen(t->msg), mac, t->vect_len);
			char output[2 * Crypto::Sha512::BLOCK_SIZE_BYTES + 1];
			output[2 * t->vect_len] = '\0';

			for (unsigned i = 0; i < t->vect_len; i++)
				snprintf(output + 2 * i, 3, "%02x", mac[i]);

//			HAP_PRINTF("H: %s", output);

			if (strcmp(t->vector, output))
			{
				HAP_PRINTF("Test failed.");
				r++;
			}
		}

		return r;
	}

}
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
	int hkdf_test()
	{
		static struct _hkdf_tst
		{
			const char* ikm;
			uint32_t ikm_len;
			const char* salt;
			uint32_t salt_len;
			const char* info;
			uint32_t info_len;
			const char* okm;
			unsigned int okm_len;
		} hkdf_tst[] =
		{
			{
				"\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b", 22,
				"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c", 13,
				"\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9", 10,
				"\x83\x23\x90\x08\x6c\xda\x71\xfb\x47\x62\x5b\xb5\xce\xb1\x68\xe4\xc8\xe2\x6a\x1a\x16\xed\x34\xd9\xfc\x7f\xe9\x2c\x14\x81\x57\x93"
				"\x38\xda\x36\x2c\xb8\xd9\xf9\x25\xd7\xcb", 42
			},
			{
				"\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b", 22,
				"", 0,
				"", 0,
				"\xf5\xfa\x02\xb1\x82\x98\xa7\x2a\x8c\x23\x89\x8a\x87\x03\x47\x2c\x6e\xb1\x79\xdc\x20\x4c\x03\x42\x5c\x97\x0e\x3b\x16\x4b\xf9\x0f"
				"\xff\x22\xd0\x48\x36\xd0\xe2\x34\x3b\xac", 42
			},
			{
				"\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b", 11,
				"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c", 13,
				"\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9", 10,
				"\x74\x13\xe8\x99\x7e\x02\x06\x10\xfb\xf6\x82\x3f\x2c\xe1\x4b\xff\x01\x87\x5d\xb1\xca\x55\xf6\x8c\xfc\xf3\x95\x4d\xc8\xaf\xf5\x35"
				"\x59\xbd\x5e\x30\x28\xb0\x80\xf7\xc0\x68", 42
			},
			{
				"\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c\x0c", 22,
				NULL, 0,
				"", 0,
				"\x14\x07\xd4\x60\x13\xd9\x8b\xc6\xde\xce\xfc\xfe\xe5\x5f\x0f\x90\xb0\xc7\xf6\x3d\x68\xeb\x1a\x80\xea\xf0\x7e\x95\x3c\xfc\x0a\x3a"
				"\x52\x40\xa1\x55\xd6\xe4\xda\xa9\x65\xbb", 42
			}
		};
		int r = 0;

		LOG_MSG("HKDF-SHA512 test\n");

		uint8_t okm[Crypto::Sha512::HASH_SIZE_BYTES];
		for (unsigned tst = 0; tst < sizeofarr(hkdf_tst); tst++)
		{
			struct _hkdf_tst* t = &hkdf_tst[tst];

			Crypto::HkdfSha512 hkdf(
				(const uint8_t*)t->salt, t->salt_len,
				(const uint8_t*)t->ikm, t->ikm_len,
				(const uint8_t*)t->info, t->info_len,
				okm, t->okm_len);

			if (memcmp(okm, t->okm, t->okm_len) != 0)
			{
				LOG_MSG("Test %d: fail.\n", tst);
				r++;
			}
		}

		return r;
	}

}
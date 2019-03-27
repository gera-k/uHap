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
#include <chrono>

namespace CryptoTest
{
	int srp_test()
	{
		int r = 0;

		// HAP test vector
		static const char* I = "alice";
		static const char p[] = "password123";
		static const uint8_t s[] = { 0xBE, 0xB2, 0x53, 0x79, 0xD1, 0xA8, 0x58, 0x1E, 0xB5, 0xA7, 0x27, 0x67, 0x3A, 0x24, 0x41, 0xEE, };
		static const uint8_t v[] = {
			0x9B, 0x5E, 0x06, 0x17, 0x01, 0xEA, 0x7A, 0xEB, 0x39, 0xCF, 0x6E, 0x35, 0x19, 0x65, 0x5A, 0x85,
			0x3C, 0xF9, 0x4C, 0x75, 0xCA, 0xF2, 0x55, 0x5E, 0xF1, 0xFA, 0xF7, 0x59, 0xBB, 0x79, 0xCB, 0x47,
			0x70, 0x14, 0xE0, 0x4A, 0x88, 0xD6, 0x8F, 0xFC, 0x05, 0x32, 0x38, 0x91, 0xD4, 0xC2, 0x05, 0xB8,
			0xDE, 0x81, 0xC2, 0xF2, 0x03, 0xD8, 0xFA, 0xD1, 0xB2, 0x4D, 0x2C, 0x10, 0x97, 0x37, 0xF1, 0xBE,
			0xBB, 0xD7, 0x1F, 0x91, 0x24, 0x47, 0xC4, 0xA0, 0x3C, 0x26, 0xB9, 0xFA, 0xD8, 0xED, 0xB3, 0xE7,
			0x80, 0x77, 0x8E, 0x30, 0x25, 0x29, 0xED, 0x1E, 0xE1, 0x38, 0xCC, 0xFC, 0x36, 0xD4, 0xBA, 0x31,
			0x3C, 0xC4, 0x8B, 0x14, 0xEA, 0x8C, 0x22, 0xA0, 0x18, 0x6B, 0x22, 0x2E, 0x65, 0x5F, 0x2D, 0xF5,
			0x60, 0x3F, 0xD7, 0x5D, 0xF7, 0x6B, 0x3B, 0x08, 0xFF, 0x89, 0x50, 0x06, 0x9A, 0xDD, 0x03, 0xA7,
			0x54, 0xEE, 0x4A, 0xE8, 0x85, 0x87, 0xCC, 0xE1, 0xBF, 0xDE, 0x36, 0x79, 0x4D, 0xBA, 0xE4, 0x59,
			0x2B, 0x7B, 0x90, 0x4F, 0x44, 0x2B, 0x04, 0x1C, 0xB1, 0x7A, 0xEB, 0xAD, 0x1E, 0x3A, 0xEB, 0xE3,
			0xCB, 0xE9, 0x9D, 0xE6, 0x5F, 0x4B, 0xB1, 0xFA, 0x00, 0xB0, 0xE7, 0xAF, 0x06, 0x86, 0x3D, 0xB5,
			0x3B, 0x02, 0x25, 0x4E, 0xC6, 0x6E, 0x78, 0x1E, 0x3B, 0x62, 0xA8, 0x21, 0x2C, 0x86, 0xBE, 0xB0,
			0xD5, 0x0B, 0x5B, 0xA6, 0xD0, 0xB4, 0x78, 0xD8, 0xC4, 0xE9, 0xBB, 0xCE, 0xC2, 0x17, 0x65, 0x32,
			0x6F, 0xBD, 0x14, 0x05, 0x8D, 0x2B, 0xBD, 0xE2, 0xC3, 0x30, 0x45, 0xF0, 0x38, 0x73, 0xE5, 0x39,
			0x48, 0xD7, 0x8B, 0x79, 0x4F, 0x07, 0x90, 0xE4, 0x8C, 0x36, 0xAE, 0xD6, 0xE8, 0x80, 0xF5, 0x57,
			0x42, 0x7B, 0x2F, 0xC0, 0x6D, 0xB5, 0xE1, 0xE2, 0xE1, 0xD7, 0xE6, 0x61, 0xAC, 0x48, 0x2D, 0x18,
			0xE5, 0x28, 0xD7, 0x29, 0x5E, 0xF7, 0x43, 0x72, 0x95, 0xFF, 0x1A, 0x72, 0xD4, 0x02, 0x77, 0x17,
			0x13, 0xF1, 0x68, 0x76, 0xDD, 0x05, 0x0A, 0xE5, 0xB7, 0xAD, 0x53, 0xCC, 0xB9, 0x08, 0x55, 0xC9,
			0x39, 0x56, 0x64, 0x83, 0x58, 0xAD, 0xFD, 0x96, 0x64, 0x22, 0xF5, 0x24, 0x98, 0x73, 0x2D, 0x68,
			0xD1, 0xD7, 0xFB, 0xEF, 0x10, 0xD7, 0x80, 0x34, 0xAB, 0x8D, 0xCB, 0x6F, 0x0F, 0xCF, 0x88, 0x5C,
			0xC2, 0xB2, 0xEA, 0x2C, 0x3E, 0x6A, 0xC8, 0x66, 0x09, 0xEA, 0x05, 0x8A, 0x9D, 0xA8, 0xCC, 0x63,
			0x53, 0x1D, 0xC9, 0x15, 0x41, 0x4D, 0xF5, 0x68, 0xB0, 0x94, 0x82, 0xDD, 0xAC, 0x19, 0x54, 0xDE,
			0xC7, 0xEB, 0x71, 0x4F, 0x6F, 0xF7, 0xD4, 0x4C, 0xD5, 0xB8, 0x6F, 0x6B, 0xD1, 0x15, 0x81, 0x09,
			0x30, 0x63, 0x7C, 0x01, 0xD0, 0xF6, 0x01, 0x3B, 0xC9, 0x74, 0x0F, 0xA2, 0xC6, 0x33, 0xBA, 0x89,
		};
		static const uint8_t a[] =
		{
			0x60, 0x97, 0x55, 0x27, 0x03, 0x5C, 0xF2, 0xAD, 0x19, 0x89, 0x80, 0x6F, 0x04, 0x07, 0x21, 0x0B,
			0xC8, 0x1E, 0xDC, 0x04, 0xE2, 0x76, 0x2A, 0x56, 0xAF, 0xD5, 0x29, 0xDD, 0xDA, 0x2D, 0x43, 0x93,
		};
		static const uint8_t A[] = {
			0xFA, 0xB6, 0xF5, 0xD2, 0x61, 0x5D, 0x1E, 0x32, 0x35, 0x12, 0xE7, 0x99, 0x1C, 0xC3, 0x74, 0x43,
			0xF4, 0x87, 0xDA, 0x60, 0x4C, 0xA8, 0xC9, 0x23, 0x0F, 0xCB, 0x04, 0xE5, 0x41, 0xDC, 0xE6, 0x28,
			0x0B, 0x27, 0xCA, 0x46, 0x80, 0xB0, 0x37, 0x4F, 0x17, 0x9D, 0xC3, 0xBD, 0xC7, 0x55, 0x3F, 0xE6,
			0x24, 0x59, 0x79, 0x8C, 0x70, 0x1A, 0xD8, 0x64, 0xA9, 0x13, 0x90, 0xA2, 0x8C, 0x93, 0xB6, 0x44,
			0xAD, 0xBF, 0x9C, 0x00, 0x74, 0x5B, 0x94, 0x2B, 0x79, 0xF9, 0x01, 0x2A, 0x21, 0xB9, 0xB7, 0x87,
			0x82, 0x31, 0x9D, 0x83, 0xA1, 0xF8, 0x36, 0x28, 0x66, 0xFB, 0xD6, 0xF4, 0x6B, 0xFC, 0x0D, 0xDB,
			0x2E, 0x1A, 0xB6, 0xE4, 0xB4, 0x5A, 0x99, 0x06, 0xB8, 0x2E, 0x37, 0xF0, 0x5D, 0x6F, 0x97, 0xF6,
			0xA3, 0xEB, 0x6E, 0x18, 0x20, 0x79, 0x75, 0x9C, 0x4F, 0x68, 0x47, 0x83, 0x7B, 0x62, 0x32, 0x1A,
			0xC1, 0xB4, 0xFA, 0x68, 0x64, 0x1F, 0xCB, 0x4B, 0xB9, 0x8D, 0xD6, 0x97, 0xA0, 0xC7, 0x36, 0x41,
			0x38, 0x5F, 0x4B, 0xAB, 0x25, 0xB7, 0x93, 0x58, 0x4C, 0xC3, 0x9F, 0xC8, 0xD4, 0x8D, 0x4B, 0xD8,
			0x67, 0xA9, 0xA3, 0xC1, 0x0F, 0x8E, 0xA1, 0x21, 0x70, 0x26, 0x8E, 0x34, 0xFE, 0x3B, 0xBE, 0x6F,
			0xF8, 0x99, 0x98, 0xD6, 0x0D, 0xA2, 0xF3, 0xE4, 0x28, 0x3C, 0xBE, 0xC1, 0x39, 0x3D, 0x52, 0xAF,
			0x72, 0x4A, 0x57, 0x23, 0x0C, 0x60, 0x4E, 0x9F, 0xBC, 0xE5, 0x83, 0xD7, 0x61, 0x3E, 0x6B, 0xFF,
			0xD6, 0x75, 0x96, 0xAD, 0x12, 0x1A, 0x87, 0x07, 0xEE, 0xC4, 0x69, 0x44, 0x95, 0x70, 0x33, 0x68,
			0x6A, 0x15, 0x5F, 0x64, 0x4D, 0x5C, 0x58, 0x63, 0xB4, 0x8F, 0x61, 0xBD, 0xBF, 0x19, 0xA5, 0x3E,
			0xAB, 0x6D, 0xAD, 0x0A, 0x18, 0x6B, 0x8C, 0x15, 0x2E, 0x5F, 0x5D, 0x8C, 0xAD, 0x4B, 0x0E, 0xF8,
			0xAA, 0x4E, 0xA5, 0x00, 0x88, 0x34, 0xC3, 0xCD, 0x34, 0x2E, 0x5E, 0x0F, 0x16, 0x7A, 0xD0, 0x45,
			0x92, 0xCD, 0x8B, 0xD2, 0x79, 0x63, 0x93, 0x98, 0xEF, 0x9E, 0x11, 0x4D, 0xFA, 0xAA, 0xB9, 0x19,
			0xE1, 0x4E, 0x85, 0x09, 0x89, 0x22, 0x4D, 0xDD, 0x98, 0x57, 0x6D, 0x79, 0x38, 0x5D, 0x22, 0x10,
			0x90, 0x2E, 0x9F, 0x9B, 0x1F, 0x2D, 0x86, 0xCF, 0xA4, 0x7E, 0xE2, 0x44, 0x63, 0x54, 0x65, 0xF7,
			0x10, 0x58, 0x42, 0x1A, 0x01, 0x84, 0xBE, 0x51, 0xDD, 0x10, 0xCC, 0x9D, 0x07, 0x9E, 0x6F, 0x16,
			0x04, 0xE7, 0xAA, 0x9B, 0x7C, 0xF7, 0x88, 0x3C, 0x7D, 0x4C, 0xE1, 0x2B, 0x06, 0xEB, 0xE1, 0x60,
			0x81, 0xE2, 0x3F, 0x27, 0xA2, 0x31, 0xD1, 0x84, 0x32, 0xD7, 0xD1, 0xBB, 0x55, 0xC2, 0x8A, 0xE2,
			0x1F, 0xFC, 0xF0, 0x05, 0xF5, 0x75, 0x28, 0xD1, 0x5A, 0x88, 0x88, 0x1B, 0xB3, 0xBB, 0xB7, 0xFE,
		};
		static const uint8_t b[] =
		{
			0xE4, 0x87, 0xCB, 0x59, 0xD3, 0x1A, 0xC5, 0x50, 0x47, 0x1E, 0x81, 0xF0, 0x0F, 0x69, 0x28, 0xE0,
			0x1D, 0xDA, 0x08, 0xE9, 0x74, 0xA0, 0x04, 0xF4, 0x9E, 0x61, 0xF5, 0xD1, 0x05, 0x28, 0x4D, 0x20,
		};
		static const uint8_t B[] =
		{
			0x40, 0xF5, 0x70, 0x88, 0xA4, 0x82, 0xD4, 0xC7, 0x73, 0x33, 0x84, 0xFE, 0x0D, 0x30, 0x1F, 0xDD,
			0xCA, 0x90, 0x80, 0xAD, 0x7D, 0x4F, 0x6F, 0xDF, 0x09, 0xA0, 0x10, 0x06, 0xC3, 0xCB, 0x6D, 0x56,
			0x2E, 0x41, 0x63, 0x9A, 0xE8, 0xFA, 0x21, 0xDE, 0x3B, 0x5D, 0xBA, 0x75, 0x85, 0xB2, 0x75, 0x58,
			0x9B, 0xDB, 0x27, 0x98, 0x63, 0xC5, 0x62, 0x80, 0x7B, 0x2B, 0x99, 0x08, 0x3C, 0xD1, 0x42, 0x9C,
			0xDB, 0xE8, 0x9E, 0x25, 0xBF, 0xBD, 0x7E, 0x3C, 0xAD, 0x31, 0x73, 0xB2, 0xE3, 0xC5, 0xA0, 0xB1,
			0x74, 0xDA, 0x6D, 0x53, 0x91, 0xE6, 0xA0, 0x6E, 0x46, 0x5F, 0x03, 0x7A, 0x40, 0x06, 0x25, 0x48,
			0x39, 0xA5, 0x6B, 0xF7, 0x6D, 0xA8, 0x4B, 0x1C, 0x94, 0xE0, 0xAE, 0x20, 0x85, 0x76, 0x15, 0x6F,
			0xE5, 0xC1, 0x40, 0xA4, 0xBA, 0x4F, 0xFC, 0x9E, 0x38, 0xC3, 0xB0, 0x7B, 0x88, 0x84, 0x5F, 0xC6,
			0xF7, 0xDD, 0xDA, 0x93, 0x38, 0x1F, 0xE0, 0xCA, 0x60, 0x84, 0xC4, 0xCD, 0x2D, 0x33, 0x6E, 0x54,
			0x51, 0xC4, 0x64, 0xCC, 0xB6, 0xEC, 0x65, 0xE7, 0xD1, 0x6E, 0x54, 0x8A, 0x27, 0x3E, 0x82, 0x62,
			0x84, 0xAF, 0x25, 0x59, 0xB6, 0x26, 0x42, 0x74, 0x21, 0x59, 0x60, 0xFF, 0xF4, 0x7B, 0xDD, 0x63,
			0xD3, 0xAF, 0xF0, 0x64, 0xD6, 0x13, 0x7A, 0xF7, 0x69, 0x66, 0x1C, 0x9D, 0x4F, 0xEE, 0x47, 0x38,
			0x26, 0x03, 0xC8, 0x8E, 0xAA, 0x09, 0x80, 0x58, 0x1D, 0x07, 0x75, 0x84, 0x61, 0xB7, 0x77, 0xE4,
			0x35, 0x6D, 0xDA, 0x58, 0x35, 0x19, 0x8B, 0x51, 0xFE, 0xEA, 0x30, 0x8D, 0x70, 0xF7, 0x54, 0x50,
			0xB7, 0x16, 0x75, 0xC0, 0x8C, 0x7D, 0x83, 0x02, 0xFD, 0x75, 0x39, 0xDD, 0x1F, 0xF2, 0xA1, 0x1C,
			0xB4, 0x25, 0x8A, 0xA7, 0x0D, 0x23, 0x44, 0x36, 0xAA, 0x42, 0xB6, 0xA0, 0x61, 0x5F, 0x3F, 0x91,
			0x5D, 0x55, 0xCC, 0x3B, 0x96, 0x6B, 0x27, 0x16, 0xB3, 0x6E, 0x4D, 0x1A, 0x06, 0xCE, 0x5E, 0x5D,
			0x2E, 0xA3, 0xBE, 0xE5, 0xA1, 0x27, 0x0E, 0x87, 0x51, 0xDA, 0x45, 0xB6, 0x0B, 0x99, 0x7B, 0x0F,
			0xFD, 0xB0, 0xF9, 0x96, 0x2F, 0xEE, 0x4F, 0x03, 0xBE, 0xE7, 0x80, 0xBA, 0x0A, 0x84, 0x5B, 0x1D,
			0x92, 0x71, 0x42, 0x17, 0x83, 0xAE, 0x66, 0x01, 0xA6, 0x1E, 0xA2, 0xE3, 0x42, 0xE4, 0xF2, 0xE8,
			0xBC, 0x93, 0x5A, 0x40, 0x9E, 0xAD, 0x19, 0xF2, 0x21, 0xBD, 0x1B, 0x74, 0xE2, 0x96, 0x4D, 0xD1,
			0x9F, 0xC8, 0x45, 0xF6, 0x0E, 0xFC, 0x09, 0x33, 0x8B, 0x60, 0xB6, 0xB2, 0x56, 0xD8, 0xCA, 0xC8,
			0x89, 0xCC, 0xA3, 0x06, 0xCC, 0x37, 0x0A, 0x0B, 0x18, 0xC8, 0xB8, 0x86, 0xE9, 0x5D, 0xA0, 0xAF,
			0x52, 0x35, 0xFE, 0xF4, 0x39, 0x30, 0x20, 0xD2, 0xB7, 0xF3, 0x05, 0x69, 0x04, 0x75, 0x90, 0x42,
		};
		static const uint8_t K[] =
		{
			0x5C, 0xBC, 0x21, 0x9D, 0xB0, 0x52, 0x13, 0x8E, 0xE1, 0x14, 0x8C, 0x71, 0xCD, 0x44, 0x98, 0x96,
			0x3D, 0x68, 0x25, 0x49, 0xCE, 0x91, 0xCA, 0x24, 0xF0, 0x98, 0x46, 0x8F, 0x06, 0x01, 0x5B, 0xEB,
			0x6A, 0xF2, 0x45, 0xC2, 0x09, 0x3F, 0x98, 0xC3, 0x65, 0x1B, 0xCA, 0x83, 0xAB, 0x8C, 0xAB, 0x2B,
			0x58, 0x0B, 0xBF, 0x02, 0x18, 0x4F, 0xEF, 0xDF, 0x26, 0x14, 0x2F, 0x73, 0xDF, 0x95, 0xAC, 0x50,
		};

		LOG_MSG("SRP test\n");
		Timer::Point d1, d2;

		// 1: I, s, v are stored in password db on host
		d1 = Timer::now();
		static Srp::Verifier ver(I, p, s);
		if (memcmp(v, ver.v, sizeof(v)) != 0)
		{
			LOG_MSG("Verifier mismatch\n");
			r++;
		}
		d2 = Timer::now();
		LOG_MSG("SRP 1 duration: %lld\n", Timer::ms(d1, d2));

		// 2: User: get name/password
		d1 = Timer::now();
		static Srp::User user(
			I,		// username	from user
			p,		// password from user
			a		// Private value in
		);
		if (memcmp(A, user.getA(), sizeof(A)) != 0)
		{
			LOG_MSG("User Public value A mismatch\n");
			r++;
		}
		d2 = Timer::now();
		LOG_MSG("SRP 2 duration: %lld\n", Timer::ms(d1, d2));

		// 3: User -> Host:  I, A
		d1 = Timer::now();
		static Srp::Host host(ver);
		host.init(
			b		// Private value in		host random value
		);
		if (memcmp(B, host.getB(), sizeof(B)) != 0)
		{
			LOG_MSG("Host Public value B mismatch\n");
			r++;
		}
		d2 = Timer::now();
		LOG_MSG("SRP 3.1 duration: %lld\n", Timer::ms(d1, d2));

		d1 = Timer::now();
		host.setA(A);
		if (memcmp(K, host.getK(), sizeof(K)) != 0)
		{
			LOG_MSG("Host Session key K mismatch\n");
			r++;
		}
		d2 = Timer::now();
		LOG_MSG("SRP 3.2 duration: %lld\n", Timer::ms(d1, d2));

		// 4: Host -> User:  s, B
		d1 = Timer::now();
		user.auth(
			s,		// salt from host
			B		// Public value from host
		);
		if (memcmp(K, user.getK(), sizeof(K)) != 0)
		{
			LOG_MSG("User Session key K mismatch\n");
			r++;
		}
		d2 = Timer::now();
		LOG_MSG("SRP 4 duration: %lld\n", Timer::ms(d1, d2));

		// 5: User -> Host:  M = H(H(N) xor H(g) | H(I) | s | A | B | K)
		d1 = Timer::now();
		static uint8_t M[Crypto::Sha512::HASH_SIZE_BYTES];
		user.proof(M);
		d2 = Timer::now();
		LOG_MSG("SRP 5 duration: %lld\n", Timer::ms(d1, d2));

		// 6: Host->User:  V = H(A | M | K)
		d1 = Timer::now();
		static uint8_t V[Crypto::Sha512::HASH_SIZE_BYTES];
		if (!host.verify(M, sizeof(M), V))
		{
			LOG_MSG("User->Host M verifiation failed\n");
			r++;
		}

		if (!user.verify(V, sizeof(V)))
		{
			LOG_MSG("Host->User V verifiation failed\n");
			r++;
		}
		d2 = Timer::now();
		LOG_MSG("SRP 6 duration: %lld\n", Timer::ms(d1, d2));

		return r;
	}
}
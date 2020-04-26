/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#define HAP_LOG_DECLARE hapCryptoTest
#include "Platform.h"

#include "Crypto/Test/CryptoTest.h"

namespace CryptoTest
{
	int md_test()
	{
		int r = 0;
		static const uint8_t N_val[] =
		{
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	0xC9, 0x0F, 0xDA, 0xA2, 0x21, 0x68, 0xC2, 0x34,
			0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,	0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74,
			0x02, 0x0B, 0xBE, 0xA6, 0x3B, 0x13, 0x9B, 0x22,	0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
			0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B,	0x30, 0x2B, 0x0A, 0x6D, 0xF2, 0x5F, 0x14, 0x37,
			0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,	0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6,
			0xF4, 0x4C, 0x42, 0xE9, 0xA6, 0x37, 0xED, 0x6B,	0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
			0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5,	0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
			0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D,	0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05,
			0x98, 0xDA, 0x48, 0x36, 0x1C, 0x55, 0xD3, 0x9A,	0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
			0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96,	0x1C, 0x62, 0xF3, 0x56, 0x20, 0x85, 0x52, 0xBB,
			0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D,	0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04,
			0xF1, 0x74, 0x6C, 0x08, 0xCA, 0x18, 0x21, 0x7C,	0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
			0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03,	0x9B, 0x27, 0x83, 0xA2, 0xEC, 0x07, 0xA2, 0x8F,
			0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9,	0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18,
			0x39, 0x95, 0x49, 0x7C, 0xEA, 0x95, 0x6A, 0xE5,	0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
			0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAA, 0xC4, 0x2D,	0xAD, 0x33, 0x17, 0x0D, 0x04, 0x50, 0x7A, 0x33,
			0xA8, 0x55, 0x21, 0xAB, 0xDF, 0x1C, 0xBA, 0x64,	0xEC, 0xFB, 0x85, 0x04, 0x58, 0xDB, 0xEF, 0x0A,
			0x8A, 0xEA, 0x71, 0x57, 0x5D, 0x06, 0x0C, 0x7D,	0xB3, 0x97, 0x0F, 0x85, 0xA6, 0xE1, 0xE4, 0xC7,
			0xAB, 0xF5, 0xAE, 0x8C, 0xDB, 0x09, 0x33, 0xD7,	0x1E, 0x8C, 0x94, 0xE0, 0x4A, 0x25, 0x61, 0x9D,
			0xCE, 0xE3, 0xD2, 0x26, 0x1A, 0xD2, 0xEE, 0x6B,	0xF1, 0x2F, 0xFA, 0x06, 0xD9, 0x8A, 0x08, 0x64,
			0xD8, 0x76, 0x02, 0x73, 0x3E, 0xC8, 0x6A, 0x64,	0x52, 0x1F, 0x2B, 0x18, 0x17, 0x7B, 0x20, 0x0C,
			0xBB, 0xE1, 0x17, 0x57, 0x7A, 0x61, 0x5D, 0x6C,	0x77, 0x09, 0x88, 0xC0, 0xBA, 0xD9, 0x46, 0xE2,
			0x08, 0xE2, 0x4F, 0xA0, 0x74, 0xE5, 0xAB, 0x31,	0x43, 0xDB, 0x5B, 0xFC, 0xE0, 0xFD, 0x10, 0x8E,
			0x4B, 0x82, 0xD1, 0x20, 0xA9, 0x3A, 0xD2, 0xCA,	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		};
		const MDl<sizeof(N_val)> N(N_val);

		static const uint8_t g_val[] = { 0x05 };
		const MDl<sizeof(g_val)> g(g_val);

		static uint8_t R_val[] =
		{
			0x01,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0xF0, 0x25, 0x5D, 0xDE, 0x97, 0x3D, 0xCB,
			0x47, 0x03, 0xCE, 0x7E, 0x2E, 0x81, 0x51, 0x97, 0xA6, 0xDB, 0x0F, 0x58, 0x84, 0x48, 0xB6, 0x11,
			0x64, 0xCF, 0xCA, 0xC5, 0xF1, 0x87, 0x2E, 0x51, 0xB1, 0xF9, 0xFB, 0xB5, 0xBF, 0x16, 0xFB, 0xE7,
			0x96, 0x89, 0xFC, 0x09, 0x03, 0xA8, 0x01, 0xE3, 0xD4, 0x80, 0x2F, 0xB8, 0xD3, 0x29, 0x55, 0x0D,
			0xC8, 0xC9, 0xD3, 0xD9, 0x22, 0xEE, 0xCE, 0x9A, 0x54, 0x75, 0xDB, 0x33, 0xDB, 0x7B, 0x83, 0xBB,
			0x5C, 0x0E, 0x13, 0xD1, 0x68, 0x04, 0x9B, 0xBC, 0x86, 0xC5, 0x81, 0x76, 0x47, 0xB0, 0x88, 0xD1,
			0x7A, 0xA5, 0xCC, 0x40, 0xE0, 0x20, 0x35, 0x58, 0x8E, 0xDB, 0x2D, 0xE1, 0x89, 0x93, 0x41, 0x37,
			0x19, 0xFC, 0x25, 0x8D, 0x79, 0xBC, 0x21, 0x7A, 0xC4, 0xB8, 0x73, 0x9C, 0xBE, 0xA0, 0x38, 0xAA,
			0xA8, 0x8D, 0x0D, 0x2F, 0x78, 0xA7, 0x7A, 0x8A, 0x6F, 0xC7, 0xFA, 0xA8, 0xB2, 0xBD, 0xCA, 0x9B,
			0xE7, 0x50, 0x2D, 0x2F, 0x5F, 0x6A, 0x7B, 0x65, 0xF5, 0xE4, 0xF0, 0x7A, 0xB8, 0xB2, 0x86, 0xE4,
			0x11, 0x15, 0xF0, 0x24, 0xA6, 0xE9, 0x76, 0xBD, 0x2B, 0xCE, 0x3E, 0x51, 0x90, 0xB8, 0x91, 0xAB,
			0xBF, 0x23, 0x31, 0xE9, 0xC9, 0x4D, 0xE9, 0x1F, 0xBE, 0x85, 0x74, 0x37, 0x04, 0x94, 0xA3, 0x54,
			0xEA, 0xC9, 0xBE, 0x0B, 0x31, 0xEB, 0x31, 0x85, 0x40, 0xE4, 0x06, 0x9D, 0x55, 0x6E, 0x9D, 0xD0,
			0x9D, 0x5D, 0x89, 0xD7, 0xDE, 0x4A, 0x75, 0xC8, 0x8B, 0xB4, 0x93, 0x16, 0xC1, 0x06, 0xE4, 0xE0,
			0x14, 0xB6, 0x36, 0xE6, 0x0F, 0xEB, 0xC2, 0x92, 0xE6, 0x24, 0x91, 0x05, 0xF5, 0xB1, 0x95, 0xFE,
			0x90, 0x6E, 0xEF, 0x7D, 0x26, 0xCA, 0xF0, 0x52, 0x9A, 0x3E, 0x0B, 0xC1, 0x0E, 0x10, 0x0C, 0xE0,
			0xA8, 0x99, 0xC5, 0x99, 0x99, 0xBF, 0x87, 0x7D, 0xBA, 0x72, 0xC5, 0x9B, 0xF5, 0xCC, 0xF3, 0x26,
			0x2E, 0xB5, 0x90, 0x41, 0xE1, 0x44, 0x78, 0x3A, 0xEE, 0x4C, 0xD8, 0x60, 0xEE, 0x0B, 0x64, 0x50,
			0x6D, 0xAB, 0x25, 0x69, 0x61, 0x1B, 0xAD, 0xDB, 0x6B, 0x78, 0xE8, 0x20, 0x43, 0x04, 0x17, 0x16,
			0xDE, 0xC1, 0x4C, 0xC9, 0x55, 0x69, 0x81, 0x1E, 0x49, 0x8F, 0xDE, 0xC9, 0xD5, 0x4B, 0xD0, 0x71,
			0x1E, 0xC9, 0x7A, 0x0B, 0x25, 0x20, 0x1C, 0x17, 0x76, 0x39, 0x00, 0x49, 0x8B, 0x0F, 0x03, 0x08,
			0x74, 0x6D, 0x18, 0xCE, 0xED, 0xB5, 0x65, 0xFF, 0x29, 0x96, 0x4A, 0xFA, 0x53, 0xE3, 0xC1, 0xB9,
			0x67, 0xED, 0x59, 0x09, 0x17, 0x2F, 0xB4, 0xD7, 0xF3, 0x45, 0xA3, 0x15, 0xC4, 0x76, 0x87, 0x65,
			0x52, 0x94, 0xF5, 0xE1, 0x27, 0x0E, 0x48, 0xC7, 0x26, 0x97, 0xCA, 0x91, 0x38, 0xD2, 0x41, 0xCD,
		};
		const MDl<sizeof(R_val)> R(R_val);

		static const uint8_t a_val[] =
		{
			0x60, 0x97, 0x55, 0x27, 0x03, 0x5C, 0xF2, 0xAD, 0x19, 0x89, 0x80, 0x6F, 0x04, 0x07, 0x21, 0x0B,
			0xC8, 0x1E, 0xDC, 0x04, 0xE2, 0x76, 0x2A, 0x56, 0xAF, 0xD5, 0x29, 0xDD, 0xDA, 0x2D, 0x43, 0x93,
		};
		const MDl<sizeof(a_val)> a(a_val);

		static const uint8_t A_val[] = {
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
		const MDl<sizeof(A_val)> A(A_val);

		static const uint8_t B_val[] =
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
		const MDl<sizeof(B_val)> B(B_val);

		static const uint8_t K_val[] =
		{
			0x5C, 0xBC, 0x21, 0x9D, 0xB0, 0x52, 0x13, 0x8E, 0xE1, 0x14, 0x8C, 0x71, 0xCD, 0x44, 0x98, 0x96,
			0x3D, 0x68, 0x25, 0x49, 0xCE, 0x91, 0xCA, 0x24, 0xF0, 0x98, 0x46, 0x8F, 0x06, 0x01, 0x5B, 0xEB,
			0x6A, 0xF2, 0x45, 0xC2, 0x09, 0x3F, 0x98, 0xC3, 0x65, 0x1B, 0xCA, 0x83, 0xAB, 0x8C, 0xAB, 0x2B,
			0x58, 0x0B, 0xBF, 0x02, 0x18, 0x4F, 0xEF, 0xDF, 0x26, 0x14, 0x2F, 0x73, 0xDF, 0x95, 0xAC, 0x50,
		};
		const MDl<sizeof(K_val)> K(K_val);

		static constexpr uint32_t N_BYTES = sizeof(N_val);
		MDl<N_BYTES> t1, t2, t3, t4;

		int cnt = 100;
		Timer::Point d1, d2;
#if 1
		d1 = Timer::now();
		for (int i = 0; i < cnt; i++)
			t1.mulMod(A, B);
		d2 = Timer::now();
		HAP_PRINTF("mulMod(96D*96D) duration: " TIMER_FORMAT " ms", Timer::ms(d1, d2) / cnt);

		d1 = Timer::now();
		for (int i = 0; i < cnt; i++)
			t2.mulMod(A, t1);
		d2 = Timer::now();
		HAP_PRINTF("mulMod(96D*192D) duration: " TIMER_FORMAT " ms", Timer::ms(d1, d2) / cnt);

		cnt = 10;
		d1 = Timer::now();
		for (int i = 0; i < cnt; i++)
			t1.expMod(g, a);
		d2 = Timer::now();
		HAP_PRINTF("expMod(1D^8D) duration: " TIMER_FORMAT " ms", Timer::ms(d1, d2) / cnt);

		d1 = Timer::now();
		for (int i = 0; i < cnt; i++)
			t1.expMod(g, K);
		d2 = Timer::now();
		HAP_PRINTF("expMod(1D^16D) duration: " TIMER_FORMAT " ms", Timer::ms(d1, d2) / cnt);

		cnt = 4;
		d1 = Timer::now();
		for (int i = 0; i < cnt; i++)
			t1.expMod(A, a);
		d2 = Timer::now();
		HAP_PRINTF("expMod(96D^8D) duration: " TIMER_FORMAT " ms", Timer::ms(d1, d2) / cnt);

		d1 = Timer::now();
		for (int i = 0; i < cnt; i++)
			t1.expMod(A, K);
		d2 = Timer::now();
		HAP_PRINTF("expMod(96D^16D) duration: " TIMER_FORMAT " ms", Timer::ms(d1, d2) / cnt);
#endif
		return r;
	}
}
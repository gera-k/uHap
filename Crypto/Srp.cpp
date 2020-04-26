/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#include "Crypto/Crypto.h"
#include "Crypto/MD.h"
#include "Crypto/Srp.h"
#include <string.h>

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
static constexpr uint32_t N_DIGITS = sizeof(N_val) / sizeof(MD::D);
static MD::D N_buf[N_DIGITS];
MD MD::N(N_buf, N_val, sizeof(N_val));
uint32_t MD::K = N_DIGITS;

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
static MD::D R_buf[N_DIGITS + 1];
MD MD::R(R_buf, R_val, sizeof(R_val));

// temp bufer for both SRP and MD
static MD::D tmp[N_DIGITS * 6 + 2];
static uint8_t* t = reinterpret_cast<uint8_t*>(tmp);
MD::D* MD::t = tmp;

namespace Srp
{
	static const uint8_t g_val[] = { 0x05 };
	const MDl<sizeof(g_val)> g(g_val);

	// shared temp data
	MDl<SRP_MODULO_BYTES> t1, t2, t3, t4;
	Crypto::Sha512 sha;
	static uint8_t hash[Crypto::Sha512::HASH_SIZE_BYTES];

	Verifier::Verifier(
		const char *I,
		const char *p,
		const uint8_t* s
	)
	{
		init(I, p, s);
	}

	void Verifier::init(
		const char *I_,
		const char *p_,
		const uint8_t* s_
	)
	{
		I = I_;
		p = p_;
		
		init(s_);
	}

	void Verifier::init(const uint8_t* s_)
	{
//		uint8_t* t = reinterpret_cast<uint8_t*>(tmp);

		if (s_ != NULL)
			memcpy(s, s_, sizeof(s));
		else
			Crypto::rnd_data(s, sizeof(s));

		// Calculate on Host when Host creates user record:
		// Calculate on User when user enters password:
		// x = H(s | H(I | ":" | p))
		memcpy(t, s, SRP_SALT_BYTES);				// t = s
		sha.init();
		sha.update(I, (uint32_t)strlen(I));
		sha.update(":", 1);
		sha.update(p, (uint32_t)strlen(p));
		sha.fini(t + SRP_SALT_BYTES);				// t = s | H(I | ":" | p)
		sha.calc(t, SRP_SALT_BYTES + Crypto::Sha512::HASH_SIZE_BYTES, hash);
		t1.init(hash, sizeof(hash));				// x = H(s | H(I | ":" | p))
		t1.val(x);

		// Precalculate on Host and store:
		// v = g^x	 - password verifier
		t2.expMod(g, t1/*, tmp*/);			// 1D^16D
		t2.val(v, SRP_VERIFIER_BYTES);
	}

	bool Host::open(uint8_t id)
	{
		if (id == 0xFF)
			return false;
		if (_id != 0xFF)
			return false;
		_id = id;
		return true;
	}
	bool Host::close(uint8_t id)
	{
		if (id == 0xFF)
			return false;
		if (_id != id)
			return false;
		_id = 0xFF;
		return true;
	}
	bool Host::active(uint8_t id)
	{
		// active in any session
		if (id == 0xFF)
		{
			if (_id != 0xFF)
				return true;
			return false;
		}
		
		// active in this session
		if (id == _id)
			return true;
		
		// not active
		return false;
	}

	void Host::init(
		const uint8_t* b
	)
	{
		if (b != nullptr)
			_b.init(b, SRP_PRIVATE_BYTES);
		else
			_b.random();

		// k = H(N, g)
		memcpy(t, N_val, SRP_MODULO_BYTES);
		memset(t + SRP_MODULO_BYTES, 0, SRP_MODULO_BYTES - sizeof(g_val));
		memcpy(t + SRP_MODULO_BYTES * 2 - sizeof(g_val), g_val, sizeof(g_val));
		sha.calc(t, SRP_MODULO_BYTES * 2, hash);
		t4.init(hash, sizeof(hash));	// t4 = k

		// B = kv + g^b
		t2.init(_ver.v, SRP_VERIFIER_BYTES);	// t2 = v
		t1.mulMod(t4, t2); 						// t1 = k*v
		t3.expMod(g, _b);						// t3 = g^b		1D^8D
		t2.addMod(t1, t3);						// t2 = B = t1 + t3 
		t2.val(B, SRP_PUBLIC_BYTES);
	}

	void Host::setA(
		const uint8_t A[SRP_PUBLIC_BYTES]		// Public value from user
	)
	{
		// u = H(A, B)
		sha.init();
		sha.update(A, SRP_PUBLIC_BYTES);
		sha.update(B, SRP_PUBLIC_BYTES);
		sha.fini(hash);
		t4.init(hash, sizeof(hash));			// t4 = u

		// S = (Av^u)^b
		t2.init(_ver.v, SRP_VERIFIER_BYTES);	// t2 = v
		t1.expMod(t2, t4);						// t1 = v^u		96D^16D
		t4.init(A, SRP_PUBLIC_BYTES);			// t4 = A
		t2.mulMod(t4, t1);						// t2 = A*v^u
		t3.expMod(t2, _b);						// t3 = S = (Av^u)^b	96D^8D

		// K = H(S)  - session key 
		t3.val(t);
		sha.calc(t, SRP_PUBLIC_BYTES, hash);
		t4.init(hash, sizeof(hash));			// t4 = K
		t4.val(K);

		// M = H(H(N) xor H(g) | H(I) | s | A | B | K)
		_M.init();
		sha.calc(N_val, sizeof(N_val), t);
		sha.calc(g_val, sizeof(g_val), t + Crypto::Sha512::HASH_SIZE_BYTES);
		for (uint32_t i = 0; i < Crypto::Sha512::HASH_SIZE_BYTES; i++)
			t[i] ^= t[i + Crypto::Sha512::HASH_SIZE_BYTES];
		_M.update(t, Crypto::Sha512::HASH_SIZE_BYTES);
		sha.calc((const uint8_t*)_ver.I, (uint32_t)strlen(_ver.I), t);
		_M.update(t, Crypto::Sha512::HASH_SIZE_BYTES);
		_M.update(_ver.s, SRP_SALT_BYTES);
		_M.update(A, SRP_PUBLIC_BYTES);
		_M.update(B, SRP_PUBLIC_BYTES);
		_M.update(K, Crypto::Sha512::HASH_SIZE_BYTES);

		// _V = H(A)
		_V.init();
		_V.update(A, SRP_PUBLIC_BYTES);
	}


	bool Host::verify(uint8_t* M, uint32_t len, uint8_t* V)
	{
		_M.fini(hash);

		if (len != Crypto::Sha512::HASH_SIZE_BYTES
			|| memcmp(M, hash, Crypto::Sha512::HASH_SIZE_BYTES) != 0)
			return false;

		// V = H(A | M | K)
		_V.update(M, Crypto::Sha512::HASH_SIZE_BYTES);
		_V.update(K, Crypto::Sha512::HASH_SIZE_BYTES);

		_V.fini(V);

		return true;
	}

	void Host::getV(uint8_t V[SRP_PROOF_BYTES])
	{
		_V.get(V);
	}
	
	User::User(
		const char * username,				// username
		const char * password,				// password
		const uint8_t a[SRP_PRIVATE_BYTES]	// Private value (random)
	)
	{
		ver.I = username;
		ver.p = password;

		// A = g^a
		_a.init(a, SRP_PRIVATE_BYTES);	// t1 = a
		t2.expMod(g, _a);				// t2 = A = g^a		1D^8D
		t2.val(_A);

		// M = (H(N) xor H(g)) | H(I)
		_M.init();
		sha.calc(N_val, sizeof(N_val), t);
		sha.calc(g_val, sizeof(g_val), t + Crypto::Sha512::HASH_SIZE_BYTES);
		for (uint32_t i = 0; i < Crypto::Sha512::HASH_SIZE_BYTES; i++)
			t[i] ^= t[i + Crypto::Sha512::HASH_SIZE_BYTES];
		_M.update(t, Crypto::Sha512::HASH_SIZE_BYTES);
		sha.calc((const uint8_t*)ver.I, (uint32_t)strlen(ver.I), t);
		_M.update(t, Crypto::Sha512::HASH_SIZE_BYTES);

		// _V = H(A)
		_V.init();
		_V.update(_A, SRP_PUBLIC_BYTES);
	}

	void User::auth(
		const uint8_t s[SRP_SALT_BYTES],	// salt from host
		const uint8_t B[SRP_PUBLIC_BYTES]	// Public value from host
	)
	{
		ver.init(s);

		// k = H(N, g)
		memcpy(t, N_val, SRP_MODULO_BYTES);
		memset(t + SRP_MODULO_BYTES, 0, SRP_MODULO_BYTES - sizeof(g_val));
		memcpy(t + SRP_MODULO_BYTES * 2 - sizeof(g_val), g_val, sizeof(g_val));
		sha.calc(t, SRP_MODULO_BYTES * 2, hash);
		MDl<sizeof(hash)> k(hash);

		// u = H(A, B)
		sha.init();
		sha.update(_A, SRP_PUBLIC_BYTES);
		sha.update(B, SRP_PUBLIC_BYTES);
		sha.fini(hash);
		MDl<sizeof(hash)> _u(hash);

		// S = (B - kg^x) ^ (a + ux)
		t4.init(ver.x, Crypto::Sha512::HASH_SIZE_BYTES);	// t4 = x
		t3.init(B, SRP_PUBLIC_BYTES);	// t3 = B
		t1.expMod(g, t4);				// t1 = g^x			1D^16D
		t2.mulMod(k, t1);				// t2 = k*g^x
		t1.sub(t3, t2);					// t1 = B - kg^x
		t2.mulMod(_u, t4);				// t2 = u*x
		t4.addMod(_a, t2);				// t4 = a+ux
		t2.expMod(t1, t4);				// t2 = S = (B - kg^x) ^ (a + ux)	96D^16D

		// K = H(S)
		t2.val(t);
		sha.calc(t, SRP_PUBLIC_BYTES, hash);
		t1.init(hash, sizeof(hash));
		t1.val(_K);

		// M = H(H(N) xor H(g) | H(I) | s | A | B | K)
		_M.update(s, SRP_SALT_BYTES);
		_M.update(_A, SRP_PUBLIC_BYTES);
		_M.update(B, SRP_PUBLIC_BYTES);
		_M.update(_K, Crypto::Sha512::HASH_SIZE_BYTES);
	}

	void User::proof(uint8_t M[Crypto::Sha512::HASH_SIZE_BYTES])
	{
		_M.fini(M);

		// V = H(A | M | K)
		_V.update(M, Crypto::Sha512::HASH_SIZE_BYTES);
		_V.update(_K, Crypto::Sha512::HASH_SIZE_BYTES);
	}

	bool User::verify(uint8_t* V, uint32_t len)
	{
		_V.fini(hash);

		if (len != Crypto::Sha512::HASH_SIZE_BYTES
			|| memcmp(V, hash, Crypto::Sha512::HASH_SIZE_BYTES) != 0)
			return false;
		
		return true;
	}
}
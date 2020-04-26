/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _SRP_H_
#define _SRP_H_

#include <stdint.h>

// SRP is a secure password-based authentication and key-exchange protocol.
// http://srp.stanford.edu/whatisit.html
// https://en.wikipedia.org/wiki/Secure_Remote_Password_protocol
// https://tools.ietf.org/html/rfc5054
//
/* Implementation is based on algorithms from http://srp.stanford.edu/design.html

The following is a description of SRP-6 and 6a, the latest versions of SRP:

  N    A large safe prime (N = 2q+1, where q is prime)
	   All arithmetic is done modulo N.
  g    A generator modulo N
  k    Multiplier parameter (k = H(N, g) in SRP-6a, k = 3 for legacy SRP-6)
  s    User's salt
  I    Username
  p    Cleartext Password
  H()  One-way hash function
  ^    (Modular) Exponentiation
  u    Random scrambling parameter
  a,b  Secret ephemeral values
  A,B  Public ephemeral values
  x    Private key (derived from p and s)
  v    Password verifier

The host stores passwords using the following formula:
  x = H(s | p)					as http://srp.stanford.edu defines
  x = H(s | H (I | ":" | p))	as RFC defines
  v = g^x						(computes password verifier)

The host then keeps {I, s, v} in its password database.
The authentication protocol itself goes as follows:
User -> Host:  I, A = g^a                  (identifies self, a = random number)
Host -> User:  s, B = kv + g^b             (sends salt, b = random number)

		Both:  u = H(A | B)

		User:  x = H(s | H (I | ":" | p))  (user enters password)
		User:  S = (B - kg^x) ^ (a + ux)   (computes session key)
		User:  K = H(S)

		Host:  S = (Av^u) ^ b              (computes session key)
		Host:  K = H(S)

Now the two parties have a shared, strong session key K. To complete authentication,
they need to prove to each other that their keys match. One possible way:

User -> Host:  M = H(H(N) xor H(g) | H(I) | s | A | B | K)
Host -> User:  V = H(A | M | K)

*/


namespace Srp
{
	constexpr uint32_t SRP_MODULO_BYTES = 384;
	constexpr uint32_t SRP_VERIFIER_BYTES = SRP_MODULO_BYTES;
	constexpr uint32_t SRP_SALT_BYTES = 16;
	constexpr uint32_t SRP_PRIVATE_BYTES = 32;
	constexpr uint32_t SRP_PUBLIC_BYTES = SRP_MODULO_BYTES;
	constexpr uint32_t SRP_KEY_BYTES = Crypto::Sha512::HASH_SIZE_BYTES;
	constexpr uint32_t SRP_PROOF_BYTES = Crypto::Sha512::HASH_SIZE_BYTES;

	// Verifier - password verifier generator
	class Verifier
	{
	public:
		Verifier() {}
		Verifier(
			const char *I,
			const char *p,
			const uint8_t* s = nullptr
		);
		void init(
			const char *I_,
			const char *p_,
			const uint8_t* s = nullptr
		);
		void init(
			const uint8_t* s_ = nullptr
		);

		const char *I = nullptr;
		const char *p = nullptr;
		uint8_t s[SRP_SALT_BYTES];
		uint8_t x[Crypto::Sha512::HASH_SIZE_BYTES];
		uint8_t v[SRP_VERIFIER_BYTES];
	};

	// Host
	class Host
	{
	public:
		Host(Verifier& ver) 
			: _ver(ver)
		{}
		
		// track SRP session, only one session per object is allowed
		bool open(uint8_t id);
		bool close(uint8_t id);
		bool active(uint8_t id = 0xFF);

		void init(
			const uint8_t* b = nullptr				// Private value [SRP_PRIVATE_BYTES]
			);

		void setA(
			const uint8_t A[SRP_PUBLIC_BYTES]		// Public value from user
			);

		const uint8_t* getB() const { return B; };
		const uint8_t* getK() const { return K; };

		bool verify(uint8_t* M, uint32_t M_len, uint8_t* V = nullptr);
		void getV(uint8_t V[SRP_PROOF_BYTES]);

	private:
		uint8_t _id = 0xFF;
		Verifier& _ver;
		Crypto::Sha512 _M, _V;
		MDl<SRP_PRIVATE_BYTES> _b;
		uint8_t B[Srp::SRP_PUBLIC_BYTES];
		uint8_t K[Crypto::Sha512::HASH_SIZE_BYTES];
	};

	// User
	class User
	{
	public:
		User(
			const char * username,				// username
			const char * password,				// password
			const uint8_t a[SRP_PRIVATE_BYTES]	// Private value (random)
		);

		void auth(
			const uint8_t s[SRP_SALT_BYTES],	// salt from host
			const uint8_t B[SRP_PUBLIC_BYTES]	// Public value from host
		);

		const uint8_t* getA() const { return _A; };
		const uint8_t* getK() const { return _K; };

		void proof(uint8_t M[Crypto::Sha512::HASH_SIZE_BYTES]);
		bool verify(uint8_t* V, uint32_t len);

	private:
		Verifier ver;
		Crypto::Sha512 _M, _V;
		MDl<SRP_PRIVATE_BYTES> _a;
		uint8_t _A[Srp::SRP_PUBLIC_BYTES];
		uint8_t _K[Crypto::Sha512::HASH_SIZE_BYTES];
	};
}

#endif /*_SRP_H_*/
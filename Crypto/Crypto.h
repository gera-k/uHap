/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _CRYPTO_H_
#define _CRYPTO_H_

/*
	Cryptography implementation for Homekit needs.
	This implementation is sw-only
*/

#include "Platform.h"
#include "Crypto/Sha512blk.h"

namespace Crypto
{
	// SHA2-512 
	//	https://en.wikipedia.org/wiki/SHA-2
	//	https://www.di-mgt.com.au/sha_testvectors.html
	class Sha512
	{
	public:
		using W = sha512_word;

		static constexpr unsigned int BLOCK_SIZE_BITS = SHA512_BLOCK_SIZE_BITS;
		static constexpr unsigned int HASH_SIZE_BITS = SHA512_HASH_SIZE_BITS;
		static constexpr unsigned int BLOCK_SIZE_BYTES = SHA512_BLOCK_SIZE_BYTES;
		static constexpr unsigned int HASH_SIZE_BYTES = SHA512_HASH_SIZE_BYTES;
		static constexpr unsigned int BLOCK_SIZE_WORDS = SHA512_BLOCK_SIZE_WORDS;
		static constexpr unsigned int HASH_SIZE_WORDS = SHA512_HASH_SIZE_WORDS;

		// calculate SHA512 of a message msg
		Sha512(const uint8_t* msg, uint32_t size, uint8_t* hash = nullptr);

		// SHA512 step-by-step 
		//	Sha512 h;
		//	h.update(msg)
		//	h.update(msg)
		//	...
		//	h.fini(&hash)
		Sha512();	// init/reset
		void init(); // reset context
		void update(const uint8_t* msg, uint32_t size);
		void update(const char* msg, uint32_t size)
		{
			update(reinterpret_cast<const uint8_t *>(msg), size);
		}
		void fini(uint8_t* hash = nullptr);
		void get(uint8_t* hash);
		void calc(const uint8_t* msg, uint32_t size, uint8_t* hash);

		W h[HASH_SIZE_WORDS];		// hash buffer

	private:
		W m[BLOCK_SIZE_WORDS * 2];	// internal buffer = two SHA512 blocks
		uint32_t p;					// current buffer position
		uint32_t L;					// total message length in bits
	};

	// HMAC	(hash-based message authentication code)
	//	https://en.wikipedia.org/wiki/HMAC
	class HmacSha512
	{
	public:
		// calculate HMAC of key/msg
		HmacSha512(const uint8_t *key, uint32_t key_size,
			const uint8_t *msg, uint32_t msg_len,
			uint8_t *mac, uint32_t mac_size);

		// HMAC step-by-step
		HmacSha512(const uint8_t *key, uint32_t key_size);
		void start(const uint8_t *key, uint32_t key_size);
		void next(const uint8_t *msg, uint32_t msg_len);
		void end(uint8_t *mac, uint32_t mac_size);

	private:
		Sha512 i_sha;
		Sha512 o_sha;

		uint8_t i_blk[Sha512::BLOCK_SIZE_BYTES];
		uint8_t o_blk[Sha512::BLOCK_SIZE_BYTES];
	};

	// HKDF (simple key derivation function(KDF) based on a hash - based message authentication[1] code(HMAC))
	//	https://en.wikipedia.org/wiki/HKDF
	//	https://tools.ietf.org/html/rfc5869
	//	https://www.kullo.net/blog/hkdf-sha-512-test-vectors/
	class HkdfSha512
	{
	public:
		HkdfSha512
		(
			const uint8_t* salt, uint32_t salt_len,		// optional salt value (a non-secret random value)
			const uint8_t* ikm, uint32_t ikm_len,		// input keying material
			const uint8_t* info, uint32_t info_len,		// optional context and application specific information
			uint8_t* okm, uint32_t okm_len				// output keying material (L = okm_len)
		)
		{
			// PRK = HMAC-Hash(salt, IKM)
			HmacSha512(salt, salt_len, ikm, ikm_len, prk, sizeof(prk));

			// RFC5869 algoithm:
			//	N = ceil(L/HashLen)
			//	T = T(1) | T(2) | T(3) | ... | T(N)
			//	OKM = first L octets of T
			// where:
			//	T(0) = empty string(zero length)
			//	T(1) = HMAC-Hash(PRK, T(0) | info | 0x01)
			//	T(2) = HMAC-Hash(PRK, T(1) | info | 0x02)
			//	T(3) = HMAC-Hash(PRK, T(2) | info | 0x03)
			//	...

			// assiming L <= HashLen (okm_len <= Sha512::HASH_SIZE_BYTES):
			//	N = 1
			//	T = T(1)
			//	OKM = first L octets of T
			HmacSha512 t1(prk, sizeof(prk)); // T(1) = HMAC(prk, info | 0x01)
			t1.next(info, info_len);
			uint8_t ctr = 0x01;
			t1.next(&ctr, 1);
			t1.end(okm, okm_len);  // T(1) -> okm
		}

	private:
		uint8_t prk[Sha512::HASH_SIZE_BYTES];
	};

	// ChaCha20 stream cipher
	// https://en.wikipedia.org/wiki/Salsa20
	// https://tools.ietf.org/html/rfc7539
	// implementation assumes LE architecture
	class Chacha20
	{
	public:
		using W = uint32_t;

		static constexpr unsigned int KEY_SIZE_BITS = 256;
		static constexpr unsigned int NONCE_SIZE_BITS = 96;
		static constexpr unsigned int COUNT_SIZE_BITS = 32;

		static constexpr unsigned int KEY_SIZE_BYTES = KEY_SIZE_BITS / 8;		// 32
		static constexpr unsigned int NONCE_SIZE_BYTES = NONCE_SIZE_BITS / 8;	// 12
		static constexpr unsigned int COUNT_SIZE_BYTES = COUNT_SIZE_BITS / 8;	// 4
		static constexpr unsigned int BLK_SIZE_BYTES = 64;

		static constexpr unsigned int KEY_SIZE_WORDS = KEY_SIZE_BYTES / sizeof(W);		// 8
		static constexpr unsigned int NONCE_SIZE_WORDS = NONCE_SIZE_BYTES / sizeof(W);	// 3
		static constexpr unsigned int COUNT_SIZE_WORDS = COUNT_SIZE_BYTES / sizeof(W);	// 1
		static constexpr unsigned int BLK_SIZE_WORDS = BLK_SIZE_BYTES / sizeof(W);		// 16

		Chacha20() {}

		Chacha20(
			const uint8_t key[KEY_SIZE_BYTES],
			uint32_t count,
			const uint8_t nonce[NONCE_SIZE_BYTES],
			const uint8_t* msg, uint32_t msg_len,
			uint8_t* out
		);

		void block(
			const uint8_t key[KEY_SIZE_BYTES],
			uint32_t count,
			const uint8_t nonce[NONCE_SIZE_BYTES],
			uint8_t out[BLK_SIZE_BYTES] = NULL
		);
		void encrypt(
			const uint8_t key[KEY_SIZE_BYTES],
			uint32_t count,
			const uint8_t nonce[NONCE_SIZE_BYTES],
			const uint8_t* msg, uint32_t msg_len,
			uint8_t* out
		);

	private:
		W blk[BLK_SIZE_WORDS];
	};


	// Poly1305 is a cryptographic message authentication code (MAC)
	// Poly1305 takes a 256-bit, one-time key and a message, and it
	//	produces a 16-byte tag that authenticates the message
	// https://en.wikipedia.org/wiki/Poly1305
	// https://tools.ietf.org/html/rfc7539
	class Poly1305
	{
	public:
		static constexpr unsigned int KEY_SIZE_BITS = 256;
		static constexpr unsigned int TAG_SIZE_BITS = 128;

		static constexpr unsigned int KEY_SIZE_BYTES = KEY_SIZE_BITS / 8;	// 32
		static constexpr unsigned int TAG_SIZE_BYTES = TAG_SIZE_BITS / 8;	// 16

		// The inputs to Poly1305 are:
		//	-  A 256-bit one-time key
		//	-  An arbitrary length message
		// The output is a 128-bit tag.

		Poly1305() {}

		Poly1305(const uint8_t key[KEY_SIZE_BYTES])
		{
			init(key);
		}

		Poly1305(
			const uint8_t key[KEY_SIZE_BYTES],
			const uint8_t* msg, uint32_t msg_len,
			uint8_t tag[TAG_SIZE_BYTES]
		)
		{
			init(key);
			update(msg, msg_len);
			finish(tag);
		}

		void init(const uint8_t key[KEY_SIZE_BYTES]);
		void update(const uint8_t *in, uint32_t len);
		void finish(uint8_t tag[TAG_SIZE_BYTES]);

	private:
		uint32_t aligner;
		uint32_t r[5];
		uint32_t h[5];
		uint32_t pad[4];
		uint32_t leftover;
		uint8_t buffer[TAG_SIZE_BYTES];
		uint8_t final;

		void blocks(const uint8_t *m, uint32_t bytes);
	};

	// AEAD_CHACHA20_POLY1305 is an authenticated encryption with additional
	//	data algorithm.The inputs to AEAD_CHACHA20_POLY1305 are :
	//	-  A 256 - bit key
	//	-  A 96 - bit nonce -- different for each invocation with the same key
	//	-  An arbitrary length plaintext
	//	-  Arbitrary length additional authenticated data(AAD)
	// The output from the AEAD is twofold:
	//	-  A ciphertext of the same length as the plaintext.
	//	-  A 128 - bit tag, which is the output of the Poly1305 function.
	class Aead
	{
	public:
		static constexpr unsigned int NONCE_SIZE_BITS = 96;

		static constexpr unsigned int KEY_SIZE_BYTES = Poly1305::KEY_SIZE_BYTES;	// 32
		static constexpr unsigned int TAG_SIZE_BYTES = Poly1305::TAG_SIZE_BYTES;	// 16
		static constexpr unsigned int NONCE_SIZE_BYTES = NONCE_SIZE_BITS / 8;		// 12	

		enum Action
		{
			Encrypt = 0,	// msg - plaintext, out - ciphertext
			Decrypt = 1		// msg - ciphertext, out - plaintext
		};

		Aead() {}
		Aead(Action action,
			uint8_t* out,					// output, same length as msg 
			uint8_t* tag,					// tag, TAG_SIZE_BYTES
			const uint8_t* key,				// key, KEY_SIZE_BYTES
			const uint8_t* nonce,			// nonce, 
			const uint8_t* msg,				// input, arbitrary length
			uint32_t msg_size,				//
			const uint8_t* aad = nullptr,	// optional additional authenticated data
			uint32_t aad_size = 0
		)
		{
			exec(action, out, tag, key, nonce, msg, msg_size, aad, aad_size);
		}
		void encrypt(
			uint8_t* out,					// output, same length as msg 
			uint8_t* tag,					// tag, TAG_SIZE_BYTES
			const uint8_t* key,				// key, KEY_SIZE_BYTES
			const uint8_t* nonce,			// nonce, 
			const uint8_t* msg,				// input, arbitrary length
			uint32_t msg_size,				//
			const uint8_t* aad = nullptr,	// optional additional authenticated data
			uint32_t aad_size = 0
		)
		{
			exec(Encrypt, out, tag, key, nonce, msg, msg_size, aad, aad_size);
		}
		void decrypt(
			uint8_t* out,					// output, same length as msg 
			uint8_t* tag,					// tag, TAG_SIZE_BYTES
			const uint8_t* key,				// key, KEY_SIZE_BYTES
			const uint8_t* nonce,			// nonce, 
			const uint8_t* msg,				// input, arbitrary length
			uint32_t msg_size,				//
			const uint8_t* aad = nullptr,	// optional additional authenticated data
			uint32_t aad_size = 0
		)
		{
			exec(Decrypt, out, tag, key, nonce, msg, msg_size, aad, aad_size);
		}
	private:
		uint8_t otk[Chacha20::BLK_SIZE_BYTES];
		uint8_t padding[15] = { 0 };

		void exec(Action action,
			uint8_t* out,					// output, same length as msg 
			uint8_t* tag,					// tag, TAG_SIZE_BYTES
			const uint8_t* key,				// key, KEY_SIZE_BYTES
			const uint8_t* nonce,			// nonce, 
			const uint8_t* msg,				// input, arbitrary length
			uint32_t msg_size,				//
			const uint8_t* aad = nullptr,	// optional additional authenticated data
			uint32_t aad_size = 0
			);
	};


	// private/public key security
	// https://en.wikipedia.org/wiki/Curve25519
	// https://tools.ietf.org/html/rfc7748
	class Curve25519
	{
	public:
		static constexpr unsigned int KEY_SIZE_BYTES = 32;

		Curve25519() {}

		// calculate secret from private and public keys
		static void calculate(
			uint8_t *secret,
			const uint8_t *prvKey,
			const uint8_t *pubKey
		);

		// init keys - create new key pair and store them internally
		void init();

		// return stored public key
		const uint8_t* pubKey();

		// generate shared secret from stored private key and other public key
		const uint8_t* sharedSecret(const uint8_t* pubKey = nullptr);

	private:
		uint8_t _prvKey[KEY_SIZE_BYTES];
		uint8_t _pubKey[KEY_SIZE_BYTES];
		uint8_t _secret[KEY_SIZE_BYTES];
	};

	// Ed25519 is a public-key signature system
	//	https://ed25519.cr.yp.to/
	//	https://tools.ietf.org/html/rfc8032
	class Ed25519
	{
	public:
		constexpr static unsigned int SIGN_SIZE_BYTES = 64;
		constexpr static unsigned int SEED_SIZE_BYTES = 32;
		constexpr static unsigned int PUBKEY_SIZE_BYTES = 32;
		constexpr static unsigned int PRVKEY_SIZE_BYTES = 64;

		Ed25519() {};

		// create key pair from random seed
		void init();

		// create key pair from given seed
		void init(
			const uint8_t *prvKey
		);

		// Init from given key pair
		void init(				
			const uint8_t *pubKey,
			const uint8_t *prvKey
		);

		// return own public key
		const uint8_t* pubKey()
		{
			return _pubKey;
		}

		// sign the message
		void sign(					
			uint8_t *sign,			// signature buffer, SIGN_SIZE_BYTES
			const uint8_t *msg,		// arbitrary length message
			uint16_t msg_len
		);

		// verify signature
		bool verify(				
			const uint8_t *sign,	// signature, SIGN_SIZE_BYTES
			const uint8_t *msg,		// arbitrary length message
			uint16_t msg_len,
			const uint8_t *pubKey	// other side public key
		);

	protected:
		uint8_t _prvKey[PRVKEY_SIZE_BYTES];
		uint8_t _pubKey[PUBKEY_SIZE_BYTES];
	};

}

#endif /*_CRYPTO_H_*/
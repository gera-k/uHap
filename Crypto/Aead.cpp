/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#include "Crypto/Crypto.h"

namespace Crypto
{
	void Aead::exec(
		Action action,
		uint8_t* out,					// output, same length as msg 
		uint8_t* tag,					// tag, TAG_SIZE_BYTES
		const uint8_t* key,				// key, KEY_SIZE_BYTES
		const uint8_t* nonce,			// nonce, 
		const uint8_t* msg,				// input, arbitrary length
		uint32_t msg_size,				//
		const uint8_t* aad,         	// optional additional authenticated data
		uint32_t aad_size
	)
	{
		//  First, a Poly1305 one - time key is generated from the 256 - bit key and nonce
		//	-  The 256-bit session integrity key is used as the ChaCha20 key.
		//	-  The block counter is set to zero.
		//	-  The protocol will specify a 96-bit or 64-bit nonce.
		Chacha20 cha;
		cha.block(key, 0, nonce, otk);

		// Next, the ChaCha20 encryption function is called to encrypt the plaintext,
		//	using the same key and nonce, and with the initial counter set to 1
		// When decrypt, the roles of ciphertext and plaintext are reversed, so the
		//	ChaCha20 encryption function is applied to the ciphertext, producing the plaintext.
		//  Note that on decrypt the msg contains ciphertetx
		cha.encrypt(key, 1, nonce, msg, msg_size, out);

		// Finally, the Poly1305 function is called with the Poly1305 key calculated above, 
		Poly1305 poly(otk);

		//	and a message constructed as a concatenation of the following :
		//	- The AAD
		//	- padding1 -- the padding is up to 15 zero bytes, and it brings
		//		the total length so far to an integral multiple of 16.  If the
		//		length of the AAD was already an integral multiple of 16 bytes,
		//		this field is zero - length.
		if (aad_size > 0)
		{
			poly.update(aad, aad_size);
			if (aad_size % 16)
				poly.update(padding, 16 - (aad_size % 16));
		}

		//	- The ciphertext, both encrypt and decrypt functions
		//	- padding2 -- the padding is up to 15 zero bytes, and it brings
		//		the total length so far to an integral multiple of 16.  If the
		//		length of the ciphertext was already an integral multiple of 16
		//		bytes, this field is zero - length.
		if (action == Encrypt)
			poly.update(out, msg_size);		// on encrypt the cipher test is in out buffer
		else
			poly.update(msg, msg_size);		// on decrypt the cipher test is in msg buffer
		if (msg_size % 16)
			poly.update(padding, 16 - (msg_size % 16));

		//	- The length of the additional data in octets(as a 64-bit little-endian integer).
		uint64_t sz = aad_size;
		poly.update((const uint8_t *)&sz, 8);

		//	- The length of the ciphertext in octets(as a 64-bit little-endian integer).
		sz = msg_size;
		poly.update((const uint8_t *)&sz, 8);

		poly.finish(tag);
	}
}
/*
MIT License

Copyright (c) 2019 Gera Kazakov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Hap.h"

namespace Hap
{
	namespace Http
	{

		const char* ContentTypeJson = "application/hap+json";
		const char* ContentTypeTlv8 = "application/pairing+tlv8";
		const char* Username = "Pair-Setup";

		// current pairing session - only one simultaneous pairing is allowed
		Srp::Verifier ver;					// SRP verifier  //TODO: pre-calculate and store in config data
		Srp::Host srp(ver);					// .active()=true - pairing in progress, only one pairing at a time
		uint8_t srp_auth_count = 0;			// auth attempts counter

		// Open
		//	returns new session ID, 0..sid_max, or sid_invalid
		sid_t Server::Open()
		{
			for (sid_t sid = 0; sid < sizeofarr(_sess); sid++)
			{
				if (_sess[sid].isOpen())
					continue;

				// open session - use same buffers for all sessions
				_sess[sid].Open(sid, &_buf);

				// open database
				_db.Open(sid);

				return sid;
			}

			return sid_invalid;
		}

		// Close
		//	returns true if opened session was closed
		bool Server::Close(sid_t sid)
		{
			if (sid > sid_max)
				return false;

			if (!_sess[sid].isOpen())
				return false;

			_db.Close(sid);

			_sess[sid].Close();

			// cancel current pairing if any
			if (srp.active(sid))
				srp.close(sid);

			return true;
		}

		bool Server::Process(sid_t sid, Recv recv, Send send)
		{
			if (sid > MaxHttpSessions)	// invalid sid
				return false;

			Session* sess = &_sess[sid];
			bool secured = sess->secured;

			Log::Msg("Http::Process Ses %d  secured %d  %s\n", sid, sess->secured, sess->ios ? (sess->ios->perm == Hap::Controller::Admin ? "admin" : "user") : "?");

			if (sid == MaxHttpSessions)	// too many sessions
			{
				// TODO: read request, create error response
				send(sid, sess->rsp.buf(), sess->rsp.len());
				return false;
			}

			// prepare for request parsing
			sess->Init();

			// read and parse the HTTP request
			uint16_t len = 0;		// total len of valid data received so far
			uint16_t http_len = 0;	// current length of http request
			while (true)
			{
				uint8_t* req = sess->data() + len;
				uint16_t req_len = sess->sizeofdata() - len;
				
				// read next portion of the request
				int l = recv(sid, (char*)req, req_len);
				if (l < 0)	// read error
				{
					Log::Err("Http: Read Error %d\n", l);
					return false;
				}
				if (l == 0)
				{
					Log::Msg("Http: Read EOF\n");
					return false;
				}

				len += l;

				if (sess->secured)
				{
					// it session is secured, decrypt the block - when the whole block is received
					//	max length of http data that can be processed is defined by MaxHttpFrame/MaxHttpBlock

					if (len < 2)	// wait fot at least two bytes of data length 
						continue;
					
					uint8_t *p = sess->data();
					uint16_t aad = p[0] + ((uint16_t)(p[1]) << 8);	// data length, also serves as AAD for decryption

					if (aad > MaxHttpBlock)
					{
						Log::Err("Http: encrypted block size is too big: %d\n", aad);
						return false;
					}

					if (len < 2 + aad + 16)	// wait for complete encrypted block
						continue;

					// ensure the new portion of data fits into the request buffer
					if (http_len + aad > sess->req.size() - 16)	// leave room for 16-bit tag
					{
						Log::Err("Http: encrypted request is too big: %d + %d = %d\n", http_len, aad, http_len + aad);
						return false;
					}

					// make 96-bit nonce from receive sequential number
					uint8_t nonce[12];
					memset(nonce, 0, sizeof(nonce));
					memcpy(nonce + 4, &sess->recvSeq, 8);

					// decrypt into request buffer
					uint8_t* b = (uint8_t*)sess->req.buf() + http_len;

					Crypto::Aead aead(Crypto::Aead::Decrypt,
						b, b + aad,							// output data and tag positions
						sess->ControllerToAccessoryKey,		// decryption key
						nonce,
						p + 2, aad,							// encrypted data
						p, 2								// aad
					);

					sess->recvSeq++;

					// compare passed in and calculated tags
					if (memcmp(b + aad, p + 2 + aad, 16) != 0)
					{
						Log::Err("Http: decrypt error\n");
						return false;
					}

					http_len += aad;

					// if there is more data after encypted block
					// move it to block start 
					if (len > 2 + aad + 16)
					{
						len -= 2 + aad + 16;
						b = (uint8_t*)sess->req.buf();
						memmove(b, b + 2 + aad + 16, len);
					}
				}
				else  // unsequred session
				{
					// ensure the new portion of data fits into the request buffer
					if (http_len + len > sess->req.size())
					{
						Log::Err("Http: request is too big: %d + %d = %d\n", http_len, len, http_len + len);
						return false;
					}

					// copy received data into request buffer as is
					memcpy(sess->req.buf() + http_len, sess->data(), len);

					http_len += len;
					len = 0;
				}

				// try parsing HTTP request
				auto status = sess->req.parse(http_len);
				if (status == sess->req.Error)	// parser error
				{
					// send response 'Internal server error'
					sess->rsp.start(HTTP_500);
					sess->rsp.end();
					send(sid, sess->rsp.buf(), sess->rsp.len());
					return false;
				}

				if (status == sess->req.Success)
					// request parsed, stop reading 
					break;

				// status = Incomplete -> read more data
			}

			auto m = sess->req.method();
			Log::Msg("Method: '%.*s'\n", m.l(), m.p());

			auto p = sess->req.path();
			Log::Msg("Path: '%.*s'\n", p.l(), p.p());

			auto d = sess->req.data();

			for (uint32_t i = 0; i < sess->req.hdr_count(); i++)
			{
				auto n = sess->req.hdr_name(i);
				auto v = sess->req.hdr_value(i);
				Log::Msg("%.*s: '%.*s'\n", n.l(), n.p(), v.l(), v.p());
			}

			if (m.l() == 4 && strncmp(m.p(), "POST", 4) == 0)
			{
				// POST
				//		/identify
				//		/pair-setup
				//		/pair-verify
				//		/pairings

				if (p.l() == 9 && strncmp(p.p(), "/identify", 9) == 0)
				{
					if (_pairings.Count() == 0)
					{
						Log::Msg("Http: Exec unpaired identify\n");
						sess->rsp.start(HTTP_204);
						sess->rsp.end();
					}
					else
					{
						Log::Msg("Http: Unpaired identify prohibited when paired\n");
						sess->rsp.start(HTTP_400);
						sess->rsp.add(ContentType, ContentTypeJson);
						sess->rsp.end("{\"status\":-70401}");
					}
				}
				else if (p.l() == 11 && strncmp(p.p(), "/pair-setup", 11) == 0)
				{
					int len;
					if (!sess->req.hdr(ContentType, ContentTypeTlv8))
					{
						Log::Msg("Http: Unknown or missing ContentType\n");
						sess->rsp.start(HTTP_400);
						sess->rsp.end();
					}
					else if (!sess->req.hdr(ContentLength, len))
					{
						Log::Err("Http: Unknown or missing ContentLength\n");
						sess->rsp.start(HTTP_400);
						sess->rsp.end();
					}
					else
					{
						sess->tlvi.parse(d.p(), d.l());
						Log::Msg("PairSetup: TLV item count %d\n", sess->tlvi.count());

						Tlv::State state;
						if (!sess->tlvi.get(Tlv::Type::State, state))
						{
							Log::Msg("PairSetup: State not found\n");
						}
						else
						{
							switch (state)
							{
							case Tlv::State::M1:
								_pairSetup1(sess);
								break;

							case Tlv::State::M3:
								_pairSetup3(sess);
								break;

							case Tlv::State::M5:
								_pairSetup5(sess);
								break;

							default:
								Log::Err("PairSetup: Unknown state %d\n", (int)state);
							}
						}
					}
				}
				else if (p.l() == 12 && strncmp(p.p(), "/pair-verify", 12) == 0)
				{
					int len;
					if (!sess->req.hdr(ContentType, ContentTypeTlv8))
					{
						Log::Err("Http: Unknown or missing ContentType\n");
						sess->rsp.start(HTTP_400);
						sess->rsp.end();
					}
					else if (!sess->req.hdr(ContentLength, len))
					{
						Log::Err("Http: Unknown or missing ContentLength\n");
						sess->rsp.start(HTTP_400);
						sess->rsp.end();
					}
					else
					{
						sess->tlvi.parse(d.p(), d.l());
						Log::Msg("PairVerify: TLV item count %d\n", sess->tlvi.count());

						Tlv::State state;
						if (!sess->tlvi.get(Tlv::Type::State, state))
						{
							Log::Err("PairVerify: State not found\n");
						}
						else
						{
							switch (state)
							{
							case Tlv::State::M1:
								_pairVerify1(sess);
								break;

							case Tlv::State::M3:
								_pairVerify3(sess);
								secured = sess->ios != nullptr;
								break;
							default:
								Log::Err("PairVerify: Unknown state %d\n", (int)state);
							}
						}
					}
				}
				else if (p.l() == 9 && strncmp(p.p(), "/pairings", 9) == 0)
				{
					int len;
					if (!sess->secured)
					{
						Log::Err("Http: Authorization required\n");
						sess->rsp.start(HTTP_470);
						sess->rsp.end();
					}
					else if (!sess->req.hdr(ContentType, ContentTypeTlv8))
					{
						Log::Err("Http: Unknown or missing ContentType\n");
						sess->rsp.start(HTTP_400);
						sess->rsp.end();
					}
					else if (!sess->req.hdr(ContentLength, len))
					{
						Log::Err("Http: Unknown or missing ContentLength\n");
						sess->rsp.start(HTTP_400);
						sess->rsp.end();
					}
					else
					{
						sess->tlvi.parse(d.p(), d.l());
						Log::Msg("Pairings: TLV item count %d\n", sess->tlvi.count());

						Tlv::State state;
						if (!sess->tlvi.get(Tlv::Type::State, state))
						{
							Log::Err("Pairings: State not found\n");
							sess->rsp.start(HTTP_400);
							sess->rsp.end();
						}
						else
						{
							if(state != Tlv::State::M1)
							{
								Log::Err("Pairings: Invalid State\n");
								sess->rsp.start(HTTP_400);
								sess->rsp.end();
							}
							else
							{
								Tlv::Method method;
								if (!sess->tlvi.get(Tlv::Type::Method, method))
								{
									Log::Err("Pairings: Method not found\n");
									sess->rsp.start(HTTP_400);
									sess->rsp.end();
								}
								else
								{
									switch (method)
									{
									case Tlv::Method::AddPairing:
										_pairingAdd(sess);
										break;

									case Tlv::Method::RemovePairing:
										_pairingRemove(sess);
										break;

									case Tlv::Method::ListPairing:
										_pairingList(sess);
										break;
									default:
										Log::Err("Pairings: Unknown method\n");
										sess->rsp.start(HTTP_400);
										sess->rsp.end();
									}
								}
							}
						}
					}
				}
				else
				{
					Log::Err("Http: Unknown path %.*s\n", p.l(), p.p());
					sess->rsp.start(HTTP_400);
					sess->rsp.end();
				}
			}
			else if (m.l() == 3 && strncmp(m.p(), "GET", 3) == 0)
			{
				// GET
				//		/accessories
				//		/characteristics
				if (!sess->secured)
				{
					Log::Err("Http: Authorization required\n");
					sess->rsp.start(HTTP_470);
					sess->rsp.end();
				}
				else if (p.l() == 12 && strncmp(p.p(), "/accessories", 12) == 0)
				{
					sess->rsp.start(HTTP_200);
					sess->rsp.add(ContentType, ContentTypeJson);
					sess->rsp.add(ContentLength, 0);
					sess->rsp.end();

					int len = _db.getDb(sess->Sid(), sess->rsp.data(), sess->rsp.size());

					Log::Dbg("Db: '%.*s'\n", len, sess->rsp.data());

					sess->rsp.setContentLength(len);
				}
				else if(strncmp(p.p(), "/characteristics?", 17) == 0)
				{

					int len = sess->sizeofdata();
					auto status = _db.Read(sess->Sid(), p.p() + 17, p.l() - 17, (char*)sess->data(), len);

					Log::Msg("Read: Status %d  '%.*s'\n", status, len, sess->data());

					sess->rsp.start(status);
					if (len > 0)
					{
						sess->rsp.add(ContentType, ContentTypeJson);
						sess->rsp.end((const char*)sess->data(), len);
					}
					else
					{
						sess->rsp.end();
					}
				}
				else
				{
					Log::Err("Http: Unknown path %.*s\n", p.l(), p.p());
					sess->rsp.start(HTTP_400);
					sess->rsp.end();
				}
			}
			else if (m.l() == 3 && strncmp(m.p(), "PUT", 3) == 0)
			{
				// PUT
				//		/characteristics
				if (p.l() == 16 && strncmp(p.p(), "/characteristics", 16) == 0)
				{
					int len;
					if (!sess->secured)
					{
						Log::Err("Http: Authorization required\n");
						sess->rsp.start(HTTP_470);
						sess->rsp.end();
					}
					else if (!sess->req.hdr(ContentType, ContentTypeJson))
					{
						Log::Err("Http: Unknown or missing ContentType\n");
						sess->rsp.start(HTTP_400);
						sess->rsp.end();
					}
					else if (!sess->req.hdr(ContentLength, len))
					{
						Log::Err("Http: Unknown or missing ContentLength\n");
						sess->rsp.start(HTTP_400);
						sess->rsp.end();
					}
					else
					{
						Log::Msg("Http: %.*s\n", d.l(), d.p());

						int len = sess->sizeofdata();
						auto status = _db.Write(sess->Sid(), (const char*)d.p(), d.l(), (char*)sess->data(), len);

						Log::Msg("Write: Status %d  '%.*s'\n", status, len, sess->data());

						sess->rsp.start(status);
						if (len > 0)
						{
							sess->rsp.add(ContentType, ContentTypeJson);
							sess->rsp.end((const char*)sess->data(), len);
						}
						else
						{
							sess->rsp.end();
						}
					}
				}
				else
				{
					Log::Err("Http: Unknown path %.*s\n", p.l(), p.p());
					sess->rsp.start(HTTP_400);
					sess->rsp.end();
				}
			}

			if (!_send(sess, send))
				return false;

			sess->secured = secured;
			Log::Msg("Http::Process exit Ses %d  secured %d\n", sid, sess->secured);

			return true;
		}

		void Server::Poll(sid_t sid, Send send)
		{
			Session* sess = &_sess[sid];
			if (!sess->secured)
				return;

			int len = sess->sizeofdata();
			auto status = _db.getEvents(sid, (char*)sess->data(), len);

			if (status != Http::Status::HTTP_200)
				return;

			if (len == 0)
				return;

			Log::Msg("Events: sid %d  '%.*s'\n", sid, len, sess->data());

			sess->rsp.event(status);
			sess->rsp.add(ContentType, ContentTypeJson);
			sess->rsp.end((const char*)sess->data(), len);

			_send(sess, send);
		}

		bool Server::_send(Session* sess, Send& send)
		{
			if (sess->secured)
			{
				// session secured - encrypt data
				const uint8_t *p = (uint8_t*)sess->rsp.buf();
				uint16_t len = sess->rsp.len();				// data length
				
				while (len > 0)
				{
					uint16_t aad = len;		// block length, and AAD for encryption

					if (aad > MaxHttpBlock)
						aad = MaxHttpBlock;

					// make 96-bit nonce from send sequential number
					uint8_t nonce[12];
					memset(nonce, 0, sizeof(nonce));
					memcpy(nonce + 4, &sess->sendSeq, 8);

					// encrypt into sess->data buffer which must be >= MaxHttpFrame
					uint8_t* b = sess->data();

					// copy data length into output buffer
					b[0] = aad & 0xFF;
					b[1] = (aad >> 8) & 0xFF;

					Crypto::Aead aead(Crypto::Aead::Encrypt,
						b + 2, b + 2 + aad,					// output data and tag positions
						sess->AccessoryToControllerKey,		// encryption key
						nonce,
						p, aad,								// data to encrypt
						b, 2								// aad
					);

					sess->sendSeq++;

					// send encrypted block
					send(sess->Sid(), (char*)b, 2 + aad + 16);

					len -= aad;
					p += aad;
				}
			}
			else
			{
				//send response as is
				send(sess->Sid(), sess->rsp.buf(), sess->rsp.len());
			}

			return true;
		}


		void Server::_pairSetup1(Session* sess)
		{
			Log::Msg("PairSetupM1\n");

			// prepare response without data
			sess->rsp.start(HTTP_200);
			sess->rsp.add(ContentType, ContentTypeTlv8);
			sess->rsp.add(ContentLength, 0);
			sess->rsp.end();

			// create response TLV in the response buffer right after HTTP headers 
			sess->tlvo.create((uint8_t*)sess->rsp.data(), sess->rsp.size());
			sess->tlvo.add(Hap::Tlv::Type::State, Hap::Tlv::State::M2);

			// verify that valid Method is present in input TLV
			Tlv::Method method;
			if (!sess->tlvi.get(Tlv::Type::Method, method))
			{
				Log::Err("PairSetupM1: Method not found\n");
				goto RetErr;
			}
			if (method != Tlv::Method::PairSetupNonMfi)
			{
				Log::Err("PairSetupM1: Invalid Method\n");
				goto RetErr;
			}

			// if the accessory is already paired it must respond Error_Unavailable
			if (_pairings.Count() != 0)
			{
				Log::Err("PairSetupM1: Already paired, return Error_Unavailable\n");
				sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Unavailable);
				goto Ret;
			}

			// if accessory received more than 100 unsuccessfull auth attempts, respond Error_MaxTries
			if (srp_auth_count > 100)
			{
				Log::Err("PairSetupM1: Too many auth attempts, return Error_MaxTries\n");
				sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::MaxTries);
				goto Ret;
			}

			// if accessory is currently performing PairSetup with different controller, respond Error_Busy
			if (srp.active())
			{
				Log::Err("PairSetupM1: Already pairing, return Error_Busy\n");
				sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Busy);
				goto Ret;
			}
			
			// create new pairing session
			if (!srp.open(sess->Sid()))
			{
				Log::Err("PairSetupM1: srp open error\n");
				goto RetErr;
			}

			srp_auth_count++;

			// init new verifier	// TODO: store salt and verifier in config data
			ver.init(Username, Hap::config->setupCode);
			Log::Hex("Srp.I", ver.I, (uint32_t)strlen(ver.I));
			Log::Hex("Srp.p", ver.p, (uint32_t)strlen(ver.p));
			Log::Hex("Srp.s", ver.s, Srp::SRP_SALT_BYTES);

			srp.init();

			Log::Hex("Srp.B", srp.getB(), Srp::SRP_PUBLIC_BYTES);

			sess->tlvo.add(Hap::Tlv::Type::PublicKey, srp.getB(), Srp::SRP_PUBLIC_BYTES);
			sess->tlvo.add(Hap::Tlv::Type::Salt, ver.s, Srp::SRP_SALT_BYTES);

			goto Ret;

		RetErr:
			sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Unknown);
		
		Ret:
			// adjust content length in response
			sess->rsp.setContentLength(sess->tlvo.length());
		}
	
		void Server::_pairSetup3(Session* sess)
		{
			uint8_t* iosKey = sess->data();
			uint16_t iosKey_size = Srp::SRP_PUBLIC_BYTES;
			uint8_t* iosProof = iosKey + Srp::SRP_PUBLIC_BYTES;
			uint16_t iosProof_size = Srp::SRP_PROOF_BYTES;

			Log::Msg("PairSetupM3\n");

			// prepare response without data
			sess->rsp.start(HTTP_200);
			sess->rsp.add(ContentType, ContentTypeTlv8);
			sess->rsp.add(ContentLength, 0);
			sess->rsp.end();

			// create response TLV in the response buffer right after HTTP headers 
			sess->tlvo.create((uint8_t*)sess->rsp.data(), sess->rsp.size());
			sess->tlvo.add(Hap::Tlv::Type::State, Hap::Tlv::State::M4);

			// verify that pairing is in progress on current session
			if (!srp.active(sess->Sid()))
			{
				Log::Err("PairSetupM3: No active pairing\n");
				goto RetErr;
			}

			// verify that required items are present in input TLV
			if (!sess->tlvi.get(Tlv::Type::PublicKey, iosKey, iosKey_size))
			{
				Log::Err("PairSetupM3: PublicKey not found\n");
				goto RetErr;
			}
			Log::Hex("iosKey", iosKey, iosKey_size);

			if (!sess->tlvi.get(Tlv::Type::Proof, iosProof, iosProof_size))
			{
				Log::Err("PairSetupM3: Proof not found\n");
				goto RetErr;
			}
			Log::Hex("iosProof", iosProof, iosProof_size);

			srp.setA(iosKey);

			Crypto::HkdfSha512(
				(const uint8_t*)"Pair-Setup-Encrypt-Salt", sizeof("Pair-Setup-Encrypt-Salt") - 1,
				srp.getK(), Srp::SRP_KEY_BYTES,
				(const uint8_t*)"Pair-Setup-Encrypt-Info", sizeof("Pair-Setup-Encrypt-Info") - 1,
				sess->key, sizeof(sess->key)
			);
			Log::Hex("SessKey", sess->key, sizeof(sess->key));

			if (!srp.verify(iosProof, iosProof_size))
			{
				Log::Err("PairSetupM3: SRP verify error\n");
				sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Authentication);
				goto Ret;
			}

			uint8_t V[Srp::SRP_PROOF_BYTES];
			srp.getV(V);
			Log::Hex("Response", V, sizeof(V));

			sess->tlvo.add(Hap::Tlv::Type::Proof, V, Srp::SRP_PROOF_BYTES);

			goto Ret;

		RetErr:	// error, cancel current pairing, if this session owns it
			if (srp.active(sess->Sid()))
				srp.close(sess->Sid());
			sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Unknown);

		Ret:
			// adjust content length in response
			sess->rsp.setContentLength(sess->tlvo.length());
		}

		void Server::_pairSetup5(Session* sess)
		{
			uint8_t* iosEncrypted;	// encrypted tata from iOS with tag attached
			uint8_t* iosTag;		// pointer to iOS tag
			uint8_t* iosTlv;		// decrypted TLV
			uint8_t* srvTag;		// calculated tag
			uint16_t iosTlv_size;

			Log::Msg("PairSetupM5\n");

			// prepare response without data
			sess->rsp.start(HTTP_200);
			sess->rsp.add(ContentType, ContentTypeTlv8);
			sess->rsp.add(ContentLength, 0);
			sess->rsp.end();

			// create response TLV in the response buffer right after HTTP headers 
			sess->tlvo.create((uint8_t*)sess->rsp.data(), sess->rsp.size());
			sess->tlvo.add(Hap::Tlv::Type::State, Hap::Tlv::State::M6);

			// verify that pairing is in progress on current session
			if (!srp.active(sess->Sid()))
			{
				Log::Err("PairSetupM5: No active pairing\n");
				goto RetErr;
			}

			// extract encrypted data into sess->data buffer
			iosEncrypted = sess->data();
			iosTlv_size = sess->sizeofdata();
			if (!sess->tlvi.get(Tlv::Type::EncryptedData, iosEncrypted, iosTlv_size))
			{
				Log::Err("PairSetupM5: EncryptedData not found\n");
				goto RetErr;
			}
			else
			{
				Hap::Tlv::Item id;
				Hap::Tlv::Item ltpk;
				Hap::Tlv::Item sign;

				// format sess->data buffer
				iosTlv = iosEncrypted + iosTlv_size;	// decrypted TLV
				iosTlv_size -= 16;						// strip off tag
				iosTag = iosEncrypted + iosTlv_size;	// iOS tag location
				srvTag = iosTlv + iosTlv_size;			// place for our tag

				// decrypt iOS data using session key
				Crypto::Aead aead(Crypto::Aead::Decrypt, iosTlv, srvTag,
					sess->key, (const uint8_t *)"\x00\x00\x00\x00PS-Msg05", 
					iosEncrypted, iosTlv_size);

				Log::Hex("iosTlv", iosTlv, iosTlv_size);
				Log::Hex("iosTag", iosTag, 16);
				Log::Hex("srvTlv", srvTag, 16);

				// compare calculated tag with passed in one
				if (memcmp(iosTag, srvTag, 16) != 0)
				{
					Log::Err("PairSetupM5: authTag does not match\n");
					sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Authentication);
					goto Ret;
				}

				// parse decrypted TLV - 3 items expected
				Hap::Tlv::Parse<3> tlv(iosTlv, iosTlv_size);
				Log::Err("PairSetupM5: TLV item count %d\n", tlv.count());

				// extract TLV items
				if (!tlv.get(Hap::Tlv::Type::Identifier, id))
				{
					Log::Err("PairSetupM5: Identifier not found\n");
					goto RetErr;
				}
				Log::Hex("iosPairingId:", id.p(), id.l());

				if (!tlv.get(Hap::Tlv::Type::PublicKey, ltpk))
				{
					Log::Err("PairSetupM5: PublicKey not found\n");
					goto RetErr;
				}
				Log::Hex("iosLTPK:", ltpk.p(), ltpk.l());

				if (!tlv.get(Hap::Tlv::Type::Signature, sign))
				{
					Log::Err("PairSetupM5: Signature not found\n");
					goto RetErr;
				}
				Log::Hex("iosSignature:", sign.p(), sign.l());

				// TODO: build iOSDeviceInfo and verify iOS device signature

				// add pairing info to pairig database
				if (!_pairings.Add(id, ltpk, Controller::Admin))
				{
					Log::Err("PairSetupM5: cannot add Pairing record\n");
					sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::MaxPeers);
					goto Ret;
				}

				// buid Accessory Info and sign it
				uint8_t* AccesoryInfo = sess->data();	// re-use sess->data buffer
				uint8_t* p = AccesoryInfo;
				int l = sess->sizeofdata();

				// add AccessoryX
				Crypto::HkdfSha512(
					(const uint8_t*)"Pair-Setup-Accessory-Sign-Salt", sizeof("Pair-Setup-Accessory-Sign-Salt") - 1,
					srp.getK(), Srp::SRP_KEY_BYTES,
					(const uint8_t*)"Pair-Setup-Accessory-Sign-Info", sizeof("Pair-Setup-Accessory-Sign-Info") - 1,
					p, 32);
				p += 32;
				l -= 32;
				
				// add Accessory PairingId
				memcpy(p, config->deviceId, strlen(config->deviceId));
				p += (uint32_t)strlen(config->deviceId);
				l -= (uint32_t)strlen(config->deviceId);

				// add Accessory LTPK
				memcpy(p, _keys.pubKey(), _keys.PUBKEY_SIZE_BYTES);
				p += _keys.PUBKEY_SIZE_BYTES;
				l -= _keys.PUBKEY_SIZE_BYTES;

				// sign the info
				_keys.sign(p, AccesoryInfo, (uint16_t)(p - AccesoryInfo));
				p += _keys.SIGN_SIZE_BYTES;
				l -= _keys.SIGN_SIZE_BYTES;

				// construct the sub-TLV
				Hap::Tlv::Create subTlv;
				subTlv.create(p, l);
				subTlv.add(Hap::Tlv::Type::Identifier, (const uint8_t*)config->deviceId, (uint16_t)strlen(config->deviceId));
				subTlv.add(Hap::Tlv::Type::PublicKey, _keys.pubKey(), _keys.PUBKEY_SIZE_BYTES);
				subTlv.add(Hap::Tlv::Type::Signature, p - _keys.SIGN_SIZE_BYTES, _keys.SIGN_SIZE_BYTES);
				p += subTlv.length();
				l -= subTlv.length();

				// enrypt AccessoryInfo using session key
				Crypto::Aead(Crypto::Aead::Encrypt,
					p,									// output encrypted TLV 
					p + subTlv.length(),				// output tag follows the encrypted TLV
					sess->key,						
					(const uint8_t *)"\x00\x00\x00\x00PS-Msg06",
					p - subTlv.length(),				// input TLV
					subTlv.length()						// TLV length
				);

				l -= subTlv.length() + 16;
				Log::Msg("PairSetupM5: sess->data unused: %d\n", l);

				// add encryped info and tag to output TLV
				sess->tlvo.add(Hap::Tlv::Type::EncryptedData, p, subTlv.length() + 16);

				Hap::config->Update();

				goto RetDone;	// pairing complete
			}

		RetErr:	// error, cancel current pairing, if this session owns it
			sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Unknown);

		RetDone:
			if (srp.active(sess->Sid()))
				srp.close(sess->Sid());

		Ret:
			// adjust content length in response
			sess->rsp.setContentLength(sess->tlvo.length());
		}

		void Server::_pairVerify1(Session* sess)
		{
			Hap::Tlv::Item iosKey;
			const uint8_t* sharedSecret;
			uint8_t* p;
			int l;

			Log::Msg("PairVerifyM1\n");

			// prepare response without data
			sess->rsp.start(HTTP_200);
			sess->rsp.add(ContentType, ContentTypeTlv8);
			sess->rsp.add(ContentLength, 0);
			sess->rsp.end();

			// create response TLV in the response buffer right after HTTP headers 
			sess->tlvo.create((uint8_t*)sess->rsp.data(), sess->rsp.size());
			sess->tlvo.add(Hap::Tlv::Type::State, Hap::Tlv::State::M2);

			// verify that PublicKey is present in input TLV
			if (!sess->tlvi.get(Tlv::Type::PublicKey, iosKey))
			{
				Log::Err("PairVerifyM1: PublicKey not found\n");
				goto RetErr;
			}

			// create new Curve25519 key pair
			sess->curve.init();

			// generate shared secret
			sharedSecret = sess->curve.sharedSecret(iosKey.p());

			// create session key from shared secret
			Crypto::HkdfSha512(
				(const uint8_t*)"Pair-Verify-Encrypt-Salt", sizeof("Pair-Verify-Encrypt-Salt") - 1,
				sharedSecret, sess->curve.KEY_SIZE_BYTES,
				(const uint8_t*)"Pair-Verify-Encrypt-Info", sizeof("Pair-Verify-Encrypt-Info") - 1,
				sess->key, sizeof(sess->key));

			// construct AccessoryInfo
			p = sess->data();
			l = sess->sizeofdata();

			//	add Curve25519 public key
			memcpy(p, sess->curve.pubKey(), sess->curve.KEY_SIZE_BYTES);
			p += sess->curve.KEY_SIZE_BYTES;
			l -= sess->curve.KEY_SIZE_BYTES;

			//	add Accessory PairingId
			memcpy(p, config->deviceId, strlen(config->deviceId));
			p += strlen(config->deviceId);
			l -= strlen(config->deviceId);

			// add iOS device public key
			memcpy(p, iosKey.p(), iosKey.l());
			p += iosKey.l();
			l -= iosKey.l();

			// sign the AccessoryInfo
			_keys.sign(p, sess->data(), p - sess->data());
			p += _keys.SIGN_SIZE_BYTES;
			l -= _keys.SIGN_SIZE_BYTES;

			// make sub-TLV
			Hap::Tlv::Create subTlv;
			subTlv.create(p, l);
			subTlv.add(Hap::Tlv::Type::Identifier, (const uint8_t*)config->deviceId, (uint16_t)strlen(config->deviceId));
			subTlv.add(Hap::Tlv::Type::Signature, p - _keys.SIGN_SIZE_BYTES, _keys.SIGN_SIZE_BYTES);
			p += subTlv.length();
			l -= subTlv.length();

			// encrypt sub-TLV using session key
			Crypto::Aead(Crypto::Aead::Encrypt,
				p,									// output encrypted TLV 
				p + subTlv.length(),				// output tag follows the encrypted TLV
				sess->key,
				(const uint8_t *)"\x00\x00\x00\x00PV-Msg02",
				p - subTlv.length(),				// input TLV
				subTlv.length()						// TLV length
			);

			l -= subTlv.length() + 16;
			Log::Msg("PairVerifyM1: sess->data unused: %d\n", l);

			// add Accessory public key to output TLV
			sess->tlvo.add(Hap::Tlv::Type::PublicKey, sess->curve.pubKey(), sess->curve.KEY_SIZE_BYTES);

			// add encryped info and tag to output TLV
			sess->tlvo.add(Hap::Tlv::Type::EncryptedData, p, subTlv.length() + 16);

			goto Ret;

		RetErr:	// error
			sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Unknown);

		Ret:
			// adjust content length in response
			sess->rsp.setContentLength(sess->tlvo.length());
		}

		void Server::_pairVerify3(Session* sess)
		{
			uint8_t* iosEncrypted;	// encrypted tata from iOS with tag attached
			uint8_t* iosTag;		// pointer to iOS tag
			uint8_t* iosTlv;		// decrypted TLV
			uint8_t* srvTag;		// calculated tag
			uint16_t iosTlv_size;

			Log::Msg("PairVerifyM3\n");

			// prepare response without data
			sess->rsp.start(HTTP_200);
			sess->rsp.add(ContentType, ContentTypeTlv8);
			sess->rsp.add(ContentLength, 0);
			sess->rsp.end();

			// create response TLV in the response buffer right after HTTP headers 
			sess->tlvo.create((uint8_t*)sess->rsp.data(), sess->rsp.size());
			sess->tlvo.add(Hap::Tlv::Type::State, Hap::Tlv::State::M4);

			// extract encrypted data into sess->data buffer
			iosEncrypted = sess->data();
			iosTlv_size = sess->sizeofdata();
			if (!sess->tlvi.get(Tlv::Type::EncryptedData, iosEncrypted, iosTlv_size))
			{
				Log::Err("PairVerifyM3: EncryptedData not found\n");
				goto RetErr;
			}
			else
			{
				Hap::Tlv::Item id;
				Hap::Tlv::Item sign;

				// format sess->data buffer
				iosTlv = iosEncrypted + iosTlv_size;	// decrypted TLV
				iosTlv_size -= 16;						// strip off tag
				iosTag = iosEncrypted + iosTlv_size;	// iOS tag location
				srvTag = iosTlv + iosTlv_size;			// place for our tag

														// decrypt iOS data using session key
				Crypto::Aead(Crypto::Aead::Decrypt, iosTlv, srvTag,
					sess->key, (const uint8_t *)"\x00\x00\x00\x00PV-Msg03",
					iosEncrypted, iosTlv_size);

				Log::Hex("iosTlv", iosTlv, iosTlv_size);
				Log::Hex("iosTag", iosTag, 16);
				Log::Hex("srvTlv", srvTag, 16);

				// compare calculated tag with passed in one
				if (memcmp(iosTag, srvTag, 16) != 0)
				{
					Log::Err("PairVerifyM3: authTag does not match\n");
					sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Authentication);
					goto Ret;
				}

				// parse decrypted TLV - 2 items expected
				Hap::Tlv::Parse<2> tlv(iosTlv, iosTlv_size);
				Log::Msg("PairVerifyM3: TLV item count %d\n", tlv.count());

				// extract TLV items
				if (!tlv.get(Hap::Tlv::Type::Identifier, id))
				{
					Log::Err("PairVerifyM3: Identifier not found\n");
					goto RetErr;
				}
				Log::Hex("iosPairingId:", id.p(), id.l());

				if (!tlv.get(Hap::Tlv::Type::Signature, sign))
				{
					Log::Err("PairVerifyM3: Signature not found\n");
					goto RetErr;
				}
				Log::Hex("iosSignature:", sign.p(), sign.l());

				// lookup iOS id in pairing database
				auto ios = _pairings.Get(id);
				if (ios == nullptr)
				{
					Log::Err("PairVerifyM3: iOS device ID not found\n");
					sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Authentication);
					goto Ret;
				}

				// TODO: construct iOSDeviceInfo and verify signature

				// create session encryption keys
				Crypto::HkdfSha512(
					(const uint8_t*)"Control-Salt", sizeof("Control-Salt") - 1,
					sess->curve.sharedSecret(), sess->curve.KEY_SIZE_BYTES,
					(const uint8_t*)"Control-Read-Encryption-Key", sizeof("Control-Read-Encryption-Key") - 1,
					sess->AccessoryToControllerKey, sizeof(sess->AccessoryToControllerKey));

				Crypto::HkdfSha512(
					(const uint8_t*)"Control-Salt", sizeof("Control-Salt") - 1,
					sess->curve.sharedSecret(), sess->curve.KEY_SIZE_BYTES,
					(const uint8_t*)"Control-Write-Encryption-Key", sizeof("Control-Write-Encryption-Key") - 1,
					sess->ControllerToAccessoryKey, sizeof(sess->ControllerToAccessoryKey));

				// mark session as secured after response is sent
				sess->ios = ios;

				goto Ret;
			}

		RetErr:	// error
			sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Unknown);

		Ret:
			// adjust content length in response
			sess->rsp.setContentLength(sess->tlvo.length());
		}

		void Server::_pairingAdd(Session* sess)
		{
			Tlv::Item id;
			Tlv::Item key;
			Controller::Perm perm;
			const Controller* ios;

			Log::Msg("PairingAdd\n");

			// prepare response without data
			sess->rsp.start(HTTP_200);
			sess->rsp.add(ContentType, ContentTypeTlv8);
			sess->rsp.add(ContentLength, 0);
			sess->rsp.end();

			// create response TLV in the response buffer right after HTTP headers 
			sess->tlvo.create((uint8_t*)sess->rsp.data(), sess->rsp.size());
			sess->tlvo.add(Hap::Tlv::Type::State, Hap::Tlv::State::M2);

			// verify that controller has admin permissions
			if (sess->ios->perm != Controller::Admin)
			{
				Log::Err("PairingAdd: No Admin permissions\n");
				sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Authentication);
				goto Ret;
			}

			// extract required items from input TLV 
			if (!sess->tlvi.get(Tlv::Type::Identifier, id))
			{
				Log::Err("PairingAdd: Identifier not found\n");
				goto RetErr;
			}
			Log::Hex("PairingAdd: Identifier", id.p(), id.l());

			if (!sess->tlvi.get(Tlv::Type::PublicKey, key))
			{
				Log::Err("PairingAdd: PublicKey not found\n");
				goto RetErr;
			}
			Log::Hex("PairingAdd: PublicKey", key.p(), key.l());

			if (!sess->tlvi.get(Tlv::Type::Permissions, perm))
			{
				Log::Err("PairingAdd: Permissions not found\n");
				goto RetErr;
			}
			Log::Msg("PairingAdd: Permissions 0x%X\n", perm);

			// locate new controller in pairing db
			ios = _pairings.Get(id);
			if (ios != nullptr)
			{
				// compare controller LTPK with stored one
				if (key.l() != Controller::KeyLen || memcmp(key.p(), ios->key, Controller::KeyLen) != 0)
				{
					Log::Err("PairingAdd: mismatch\n");
					goto RetErr;
				}

				_pairings.Update(id, perm);
			}
			else if (!_pairings.Add(id, key, perm))
			{
				Log::Err("PairingAdd: Unable to add\n");
				sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::MaxPeers);
				goto Ret;
			}

			Hap::config->Update();

			goto Ret;

		RetErr:	// error
			sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Unknown);

		Ret:
			// adjust content length in response
			sess->rsp.setContentLength(sess->tlvo.length());
		}

		void Server::_pairingRemove(Session* sess)
		{
			Tlv::Item id;

			Log::Msg("PairingRemove\n");

			// prepare response without data
			sess->rsp.start(HTTP_200);
			sess->rsp.add(ContentType, ContentTypeTlv8);
			sess->rsp.add(ContentLength, 0);
			sess->rsp.end();

			// create response TLV in the response buffer right after HTTP headers 
			sess->tlvo.create((uint8_t*)sess->rsp.data(), sess->rsp.size());
			sess->tlvo.add(Hap::Tlv::Type::State, Hap::Tlv::State::M2);

			// verify that controller has admin permissions
			if (sess->ios->perm != Controller::Admin)
			{
				Log::Err("PairingRemove: No Admin permissions\n");
				sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Authentication);
				goto Ret;
			}

			// extract required items from input TLV 
			if (!sess->tlvi.get(Tlv::Type::Identifier, id))
			{
				Log::Err("PairingRemove: Identifier not found\n");
				goto RetErr;
			}
			Log::Hex("PairingRemove: Identifier", id.p(), id.l());

			if (!_pairings.Remove(id))
			{
				Log::Err("PairingRemove: Remove error\n");
				goto RetErr;
			}

			Hap::config->Update();

			// TODO: close all sessions to removed controller

			goto Ret;

		RetErr:	// error
			sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Unknown);

		Ret:
			// adjust content length in response
			sess->rsp.setContentLength(sess->tlvo.length());
		}

		void Server::_pairingList(Session* sess)
		{
			bool first = true;
			bool rc;

			Log::Msg("PairingList\n");

			// prepare response without data
			sess->rsp.start(HTTP_200);
			sess->rsp.add(ContentType, ContentTypeTlv8);
			sess->rsp.add(ContentLength, 0);
			sess->rsp.end();

			// create response TLV in the response buffer right after HTTP headers 
			sess->tlvo.create((uint8_t*)sess->rsp.data(), sess->rsp.size());
			sess->tlvo.add(Hap::Tlv::Type::State, Hap::Tlv::State::M2);

			// verify that controller has admin permissions
			if (sess->ios->perm != Controller::Admin)
			{
				Log::Err("PairingList: No Admin permissions\n");
				sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Authentication);
				goto Ret;
			}

			rc = _pairings.forEach([sess, &first](const Controller* ios) -> bool {

				if (!first)
				{
					if (!sess->tlvo.add(Hap::Tlv::Type::Separator))
						return false;
				}

				if (!sess->tlvo.add(Hap::Tlv::Type::Identifier, ios->id, ios->IdLen))	// TODO: store real ID length (or zero-terminate?)
					return false;

				if (!sess->tlvo.add(Hap::Tlv::Type::PublicKey, ios->key, ios->KeyLen))
					return false;

				if (!sess->tlvo.add(Hap::Tlv::Type::Permissions, ios->perm))
					return false;

				first = false;

				return true;
			});

			if(rc)
				goto Ret;

		//RetErr:	// error
			Log::Err("PairingList: TLV overflow\n");
			sess->tlvo.add(Hap::Tlv::Type::Error, Hap::Tlv::Error::Unknown);

		Ret:
			// adjust content length in response
			sess->rsp.setContentLength(sess->tlvo.length());
		}
	}
}

/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#define HAP_LOG_REGISTER hapPairing
#include "Platform.h"

#include "Hap.h"
#include "HapPairing.h"

#include "Crypto/Crypto.h"
#include "Crypto/MD.h"
#include "Crypto/Srp.h"

namespace Hap::Pairing
{
    #define CASE_TYPE_STR(n) case TlvType::n: return #n
    const char* TlvTypeToStr(TlvType t)
    {
        switch (t)
        {
		CASE_TYPE_STR(Method);
		CASE_TYPE_STR(Identifier);
		CASE_TYPE_STR(Salt);
		CASE_TYPE_STR(PublicKey);
		CASE_TYPE_STR(Proof);
		CASE_TYPE_STR(EncryptedData);
		CASE_TYPE_STR(State);
		CASE_TYPE_STR(Error);
		CASE_TYPE_STR(RetryDelay);
		CASE_TYPE_STR(Certificate);
		CASE_TYPE_STR(Signature);
		CASE_TYPE_STR(Permissions);
		CASE_TYPE_STR(FragmentData);
		CASE_TYPE_STR(Fragmentlast);
		CASE_TYPE_STR(Flags);
        CASE_TYPE_STR(SessionID);
		CASE_TYPE_STR(Separator);
        }
        return "";
    }

    #undef CASE_TYPE_STR
    #define CASE_TYPE_STR(n) case Method::n: return #n
    const char* MethodToStr(Method m)
    {
        switch(m)
        {
		CASE_TYPE_STR(PairSetup);
		CASE_TYPE_STR(PairSetupWithAuth);
		CASE_TYPE_STR(PairVerify);
		CASE_TYPE_STR(AddPairing);
		CASE_TYPE_STR(RemovePairing);
		CASE_TYPE_STR(ListPairing);
        default:
            break;
        }
        return "";
    }

	// current pairing session - only one simultaneous pairing is allowed
	static Srp::Verifier ver;				// SRP verifier
	static Srp::Host srp(ver);				// .active()=true - pairing in progress, only one pairing at a time
    static uint8_t srp_auth_count;			// auth attempts counter

    void init()
    {
        srp_auth_count = 0;

		// init verifier from setup code
		//ver.init("Pair-Setup", Config::setupCode);

        // init verifier from saved config
        ver.init("Pair-Setup", Config::setupVerifier, Config::setupSalt);

		//HAP_LOG_HEX(ver.I, strlen(ver.I), "Srp.I");
		//HAP_LOG_HEX(ver.p, strlen(ver.p), "Srp.p");
		//HAP_LOG_HEX(ver.s, Srp::SRP_SALT_BYTES, "Srp.s");

		srp.init();

		//HAP_LOG_HEX(srp.getB(), Srp::SRP_PUBLIC_BYTES, "Srp.B");

        //for (uint16_t b = 0; b < Srp::SRP_PUBLIC_BYTES; b++)
        //    srp.B[b] = b & 0xFF;
    }

    // Pairing setup M1
    void SetupM1(Hap::Operation* op, Hap::Buf& req, Hap::Buf& rsp)
    {
        HAP_LOG_DBG("Pairing Setup M1");

        Hap::Session* sess = op->sess;

        rsp.append(TlvState(State::M2));

        // validate request
        Tlv tlv = Tlv::Get(req, (uint8_t)TlvType::Flags);
        PairingType::T flags = 0;
        if (tlv.valid())
        {
            flags = *(PairingType::T*)tlv.p();
        }

        tlv = Tlv::Get(req, (uint8_t)TlvType::Method);
        if (!tlv.valid())
        {
            HAP_LOG_ERR("Pairing Setup: no method TLV");
            rsp.append(TlvError(Error:: Unknown));
            return;
        }

        auto method = Method(*tlv.p());
        HAP_LOG_DBG("M1: %s  flags %08X", MethodToStr(method), flags);

        // TODO: if the accessory is already paired it must respond Error_Unavailable
   		//HAP_LOG_ERR("PairSetupM1: Already paired, return Error_Unavailable\n");

		// if accessory received more than 100 unsuccessfull auth attempts, respond Error_MaxTries
		if (srp_auth_count > 100)
		{
			HAP_LOG_ERR("PairSetupM1: Too many auth attempts, return Error_MaxTries");
            rsp.append(TlvError(Error::MaxTries));
			return;
		}

		// if accessory is currently performing PairSetup with different controller, respond Error_Busy
		if (srp.active())
		{
			HAP_LOG_ERR("PairSetupM1: Already pairing, return Error_Busy");
            rsp.append(TlvError(Error::Busy));
			return;
		}

		// create new pairing session
		if (!srp.open(sess->id))
		{
			HAP_LOG_WRN("PairSetupM1: srp open error");
            rsp.append(TlvError(Error:: Unknown));
            return;
		}

		srp_auth_count++;

        // construct response and send it back
        Tlv::Append(rsp, (uint8_t)TlvType::PublicKey, srp.getB(), Srp::SRP_PUBLIC_BYTES);
        Tlv::Append(rsp, (uint8_t)TlvType::Salt, ver.s, Srp::SRP_SALT_BYTES);
    }

    void SetupM3(Hap::Operation* op, Hap::Buf& req, Hap::Buf& rsp)
    {
        HAP_LOG_DBG("Pairing Setup M3");

        Hap::Session* sess = op->sess;

        rsp.append(TlvState(State::M4));

   		// verify that pairing is in progress on current session
		if (!srp.active(sess->id))
		{
			HAP_LOG_ERR("PairSetupM3: No active pairing");
            rsp.append(TlvError(Error::Unknown));
            return;
		}

        // extract Device TLVs
        Tmp tmp;
		uint8_t* key = tmp.p();
        auto sKey = Tlv::Get(req, (uint8_t)Hap::Pairing::TlvType::PublicKey,
            key, Srp::SRP_PUBLIC_BYTES);
        if (sKey != Srp::SRP_PUBLIC_BYTES)
        {
            HAP_LOG_ERR("Pairing Setup: no Public Key TLV");
            rsp.append(TlvError(Error:: Unknown));
            return;
        }
		uint8_t* proof = key + Srp::SRP_PUBLIC_BYTES;
        auto sProof = Tlv::Get(req, (uint8_t)Hap::Pairing::TlvType::Proof,
            proof, Srp::SRP_PROOF_BYTES);
        if (sProof != Srp::SRP_PROOF_BYTES)
        {
            HAP_LOG_ERR("Pairing Setup: no Proof TLV");
            rsp.append(TlvError(Error:: Unknown));
            return;
        }

        HAP_LOG_DBG("M3: key %d  proof %d", sKey, sProof);

		srp.setA(key);

		hkdf.calc(
			(const uint8_t*)"Pair-Setup-Encrypt-Salt", sizeof("Pair-Setup-Encrypt-Salt") - 1,
			srp.getK(), Srp::SRP_KEY_BYTES,
			(const uint8_t*)"Pair-Setup-Encrypt-Info", sizeof("Pair-Setup-Encrypt-Info") - 1,
			sess->key, sizeof(sess->key)
		);
		//HAP_LOG_HEX(sess->key, sizeof(sess->key), "SessKey");

		if (!srp.verify(proof, Srp::SRP_PROOF_BYTES))
		{
			HAP_LOG_ERR("PairSetupM3: SRP verify error\n");
			rsp.append(TlvError(Error::Authentication));
			return;
		}

        // get Accessory proof and send it back
		srp.getV(proof);
		//HAP_LOG_HEX(proof, Srp::SRP_PROOF_BYTES, "Accessory Proof");

        Tlv::Append(rsp, (uint8_t)TlvType::Proof, proof, Srp::SRP_PROOF_BYTES);
    }

    void SetupM5(Hap::Operation* op, Hap::Buf& req, Hap::Buf& rsp)
    {
        HAP_LOG_DBG("Pairing Setup M5");

        Hap::Session* sess = op->sess;

        rsp.append(TlvState(State::M6));

   		// verify that pairing is in progress on current session
		if (!srp.active(sess->id))
		{
			HAP_LOG_ERR("Pairing Setup M5: No active pairing");
            rsp.append(TlvError(Error::Unknown));
            return;
		}

        // extract encrypted data
        Tmp tmp;
        uint8_t* iosEnc = tmp.p();
        auto iosTlv_size = Tlv::Get(req, (uint8_t)TlvType::EncryptedData,
            iosEnc, tmp.s());
        if (iosTlv_size == 0)
        {
            HAP_LOG_ERR("Pairing Setup M5: no Encrypted Data TLV");
            rsp.append(TlvError(Error:: Unknown));
            return;
        }

        // format tmp buffer
        uint8_t* iosTlv = iosEnc + iosTlv_size;	    // decrypted TLV
        iosTlv_size -= 16;						    // strip off tag
        uint8_t* iosTag = iosEnc + iosTlv_size;	    // iOS tag location
        uint8_t* srvTag = iosTlv + iosTlv_size;		// place for our tag

        // decrypt iOS data using session key
        Crypto::Aead aead(Crypto::Aead::Decrypt, iosTlv, srvTag,
            sess->key, (const uint8_t *)"\x00\x00\x00\x00PS-Msg05", 
            iosEnc, iosTlv_size);

        //HAP_LOG_HEX(iosTlv, iosTlv_size, "iosTlv");
        //HAP_LOG_HEX(iosTag, 16, "iosTag");
        //HAP_LOG_HEX(srvTag, 16, "srvTag");

        // compare calculated tag with passed in one
        if (memcmp(iosTag, srvTag, 16) != 0)
        {
            HAP_LOG_ERR("Pairing Setup M5: authTag does not match\n");
			rsp.append(TlvError(Error::Authentication));
			return;
        }

        // parse decrypted TLV - 3 items expected
        Buf iosData(iosTlv, iosTlv_size, iosTlv_size);

        Tlv id = Tlv::Get(iosData, (uint8_t)Hap::Pairing::TlvType::Identifier);
        if (!id.valid())
        {
            HAP_LOG_ERR("Pairing Setup M5: no Identifier TLV");
            rsp.append(TlvError(Error:: Unknown));
            return;
        }
        //HAP_LOG_HEX(id.p(), id.l(), "iosPairingId");

        Tlv ltpk = Tlv::Get(iosData, (uint8_t)Hap::Pairing::TlvType::PublicKey);
        if (!ltpk.valid())
        {
            HAP_LOG_ERR("Pairing Setup M5: no PublicKey TLV");
            rsp.append(TlvError(Error:: Unknown));
            return;
        }
        //HAP_LOG_HEX(ltpk.p(), ltpk.l(), "iosLTPK");

        Tlv sign = Tlv::Get(iosData, (uint8_t)Hap::Pairing::TlvType::Signature);
        if (!sign.valid())
        {
            HAP_LOG_ERR("Pairing Setup M5: no Signature TLV");
            rsp.append(TlvError(Error::Unknown));
            return;
        }
        //HAP_LOG_HEX(sign.p(), sign.l(), "iosSignature");

        // TODO: build iOSDeviceInfo and verify iOS device signature

        // add pairing info to pairig database
        if (Config::pairings.Add(id, ltpk, Controller::Perm::Admin) == nullptr)
        {
            HAP_LOG_ERR("PairSetupM5: cannot add Pairing record");
            rsp.append(TlvError(Error::MaxPeers));
            return;
        }

        // buid Accessory Info (in tmp buf) and sign it
        uint8_t* AccesoryInfo = tmp.p();
        uint8_t* p = AccesoryInfo;
        int l = tmp.s();

        // add AccessoryX
        hkdf.calc(
            (const uint8_t*)"Pair-Setup-Accessory-Sign-Salt", sizeof("Pair-Setup-Accessory-Sign-Salt") - 1,
            srp.getK(), Srp::SRP_KEY_BYTES,
            (const uint8_t*)"Pair-Setup-Accessory-Sign-Info", sizeof("Pair-Setup-Accessory-Sign-Info") - 1,
            p, 32);
        p += 32;
        l -= 32;
				
        // add Accessory PairingId
        memcpy(p, Hap::Config::deviceIdStr(), Hap::Config::deviceIdLen());
        p += Hap::Config::deviceIdLen();
        l -= Hap::Config::deviceIdLen();

        // add Accessory LTPK
        memcpy(p, Config::keys.pubKey(), Config::keys.PUBKEY_SIZE_BYTES);
        p += Config::keys.PUBKEY_SIZE_BYTES;
        l -= Config::keys.PUBKEY_SIZE_BYTES;

        // sign the info
        Config::keys.sign(p, AccesoryInfo, (uint16_t)(p - AccesoryInfo));
        p += Config::keys.SIGN_SIZE_BYTES;
        l -= Config::keys.SIGN_SIZE_BYTES;

        // construct the sub-TLV
        Buf subTlv(p,l,0); // buffer for subTlv in tmp memory, initial length 0
        subTlv.append(Tlv(TlvType::Identifier, Buf(Hap::Config::deviceIdStr(), Hap::Config::deviceIdLen())));
        subTlv.append(Tlv(TlvType::PublicKey, Buf(Config::keys.pubKey(), Config::keys.PUBKEY_SIZE_BYTES)));
        subTlv.append(Tlv(TlvType::Signature, Buf(p - Config::keys.SIGN_SIZE_BYTES, Config::keys.SIGN_SIZE_BYTES)));
        p += subTlv.l();
        l -= subTlv.l();

        // enrypt AccessoryInfo using session key
        Crypto::Aead(Crypto::Aead::Encrypt,
            p,						// output encrypted TLV 
            p + subTlv.l(),			// output tag follows the encrypted TLV
            sess->key,						
            (const uint8_t *)"\x00\x00\x00\x00PS-Msg06",
            p - subTlv.l(),			// input TLV
            subTlv.l()				// TLV length
        );

        l -= subTlv.l() + 16;
        HAP_LOG_INF("PairSetupM5: tmp unused: %d", l);

        // add encryped info and tag
        rsp.append(Tlv(TlvType::EncryptedData, Buf(p, subTlv.l() + 16)));

        // update configuration (non-paired->paired)
        Hap::Config::Update();

		if (srp.active(sess->id))
			srp.close(sess->id);
    }

    void VerifyM1(Hap::Operation* op, Hap::Buf& req, Hap::Buf& rsp)
    {
        HAP_LOG_DBG("Pairing Verify M1");

        Hap::Session* sess = op->sess;

        rsp.append(TlvState(State::M2));

        Tlv ltpk = Tlv::Get(req, (uint8_t)TlvType::PublicKey);
        if (!ltpk.valid())
        {
            HAP_LOG_ERR("Pairing Verify M1: no PublicKey TLV");
            rsp.append(TlvError(Error:: Unknown));
            return;
        }
        //HAP_LOG_HEX(ltpk.p(), ltpk.l(), "iosLTPK");

		// create new Curve25519 key pair
		sess->curve.init();

		// generate shared secret
		const uint8_t* sharedSecret = sess->curve.sharedSecret(ltpk.p());

		// create session key from shared secret
		hkdf.calc(
			(const uint8_t*)"Pair-Verify-Encrypt-Salt", sizeof("Pair-Verify-Encrypt-Salt") - 1,
			sharedSecret, sess->curve.KEY_SIZE_BYTES,
			(const uint8_t*)"Pair-Verify-Encrypt-Info", sizeof("Pair-Verify-Encrypt-Info") - 1,
			sess->key, sizeof(sess->key));

		// construct AccessoryInfo
        Tmp tmp;
        uint8_t* AccesoryInfo = tmp.p();
		uint8_t* p = AccesoryInfo;
		uint32_t l = tmp.s();

		//	add Curve25519 public key
		memcpy(p, sess->curve.pubKey(), sess->curve.KEY_SIZE_BYTES);
		p += sess->curve.KEY_SIZE_BYTES;
		l -= sess->curve.KEY_SIZE_BYTES;

		//	add Accessory PairingId
        memcpy(p, Hap::Config::deviceIdStr(), Hap::Config::deviceIdLen());
        p += Hap::Config::deviceIdLen();
        l -= Hap::Config::deviceIdLen();

		// add iOS device public key
		memcpy(p, ltpk.p(), ltpk.l());
		p += ltpk.l();
		l -= ltpk.l();

		// sign the AccessoryInfo
		Config::keys.sign(p, AccesoryInfo, (uint16_t)(p - AccesoryInfo));
		p += Config::keys.SIGN_SIZE_BYTES;
		l -= Config::keys.SIGN_SIZE_BYTES;

		// make sub-TLV
        Buf subTlv(p,l,0); // buffer for subTlv in tmp memory, initial length 0
        subTlv.append(Tlv(TlvType::Identifier, Buf(Hap::Config::deviceIdStr(), Hap::Config::deviceIdLen())));
        subTlv.append(Tlv(TlvType::Signature, Buf(p - Config::keys.SIGN_SIZE_BYTES, Config::keys.SIGN_SIZE_BYTES)));
        p += subTlv.l();
        l -= subTlv.l();

		// encrypt sub-TLV using session key
		Crypto::Aead(Crypto::Aead::Encrypt,
			p,							// output encrypted TLV 
			p + subTlv.l(),				// output tag follows the encrypted TLV
			sess->key,
			(const uint8_t *)"\x00\x00\x00\x00PV-Msg02",
			p - subTlv.l(),				// input TLV
			subTlv.l()					// TLV length
		);

		l -= subTlv.l() + 16;
        HAP_LOG_INF("Pairing Verify M1: tmp unused: %d", l);

		// add Accessory public key to output TLV
		rsp.append(Tlv(TlvType::PublicKey, Buf(sess->curve.pubKey(), sess->curve.KEY_SIZE_BYTES)));

        // add encryped info and tag
        rsp.append(Tlv(TlvType::EncryptedData, Buf(p, subTlv.l() + 16)));

    }
    
    void VerifyM3(Hap::Operation* op, Hap::Buf& req, Hap::Buf& rsp)
    {
        HAP_LOG_DBG("Pairing Verify M3");

        Hap::Session* sess = op->sess;

        rsp.append(TlvState(State::M4));

        // extract encrypted data
        Tmp tmp;
        uint8_t* iosEnc = tmp.p();
        auto iosTlv_size = Tlv::Get(req, (uint8_t)TlvType::EncryptedData,
            iosEnc, tmp.s());
        if (iosTlv_size == 0)
        {
            HAP_LOG_ERR("Pairing Verify M3: no Encrypted Data TLV");
            rsp.append(TlvError(Error:: Unknown));
            return;
        }

        // prepare response
        // format tmp buffer
        uint8_t* iosTlv = iosEnc + iosTlv_size;	    // decrypted TLV
        iosTlv_size -= 16;						    // strip off tag
        uint8_t* iosTag = iosEnc + iosTlv_size;	    // iOS tag location
        uint8_t* srvTag = iosTlv + iosTlv_size;		// place for our tag

        // decrypt iOS data using session key
        Crypto::Aead aead(Crypto::Aead::Decrypt, iosTlv, srvTag,
            sess->key, (const uint8_t *)"\x00\x00\x00\x00PV-Msg03", 
            iosEnc, iosTlv_size);

        //HAP_LOG_HEX(iosTlv, iosTlv_size, "iosTlv");
        //HAP_LOG_HEX(iosTag, 16, "iosTag");
        //HAP_LOG_HEX(srvTag, 16, "srvTag");

        // compare calculated tag with passed in one
        if (memcmp(iosTag, srvTag, 16) != 0)
        {
            HAP_LOG_ERR("Pairing Verify M3: authTag does not match\n");
			rsp.append(TlvError(Error::Authentication));
			return;
        }

        // parse decrypted TLV - 2 items expected
        Buf iosData(iosTlv, iosTlv_size, iosTlv_size);

        Tlv id = Tlv::Get(iosData, (uint8_t)Hap::Pairing::TlvType::Identifier);
        if (!id.valid())
        {
            HAP_LOG_ERR("Pairing Verify M3: no Identifier TLV");
            rsp.append(TlvError(Error:: Unknown));
            return;
        }
        //HAP_LOG_HEX(id.p(), id.l(), "iosPairingId");

        Tlv sign = Tlv::Get(iosData, (uint8_t)Hap::Pairing::TlvType::Signature);
        if (!sign.valid())
        {
            HAP_LOG_ERR("Pairing Verify M3: no Signature TLV");
            rsp.append(TlvError(Error::Unknown));
            return;
        }
        //HAP_LOG_HEX(sign.p(), sign.l(), "iosSignature");

        // lookup iOS id in pairing database
        Controller* ctlr = Config::pairings.Get(id);
        if (ctlr == nullptr)
        {
            HAP_LOG_ERR("Pairing Verify M3: iOS device ID not found");
            rsp.append(TlvError(Error::Authentication));
            return;
        }

        // TODO: construct iOSDeviceInfo and verify signature

        // create session encryption keys
        hkdf.calc(
            (const uint8_t*)"Control-Salt", sizeof("Control-Salt") - 1,
            sess->curve.sharedSecret(), sess->curve.KEY_SIZE_BYTES,
            (const uint8_t*)"Control-Read-Encryption-Key", sizeof("Control-Read-Encryption-Key") - 1,
            sess->AccessoryToControllerKey, sizeof(sess->AccessoryToControllerKey));

        hkdf.calc(
            (const uint8_t*)"Control-Salt", sizeof("Control-Salt") - 1,
            sess->curve.sharedSecret(), sess->curve.KEY_SIZE_BYTES,
            (const uint8_t*)"Control-Write-Encryption-Key", sizeof("Control-Write-Encryption-Key") - 1,
            sess->ControllerToAccessoryKey, sizeof(sess->ControllerToAccessoryKey));

        // mark session as secured after response is sent
        sess->ctlrPaired = ctlr;

        // create and save Resumable session
        Mem<8> sessID;
        hkdf.calc(
            (const uint8_t*)"Pair-Verify-ResumeSessionID-Salt", sizeof("Pair-Verify-ResumeSessionID-Salt") - 1,
            sess->curve.sharedSecret(), sess->curve.KEY_SIZE_BYTES,
            (const uint8_t*)"Pair-Verify-ResumeSessionID-Info", sizeof("Pair-Verify-ResumeSessionID-Info") - 1,
            sessID.p(), sessID.s());

        Config::pairings.AddSess(ctlr, sessID, Buf(sess->curve.sharedSecret(), sess->curve.KEY_SIZE_BYTES));
        Config::Update();
    }

    void ResumeM1(Hap::Operation* op, Hap::Buf& req, Hap::Buf& rsp)
    {
        HAP_LOG_DBG("Pairing Resume M1");

        Hap::Session* sess = op->sess;

        Tlv iosLtpk = Tlv::Get(req, (uint8_t)TlvType::PublicKey);
        if (!iosLtpk.valid())
        {
            HAP_LOG_ERR("Pairing Resume M1: no PublicKey TLV");
            rsp.append(TlvError(Error:: Unknown));
            return;
        }
        //HAP_LOG_HEX(iosLtpk.p(), iosLtpk.l(), "iosLTPK");

        Tlv iosSid = Tlv::Get(req, (uint8_t)TlvType::SessionID);
        if (!iosSid.valid())
        {
            HAP_LOG_ERR("Pairing Resume M1: no SessionID TLV");
            rsp.append(TlvError(Error:: Unknown));
            return;
        }
        //HAP_LOG_HEX(iosSid.p(), iosSid.l(), "iosSID");

        Tlv iosTag = Tlv::Get(req, (uint8_t)TlvType::EncryptedData);
        if (!iosTag.valid())
        {
            HAP_LOG_ERR("Pairing Resume M1: no EncryptedData TLV");
            rsp.append(TlvError(Error:: Unknown));
            return;
        }
        //HAP_LOG_HEX(iosTag.p(), iosTag.l(), "iosTAG");

        Controller* ctlr = Config::pairings.GetSess(iosSid);
        if (ctlr == nullptr)
        {
            LOG_WRN("Pairing Resume M1: No saved session found");
            // fall back to Verify
            VerifyM1(op, req, rsp);
            return;
        }

        rsp.append(TlvState(State::M2));

        // derive encryption key; salt = <controller public key><session id>
        Mem<Crypto::Curve25519::KEY_SIZE_BYTES + Controller::SessLen> salt;
        salt.init();
        salt.append(iosLtpk.p(), iosLtpk.l());
        salt.append(iosSid.p(), iosSid.l());
        //HAP_LOG_HEX(salt.p(), salt.l(), "salt");
		hkdf.calc(
			salt.p(), salt.l(),
			ctlr->ssec, ctlr->SsecLen,    // key - shared secret from previous session
			(const uint8_t*)"Pair-Resume-Request-Info", sizeof("Pair-Resume-Request-Info") - 1,
			sess->key, sizeof(sess->key));

        // generate tag by decrypting empty data
        uint8_t tag[Crypto::Aead::TAG_SIZE_BYTES];
		Crypto::Aead(Crypto::Aead::Encrypt,
			tag,					// output data 
			tag,				    // output tag
			sess->key,
			(const uint8_t *)"\x00\x00\x00\x00PR-Msg01",
			NULL,				    // input data
			0					    // input length
		);
        //HAP_LOG_HEX(tag, sizeof(tag), "TAG");

        if (memcmp(tag, iosTag.p(), Crypto::Aead::TAG_SIZE_BYTES) != 0)
        {
            HAP_LOG_ERR("Pairing Resume M1: tag mismatch");
            rsp.append(TlvError(Error::Authentication));
            return;
        }

        // generate new random session ID
        Crypto::rnd_data((unsigned char*)&ctlr->sess, ctlr->SessLen);
        ctlr->sessValid = true;

        // derive response encryption key; salt = <controller public key><new session id>
        salt.init();
        salt.append(iosLtpk.p(), iosLtpk.l());
        salt.append(&ctlr->sess, ctlr->SessLen);
		hkdf.calc(
			salt.p(), salt.l(),
			ctlr->ssec, ctlr->SsecLen,    // key - shared secret from previous session
			(const uint8_t*)"Pair-Resume-Response-Info", sizeof("Pair-Resume-Response-Info") - 1,
			sess->key, sizeof(sess->key));

        // encrypt and generate auth-tag with an empty response data
		Crypto::Aead(Crypto::Aead::Encrypt,
			tag,					// output data 
			tag,				    // output tag
			sess->key,
			(const uint8_t *)"\x00\x00\x00\x00PR-Msg02",
			NULL,				    // input data
			0					    // input length
		);
        //HAP_LOG_HEX(tag, sizeof(tag), "TAG");

        // send response to controller
        rsp.append(TlvMethod(Method::Resume));
        rsp.append(Tlv(TlvType::SessionID, Buf(&ctlr->sess, ctlr->SessLen)));
        rsp.append(Tlv(TlvType::EncryptedData, Buf(tag, sizeof(tag))));

        // derive a new shared secret
		hkdf.calc(
			salt.p(), salt.l(),
			ctlr->ssec, ctlr->SsecLen,    // key - shared secret from previous session
			(const uint8_t*)"Pair-Resume-Shared-Secret-Info", sizeof("Pair-Resume-Shared-Secret-Info") - 1,
			sess->curve.sharedSecret(), Crypto::Curve25519::KEY_SIZE_BYTES);

        // update shared secret in pairing record
        memcpy(ctlr->ssec, sess->curve.sharedSecret(), Crypto::Curve25519::KEY_SIZE_BYTES);
        ctlr->ssecValid = true;

        // create session encryption (as in Verify M3)
        hkdf.calc(
            (const uint8_t*)"Control-Salt", sizeof("Control-Salt") - 1,
            sess->curve.sharedSecret(), sess->curve.KEY_SIZE_BYTES,
            (const uint8_t*)"Control-Read-Encryption-Key", sizeof("Control-Read-Encryption-Key") - 1,
            sess->AccessoryToControllerKey, sizeof(sess->AccessoryToControllerKey));

        hkdf.calc(
            (const uint8_t*)"Control-Salt", sizeof("Control-Salt") - 1,
            sess->curve.sharedSecret(), sess->curve.KEY_SIZE_BYTES,
            (const uint8_t*)"Control-Write-Encryption-Key", sizeof("Control-Write-Encryption-Key") - 1,
            sess->ControllerToAccessoryKey, sizeof(sess->ControllerToAccessoryKey));

        // mark session secured after response is sent
        sess->ctlrPaired = ctlr;

        Config::Update();
    }

    void AddM1(Hap::Operation* op, Hap::Buf& req, Hap::Buf& rsp)
    {
        HAP_LOG_DBG("Pairing Add M1");

        Hap::Session* sess = op->sess;

        rsp.append(TlvState(State::M2));

		// verify that controller has admin permissions
		if (sess->ctlrPaired->perm != Controller::Perm::Admin)
		{
			HAP_LOG_ERR("Pairing Remove M1: No Admin permissions\n");
			rsp.append(TlvError(Error::Authentication));
            return;
		}

        Tlv id = Tlv::Get(req, (uint8_t)Hap::Pairing::TlvType::Identifier);
        if (!id.valid())
        {
            HAP_LOG_ERR("Pairing Add M1: no Identifier TLV");
            rsp.append(TlvError(Error::Unknown));
            return;
        }
        //HAP_LOG_HEX(id.p(), id.l(), "Identifier");

        Tlv ltpk = Tlv::Get(req, (uint8_t)Hap::Pairing::TlvType::PublicKey);
        if (!ltpk.valid())
        {
            HAP_LOG_ERR("Pairing Add M1: no PublicKey TLV");
            rsp.append(TlvError(Error::Unknown));
            return;
        }
        //HAP_LOG_HEX(key.p(), key.l(), "PublicKey");

        Tlv perm = Tlv::Get(req, (uint8_t)Hap::Pairing::TlvType::Permissions);
        if (!perm.valid())
        {
            HAP_LOG_ERR("Pairing Add M1: no Permissions TLV");
            rsp.append(TlvError(Error::Unknown));
            return;
        }
        //HAP_LOG_HEX(perm.p(), perm.l(), "Permissions");

        auto ios = Config::pairings.Get(id);
        if (ios != nullptr)
        {
			// compare controller LTPK with stored one
			if (ltpk.l() != Controller::LtpkLen || memcmp(ltpk.p(), ios->ltpk, Controller::LtpkLen) != 0)
			{
				HAP_LOG_ERR("Pairing Add M1: key mismatch");
                rsp.append(TlvError(Error::Unknown));
                return;
			}

            Config::pairings.Update(id, Controller::Perm(*perm.p()));
        }
        else if (Config::pairings.Add(id, ltpk, Controller::Perm(*perm.p())) == nullptr)
        {
            HAP_LOG_ERR("Pairing Add M1: Pairing add error");
            rsp.append(TlvError(Error::Unknown));
            return;
        }

        // update configuration
        Hap::Config::Update();
    }

    void RemoveM1(Hap::Operation* op, Hap::Buf& req, Hap::Buf& rsp)
    {
        HAP_LOG_DBG("Pairing Remove M1");

        Hap::Session* sess = op->sess;

        rsp.append(TlvState(State::M2));

		// verify that controller has admin permissions
		if (sess->ctlrPaired->perm != Controller::Perm::Admin)
		{
			HAP_LOG_ERR("Pairing Remove M1: No Admin permissions");
			rsp.append(TlvError(Error::Authentication));
            return;
		}

        Tlv id = Tlv::Get(req, (uint8_t)Hap::Pairing::TlvType::Identifier);
        if (!id.valid())
        {
            HAP_LOG_ERR("Pairing Remove M1: no Identifier TLV");
            rsp.append(TlvError(Error::Unknown));
            return;
        }
        //HAP_LOG_HEX(id.p(), id.l(), "Identifier");

        // Remove returns ptr to pairing record marked as removed
        //  but still containing valid controller ID.
        // After sending this response the Session code will close
        //  all sessions to removed controller. 
        sess->ctlrRemoved = Config::pairings.Remove(id);

        // update configuration (paired->non-paired)
        Hap::Config::Update();
    }

    void ListM1(Hap::Operation* op, Hap::Buf& req, Hap::Buf& rsp)
    {
        HAP_LOG_DBG("Pairing List M1");

        Hap::Session* sess = op->sess;

        rsp.append(TlvState(State::M2));

		// verify that controller has admin permissions
		if (sess->ctlrPaired->perm != Controller::Perm::Admin)
		{
			HAP_LOG_ERR("Pairing List M1: No Admin permissions");
			rsp.append(TlvError(Error::Authentication));
            return;
		}

        bool first;
        bool rc = Config::pairings.forEach([&rsp, &first](const Controller* ios) -> bool
        {
            if (!first)
                rsp.append(TlvSeparator());

            if (!rsp.append(Tlv(TlvType::Identifier, Buf(ios->ctlr, ios->CtlrLen))))
                return false;
            if (!rsp.append(Tlv(TlvType::PublicKey, Buf(ios->ltpk, ios->LtpkLen))))
                return false;
            if (!rsp.append(Tlv(TlvType::Permissions, Buf(&ios->perm, 1))))
                return false;

            first = false;

            return true;
        });

        if (!rc)
        {
			HAP_LOG_ERR("Pairing List M1: Resonse overflow");
            rsp.init();
            rsp.append(TlvState(State::M2));
			rsp.append(TlvError(Error::Unknown));
            return;
        }
    }

	Hap::Status dispatch(Hap::Operation* op, Path path, Hap::Buf& req, Hap::Buf& rsp)
    {
        Hap::Status status = Hap::Status::Busy;

        Hap::Session* sess = op->sess;

        Method method = Method::Unknown;
        Tlv methodTlv = Tlv::Get(req, (uint8_t)Hap::Pairing::TlvType::Method);
        if (methodTlv.valid() && methodTlv.l() == 1)     // 1-byte integer expected
        {
            method = Method(*methodTlv.p());
        }

        Tlv stateTlv = Tlv::Get(req, (uint8_t)Hap::Pairing::TlvType::State);
        if (!stateTlv.valid() || stateTlv.l() != 1)     // 1-byte integer expected
        {
            HAP_LOG_ERR("Pairing: no State TLV");
            return Hap::Status::InvalidRequest;
        }
        State state = State(*stateTlv.p());

        switch (path)
        {
        case Path::Setup:
            if (sess->secured)
            {
                HAP_LOG_ERR("Pairing Setup: Session is secured");
                status = Hap::Status::InvalidRequest;
            }
            else if (method == Method::Unknown ||
                method == Method::PairSetup ||
                method == Method::PairSetupWithAuth)
            {
                switch(state)
                {
                case State::M1:
                    op->async.exec(SetupM1, req, rsp);
                    break;
                case State::M3:
                    op->async.exec(SetupM3, req, rsp);
                    break;
                case State::M5:
                    op->async.exec(SetupM5, req, rsp);
                    break;
                default:
                    HAP_LOG_ERR("Pairing Setup: invalid State %d", (uint8_t)state);
                    status = Hap::Status::InvalidRequest;
                }
            }
            else
            {
                HAP_LOG_ERR("Pairing Setup: invalid Method %d", (uint8_t)method);
                status = Hap::Status::InvalidRequest;
            }
            break;

        case Path::Verify:
            if (method == Method::Unknown ||
                method == Method::PairVerify)
            {
                switch(state)
                {
                case State::M1:
                    op->async.exec(Hap::Pairing::VerifyM1, req, rsp);
                    break;
                case State::M3:
                    op->async.exec(Hap::Pairing::VerifyM3, req, rsp);
                    break;
                default:
                    HAP_LOG_ERR("Pairing Verify: invalid State %d", (uint8_t)state);
                    status = Hap::Status::InvalidRequest;
                }
            }
            else if (method == Method::Resume)
            {
                if (state == State::M1)
                {
                    op->async.exec(Hap::Pairing::ResumeM1, req, rsp);
                }
                else
                {
                    HAP_LOG_ERR("Pairing Resume: invalid State %d", (uint8_t)state);
                    status = Hap::Status::InvalidRequest;
                }
            }
            else
            {
                HAP_LOG_ERR("Pairing Verify: invalid Method %d", (uint8_t)method);
                status = Hap::Status::InvalidRequest;
            }
            break;

        case Path::Pairings:
            if (method ==Method::AddPairing)
            {
                switch(state)
                {
                case State::M1:
                    op->async.exec(Hap::Pairing::AddM1, req, rsp);
                    break;
                default:
                    HAP_LOG_ERR("Pairing Add: invalid State %d", (uint8_t)state);
                    status = Hap::Status::InvalidRequest;
                }
            }
            else if (method == Method::RemovePairing)
            {
                switch(state)
                {
                case State::M1:
                    op->async.exec(Hap::Pairing::RemoveM1, req, rsp);
                    break;
                default:
                    HAP_LOG_ERR("Pairing Remove: invalid State %d", (uint8_t)state);
                    status = Hap::Status::InvalidRequest;
                }
            }
            else if (method == Method::ListPairing)
            {
                switch(state)
                {
                case State::M1:
                    op->async.exec(Hap::Pairing::ListM1, req, rsp);
                    break;
                default:
                    HAP_LOG_ERR("Pairing List: invalid State %d", (uint8_t)state);
                    status = Hap::Status::InvalidRequest;
                }
            }
            else
            {
                HAP_LOG_ERR("Pairing Pairings: invalid Method %d", (uint8_t)method);
                status = Hap::Status::InvalidRequest;
            }
            break;

        default:
            HAP_LOG_ERR("Pairing: invalid Path %d", (uint8_t)path);
            status = Hap::Status::InvalidRequest;
        }

        return status;
    }

}



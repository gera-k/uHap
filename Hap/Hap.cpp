/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#define HAP_LOG_REGISTER hapDb
#include "Platform.h"

#include "Hap.h"

namespace Hap
{

	Crypto::HkdfSha512 hkdf;

    #define CASE_TYPE_STR(n) case Type::n: return #n
    const char* TypeToStr(Type t)
    {
        switch (t)
        {
		CASE_TYPE_STR(Null);
		CASE_TYPE_STR(AccIid);
		CASE_TYPE_STR(AccSvc);
		CASE_TYPE_STR(SvcType);
		CASE_TYPE_STR(SvcIid);
		CASE_TYPE_STR(SvcChar);
		CASE_TYPE_STR(SvcProp);
		CASE_TYPE_STR(SvcLinked);
		CASE_TYPE_STR(CharType);
		CASE_TYPE_STR(CharIid);
		CASE_TYPE_STR(Value);
		CASE_TYPE_STR(Permissions);
		CASE_TYPE_STR(Event);
		CASE_TYPE_STR(Description);
		CASE_TYPE_STR(Format);
		CASE_TYPE_STR(Unit);
		CASE_TYPE_STR(MinValue);
		CASE_TYPE_STR(MaxValue);
		CASE_TYPE_STR(StepValue);
		CASE_TYPE_STR(MaxLength);
		CASE_TYPE_STR(MaxDataLength);
		CASE_TYPE_STR(ValidValues);
		CASE_TYPE_STR(ValidRange);
		CASE_TYPE_STR(Ttl);
        }
        return "";
    }

    #define CASE_FMT_STR(n) case Format::n: return #n
    const char* FormatToStr(Format f)
    {
        switch (f)
        {
		CASE_FMT_STR(Null);
		CASE_FMT_STR(Bool);
		CASE_FMT_STR(Uint8);
		CASE_FMT_STR(Uint16);
		CASE_FMT_STR(Uint32);
		CASE_FMT_STR(Uint64);
		CASE_FMT_STR(Int);
		CASE_FMT_STR(Float);
		CASE_FMT_STR(String);
		CASE_FMT_STR(Data);
		CASE_FMT_STR(Tlv);
		CASE_FMT_STR(Uuid);
		CASE_FMT_STR(Format);
		CASE_FMT_STR(Unit);
		CASE_FMT_STR(Iid);
        }
        return "";
    }

	// Init pairings - destroy all existing records
	void Pairings::Init()		
	{
		for (unsigned i = 0; i < sizeofarr(ctl); i++)
		{
			ctl[i].perm = Controller::Perm::None;
			ctl[i].sessValid = false;
			ctl[i].ssecValid = false;
		}
	}

	uint8_t Pairings::Count(Controller::Perm perm)
	{
		uint8_t cnt = 0;
		for (unsigned i = 0; i < sizeofarr(ctl); i++)
		{
			Controller* rec = &ctl[i];

			if (rec->perm == Controller::Perm::None)
				continue;

			if (perm == Controller::Perm::None || perm == rec->perm)
				cnt++;
		}

		return cnt;
	}

	Controller* Pairings::Add(const uint8_t* ctlr, uint32_t ctlr_len, const uint8_t* ltpk, Controller::Perm perm)
	{
		for (unsigned i = 0; i < sizeofarr(ctl); i++)
		{
			Controller* rec = &ctl[i];
			if (rec->perm == Controller::Perm::None)
			{
				// add new record
				memset(rec->ctlr, 0, sizeof(rec->ctlr));
				uint8_t ctlrLen = uint8_t(ctlr_len > Controller::CtlrLen ? Controller::CtlrLen : ctlr_len);
				memcpy(rec->ctlr, ctlr, ctlrLen);
				memcpy(rec->ltpk, ltpk, Controller::LtpkLen);
				rec->perm = perm;

				return rec;
			}
			else if (memcmp(rec->ctlr, ctlr, ctlr_len) == 0)
			{
				// Id matches, TODO

				return rec;
			}
		}

		return nullptr;
	}

	Controller* Pairings::Add(const Hap::Tlv& ctlr, const Hap::Tlv& ltpk, Controller::Perm perm)
	{
		if (ctlr.l() > Controller::CtlrLen)
		{
			HAP_LOG_ERR("Pairings: Invalid ID length %d\n", ctlr.l());
			return nullptr;
		}

		if (ltpk.l() != Controller::LtpkLen)
		{
			HAP_LOG_ERR("Pairings: Invalid key length %d\n", ltpk.l());
			return nullptr;
		}

		return Add(ctlr.p(), ctlr.l(), ltpk.p(), perm);
	}

	bool Pairings::Update(const Hap::Tlv& ctlr, Controller::Perm perm)
	{
		if (ctlr.l() > Controller::CtlrLen)
			return false;

		for (unsigned i = 0; i < sizeofarr(ctl); i++)
		{
			Controller* rec = &ctl[i];

			if (rec->perm == Controller::Perm::None)	// empty record
				continue;

			if (memcmp(rec->ctlr, ctlr.p(), ctlr.l()) == 0)
			{
				rec->perm = perm;
				return true;
			}
		}

		return false;
	}

	const Controller* Pairings::Remove(const Hap::Tlv& ctlr)
	{
		if (ctlr.l() > Controller::CtlrLen)
			return nullptr;

		for (unsigned i = 0; i < sizeofarr(ctl); i++)
		{
			Controller* rec = &ctl[i];

			if (memcmp(rec->ctlr, ctlr.p(), ctlr.l()) == 0)
			{
				rec->perm = Controller::Perm::None;	// mark record empty
				return rec;
			}
		}

		return nullptr;
	}

	Controller* Pairings::Get(const Hap::Tlv& ctlr)
	{
		if (ctlr.l() > Controller::CtlrLen)
			return nullptr;

		for (unsigned i = 0; i < sizeofarr(ctl); i++)
		{
			Controller* rec = &ctl[i];

			if (rec->perm == Controller::Perm::None)	// empty record
				continue;

			if (memcmp(rec->ctlr, ctlr.p(), ctlr.l()) == 0)
				return rec;
		}

		return nullptr;
	}

	bool Pairings::forEach(std::function<bool(const Controller*)> cb)
	{
		for (unsigned i = 0; i < sizeofarr(ctl); i++)
		{
			Controller* rec = &ctl[i];

			if (rec->perm == Controller::Perm::None)	// empty record
				continue;

			if (!cb(rec))
				return false;
		}

		return true;
	}

	// Add resumable session ID
	void Pairings::AddSess(Controller* rec, const Hap::Buf& sess, const Hap::Buf& ssec)
	{
		memcpy(&rec->sess, sess.p(), Controller::SessLen);
		memcpy(rec->ssec, ssec.p(), Controller::SsecLen);
		rec->sessValid = true;
		rec->ssecValid = true;
	}

	// Find controller by resumable session ID
	Controller* Pairings::GetSess(const Hap::Tlv& sess)
	{
		for (unsigned i = 0; i < sizeofarr(ctl); i++)
		{
			Controller* rec = &ctl[i];

			if (rec->perm == Controller::Perm::None)	// empty record
				continue;

			if (!rec->sessValid || !rec->ssecValid)
				continue;

			if (memcmp(&rec->sess, sess.p(), Controller::SessLen) == 0)
			{
				rec->sessValid = false;
				rec->ssecValid = false;
				return rec;
			}
		}

		return nullptr;
	}

	namespace Config
	{
		const char* deviceIdStr()
		{
			static char s[18];
			snprintf(s, sizeof(s), "%02X:%02X:%02X:%02X:%02X:%02X",
				deviceId[0], deviceId[1], deviceId[2],
				deviceId[3], deviceId[4], deviceId[5]
				);
			return s;
		}
		uint8_t deviceIdLen()
		{
			return 17;
		}
	}


	// characteristic default read
	Hap::Status Characteristic::Read(Operation* op, Buf& req, Buf& rsp)
	{
		if (onRead)
			onRead();

		// append characteristic value
        rsp.append(ptrVal->value);

		return Hap::Status::Success;
	}

	// characteristic default write
	Hap::Status Characteristic::Write(Operation* op, Buf& req, Buf& rsp)
	{
		// copy received value to characteristic value, length must match
		if (req.l() != ptrVal->len())
			return Hap::Status::InvalidRequest;

		memcpy(ptrVal->data(), req.p(), req.l());

		if (onWrite)
			onWrite();

		return Hap::Status::Success;
	}

	// default connected event enable/disable
	void Characteristic::ConnectedEvent(Session* sess, bool enable)
	{
		HAP_LOG_DBG("%s: ConnectedEvent: %s", name, enable ? "enable" : "disable");
		event = enable;
	}

	// default Broadcast event enable/disable
	void Characteristic::BroadcastEvent(Session* sess, bool enable, uint8_t interval)
	{
		HAP_LOG_DBG("%s: BroadcastEvent: %s  interval %d", name, enable ? "enable" : "disable", interval);
		bc_event = enable;
		bc_int = interval;
	}

	void Characteristic::Indicate()
	{
		if (perm() & (Perm::ev | Perm::de | Perm::bn))
		{
			// call platform 
			EventIndicate(this);
		}
	}
}
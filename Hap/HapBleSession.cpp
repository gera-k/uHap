/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#define HAP_LOG_REGISTER hapBleSess
#include "Platform.h"

#include "HapBle.h"

namespace Hap::Ble
{
    void Session::Stop()
    {
        HAP_LOG_DBG("Session::Stop");

        while (!STAILQ_EMPTY(&proc_list))
        {
            Procedure* proc = STAILQ_FIRST(&proc_list);
            STAILQ_REMOVE_HEAD(&proc_list, entry);

            // TODO: if async is active, terminate/wait

            ProcFree(proc);
        }

        // reset all session data
        reset();
    }

    // GATT read from ch->value property
    ssize_t Session::Read(Hap::Property* prop, void *buf, u16_t len, u16_t offset)
    {
        Hap::Characteristic* ch = prop->charOwner;
        HAP_LOG_DBG("Session::Rd %s.%s  len %d off %d", ch->name, Hap::TypeToStr(prop->type), len, offset);

        lastActivity = Hap::Time::stampMs();

        // find active procedure
        Procedure* proc = procGet(ch);
        if (proc != nullptr)
        {
            // verify if procedure timeout has expired
            if (proc->timeout > 0 && proc->timeout < Time::stampMs())
            {
                procDel(proc);
                ProcFree(proc);
                proc = nullptr;
            }
        }
        if (proc == nullptr)
        {
            HAP_LOG_ERR("Rd %s.%s: no active procedure or timeout", ch->name, Hap::TypeToStr(prop->type));

            // alloc new procedure to build/encrypt the response
            proc = ProcAlloc(ch);
            if (proc == nullptr)
            {
                return BleErrorUnlikely; // TODO: send HAP Status PDU MaxProcedures
            }
            procAdd(proc);

            proc->rsp.init(0, true);
            proc->rsp.Sts(Hap::Status::InvalidRequest);
        }

        // wait for procedure completion
        //  non-async procedure returns immediately
        if (!proc->async.wait())
        {
            HAP_LOG_ERR("Rd %s.%s: procedure timeout", ch->name, Hap::TypeToStr(prop->type));
            proc->rsp.init(proc->req.TID(), true);
            proc->rsp.Sts(Hap::Status::InvalidRequest);
        }

        // if first read (off == 0) encrypt PDU
        if (offset == 0)
            Encode(proc);

        //HAP_LOG_HEX(proc->rsp.p(), proc->rsp.l(), "Response");

        // copy response PDU to buffer
        if (offset + len > proc->tmp.l())
            len = proc->tmp.l() - offset;
        
        memcpy(buf, proc->tmp.p(offset), len);

        if (offset + len == proc->tmp.l())
        {
            // procedure complete when complete response is transmitted 
            //  TODO: the spec shows pairing procedures may contain multiple Wr/Rd exchanges 
            procDel(proc);
            ProcFree(proc);

            // if paired, mark session secure
            secured = ctlrPaired != nullptr;
            HAP_LOG_DBG("Session is %ssecured", secured ? "" : "not ");

            if (ctlrRemoved != NULL)
            {
                Disconnect(ctlrRemoved); 
                ctlrRemoved = NULL;
            }
        }

        LOG_DBG("return %d", len);
        return len;
    }

    // GATT write to ch->value property
    ssize_t Session::Write(Hap::Property* prop, const void *buf, u16_t len, u16_t offset)
    {
        Hap::Characteristic* ch = prop->charOwner;
        HAP_LOG_DBG("Session::Wr %s.%s  len %d off %d", ch->name, Hap::TypeToStr(prop->type), len, offset);

        lastActivity = Hap::Time::stampMs();

        // write to Pairing.Verify should drop session secured state (R2: 7.4.7.2)
        //  and tear down any pending transactions
        if (secured && memcmp(ch->uuid(), Pairing::uuidVerify.data(), Pairing::uuidVerify.size()) == 0)
        {
            Stop();
        }

        // find active procedure
        Procedure* proc = procGet(ch);

        // if there is no active procedure start new 
        if (proc == nullptr)
        {
            proc = ProcAlloc(ch);
            if (proc == nullptr)
            {
                return BleErrorUnlikely;    // TODO: send HAP Status PDU MaxProcedures
            }

            // set procedure expiration timeout to 10 sec
            if (ch->perm() & Perm::nt)      // no timeout
                proc->timeout = 0;
            else
                proc->timeout = Time::stampMs() + 10000;

            procAdd(proc);

            // cancel any expired timed write on this char
            if (ch->twValue != nullptr && ch->twExpire < Time::stampMs())
            {
                HAP_LOG_WRN("%s: Cancel expired timed write", ch->name);
                BufFree(ch->twValue);
                ch->twValue = nullptr;
            }
        }

        // decode if session is secured or copy if not
        //  puts result into Tmp buffer
        Decode(proc, buf, len);

        if (!proc->req.append(proc->tmp))
        {
            //TODO: terminate session
        }

        if (!proc->req.is_complete())
            return len;

        //HAP_LOG_HEX(proc->req.p(), proc->req.l(), "Request");

         // process received PDU
        proc->Process();

        // Always return total length indicating that all write data was accepted.
        // In case of HAP error the HAP Status is already set in rsp by Process
        return len;
    }

    // Decode or copy input data into tmp
    bool Session::Decode(Procedure* proc, const void *buf, u16_t len)
    {
        bool ret;
        Buf& tmp(proc->tmp);

        proc->tmp.init();
        if (!secured)
        {
            // copy data to tmp buffer
            ret = tmp.append(buf, len);
        }
        else
        {
            // decrypt data into temp buffer
            uint8_t* p = tmp.p();

            len -= 16;      // tag size
            
            // make 96-bit nonce from receive sequential number
            uint8_t nonce[12];
            memset(nonce, 0, sizeof(nonce));
            memcpy(nonce + 4, &recvSeq, 8);

            Crypto::Aead aead(Crypto::Aead::Decrypt,
                p, p + len,						// output data and tag positions
                ControllerToAccessoryKey,		// decryption key
                nonce,
                (uint8_t*)buf, len				// encrypted data
            );

            recvSeq++;

            tmp.l(len);

            // compare passed in and calculated tags
            ret = memcmp(p + len, (uint8_t*)buf + len, 16) == 0;
        }

        if (!ret)
        {
            HAP_LOG_ERR("Session::Decode failure");
        }
        else
        {
            //HAP_LOG_HEX(tmp.p(), tmp.l(), "Decoded");
        }

        return ret;
    }

    bool Session::Encode(Procedure* proc)
    {
        bool ret;
        PduRspBuf& rsp(proc->rsp);
        Buf& tmp(proc->tmp);

        tmp.init();
        if (!secured)
        {
            // copy rsp to tmp buffer
            tmp.append(rsp.p(), rsp.l());
            ret = true;
        }
        else
        {
            // make 96-bit nonce from send sequential number
            uint8_t nonce[12];
            memset(nonce, 0, sizeof(nonce));
            memcpy(nonce + 4, &sendSeq, 8);

            // encrypt into tmp buffer
            uint8_t* b = tmp.p();

            Crypto::Aead aead(Crypto::Aead::Encrypt,
                b, b + rsp.l(),					// output data and tag positions
                AccessoryToControllerKey,	    // encryption key
                nonce,
                rsp.p(), rsp.l()				// data to encrypt
            );

            sendSeq++;

            tmp.l(rsp.l() + 16);

            ret = true;
        }

        if (!ret)
        {
            HAP_LOG_ERR("Session::Encode failure");
        }
        else
        {
            //HAP_LOG_HEX(tmp.p(), tmp.l(), "Encoded");
        }

        return ret;
    }

    void Procedure::Process()
    {
        Hap::Status status;

        rsp.init(req.TID());

        switch (req.opcode())
        {
        case PduReq::Opcode::CharSignatureRead:
            // Signature read must be supported with and without a secure session (R2: 7.3.2, 7.3.5.1)
            status = charSignatureRead();
            break;

        case PduReq::Opcode::CharWrite:
            // Perm::wr - without secure session (pairing service)
            // Perm::pw - with secure session
            if (ch->perm() & Perm::wr)
            {
                status = charWrite();
            }
            else if (ch->perm() & Perm::pw)
            {
                if (sess->secured)
                    status = charWrite();
                else
                    status = Hap::Status::InsufficientAuthentication;
            }
            else
                status = Hap::Status::InvalidRequest;
            break;

        case PduReq::Opcode::CharRead:
            // Perm::pr - with secure session
            // Perm::rd - without secure session (pairing service)
            if (ch->perm() & Perm::rd)
            {
                status = charRead();
            }
            else if (ch->perm() & Perm::pr)
            {
                if (sess->secured)
                    status = charRead();
                else
                    status = Hap::Status::InsufficientAuthentication;
            }
            else
                status = Hap::Status::InvalidRequest;
            break;

        case PduReq::Opcode::CharTimedWrite:
            // Perm::pw - with secure session
            if (ch->perm() & Perm::pw)
            {
                if (sess->secured)
                    status = charTimedWrite();
                else
                    status = Hap::Status::InsufficientAuthentication;
            }
            else
                status = Hap::Status::InvalidRequest;
            break;

        case PduReq::Opcode::CharExecuteWrite:
            // Perm::pw - with secure session
            if (ch->perm() & Perm::pw)
            {
                if (sess->secured)
                    status = charExecuteWrite();
                else
                    status = Hap::Status::InsufficientAuthentication;
            }
            else
                status = Hap::Status::InvalidRequest;
            break;

        case PduReq::Opcode::SvcSignatureRead:
            // supported with and without a secure session (R2: 7.3.5.1)
            status = svcSignatureRead();
            break;

        case PduReq::Opcode::CharConfiguration: // R2: 7.3.5.8
            // assume secure session is required
            if (sess->secured)
                status = charConfiguration();
            else
                status = Hap::Status::InsufficientAuthentication;
            break;

        case PduReq::Opcode::ProtoConfiguration:
            // assume secure session is required
            if (sess->secured)
                status = protoConfiguration();
            else
                status = Hap::Status::InsufficientAuthentication;
            break;

        default:
            status = Hap::Status::InvalidRequest;
        }

        if (status != Hap::Status::Success)
        {
            if (status != Hap::Status::Busy)
            {
                // error - return error response
                rsp.Sts(status);
            }
            else
            {
                // busy status means async processing started
                // return Success - already set above in rsp.init()
            }
        }
    }

    Hap::Status Procedure::charSignatureRead()
    {
        HAP_LOG_DBG("%d:%s: CharSignatureRead", sess->id, ch->name);
        
        // build character signature
        rsp.append(TlvCharType(ch->uuid()));
        rsp.append(TlvSvcIid(ch->svcOwner->iid()));
        rsp.append(TlvSvcType(ch->svcOwner->uuid()));
        rsp.append(TlvCharProp(ch->perm()));

        Format f = ch->format();
        Unit u = Hap::Unit::Unitless;
        Hap::Property* unit = ch->get(Hap::Type::Unit);
        if (unit != NULL)
            u = (*(static_cast<Hap::PropUnit*>(unit)))();
        rsp.append(TlvFormat(f, u));

        Hap::Property* min = ch->get(Hap::Type::MinValue);
        Hap::Property* max = ch->get(Hap::Type::MaxValue);
        if (min != NULL && max != NULL)
            rsp.append(TlvValidRange(min->value, max->value));

        Hap::Property* step = ch->get(Hap::Type::StepValue);
        if (step != NULL)
            rsp.append(TlvStep(step->value));

        //if (ch->ptrVal != NULL && ch->ptrVal->value.l() < 8)
        //    rsp.append(Tlv(TlvType::Value, ch->ptrVal->value));

        return Hap::Status::Success;
    }

    Hap::Status Procedure::charWrite()
    {
        HAP_LOG_DBG("%d:%s: CharWrite", sess->id, ch->name);
        
        // when Return response is not set, Write returns Status PDU
        // when it is set, Write returns response Value set by ch->Write callback
        bool retResp = false;

        // extract Value and RetResp from PDU body
        if (!req.extractValue(&retResp, nullptr))
            return Hap::Status::UnsupportedPDU;

        // pass write request to the characteristic
        //  Default write (Hap::Characteristic::Write) copies
        //  rx data to characteristic value prop.
        //  Custom write (e.g. pairing) parses received data as required,
        //  and prepares response in rsp buf (if retResp is requested).
        //  Write can schedule async work, it this case it returns Busy,
        //  and subsequent GATT read waits for async work completion
        //  in Async::wait.
        auto status = ch->Write(this, req.body(), rsp.body());
        if (status == Hap::Status::Busy)
        {
            status = Hap::Status::Success;
        }
        else if (status != Hap::Status::Success)
        {
            // re-init response with status returned from Write
            rsp.Sts(status);
        }
        else if (retResp)
        {
            // reformat PDU body for BLE (add TLV prefixes, split as necessary)
            rsp.formatValue();
        }

        return Hap::Status::Success;
    }

    Hap::Status Procedure::charRead()
    {
        HAP_LOG_DBG("%d:%s: CharRead", sess->id, ch->name);

        // defaut read in Hap.cpp just appends the charactristic value to the tx buffer
        //  custom read can put whatever it needs into the tx buffer
        auto status = ch->Read(this, req.body(), rsp.body());

        if (status != Hap::Status::Success)
        {
            // set response status
            rsp.Sts(status);
        }
        else
        {
            // reformat PDU body for BLE
            rsp.formatValue();
        }

        return Hap::Status::Success;
    }

    Hap::Status Procedure::charTimedWrite()
    {
        HAP_LOG_INF("%d:%s: CharTimedWrite", sess->id, ch->name);

        bool retResp = false;
        uint8_t ttl;

        // extract TTL and Valuefrom PDU body
        if (!req.extractValue(&retResp, &ttl))
            return Hap::Status::UnsupportedPDU;

        if (ch->twValue != nullptr)
        {
            BufFree(ch->twValue);
            ch->twValue = nullptr;
        }
        ch->twValue = BufAlloc();
        if (ch->twValue == nullptr)
        {
            HAP_LOG_ERR("%d:%s: CharTimedWrite: Buf allocation error", sess->id, ch->name);
            return Hap::Status::InvalidRequest;
        }
        ch->twValue->init();
        ch->twValue->append(req.body());
        ch->twRetResp = retResp;
        ch->twExpire = Time::stampMs() + (uint32_t)ttl * 100;

        HAP_LOG_INF("%d:%s: CharTimedWrite: ttl %d", sess->id, ch->name, ttl);

        return Hap::Status::Success;
    }

    Hap::Status Procedure::charExecuteWrite()
    {
        HAP_LOG_INF("%d:%s: CharExecuteWrite", sess->id, ch->name);

        if (ch->twValue == nullptr)
        {
            HAP_LOG_ERR("%d:%s: CharExecuteWrite: no pending timed write", sess->id, ch->name);
            return Hap::Status::InvalidRequest;
        }

        if (ch->twExpire < Time::stampMs())
        {
            HAP_LOG_ERR("%d:%s: CharExecuteWrite: timed write expired", sess->id, ch->name);
            BufFree(ch->twValue);
            ch->twValue = nullptr;
            return Hap::Status::InvalidRequest;
        }

        // copy saved value to req.body
        req.copy(*ch->twValue);
        BufFree(ch->twValue);
        ch->twValue = nullptr;

        // exec write as in CharWrite
        auto status = ch->Write(this, req.body(), rsp.body());
        if (status == Hap::Status::Busy)
        {
            status = Hap::Status::Success;
        }
        else if (status != Hap::Status::Success)
        {
            // re-init response with status returned from Write
            rsp.Sts(status);
        }
        else if (ch->twRetResp)
        {
            // reformat PDU body for BLE (add TLV prefixes, split as necessary)
            rsp.formatValue();
        }

        return Hap::Status::Success;
    }

    Hap::Status Procedure::svcSignatureRead()
    {
        HAP_LOG_DBG("%d:%s: SvcSignatureRead", sess->id, ch->name);

        Hap::Service* svc = ch->svcOwner;

        if (svc->prop() != 0)
            rsp.append(TlvSvcProp(svc->prop.value));

        // TODO: linked services

        return Hap::Status::Success;
    }

    Hap::Status Procedure::charConfiguration()
    {
        HAP_LOG_DBG("%d:%s: CharConfiguration", sess->id, ch->name);
        
        //HAP_LOG_HEX(req.p(), req.l(), "Request");

        auto rqb = req.body();

        uint16_t prop = ch->bc_event ? 1 : 0;
        uint8_t bc_int = ch->bc_int;

        Tlv tlv = Tlv::Get(rqb, (uint8_t)TlvCharConfigType::Prop);
        if (tlv.valid())
        {
            prop = *tlv.p();
            HAP_LOG_DBG("CharConfig: Properties %04X", prop);
        }
        tlv = Tlv::Get(rqb, (uint8_t)TlvCharConfigType::BcInt);
        if (tlv.valid())
        {
            bc_int = *tlv.p();
            HAP_LOG_DBG("CharConfig: Interval %02X", bc_int);
        }

        ch->BroadcastEvent(sess, prop & 0x0001, bc_int);

        rsp.append(Tlv((uint8_t)TlvCharConfigType::Prop, Buf(&prop, sizeof(prop))));
        rsp.append(Tlv((uint8_t)TlvCharConfigType::BcInt, Buf(&bc_int, sizeof(bc_int))));

        return Hap::Status::Success;
    }

    Hap::Status Procedure::protoConfiguration()
    {
        HAP_LOG_DBG("%d:%s: ProtoConfiguration", sess->id, ch->name);

        HAP_LOG_HEX(req.p(), req.l(), "Request");
        
        auto rqb = req.body();
        HAP_LOG_HEX(rqb.p(), rqb.l(), "Body");

        Tlv tlv = Tlv::Get(rqb, (uint8_t)TlvProtoConfigReqType::BEK);
        if (tlv.valid())
        {
            HAP_LOG_DBG("ProtoConfig: Generate Broacast Encryption Key");
            Adv::GenBEK(sess);
        }
        tlv = Tlv::Get(rqb, (uint8_t)TlvProtoConfigReqType::AAI);
        if (tlv.valid())
        {
            HAP_LOG_DBG("ProtoConfig: Set Accessory Advertising Identifier");
            Adv::SetAAI(tlv.v());
        }
        tlv = Tlv::Get(rqb, (uint8_t)TlvProtoConfigReqType::All);
        if (tlv.valid())
        {
            HAP_LOG_DBG("ProtoConfig: Get All Params");

            rsp.append(Tlv((uint8_t)TlvProtoConfigRspType::SN, Buf(&Config::globalStateNum, sizeof(uint16_t))));
            rsp.append(Tlv((uint8_t)TlvProtoConfigRspType::CN, Buf(&Config::configNum, sizeof(uint8_t))));
            rsp.append(Tlv((uint8_t)TlvProtoConfigRspType::AAI, Adv::AAI));
            rsp.append(Tlv((uint8_t)TlvProtoConfigRspType::BEK, Adv::BEK));
        }

        HAP_LOG_HEX(rsp.p(), rsp.l(), "Response");
        
        return Hap::Status::Success;
    }

}
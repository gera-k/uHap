#define HAP_LOG_REGISTER hapBlePdu
#include "Platform.h"

#include "HapBle.h"

namespace Hap::Ble
{

    // append PDU fragment received from transport
    bool PduReq::append(const Buf& frag_)
    {
        //HAP_LOG_HEX(frag_.p(), frag_.l(), "Fragment");

        // parse received fragment
        Pdu frag(frag_);

        //HAP_LOG_HEX(frag.p(), frag.l(), "PduBase");

        if (frag.pdu_type() != Pdu::PduType::Request)
        {
            HAP_LOG_ERR("Received wrong PDU type %d, Request expected", (uint8_t)frag.pdu_type());
            return false;
        }

        if (!frag.pdu_is_cont())    // First fragment
        {
            if (_buf.l() != 0)
            {
                HAP_LOG_ERR("Received First fragment, current length %d", _buf.l());
                return false;
            }

            // copy fragment into PDU buffer
            bool ret = _buf.append(frag_);
            //HAP_LOG_HEX(_buf.p(), _buf.l(), "PduReq");
            return ret;
        }
        else
        {
            if (_buf.l() == 0)
            {
                HAP_LOG_ERR("Received Continuation fragment, current length %d", _buf.l());
                return false;
            }

            // continuation fragment
            if (frag.cont_tid() != TID())
            {
                HAP_LOG_ERR("Received Continuation fragment TID %02X, expected %02X", frag.cont_tid(), TID());
                return false;
            }

            return _buf.append(frag.cont_body());
        }
    }

    //  Convert sequence of value TLVs to contiguous value data.
    //  The method concatenates all fragments of Value TLV
    //  and moves then to the beginning of PDU body
    //  destroying value TLV prefixes and all other TLVs.
    //  Adjust buf length as necessary.
    bool PduReq::extractValue(bool* retResp, uint8_t* ttl)
    {
        uint8_t* b = _buf.p(7);
        uint16_t l = _len();
        _buf.l(7);  // reset buffer length to PDU header
        while (l > 0)
        {
            Tlv tlv(b, l);
            if (!tlv.valid())
            {
                HAP_LOG_ERR("PDU parse error");
                return false;
            }

            //HAP_LOG_DBG("extractValue: Tlv %s  len %d", TypeToStr(TlvType(tlv.t())), tlv.l());

            //  copy data over previous body content destroying TLV prefix(ex)
            //  and concatenate fragments
            if (TlvType(tlv.t()) == TlvType::Value)
                _buf.append(tlv.p(), tlv.l());   // add value only

            if (TlvType(tlv.t()) == TlvType::RetResp)
            {
                if (retResp != nullptr)
                {
                    *retResp = true;
                }
                else
                {
                    HAP_LOG_ERR("extractValue: unexpected RetResp TLV");
                    return false;
                }
            }

            if (TlvType(tlv.t()) == TlvType::Ttl)
            {
                if (ttl != nullptr)
                {
                    *ttl = *tlv.p();
                }
                else
                {
                    HAP_LOG_ERR("extractValue: unexpected TTp TLV");
                    return false;
                }
            }

            b += tlv.s();
            l -= tlv.s();
        }

        //HAP_LOG_DBG("extractValue: buf %p %d", _buf.p(), _buf.l());

        return true;
    }

    // init - init header, destroy content
    void PduRsp::init(uint8_t tid, bool nodata)
    {
        _buf.l(nodata ? 3 : 5);
        ctl(PduType::Response); 
        TID(tid);
        Sts(Hap::Status::Success);
        _len(0);
        _body = Buf(_buf.p(5), _buf.s() - 5, 0);
    }

    // append TLV prefix and value to PDU body
    void PduRsp::append(const Tlv& tlv)
    {
        _buf.append(tlv);
        _len(_buf.l() - 5);
    }

    //  convert contiguous value data to sequence of Value TLVs
    //  adjust body length (including PDU header) as necessary
    void PduRsp::formatValue()
    {
        // adjust PDU length
        _buf.l(5 + _body.l());

        // make Value fragments
        uint8_t* b = _body.p();
        uint16_t len = _body.l();
        
        //HAP_LOG_HEX(b, len, "PduRsp body");

        while (len > 0)
        {
            uint8_t l = len > 255 ? 255 : len;
            // make room for value TLV prefix
            memmove(b+2, b, len);
            _buf.l(_buf.l() + 2);

            *b++ = (uint8_t)TlvType::Value;
            *b++ = l;

            b += l;
            len -= l;
        }

        // adjust length in PDU header
        _len(_buf.l() - 5);

        //HAP_LOG_HEX(p(), l(), "PduRsp");
    }

    PduReqBuf::PduReqBuf()
    {
        _mem = BufAlloc();
        if (_mem == NULL)
            HAP_LOG_ERR("PduReq buf allocation failure");

        //HAP_LOG_DBG("Req: mem %p %d", _mem->p(), _mem->l());
        attach(*_mem);
        //HAP_LOG_DBG("Req: buf %p %d", p(), l());
        init();
    }

    PduReqBuf::~PduReqBuf()
    {
        if (_mem != nullptr)
            BufFree(_mem);
    }

    PduRspBuf::PduRspBuf()
    {
        _mem = BufAlloc();
        if (_mem == NULL)
            HAP_LOG_ERR("PduRsp buf allocation failure");

        //HAP_LOG_DBG("Rsp: mem %p %d", _mem->p(), _mem->l());
        attach(*_mem);
        //HAP_LOG_DBG("Rsp: buf %p %d", p(), l());
        init(0);
    }

    PduRspBuf::~PduRspBuf()
    {
        if (_mem != nullptr)
            BufFree(_mem);
    }

}
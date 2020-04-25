#ifndef _HAP_BLE_PDU_H_
#define _HAP_BLE_PDU_H_

#include "Platform.h"

#include "Hap.h"

namespace Hap::Ble
{
    class Pdu   // common functionality for all PDU types
    {
    protected:
        Buf _buf;   // reference to external memory buffer
        Buf _body;  // reference to PDU body

        // reference to PDU byte
        uint8_t& _b(uint16_t off)
        {
            return *_buf.p(off);
        }

        // control byte ref
        uint8_t& _ctl()
        { 
            return _b(0);
        }

    public:
        enum class PduType : uint8_t
        {
            Request = 0,
            Response = 1,
            Invalid = 0xFF
        };

        Pdu()
        {}

        Pdu(const Buf& buf)
        : _buf(buf)
        {}

        // attach to external buffer
        void attach(const Buf& buf)
        {
            _buf = buf;
        }

        // access to PDU buffer
        Buf::T* p(Buf::S off = 0)
        {
            return _buf.p(off);
        }

        Buf::S l()
        {
            return _buf.l();
        }

        // get PDU type
        PduType pdu_type()
        {
            PduType type = PduType(FLD_V(_ctl(), 1, 3));
            switch (type)
            {
            case PduType::Request:
            case PduType::Response:
                return type;
            default:
                break;
            }
            return PduType::Invalid;
        }

        // true if this is continuation PDU
        bool pdu_is_cont()
        {
            return (_ctl() & BIT_(7)) != 0;
        }

        // set PDU control field
        void ctl(PduType type, bool cont = false)
        {
            _ctl() = FLD_(uint8_t(type), 1, 3)
                | (cont ? BIT_(7) : 0);
        }
        
        // return TID of continuation PDU
        uint8_t cont_tid()
        {
            return _b(1);
        }

        // return body of continuation PDU
        Buf cont_body()
        {
            uint16_t len = _buf.l() - 2;
            return Buf(_buf.p(2), len, len);
        }
    };

    // request PDU
    class PduReq : public Pdu
    {
        // opcode ref
        uint8_t& _opc()
        { 
            return _b(1);
        }

        // tid ref
        uint8_t& _tid()
        { 
            return _b(2);
        }

        // id get/set
        uint16_t _id()
        {
            uint16_t id = _b(4);
            id <<= 8;
            id += _b(3);
            return id;
        }
        void _id(uint16_t id)
        {
            _b(3) = id & 0xFF;
            _b(4) = (id >> 8) & 0xFF;
        }

        // body length get/set
        uint16_t _len()
        {
            if (_buf.l() < 7)
                return 0;
            uint16_t len = _b(6);
            len <<= 8;
            len += _b(5);
            return len;
        }
        void _len(uint16_t len)
        {
            if (_buf.l() < 7)
                return;
            _b(5) = len & 0xFF;
            _b(6) = (len >> 8) & 0xFF;
        }

    public:
        // PDU opcodes (R2: 7.3.3.2)
        enum class Opcode : uint8_t
        {
            CharSignatureRead = 0x01,
            CharWrite = 0x02,
            CharRead = 0x03,
            CharTimedWrite = 0x04,
            CharExecuteWrite = 0x05,
            SvcSignatureRead = 0x06,
            CharConfiguration = 0x07,
            ProtoConfiguration = 0x08
        };

        PduReq()
        {}

        PduReq(const Buf& buf)
        : Pdu(buf)
        {}

        // access PDU elements
        Opcode opcode()
        {
            return Opcode(_opc());
        }

        uint8_t TID()
        {
            return _tid();
        }

        uint16_t ID()
        {
            return _id();
        }

        Buf& body()
        {
            if (_buf.l() >= 7)
            {
                _body = Buf(_buf.p(7), _buf.l() - 7, _buf.l() - 7);
            }
            else
            {
                _body = Buf();
            }
            return _body;
        }

        // init - destroy content
        void init()
        {
            _buf.l(0);
        }

        // copy - copy buf content into PDU body
        void copy(const Buf& buf)
        {
            memcpy(_buf.p(7), buf.p(), buf.l());
            _buf.l(buf.l() + 7);
            _len(buf.l());
        }

        // true if PDU is complete - all body fragments are received
        bool is_complete()
        {
            return (_len() == 0 || _buf.l() >= _len() + 7);
        }

        // append PDU fragment received from transport
        bool append(const Buf& frag);

        //  convert sequence of value TLVs in PDU body to contiguous value data
        //  update retResp if RetResp TLV is present
        //  find and extract TTL if ttl i not NULL
        bool extractValue(bool* retResp, uint8_t* ttl);
    };

    // response PDU
    class PduRsp : public Pdu
    {
        // tid ref
        uint8_t& _tid()
        { 
            return *_buf.p(1);
        }

        // sts ref
        uint8_t& _sts()
        { 
            return *_buf.p(2);
        }

        // body size
        uint16_t _size()
        {
            return _buf.s() - 5;
        }

        // body length get/set
        uint16_t _len()
        {
            if (_buf.l() < 5)
                return 0;
            uint16_t len = *_buf.p(4);
            len <<= 8;
            len += *_buf.p(3);
            return len;
        }
        void _len(uint16_t len)
        {
            if (_buf.l() < 5)
                return;
            *_buf.p(3) = len & 0xFF;
            *_buf.p(4) = (len >> 8) & 0xFF;
        }

    public:
        PduRsp()
        {}

        PduRsp(const Buf& buf)
        : Pdu(buf)
        {}

        PduRsp(void* mem, Buf::L len)
        : Pdu(Buf(mem, len, len))
        {} 

        // Access PDU elements

        void TID(uint8_t tid)
        {
            _tid() = tid;
        }

        void Sts(Hap::Status sts)
        {
            _sts() = (uint8_t)sts;
        }

        // get ref to body
        Buf& body()
        {
            return _body;
        }

        // init - init header, destroy content
        void init(uint8_t tid, bool nodata = false);

        // append TLV to value data
        void append(const Tlv& tlv);

        //  convert contiguous value data to sequence of Value TLVs
        //  adjust body length (including PDU header) as necessary
        void formatValue();
    };

    // PDU with associated buffer
    class PduReqBuf : public PduReq
    {
    private:
        Buf* _mem = nullptr;
    public:
		PduReqBuf();
		~PduReqBuf();
    };

    class PduRspBuf : public PduRsp
    {
    private:
        Buf* _mem = nullptr;
    public:
		PduRspBuf();
		~PduRspBuf();
    };
}

#endif
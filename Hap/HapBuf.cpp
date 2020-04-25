#define HAP_LOG_REGISTER hapBuf
#include "Platform.h"

#include "HapBuf.h"
#include "HapTlv.h"

namespace Hap
{
    // append (copy) data to buffer memory
    bool Buf::append(const void* p, L l)
    {
        //HAP_LOG_DBG("append %p %d %d <- %p %d", _p, _l, _s, p, l);

        if (_l + l > _s)
            return false;

        memmove(_p + _l, p, l);
        _l += l;
        return true;
    }

    // append another Buffer
    bool Buf::append(const Buf& buf)
    {
        return append(buf.p(), buf.l());
    }

    // append TLV prefix and data
    bool Buf::append(const Tlv& tlv, bool copy)
    {
        if (_l + tlv.s() > _s)
            return false;

        uint8_t* p = _p + _l;

        *p++ = tlv.t();
        *p++ = tlv.l();
        if (copy)
            memmove(p, tlv.p(), tlv.l());

        _l += tlv.s();
        return true;
    }

    Tmp::Tmp()
    {
        _mem = BufAlloc();
        if (_mem == NULL)
            HAP_LOG_ERR("Tmp buf allocation failure");

        //HAP_LOG_DBG("Tmp: mem %p %d", _mem->p(), _mem->l());
        attach(*_mem);
        //HAP_LOG_DBG("Tmp: buf %p %d", p(), l());
    }

    Tmp::~Tmp()
    {
        if (_mem != nullptr)
            BufFree(_mem);
    }
}
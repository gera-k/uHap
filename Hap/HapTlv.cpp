/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#define HAP_LOG_REGISTER hapTlv
#include "Platform.h"

#include "HapTlv.h"
#include "HapBuf.h"

namespace Hap
{
    // parse buffer and count number of TPs in it
    uint8_t Tlv::Parse(Buf& buf)
    {
        uint8_t cnt = 0;
        uint8_t* b = buf.p();
        uint16_t l = buf.l();
        while (l > 0)
        {
            Tlv tlv(b, l);
            if (!tlv.valid())
            {
                LOG_ERR("Tlv: PDU parse error");
                return 0;
            }

            LOG_INF("Tlv: Type %02X  len %d", tlv.t(), tlv.l());

            b += tlv.s();
            l -= tlv.s();
            cnt++;
        }

        return cnt;
    }

    // find and return Tlv of requested type
    Tlv Tlv::Get(Buf& buf, uint8_t type)
    {
        uint8_t* b = buf.p();
        uint16_t l = buf.l();

        //HAP_LOG_DBG("Get type %d:  b %p  l %d", type, b, l); 

        while (l > 0)
        {
            Tlv tlv(b, l);
            if (!tlv.valid())
            {
                LOG_ERR("Tlv: PDU parse error");
                break;
            }

            //HAP_LOG_DBG("Get: t %d  l %d  s %d", tlv.t(), tlv.l(), tlv.s());

            if (tlv.t() == type)
                return tlv;

            b += tlv.s();
            l -= tlv.s();
        }
        //HAP_LOG_DBG("Get: no %d TLV found", type);
        return Tlv();
    }


    // get multi-TLV data and copy into dst_buf
    Tlv::S Tlv::Get(Buf& buf, uint8_t type, T* dst_buf, S dst_size)
    {
        S len = 0;
        uint8_t* b = buf.p();
        uint16_t l = buf.l();
        bool copy = false;
        while (l > 0)
        {
            Tlv tlv(b, l);
            if (!tlv.valid())
            {
                LOG_ERR("Tlv: PDU parse error");
                break;
            }

            if (!copy)
            {
                copy = tlv.t() == type;
            }
            else if (tlv.t() != type)
            {
                    break;
            }

            if (copy)
            {
                uint16_t bl = tlv.l();
                if (bl > dst_size)
                    bl = dst_size;
                memcpy(dst_buf, tlv.p(), bl);

                // move dest buffer position
                dst_buf += bl;
                dst_size -= bl;
                len += bl;
            }

            // 
            b += tlv.s();
            l -= tlv.s();
        }

        return len;
    }

    // append src_buf as multi-TLV sequence if necessary
    Tlv::S Tlv::Append(Buf& buf, uint8_t type, const T* src_buf, S src_size)
    {
        S len = 0;

        while (src_size > 0)
        {
            uint8_t l = src_size > 255 ? 255 : src_size;

            if (!buf.append(&type, 1))
                break;
            if (!buf.append(&l,1))
                break;
            if (!buf.append(src_buf, l))
                break;

            src_buf += l;
            src_size -= l;
            len += l;
        }

        HAP_LOG_DBG("Tlv::Append len %d, buf len %d", len, buf.l());

        return len;
    }


}
#ifndef _HAP_BLE_SESSION_H_
#define _HAP_BLE_SESSION_H_

#include "Platform.h"

#include "HapBle.h"

namespace Hap::Ble
{
    struct Session;

    using asyncHandler = Hap::Status(Hap::Operation*);

    struct Procedure : public Hap::Operation
    {
        STAILQ_ENTRY(Procedure) entry;      // entry in list of active procedures

        Hap::Characteristic* ch = nullptr;  // characteristic this procedure is active on
                                    // There shall be only one outstanding procedure on a char (R2: 7.3.1)
        Time::T timeout;            // procedure expiration timeout - procedute must complete in 10 sec.
        
        PduReqBuf req;              // request PDU
        PduRspBuf rsp;              // response PDU
        Tmp tmp;        	        // temp buffer for encode/decode

        Procedure(Hap::Characteristic* ch_)
        : ch(ch_)
        {}

        ~Procedure()
        {}

        void Process();

    private:
        Hap::Status charSignatureRead();
        Hap::Status charWrite();
        Hap::Status charRead();
        Hap::Status charTimedWrite();
        Hap::Status charExecuteWrite();
        Hap::Status svcSignatureRead();
        Hap::Status charConfiguration();
        Hap::Status protoConfiguration();
    };

    // Proc memory - platform defined
    Procedure* ProcAlloc(Hap::Characteristic* ch);
    void ProcFree(Procedure*);

    struct Session : public Hap::Session
    {
        STAILQ_HEAD(_proc_list, Procedure) proc_list;

        void* conn;                 // platform-dependent connection context
        Hap::Time::T lastActivity;  // timestamp of last activity on the session

        Session(void* conn_)
        : conn(conn_)
        {
            STAILQ_INIT(&proc_list);

            lastActivity = Hap::Time::stampMs();
        }

        ~Session()
        {
            Stop();
        }

        void procAdd(Procedure* proc)
        {
            STAILQ_INSERT_TAIL(&proc_list, proc, entry);
            proc->sess = this;
        }

        void procDel(Procedure* proc)
        {
            proc->sess = nullptr;
            STAILQ_REMOVE(&proc_list, proc, Procedure, entry);
        }

        Procedure* procGet(Hap::Characteristic* ch)
        {
            Procedure* proc;
            STAILQ_FOREACH(proc, &proc_list, entry)
            {
                if (proc->ch == ch)
                    return proc;
            }

            return nullptr;
        }

        // stop session - drop security status and terminate procedures
        void Stop();

        // read/write to Value property
        ssize_t Read(Hap::Property* prop, void *buf, u16_t len, u16_t offset);
        ssize_t Write(Hap::Property* prop, const void *buf, u16_t len, u16_t offset);

        // Decode or copy input data into proc->tmp
        bool Decode(Procedure* proc, const void *buf, u16_t len);

        // Encode rsp 
        bool Encode(Procedure* proc);
    };

    // Session memory - platform defined
    Session* SessAlloc(void* conn);
    void SessFree(Session*);

}

#endif /* _HAP_BLE_SESSION_H_ */
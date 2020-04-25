#include <logging/log.h>
LOG_MODULE_REGISTER(appMem, LOG_LEVEL_DBG);

#include "App.h"

#include <new>
#include "HapBle.h"

// platform-dependent memory allocators

#define HAP_SESS_MAX CONFIG_BT_MAX_CONN
#define HAP_PROC_MAX (HAP_SESS_MAX * 4)
#define HAP_BUF_MAX (HAP_PROC_MAX * 4)

using HapMem = Hap::Mem<600>;
#define HAP_BUF_SIZE sizeof(HapMem)

K_MEM_SLAB_DEFINE(hap_buf_slab, HAP_BUF_SIZE, HAP_BUF_MAX, 4);
K_MEM_SLAB_DEFINE(hap_proc_slab, sizeof(Hap::Ble::Procedure), HAP_PROC_MAX, 4);
K_MEM_SLAB_DEFINE(hap_sess_slab, sizeof(Hap::Ble::Session), HAP_SESS_MAX, 4);

namespace Hap
{
	// Buffers
	Buf* BufAlloc()
	{
		void* mem;
		k_mem_slab_alloc(&hap_buf_slab, &mem, K_NO_WAIT);

		if (mem != NULL)
		{
			HapMem* buf = new(mem) HapMem;
			//LOG_DBG("BufAlloc %p  size %d", buf, buf->s());
			return buf;
		}

		return nullptr;
	}
	
	void BufFree(Buf* buf)
	{
		void* mem = buf;
		if (mem != nullptr)
		{
			buf->~Buf();
			//LOG_DBG("BufFree %p  used %d", buf, k_mem_slab_num_used_get(&hap_buf_slab));
			k_mem_slab_free(&hap_buf_slab, &mem);
		}
	}
}

namespace Hap::Ble
{
	// Proc memory
    Procedure* ProcAlloc(Hap::Characteristic* ch)
	{
		void* mem;
		k_mem_slab_alloc(&hap_proc_slab, &mem, K_NO_WAIT);

		if (mem != NULL)
		{
			Procedure* proc = new(mem) Procedure(ch);

			//LOG_DBG("ProcAlloc %p", proc);

			return proc;
		}

		LOG_ERR("Proc allocation error");
		return nullptr;
	}

    void ProcFree(Procedure* proc)
	{
		void* mem = proc;
		if (mem != nullptr)
		{
			proc->~Procedure();
			//LOG_DBG("ProcFree %p, used %d", proc, k_mem_slab_num_used_get(&hap_proc_slab));
			k_mem_slab_free(&hap_proc_slab, &mem);
		}
	}

    // Session memory - platform defined
    Session* SessAlloc(void* conn)
	{
		void* mem;
		k_mem_slab_alloc(&hap_sess_slab, &mem, K_NO_WAIT);

		if (mem != NULL)
		{
			Session* sess = new(mem) Session(conn);

			//LOG_DBG("SessAlloc %p", sess);

			return sess;
		}

		LOG_ERR("Session allocation error");
		return NULL;
	}

    void SessFree(Session* sess)
	{
		void* mem = sess;
		if (mem != nullptr)
		{
			//LOG_DBG("SessFree %p, used %d", sess, k_mem_slab_num_used_get(&hap_sess_slab));

			k_mem_slab_free(&hap_sess_slab, &mem);
		}
	}
}


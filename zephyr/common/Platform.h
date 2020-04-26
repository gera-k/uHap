/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

// Platform definitions for Zephyr OS

#include <logging/log.h>
#if defined(HAP_LOG_REGISTER)
LOG_MODULE_REGISTER(HAP_LOG_REGISTER, LOG_LEVEL_DBG);
#elif defined(HAP_LOG_DECLARE) 
LOG_MODULE_DECLARE(HAP_LOG_DECLARE, LOG_LEVEL_DBG);
#endif

#define HAP_LOG_ERR(...)  Z_LOG(LOG_LEVEL_ERR, __VA_ARGS__)
#define HAP_LOG_WRN(...)  Z_LOG(LOG_LEVEL_WRN, __VA_ARGS__)
#define HAP_LOG_INF(...)  Z_LOG(LOG_LEVEL_INF, __VA_ARGS__)
#define HAP_LOG_DBG(...)  Z_LOG(LOG_LEVEL_DBG, __VA_ARGS__)
#define HAP_LOG_HEX(_data, _length, _str) Z_LOG_HEXDUMP(LOG_LEVEL_DBG, _data, _length, _str)

//#define HAP_PRINTF(f,...) printk(f "\n", ##__VA_ARGS__)
#define HAP_PRINTF(...) Z_LOG(LOG_LEVEL_DBG, __VA_ARGS__); k_sleep(100)

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "queue.h"

#include <type_traits>
#include <functional>

#define BIT_(n) (1<<(n))                        // bit mask
#define MSK_(n,s) (((1<<(s))-1)<<(n))           // field mask (starts at bit n, size s bits)
#define FLD_(n,s,v) ((((1<<(s))-1)&(v))<<(n))   // field value set (starts at bit n, size s bits, value v)
#define FLD_V(r,n,s) (((r)&MSK_((n),(s)))>>(n)) // field value get (from reg value r, starts at bit n, size s bits)

#define BLE_MTU 128

namespace Hap
{
	struct Characteristic;
	struct Operation;
	struct Session;
	struct Controller;

	// request disconnect of the session
	void Disconnect(const Hap::Session* sess, uint32_t delay = 500);
	
	// request disconnect of all sessions to controller
	void Disconnect(const Controller* ctlr, uint32_t delay = 500);

	// platform-devendent event notification context
	//	included into each characteristic
	struct EventCtx
	{
		struct _bt_gatt_ccc _ccc;
		bt_gatt_indicate_params params;
	};
	void EventIndicate(Characteristic* ch);

	// platform-dependent timestamp
	namespace Time
	{
		using T = s64_t;

		static inline T stampMs()	// current system time in ms
		{
			return k_uptime_get();
		}
	}
}

#include "HapBuf.h"

using AsyncOp = void(Hap::Operation*, Hap::Buf&, Hap::Buf&);
struct Async	// async operation context
{
	k_work work;
	Hap::Operation* op;
	AsyncOp* asyncOp;
	Hap::Buf* req;
	Hap::Buf* rsp;
	atomic_val_t busy;
	struct k_sem sem;

	void init(Hap::Operation* op);
	void exec(AsyncOp asyncOp, Hap::Buf& req, Hap::Buf& rsp);
	bool wait();
	void release();
};

struct Uuid : public bt_uuid_128
{
	Uuid()
    : bt_uuid_128{ {BT_UUID_TYPE_128}, {0}}
	{
    }
	
    Uuid(uint32_t w32, uint16_t w1, uint16_t w2, uint16_t w3, uint64_t w48)
	: bt_uuid_128{ {BT_UUID_TYPE_128}, {
		uint8_t(w48 >>  0),
		uint8_t(w48 >>  8),
		uint8_t(w48 >> 16),
		uint8_t(w48 >> 24),
		uint8_t(w48 >> 32),
		uint8_t(w48 >> 40),
		uint8_t(w3  >>  0),
		uint8_t(w3  >>  8),
		uint8_t(w2  >>  0),
		uint8_t(w2  >>  8),
		uint8_t(w1  >>  0),
		uint8_t(w1  >>  8),
		uint8_t(w32 >>  0),
		uint8_t(w32 >>  8),
		uint8_t(w32 >> 16),
		uint8_t(w32 >> 24)
	}}
	{
	}

    const uint8_t* data() const
    {
        return val;
    }

    uint8_t* data()
    {
        return val;
    }

    static constexpr uint8_t size()
    { 
        return sizeof(val);
    }
};

enum BleError : ssize_t
{
	BleErrorAuthentication = BT_GATT_ERR(BT_ATT_ERR_AUTHENTICATION),
	BleErrorInvalidOffset = BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET),
	BleErrorUnlikely = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY),
};

// random number generator
namespace Crypto
{
	static inline void rnd_init()
	{
	}

	static inline void rnd_data(unsigned char* data, unsigned size)
	{
		sys_rand_get(data, size);
	}
}

namespace Timer
{
	using Point = u32_t;
	using DurMs = u32_t;

	static inline Point now()
	{
		return k_uptime_get_32();
	}

	static inline DurMs ms(Point t1, Point t2)
	{
		return t2 - t1;
	}
}
#define TIMER_FORMAT "%d"

#define sizeofarr(a) ARRAY_SIZE(a)

#endif /*_PLATFORM_H_*/
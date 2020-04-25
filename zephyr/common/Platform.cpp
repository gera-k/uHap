#include <logging/log.h>
LOG_MODULE_REGISTER(appPlatform, LOG_LEVEL_DBG);

#include "App.h"

// Platfom API for HAP
namespace Hap
{
    // TODO: support both IP/BLE
   	void Disconnect(const Hap::Session* sess, uint32_t delay)
    {
        hap_ble_disconnect(sess, delay);
    }

    // TODO: support both IP/BLE
    void Disconnect(const Controller* ctlr, uint32_t delay)
    {
        hap_ble_disconnect(ctlr, delay);
    }

	// called from Characteristic to generate platform-dependent event indication
    // TODO: support both IP/BLE
	void EventIndicate(Characteristic* ch)
	{
		if (hap_ble_is_connected())
		{
			// connected event
			if (ch->perm() & Perm::ev)
			{
				// - connected state only
				// - increment GSN on first change during connected state
				hap_ble_gsn_update();
				// - send to all controllers that have indication enabled through CCCD
				int rc = bt_gatt_indicate(NULL, &ch->eventCtx.params);
				LOG_DBG("bt_gatt_indicate: %s  rc %d", ch->name, rc);

				// TODO:
				// - do not send to the controller that wrote to the char (do send to all other)
			}
		}
		else
		{
			// broadcast events
			if (ch->perm() & Perm::bn && ch->bc_event)
			{
				// - update GSN on every char value change, update adv payload
				hap_ble_gsn_update(true);

				// request switch to broadcast notifications
				// - broadcast for at least 3 sec
				// - if controler connects within 3 sec, abort broadcasts
				// - if controller does not connect within 3 sec, stop broadcasts, fallback to disconnected events
				// - if char change occurs within 3 sec, update GSN and restart 3 sec counter
				hap_adv_broadcast(ch);
			}

			// disconnected events are always enabled when supported
			else if (ch->perm() & Perm::de)
			{
				// - increment GSN on first change during disconnected state
				hap_ble_gsn_update();
			}
		}
	}
}


// Async operations
static void async_work_handler(struct k_work* work);

void Async::init(Hap::Operation* op_)
{
	op = op_;
	atomic_set(&busy, 0);
	k_sem_init(&sem, 0, 1);
}

void Async::exec(AsyncOp* asyncOp_, Hap::Buf& req_, Hap::Buf& rsp_)
{
	k_work_init(&work, async_work_handler);
	atomic_set_bit(&busy, 0);
	asyncOp = asyncOp_;
	req = &req_;
	rsp = &rsp_;

	k_work_submit(&work);
}

bool Async::wait()
{
	if (atomic_test_and_clear_bit(&busy, 0))
		return k_sem_take(&sem, 10000) == 0;

	return true;
}

void Async::release()
{
	k_sem_give(&sem);
}

static void async_work_handler(struct k_work* work)
{
	Async* async = CONTAINER_OF(work, Async, work);

	async->asyncOp(async->op, *async->req, *async->rsp);

    // reformat PDU body for BLE
	Hap::Ble::Procedure* proc = static_cast<Hap::Ble::Procedure*>(async->op);
    proc->rsp.formatValue();

	async->release();
}

namespace std {
    void __throw_bad_function_call()
	{
		LOG_ERR("invalid function called"); 
		while(1)
			k_sleep(1000);
	}
}

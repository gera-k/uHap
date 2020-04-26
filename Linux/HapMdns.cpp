/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#include "Hap/Hap.h"

#include <thread>
#include <mutex>
#include <condition_variable>

#include "dns_sd.h"

namespace Hap
{
	class MdnsImpl : public Mdns
	{
	private:
		std::thread task;
		std::mutex mtx;
		std::condition_variable cv;
		bool running = false;
		DNSServiceRef client = NULL;

		void callback(
			DNSServiceRef sdRef,
			DNSServiceFlags flags,
			DNSServiceErrorType errorCode,
			const char *name,
			const char *regtype,
			const char *domain)
		{
			Log::Msg("Mdns callback: err %d  name '%s'  type '%s'  dom '%s'\n",
				errorCode, name, regtype, domain);
		}

		void run()
		{
			Log::Msg("Mdns::Run - enter\n");

			update();

			DNSServiceFlags flags = 0;

			DNSServiceErrorType rc = DNSServiceRegister(
				&client,				// DNSServiceRef *sdRef,
				flags,					// DNSServiceFlags flags,
				0,						// uint32_t interfaceIndex,
				_name,					// const char *name,
				"_hap._tcp",			// const char *regtype,
				"local",				// const char *domain,
				NULL,					// const char *host,
				Hap::config->port,		// uint16_t port,
				_txt_len,				// uint16_t txtLen,
				_txt,					// const void *txtRecord,
				[]						// DNSServiceRegisterReply callBack,
			(
				DNSServiceRef sdRef,
				DNSServiceFlags flags,
				DNSServiceErrorType errorCode,
				const char *name,
				const char *regtype,
				const char *domain,
				void *context
				) -> void
				{
					((MdnsImpl*)context)->callback(sdRef, flags, errorCode, name, regtype, domain);
				},
				this					//void *context
			);

			if (rc != kDNSServiceErr_NoError)
			{
				Log::Msg("DNSServiceRegister error %d\n", rc);
			}
			else
			{
				Log::Msg("MDNS Service '%s' registered\n", _name);

				std::unique_lock<std::mutex> lock(mtx);
				cv.wait(lock, [this] {return !running; });
			}

			if (client)
			{
				DNSServiceRefDeallocate(client);
				Log::Msg("MDNS Service '%s' unregistered\n", _name);
			}
			client = NULL;

			Log::Msg("Mdns::Run - exit\n");
		}

	public:
		~MdnsImpl()
		{
			Stop();
		}

		virtual void Start() override
		{
			running = true;
			task = std::thread(&MdnsImpl::run, this);
		}

		virtual void Stop() override
		{
			running = false;
			cv.notify_one();
			if (task.joinable())
				task.join();
		}

		virtual void Update() override
		{
			update();

			Log::Msg("update: client %p", client);

			if (!client)
				return;

			DNSServiceFlags flags = 0;

			DNSServiceErrorType rc = DNSServiceUpdateRecord(
				client,				// DNSServiceRef sdRef,
				NULL,				// DNSRecordRef RecordRef,
				flags,				// DNSServiceFlags flags,
				_txt_len,			// uint16_t rdlen,
				_txt,				// const void *rdata,
				0					// uint32_t ttl
			);

			if (rc != kDNSServiceErr_NoError)
			{
				Log::Msg("DNSServiceUpdateRecord error %d\n", rc);
			}
			else
			{
				Log::Msg("MDNS Service '%s' updated\n", _name);
			}
		}
	} mdns;

	Mdns* Mdns::Create()
	{
		return &mdns;
	}
}


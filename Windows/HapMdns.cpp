/*
MIT License

Copyright (c) 2019 Gera Kazakov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Hap.h"

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
			Log::Msg("MdnsImpl::Run - enter\n");

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

				std::unique_lock lock(mtx);
				cv.wait(lock, [this] {return !running; });
			}

			if (client)
			{
				DNSServiceRefDeallocate(client);
				Log::Msg("MDNS Service '%s' unregistered\n", _name);
			}
			client = NULL;

			Log::Msg("MdnsImpl::Run - exit\n");
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

			Log::Msg("update");

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
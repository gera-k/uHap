/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _HAP_TCP_H_
#define _HAP_TCP_H_

// Interface to system-dependent TCP server implementation

namespace Hap
{
	class Tcp
	{
	protected:
		Hap::Http::Server* _http;
	public:
		static Tcp* Create(Hap::Http::Server* _http);
		virtual bool Start() = 0;
		virtual void Stop() = 0;
	};
}

#endif

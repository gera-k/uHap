/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _LOG_H_
#define _LOG_H_

#include "Platform.h"

namespace Log
{
	extern bool Debug;
	extern bool Info;
	extern bool Console;

	void Init(const char* file = NULL);
	void Exit();

	void Err(const char* f, ...);
	void Msg(const char* f, ...);
	void Dbg(const char* f, ...);
	void Hex(const char* Header, const void* Buffer, uint32_t Length);
}

#endif


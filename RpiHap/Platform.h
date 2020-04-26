/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <iostream>
#include <chrono>

#include "Util/FileLog.h"

#define sizeofarr(arr) (sizeof(arr) / sizeof((arr)[0]))

#define LOG_MSG(...) Log::Msg(__VA_ARGS__)

static inline uint16_t swap_16(uint16_t v)
{
	return (uint16_t)(((v & 0xFF) << 8) | (v >> 8));
}

// random number generator
namespace Crypto
{
	static inline void rnd_init()
	{
		srand((unsigned)time(NULL));
		//srand(0);
	}

	static inline void rnd_data(unsigned char* data, unsigned size)
	{
		for (unsigned i = 0; i < size; i++)
			*data++ = (uint8_t)(rand() & 0xFF);
	}
}

namespace Timer
{
	using Point = std::chrono::high_resolution_clock::time_point;
	using DurMs = std::chrono::milliseconds::rep;

	static inline Point now()
	{
		return std::chrono::high_resolution_clock::now();
	}

	static inline DurMs ms(Point t1, Point t2)
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
	}
}


#endif /*_PLATFORM_H_*/

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

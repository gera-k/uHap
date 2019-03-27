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

#include "FileLog.h"

namespace Log
{
	bool Console = false;
	bool Debug = false;
	bool Info = false;
	FILE* Log = NULL;

	void hex(const char* Header, const void* Buffer, uint32_t Length);

	void Init(const char* file)
	{
		if (file)
			Log = ::fopen(file, "w");
	}

	void Exit()
	{
		if (Log)
			::fclose(Log);
	}

	void log(const char* f, va_list arg)
	{
		if (Console)
			vprintf(f, arg);
		
		if (Log)
		{
			vfprintf(Log, f, arg);
			fflush(Log);
		}
	}

	void Err(const char* f, ...)
	{
		va_list arg;
		va_start(arg, f);
		log(f, arg);
	}

	void Msg(const char* f, ...)
	{
		va_list arg;

		if (!Info)
			return;

		va_start(arg, f);
		log(f, arg);
	}

	void Dbg(const char* f, ...)
	{
		va_list arg;

		if (!Debug)
			return;

		va_start(arg, f);
		log(f, arg);
	}


	void Hex(const char* Header, const void* Buffer, uint32_t Length)
	{
		if (!Debug)
			return;

		hex(Header, Buffer, Length);
	}

	void hex(const char* Header, const void* Buffer, uint32_t Length)
	{
		static const char hex[] = "0123456789ABCDEF";
		const uint8_t* a = (const uint8_t*)Buffer;
		uint32_t i;
		uint32_t max = 16;

		Msg("%s addr %p size 0x%X(%d):\n", Header, Buffer, (unsigned)Length, (unsigned)Length);

		while (Length > 0)
		{
			char line[52];
			char *p = line;

			if (Length < max)
				max = Length;

			memset(line, 0, sizeof(line));

			for (i = 0; i < 16; i++)
			{
				if (i < max)
				{
					*p++ = hex[(a[i] & 0xf0) >> 4];
					*p++ = hex[a[i] & 0x0f];
				}
				else
				{
					*p++ = ' ';
					*p++ = ' ';
				}
			}

			*p++ = ' ';
			*p++ = ' ';
			*p++ = ' ';

			for (i = 0; i < max; i++)
			{
				if (a[i] < 0x20 || a[i] > 0x7e) *p++ = '.';
				else *p++ = a[i];
			}

			Msg("0x%04lX:%s\n", (intptr_t)a & 0xFFFF, line);

			Length -= max;
			a += max;
		}
	}
}

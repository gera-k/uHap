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

namespace Hap
{
	const char* StatusStr(Status c)
	{
		static const char* const str[] =
		{
			"0",
			"-70401",
			"-70402",
			"-70403",
			"-70404",
			"-70405",
			"-70406",
			"-70407",
			"-70408",
			"-70409",
			"-70410",
			"-70411",
		};
		return str[int(c)];
	}

	const char* UnitStr(UnitId c)
	{
		static const char* const str[] =
		{
			"celsius",
			"percentage",
			"arcdegrees",
			"lux",
			"seconds"
		};
		return str[int(c)];
	}

	const char* KeyStr(KeyId c)
	{
		const char* const str[] =
		{
			"aid",
			"services",

			"type",
			"iid",
			"characteristics",
			"hidden",
			"primary",
			"linked",

			"value",
			"perms",
			"ev",
			"description",
			"format",
			"unit",
			"minValue",
			"maxValue",
			"minStep",
			"maxLen",
			"maxDataLen",
			"valid-values",
			"valid-values-range"
		};
		return str[int(c)];
	};

	const char* FormatStr(FormatId c)
	{
		const char* const str[] =
		{
			"null",
			"bool",
			"uint8",
			"uint16",
			"uint32",
			"uint64",
			"int",
			"float",
			"string",
			nullptr,
			nullptr,
			"string",
			"tlv8",
			"data",
			nullptr,
			nullptr,
			nullptr
		};
		return str[int(c)];
	}
}
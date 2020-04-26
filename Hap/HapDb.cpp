/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
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
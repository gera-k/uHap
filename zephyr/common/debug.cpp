/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#include <logging/log.h>
LOG_MODULE_REGISTER(appDebug, LOG_LEVEL_DBG);

#include "App.h"

struct dump_ctx
{
	char* s;
	int l;
};
static char dump_buf[64];
static dump_ctx _dump_ctx {dump_buf, sizeof(dump_buf)};

void dump(Hap::Property* pr, dump_ctx* ctx)
{
	snprintf(ctx->s, ctx->l, "    Pr %p  %s:%s - %p %d %d",
		pr,  Hap::TypeToStr(pr->type), Hap::FormatToStr(pr->fmt), pr->value.p(), pr->value.s(), pr->value.l());
	LOG_DBG("%s", log_strdup(ctx->s));
}

void dump(Hap::Characteristic* ch, dump_ctx* ctx)
{
	bt_uuid_to_str(&CONTAINER_OF(ch->uuid(), bt_uuid_128, val)->uuid, ctx->s, ctx->l);
	LOG_DBG("  Ch %p  iid %d  type %s", ch,  ch->iid(), log_strdup(ctx->s));

	ch->forEachProp([&ctx](Hap::Property* pr)
	{
		dump(pr, ctx);
	});
}

void dump(Hap::Service* svc, dump_ctx* ctx)
{
	bt_uuid_to_str(&CONTAINER_OF(svc->uuid(), bt_uuid_128, val)->uuid, ctx->s, ctx->l);
	LOG_DBG("Svc %p  iid %d  type %s", svc,  svc->iid(), log_strdup(ctx->s));

	svc->forEachChar([&ctx](Hap::Characteristic* ch)
	{
		dump(ch, ctx);
	});
}

void dump(Hap::Service* svc)
{
	dump(svc, &_dump_ctx);
}

void dump_gatt(bt_gatt_attr* attr, uint32_t cnt)
{
	for (uint32_t i = 0; i < cnt; i++)
	{
		char s[64];

		LOG_DBG("attr[%2d] uuid %p, user_data %p", i, attr->uuid, attr->user_data);
		if (attr->uuid != NULL)
		{
			bt_uuid_to_str(attr->uuid, s, sizeof(s));
			LOG_DBG("          %s", log_strdup(s));    
		}

		attr++;
	}
}


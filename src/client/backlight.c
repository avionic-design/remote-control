/*
 * Copyright (C) 2010-2011 Avionic Design GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "remote-control.h"

int32_t medcom_backlight_enable(void *priv, uint32_t flags)
{
	struct rpc_client *client = rpc_client_from_priv(priv);
	int32_t ret;
	int err;

	err = medcom_backlight_enable_stub(client, &ret, flags);
	if (err < 0)
		return err;

	return ret;
}

int32_t medcom_backlight_get(void *priv, uint8_t *brightness)
{
	struct rpc_client *client = rpc_client_from_priv(priv);
	int32_t ret;
	int err;

	err = medcom_backlight_get_stub(client, &ret, brightness);
	if (err < 0)
		return err;

	return ret;
}

int32_t medcom_backlight_set(void *priv, uint8_t brightness)
{
	struct rpc_client *client = rpc_client_from_priv(priv);
	int32_t ret;
	int err;

	err = medcom_backlight_set_stub(client, &ret, brightness);
	if (err < 0)
		return err;

	return ret;
}

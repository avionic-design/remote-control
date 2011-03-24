/*
 * Copyright (C) 2010-2011 Avionic Design GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>

#include "remote-control-stub.h"
#include "remote-control.h"

remote_public
int remote_media_player_start(void *priv)
{
	struct rpc_client *rpc = rpc_client_from_priv(priv);
	int32_t ret;
	int err;

	err = RPC_STUB(media_player_start)(rpc, &ret);
	if (err < 0)
		return err;

	return ret;
}

remote_public
int remote_media_player_stop(void *priv)
{
	struct rpc_client *rpc = rpc_client_from_priv(priv);
	int32_t ret;
	int err;

	err = RPC_STUB(media_player_stop)(rpc, &ret);
	if (err < 0)
		return err;

	return ret;
}

remote_public
int remote_media_player_is_running(void *priv, bool *running)
{
	struct rpc_client *rpc = rpc_client_from_priv(priv);
	int32_t ret = 0;
	int err;

	err = RPC_STUB(media_player_is_running)(rpc, &ret, running);
	if (err < 0)
		return err;

	return ret;
}

remote_public
int remote_media_player_get_stream(void *priv, char **url)
{
	struct rpc_client *rpc = rpc_client_from_priv(priv);
	int32_t ret;
	int err;

	err = RPC_STUB(media_player_get_stream)(rpc, &ret, url);
	if (err < 0)
		return err;

	return ret;
}

remote_public
int remote_media_player_set_stream(void *priv, const char *url)
{
	struct rpc_client *rpc = rpc_client_from_priv(priv);
	int32_t ret;
	int err;

	err = RPC_STUB(media_player_set_stream)(rpc, &ret, url);
	if (err < 0)
		return err;

	return ret;
}

remote_public
int remote_media_player_set_output_window(void *priv, uint16_t x, uint16_t y,
		uint16_t width, uint16_t height)
{
	struct rpc_client *rpc = rpc_client_from_priv(priv);
	int32_t ret;
	int err;

	err = RPC_STUB(media_player_set_output_window)(rpc, &ret, x, y, width,
			height);
	if (err < 0)
		return err;

	return ret;
}

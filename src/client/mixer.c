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

#include "remote-control-stub.h"
#include "remote-control.h"

remote_public
int32_t medcom_mixer_set_mute(void *priv, enum medcom_mixer_control control, bool mute)
{
	struct rpc_client *rpc = rpc_client_from_priv(priv);
	int32_t ret = 0;
	int32_t err;

	err = medcom_mixer_set_mute_stub(rpc, &ret, control, mute);
	if (err < 0)
		ret = err;

	return ret;
}

remote_public
int32_t medcom_mixer_get_mute(void *priv, enum medcom_mixer_control control, bool *mutep)
{
	struct rpc_client *rpc = rpc_client_from_priv(priv);
	int32_t ret = 0;
	int32_t err;

	err = medcom_mixer_is_muted_stub(rpc, &ret, control, mutep);
	if (err < 0)
		ret = err;

	return ret;
}

remote_public
int32_t medcom_mixer_set_volume(void *priv, enum medcom_mixer_control control, uint8_t volume)
{
	struct rpc_client *rpc = rpc_client_from_priv(priv);
	int32_t ret = 0;
	int32_t err;

	err = medcom_mixer_set_volume_stub(rpc, &ret, control, volume);
	if (err < 0)
		ret = err;

	return ret;
}

remote_public
int32_t medcom_mixer_get_volume(void *priv, enum medcom_mixer_control control, uint8_t *volumep)
{
	struct rpc_client *rpc = rpc_client_from_priv(priv);
	int32_t ret = 0;
	int32_t err;

	err = medcom_mixer_get_volume_stub(rpc, &ret, control, volumep);
	if (err < 0)
		ret = err;

	return ret;
}

remote_public
int32_t medcom_mixer_set_input_source(void *priv, enum medcom_mixer_input_source source)
{
	struct rpc_client *rpc = rpc_client_from_priv(priv);
	int32_t ret = 0;
	int32_t err;

	err = medcom_mixer_set_input_source_stub(rpc, &ret, source);
	if (err < 0)
		ret = err;

	return ret;
}

remote_public
int32_t medcom_mixer_get_input_source(void *priv, enum medcom_mixer_input_source *sourcep)
{
	struct rpc_client *rpc = rpc_client_from_priv(priv);
	int32_t ret = 0;
	int32_t err;

	err = medcom_mixer_get_input_source_stub(rpc, &ret, sourcep);
	if (err < 0)
		ret = err;

	return ret;
}

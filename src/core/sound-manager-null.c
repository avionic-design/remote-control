/*
 * Copyright (C) 2011 Avionic Design GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "remote-control.h"

int sound_manager_create(struct sound_manager **managerp, struct audio *audio,
		GKeyFile *config)
{
	return 0;
}

int sound_manager_free(struct sound_manager *manager)
{
	return 0;
}

int sound_manager_play(struct sound_manager *manager, const char *uri)
{
	return -ENOSYS;
}

int sound_manager_pause(struct sound_manager *manager)
{
	return -ENOSYS;
}

int sound_manager_stop(struct sound_manager *manager)
{
	return -ENOSYS;
}

int sound_manager_get_state(struct sound_manager *manager,
		enum sound_manager_state *statep)
{
	return -ENOSYS;
}

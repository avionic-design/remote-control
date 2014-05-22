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

#include <vlc/vlc.h>

#include "remote-control-stub.h"
#include "remote-control.h"

#define LIBVLC_AUDIO_VOLUME_MAX 200

struct sound_manager {
	libvlc_instance_t *vlc;
	libvlc_media_player_t *player;
	libvlc_event_manager_t *evman;
	libvlc_media_t *media;
	struct audio *audio;
};

#if defined(__arm__)
static void sound_manager_update_alsa_device(struct sound_manager *manager)
{
	enum audio_state state = AUDIO_STATE_INACTIVE;
	const char* device = "default";

	if (audio_get_state(manager->audio, &state) < 0) {
		g_warning("sound-manager: failed to recieve audio state");
		return;
	}

	switch (state) {
	case AUDIO_STATE_VOICECALL_HANDSET:
	case AUDIO_STATE_VOICECALL_IP_HANDSET:
		device = "handset";
		break;

	default:
		/* restore default */
		break;
	}

	libvlc_audio_output_device_set(manager->player, "alsa", device);
}
#else
#define sound_manager_update_alsa_device(x)
#endif

int sound_manager_create(struct sound_manager **managerp, struct audio *audio)
{
	const char *const argv[] = { "--file-caching", "0", NULL };
	struct sound_manager *manager;
	int argc = 2;

	if (!managerp)
		return -EINVAL;

	manager = g_malloc0(sizeof(*manager));
	if (!manager)
		return -ENOMEM;

	manager->vlc = libvlc_new(argc, argv);
	manager->player = libvlc_media_player_new(manager->vlc);
	libvlc_audio_set_volume(manager->player, LIBVLC_AUDIO_VOLUME_MAX);
	manager->audio = audio;

	*managerp = manager;
	return 0;
}

int sound_manager_free(struct sound_manager *manager)
{
	if (!manager)
		return -EINVAL;

	libvlc_media_player_release(manager->player);
	libvlc_media_release(manager->media);
	libvlc_release(manager->vlc);
	g_free(manager);
	return 0;
}

int sound_manager_play(struct sound_manager *manager, const char *uri)
{
	if (!manager || !uri)
		return -EINVAL;

	if (manager->media) {
		libvlc_media_release(manager->media);
		manager->media = NULL;
	}

	sound_manager_update_alsa_device(manager);

	manager->media = libvlc_media_new_location(manager->vlc, uri);
	libvlc_media_player_set_media(manager->player, manager->media);
	return libvlc_media_player_play(manager->player) ? -EINVAL : 0;
}

int sound_manager_pause(struct sound_manager *manager)
{
	if (!manager)
		return -EINVAL;

	libvlc_media_player_pause(manager->player);
	return 0;
}

int sound_manager_stop(struct sound_manager *manager)
{
	if (!manager)
		return -EINVAL;

	libvlc_media_player_stop(manager->player);
	if (manager->media) {
		libvlc_media_release(manager->media);
		manager->media = NULL;
	}

	return 0;
}

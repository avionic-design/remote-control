/*
 * Copyright (C) 2010-2012 Avionic Design GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <ctype.h>
#include <linphonecore.h>
#include <glib.h>

#include "remote-control-stub.h"
#include "remote-control.h"

struct voip {
	GSource *timeout;
	LinphoneCore *core;
	gchar *contact;
	int expires;
};

static void linphone_global_state_changed_cb(LinphoneCore *core,
		LinphoneGlobalState state, const char *message)
{
	const char *name = linphone_global_state_to_string(state);

	if (message)
		g_debug("voip-linphone: global state changed to %s: %s",
				name, message);
	else
		g_debug("voip-linphone: global state changed to %s", name);
}

static void linphone_registration_state_changed_cb(LinphoneCore *core,
		LinphoneProxyConfig *proxy, LinphoneRegistrationState state,
		const char *message)
{
	const char *name = linphone_registration_state_to_string(state);

	if (message)
		g_debug("voip-linphone: registration state on proxy %p "
				"changed to %s: %s", proxy, name, message);
	else
		g_debug("voip-linphone: registration state on proxy %p "
				"changed to %s", proxy, name);
}

static void linphone_call_state_changed_cb(LinphoneCore *core,
		LinphoneCall *call, LinphoneCallState state,
		const char *message)
{
	struct remote_control *rc = linphone_core_get_user_data(core);
	struct event_manager *manager = remote_control_get_event_manager(rc);
	struct voip *voip = remote_control_get_voip(rc);
	const LinphoneAddress *address;
	struct event event;
	const char *name;
	int err;

	name = linphone_call_state_to_string(state);

	if (message)
		g_debug("voip-linphone: call state changed to %s: %s", name,
				message);
	else
		g_debug("voip-linphone: call state changed to %s", name);

	memset(&event, 0, sizeof(event));
	event.source = EVENT_SOURCE_VOIP;

	switch (state) {
	case LinphoneCallIncomingReceived:
		/* reject incoming calls if a call is already active */
		if (!linphone_core_in_call(core)) {
			g_debug("voip-linphone: busy, rejecting call\n");
			linphone_core_terminate_call(core, call);
			return;
		}

		event.voip.state = EVENT_VOIP_STATE_INCOMING;

		err = event_manager_report(manager, &event);
		if (err < 0)
			g_debug("voip-linphone: failed to report event: %s",
				g_strerror(-err));

		if (voip->contact) {
			g_free(voip->contact);
			voip->contact = NULL;
		}

		address = linphone_call_get_remote_address(call);
		if (address) {
			const char *name;

			name = linphone_address_get_display_name(address);
			if (!name)
				name = linphone_address_get_username(address);

			if (name)
				voip->contact = g_strdup(name);
		}
		break;

	case LinphoneCallConnected:
		event.voip.state = EVENT_VOIP_STATE_INCOMING_CONNECTED;

		err = event_manager_report(manager, &event);
		if (err < 0)
			g_debug("voip-linphone: failed to report event: %s",
				g_strerror(-err));
		break;

	case LinphoneCallEnd:
		event.voip.state = EVENT_VOIP_STATE_INCOMING_DISCONNECTED;

		err = event_manager_report(manager, &event);
		if (err < 0)
			g_debug("voip-linphone: failed to report event: %s",
				g_strerror(-err));
		break;

	case LinphoneCallIncomingEarlyMedia:
		break;

	case LinphoneCallOutgoingEarlyMedia:
		break;

	case LinphoneCallOutgoingProgress:
		event.voip.state = EVENT_VOIP_STATE_OUTGOING;

		err = event_manager_report(manager, &event);
		if (err < 0)
			g_debug("voip-linphone: failed to report event: %s",
				g_strerror(-err));
		break;

	case LinphoneCallError:
		event.voip.state = EVENT_VOIP_STATE_OUTGOING_DISCONNECTED;

		err = event_manager_report(manager, &event);
		if (err < 0)
			g_debug("voip-linphone: failed to report event: %s",
				g_strerror(-err));
		break;

	default:
		break;
	}
}

static void linphone_notify_presence_recv_cb(LinphoneCore *core,
		LinphoneFriend *friend)
{
	g_debug("voip-linphone: presence notification for %p", friend);
}

static void linphone_new_subscription_request_cb(LinphoneCore *core,
		LinphoneFriend *friend, const char *url)
{
	g_debug("voip-linphone: subscription request from %p: %s", friend,
			url);
}

static void linphone_auth_info_requested_cb(LinphoneCore *core,
		const char *realm, const char *username)
{
	g_debug("voip-linphone: authorization info requested for %s@%s",
			username, realm);
}

static void linphone_call_log_updated_cb(LinphoneCore *core,
		LinphoneCallLog *log)
{
	g_debug("voip-linphone: call log update: %p", log);
}

static void linphone_text_received_cb(LinphoneCore *core,
		LinphoneChatRoom *room, const LinphoneAddress *from,
		const char *message)
{
	g_debug("voip-linphone: text received from %p/%p: %s", room, from,
			message);
}

static void linphone_dtmf_received_cb(LinphoneCore *core, LinphoneCall *call,
		int dtmf)
{
	g_debug("voip-linphone: dtmf received: %02x, '%c'", dtmf,
			isprint(dtmf) ? dtmf : '?');
}

static void linphone_refer_received_cb(LinphoneCore *core,
		const char *refer_to)
{
	g_debug("voip-linphone: referring call to %s", refer_to);
}

static void linphone_buddy_info_updated_cb(LinphoneCore *core,
		LinphoneFriend *friend)
{
	g_debug("voip-linphone: buddy info updated for %p", friend);
}

static void linphone_notify_recv_cb(LinphoneCore *core, LinphoneCall *call,
		const char *from, const char *event)
{
	g_debug("voip-linphone: notification received from %s: %s", from,
			event);
}

static void linphone_display_status_cb(LinphoneCore *core,
		const char *message)
{
	g_debug("voip-linphone: status: %s", message);
}

static void linphone_display_message_cb(LinphoneCore *core,
		const char *message)
{
	g_debug("voip-linphone: message: %s", message);
}

static void linphone_display_warning_cb(LinphoneCore *core,
		const char *message)
{
	g_debug("voip-linphone: warning: %s", message);
}

static void linphone_display_url_cb(LinphoneCore *core, const char *message,
		const char *url)
{
	g_debug("voip-linphone: url: %s: %s", message, url);
}

static void linphone_show_cb(LinphoneCore *core)
{
	g_debug("voip-linphone: show notification received");
}

static const LinphoneCoreVTable vtable = {
	.global_state_changed = linphone_global_state_changed_cb,
	.registration_state_changed = linphone_registration_state_changed_cb,
	.call_state_changed = linphone_call_state_changed_cb,
	.notify_presence_recv = linphone_notify_presence_recv_cb,
	.new_subscription_request = linphone_new_subscription_request_cb,
	.auth_info_requested = linphone_auth_info_requested_cb,
	.call_log_updated = linphone_call_log_updated_cb,
	.text_received = linphone_text_received_cb,
	.dtmf_received = linphone_dtmf_received_cb,
	.refer_received = linphone_refer_received_cb,
	.buddy_info_updated = linphone_buddy_info_updated_cb,
	.notify_recv = linphone_notify_recv_cb,
	.display_status = linphone_display_status_cb,
	.display_message = linphone_display_message_cb,
	.display_warning = linphone_display_warning_cb,
	.display_url = linphone_display_url_cb,
	.show = linphone_show_cb,
};

static void linphone_log(OrtpLogLevel level, const char *fmt, va_list ap)
{
	if (level >= ORTP_WARNING) {
		gchar *message = g_strdup_vprintf(fmt, ap);
		g_debug("voip-linphone: * %s", message);
		g_free(message);
	}
}

static gboolean voip_timeout(gpointer user_data)
{
	struct voip *voip = user_data;

	linphone_core_iterate(voip->core);

	return TRUE;
}

static void voip_disable_codec(struct voip *voip, const char *mime_type)
{
	const MSList *codecs;
	const MSList *node;

	codecs = linphone_core_get_audio_codecs(voip->core);

	for (node = codecs; node; node = ms_list_next(node)) {
		PayloadType *pt = node->data;

		if (g_ascii_strcasecmp(pt->mime_type, mime_type) == 0) {
			linphone_core_enable_payload_type(voip->core, pt, 0);
			g_debug("voip-linphone: disabling payload type: %s, "
				"%d Hz", pt->mime_type, pt->clock_rate);
		}
	}
}

int voip_create(struct voip **voipp, struct rpc_server *server,
		GKeyFile *config)
{
	const char *factory_config = SYSCONF_DIR "/linphone.conf";
	struct remote_control *rc = rpc_server_priv(server);
	struct voip *voip;
	gchar **codecs;
	gboolean ec;

	if (!voipp)
		return -EINVAL;

	voip = g_new0(struct voip, 1);
	if (!voip)
		return -ENOMEM;

	voip->timeout = g_timeout_source_new(50);
	if (!voip->timeout) {
		g_free(voip);
		return -ENOMEM;
	}

	g_source_set_callback(voip->timeout, voip_timeout, voip, NULL);

	linphone_core_enable_logs_with_cb(linphone_log);

	voip->core = linphone_core_new(&vtable, NULL, factory_config, rc);
	if (!voip->core) {
		g_source_unref(voip->timeout);
		g_free(voip);
		return -ENOMEM;
	}

	codecs = g_key_file_get_string_list(config, "linphone",
			"disable-codecs", NULL, NULL);
	if (codecs) {
		gchar **codec;

		for (codec = codecs; *codec; codec++)
			voip_disable_codec(voip, *codec);

		g_strfreev(codecs);
	}

	/*
	 * We don't bother checking for errors here because by default we want
	 * to turn echo cancellation off, which is the value returned by the
	 * g_key_file_get_boolean() function if the key either was not found
	 * or cannot be interpreted as a boolean.
	 */
	ec = g_key_file_get_boolean(config, "linphone", "echo-cancellation", NULL);
	g_debug("voip-linphone: echo cancellation %sabled", ec ? "en" : "dis");
	linphone_core_enable_echo_cancellation(voip->core, ec);

	/*
	 * Allow the default expiration time for SIP registrations to be
	 * overridden.
	 */
	if (g_key_file_has_key(config, "linphone", "registration-expiry", NULL)) {
		voip->expires = g_key_file_get_integer(config, "linphone",
						       "registration-expiry",
						       NULL);
		if (voip->expires != 0)
			voip->expires = CLAMP(voip->expires, 30, 3600);
	}

	voip->contact = NULL;

	/*
	 * FIXME: This code was initially meant to set the output volume to
	 * 100% but it doesn't. It actually sets the output volume gain,
	 * meaning that the volume is really amplified by about 26%. Alas,
	 * some devices have already shipped with this code, so we'll
	 * probably have to stick with it forever.
	 */
	linphone_core_set_playback_gain_db(voip->core, 1.0f);
	linphone_core_set_ring(voip->core, NULL);

	/*
	 * The keepi-alive period can be set in .linphonerc or /etc/linphone.conf.
	 * But the API does not allow to set the period directly, so we can only
	 * enable/disable keep-alive.
	 */
	if (g_key_file_has_key(config, "linphone", "keep-alive", NULL)) {
		gboolean enable = g_key_file_get_boolean(config, "linphone",
		                                         "keep-alive", NULL);
		linphone_core_enable_keep_alive(voip->core, enable);
	}

	/*
	 * FIXME: This should be removed, the configuration should be done
	 *        via linphone's own config file. And it is only possible with
	 *        a patched linphone version.
	 */
	if (g_key_file_has_key(config, "linphone", "qos-dscp", NULL)) {
		int dscp = g_key_file_get_integer(config, "linphone",
						  "qos-dscp", NULL);
		linphone_core_set_rtp_dscp(voip->core, dscp);
	}

	*voipp = voip;
	return 0;
}

int voip_free(struct voip *voip)
{
	if (!voip)
		return -EINVAL;

	linphone_core_terminate_all_calls(voip->core);
	linphone_core_destroy(voip->core);
	g_free(voip->contact);
	g_free(voip);

	return 0;
}

GSource *voip_get_source(struct voip *voip)
{
	return voip ? voip->timeout : NULL;
}

static int is_valid_string(const char* str)
{
	if (!str)
		return FALSE;

	return strlen(str) > 0;
}

int voip_login(struct voip *voip, const char *host, uint16_t port,
	       const char *username, const char *password,
	       enum voip_transport transport)
{
	LinphoneProxyConfig *proxy;
	LCSipTransports transports;
	const char *domain = host;
	LinphoneAddress *address;
	bool use_default = true;
	LinphoneAuthInfo *auth;
	char *server = NULL;
	char *user = NULL;
	char *identity;
	int len;

	if (!voip)
		return -EINVAL;

	memset(&transports, 0, sizeof(transports));

	switch (transport) {
	case VOIP_TRANSPORT_UDP:
		transports.udp_port = port;
		break;

	case VOIP_TRANSPORT_TCP:
		transports.tcp_port = port;
		break;

	case VOIP_TRANSPORT_TLS:
		transports.tls_port = port;
		break;

	default:
		return -EINVAL;
	}

	if (linphone_core_set_sip_transports(voip->core, &transports) < 0)
		g_debug("voip-linphone: failed to set transports");

	if (!port || !is_valid_string(host) || !is_valid_string(username) ||
	    !is_valid_string(password))
	{
		linphone_core_get_default_proxy(voip->core, &proxy);
		if (proxy) {
			if (!linphone_proxy_config_is_registered(proxy)) {
				linphone_proxy_config_edit(proxy);
				linphone_proxy_config_enable_register(proxy,
						TRUE);
				linphone_proxy_config_done(proxy);
			}
		} else {
			return -EIO;
		}

		return 0;
	}

	linphone_core_get_default_proxy(voip->core, &proxy);
	if (!proxy) {
		proxy = linphone_proxy_config_new();
		if (!proxy)
			return -ENOMEM;

		use_default = false;
	} else {
		linphone_proxy_config_edit(proxy);
	}

	len = strlen(host) + strlen("sip:");
	server = malloc(len + 1);
	if (!server)
		return -ENOMEM;

	snprintf(server, len + 1, "sip:%s", host);

	user = strdup(username);
	if (!user) {
		free(server);
		return -ENOMEM;
	}

	if (strchr(username, '@') != NULL) {
		char *ptr = strchr(user, '@');
		domain = ptr + 1;
		ptr[0] = '\0';
	}

	address = linphone_address_new(server);
	linphone_address_set_display_name(address, user);
	linphone_address_set_username(address, user);
	linphone_address_set_domain(address, domain);
	linphone_address_set_port_int(address, port);
	identity = linphone_address_as_string(address);
	linphone_address_destroy(address);

	auth = linphone_auth_info_new(user, NULL, password, NULL, NULL);
	linphone_core_add_auth_info(voip->core, auth);

	linphone_proxy_config_set_server_addr(proxy, server);
	linphone_proxy_config_set_identity(proxy, identity);

	if (voip->expires) {
		g_debug("voip-linphone: registration expires in %d seconds",
			voip->expires);
		linphone_proxy_config_expires(proxy, voip->expires);
	}

	linphone_proxy_config_enable_register(proxy, TRUE);

	if (!use_default) {
		linphone_core_add_proxy_config(voip->core, proxy);
		linphone_core_set_default_proxy(voip->core, proxy);
	} else {
		int err = linphone_proxy_config_done(proxy);
		if (err < 0)
			g_debug("voip-linphone: proxy configuration failed");
	}

	ms_free(identity);
	free(server);
	free(user);

	return 0;
}

int voip_logout(struct voip *voip)
{
	LinphoneProxyConfig *proxy = NULL;

	if (!voip)
		return -EINVAL;

	linphone_core_get_default_proxy(voip->core, &proxy);
	if (proxy && linphone_proxy_config_is_registered(proxy)) {
		int err;

		linphone_proxy_config_edit(proxy);
		linphone_proxy_config_enable_register(proxy, FALSE);

		err = linphone_proxy_config_done(proxy);
		if (err < 0)
			g_debug("voip-linphone: proxy configuration failed");
	}

	linphone_core_clear_proxy_config(voip->core);
	return 0;
}

int voip_call(struct voip *voip, const char *uri)
{
	LinphoneCallParams *params;
	LinphoneCall *call;

	if (!voip)
		return -EINVAL;

	params = linphone_core_create_default_call_parameters(voip->core);
	linphone_call_params_enable_early_media_sending(params, TRUE);

	call = linphone_core_invite_with_params(voip->core, uri, params);
	if (!call) {
		linphone_call_params_destroy(params);
		return -EIO;
	}

	linphone_call_params_destroy(params);
	return 0;
}

int voip_accept(struct voip *voip, char **caller)
{
	int err;

	if (!voip)
		return -EINVAL;

	err = linphone_core_accept_call(voip->core, NULL);
	if (err < 0)
		return err;

	if (caller && voip->contact)
		*caller = g_strdup(voip->contact);

	return 0;
}

int voip_terminate(struct voip *voip)
{
	int err;

	if (!voip)
		return -EINVAL;

	err = linphone_core_terminate_call(voip->core, NULL);
	if (err < 0)
		return err;

	return 0;
}

int voip_get_state(struct voip *voip, enum voip_state *statep)
{
	enum voip_state state = VOIP_STATE_LOGGED_OUT;
	LinphoneProxyConfig *proxy = NULL;

	if (!voip || !statep)
		return -EINVAL;

	linphone_core_get_default_proxy(voip->core, &proxy);
	if (proxy && linphone_proxy_config_is_registered(proxy))
		state = VOIP_STATE_LOGGED_IN;

	*statep = state;
	return 0;
}

int voip_get_contact(struct voip *voip, const char **contactp)
{
	if (!voip || !contactp)
		return -EINVAL;

	*contactp = voip->contact;
	return 0;
}

int voip_dial(struct voip *voip, uint8_t dtmf)
{
	if (!voip)
		return -EINVAL;

	/* we can only send dtmf tone on running calls */
	if (!linphone_core_in_call(voip->core))
		return -ENOTCONN;

	linphone_core_send_dtmf(voip->core, (char)dtmf);
	return 0;
}

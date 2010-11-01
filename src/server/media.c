#include <glib.h>

#include "remote-control.h"

int32_t medcom_media_run(void *priv, const char *media)
{
	int32_t ret = -ENOSYS;
	g_debug("> %s(priv=%p, media=%s)", __func__, priv, media);
	g_debug("< %s() = %d", __func__, ret);
	return ret;
}

int32_t medcom_media_command(void *priv, uint32_t command, int32_t p1, const char *p2)
{
	int32_t ret = -ENOSYS;
	g_debug("> %s(priv=%p, command=%#x, p1=%#x, p2=%s)", __func__, priv,
			command, p1, p2);
	g_debug("< %s() = %d", __func__, ret);
	return ret;
}

#include <glib.h>

#include <smartcard.h>

#include "remote-control-stub.h"
#include "remote-control.h"

static const char I2C_DEVICE[] = "/dev/i2c-1";
static const unsigned int I2C_SLAVE = 0x50;

struct smartcard {
	struct smartcard_i2c *i2c;
};

int smartcard_create(struct smartcard **smartcardp)
{
	struct smartcard *smartcard;
#if 0
	int err;
#endif

	g_debug("> %s(smartcardp=%p)", __func__, smartcardp);

	if (!smartcardp)
		return -EINVAL;

	smartcard = malloc(sizeof(*smartcard));
	if (!smartcard)
		return -ENOMEM;

	memset(smartcard, 0, sizeof(smartcard));

#if 0
	err = smartcard_i2c_open(&smartcard->i2c, I2C_DEVICE, I2C_SLAVE);
	if (err < 0) {
		free(smartcard);
		return err;
	}
#endif

	*smartcardp = smartcard;
	g_debug("< %s()", __func__);
	return 0;
}

int smartcard_free(struct smartcard *smartcard)
{
	if (!smartcard)
		return -EINVAL;

#if 0
	smartcard_i2c_close(smartcard->i2c);
#endif
	free(smartcard);
	return 0;
}

int smartcard_get_type(struct smartcard *smartcard, unsigned int *typep)
{
	if (!smartcard || !typep)
		return -EINVAL;

	*typep = CARD_TYPE_I2C;
	return 0;
}

ssize_t smartcard_read(struct smartcard *smartcard, off_t offset, void *buffer, size_t size)
{
	ssize_t ret;

	g_debug("> %s(smartcard=%p, offset=%ld, buffer=%p, size=%zu)", __func__, smartcard, offset, buffer, size);

	ret = smartcard_i2c_read(smartcard->i2c, offset, buffer, size);

	g_debug("< %s() = %zd", __func__, ret);
	return ret;
}

ssize_t smartcard_write(struct smartcard *smartcard, off_t offset, const void *buffer, size_t size)
{
	ssize_t ret;

	g_debug("> %s(smartcard=%p, offset=%ld, buffer=%p, size=%zu)", __func__, smartcard, offset, buffer, size);

	ret = smartcard_i2c_write(smartcard->i2c, offset, buffer, size);

	g_debug("< %s() = %zd", __func__, ret);
	return ret;
}

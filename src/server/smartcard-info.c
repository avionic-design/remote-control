/*
 * Copyright (C) 2016 Avionic Design GmbH
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define pr_fmt(fmt) "smartcard-info: " fmt

#include "remote-control-stub.h"
#include "remote-control.h"
#include "glogging.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>

const unsigned char EGK_SUCCESS[] = { 0x90, 0x00 };
const unsigned char EGK_REQUEST_ICC[] = { 0x20, 0x12, 0x01, 0x1, 0x01, 0x01 };
const unsigned char EGK_SELECT_EGK_ROOT[] = { 0x00, 0xA4, 0x04, 0x0C, 0x07,
	0xD2, 0x76, 0x00, 0x01, 0x44, 0x80, 0x00 };
const unsigned char EGK_SELECT_DF_HCA[] = {
	0x00, 0xA4, 0x04, 0x0C, 0x06, 0xD2, 0x76, 0x00, 0x00, 0x01, 0x02 };
const unsigned char EGK_READ_EF_GDO[] = { 0x00, 0xB0, 0x82, 0x00, 0x00 };
const unsigned char EGK_READ_EF_ATR[] = { 0x00, 0xB0, 0x9D, 0x00, 0x00 };

#define EGK_EF_VD_SHORT_ID 2
#define EGK_EF_VD_LEN_POS 6

#define EGK_EF_PD_SHORT_ID 1
#define EGK_EF_PD_LEN_POS 0

static int smartcard_command(struct smartcard *smartcard,
		const unsigned char *cmd, int cmd_len, unsigned char *res,
		int res_len)
{
	int ret;

	ret = smartcard_write(smartcard, 0, cmd, cmd_len);
	if (ret < 0)
		return ret;
	if (ret != cmd_len)
		return -ECOMM;

	ret = smartcard_read(smartcard, 0, res, res_len);
	if (ret < 0)
		return ret;
	if (ret < sizeof(EGK_SUCCESS))
		return -EPROTO;

	if (memcmp(&res[ret - sizeof(EGK_SUCCESS)], EGK_SUCCESS,
			sizeof(EGK_SUCCESS)))
		return -EIO;

	return ret - sizeof(EGK_SUCCESS);
}

#define READ_FILE_MODE_LENGTH_0 0
#define READ_FILE_MODE_OFFSET_0 1

static int smartcard_read_file(struct smartcard *smartcard, int short_id,
		int mode, unsigned char *buf, size_t buf_size)
{
	int ret, buf_pos, read_pos, read_len, cmd_len;
	unsigned char cmd[8];

	switch (mode) {
	case READ_FILE_MODE_LENGTH_0:
		read_len = 2;
		break;
	case READ_FILE_MODE_OFFSET_0:
		read_len = 4;
		break;
	default:
		return -EINVAL;
	}

	read_pos = buf_pos = cmd_len = 0;
	cmd[cmd_len++] = 0x00;
	cmd[cmd_len++] = 0xB0;
	cmd[cmd_len++] = 0x80 | short_id;
	cmd[cmd_len++] = read_pos;
	cmd[cmd_len++] = read_len;

	ret = smartcard_command(smartcard, cmd, cmd_len, &buf[buf_pos],
			buf_size - buf_pos);
	if (ret != read_len) {
		pr_debug("Read file failed %d (%d)", ret, read_len);
		return ret;
	}

	switch (mode) {
	case READ_FILE_MODE_LENGTH_0:
		read_pos += ret;
		read_len += (buf[0] << 8) + buf[1];
		break;
	case READ_FILE_MODE_OFFSET_0:
		read_pos = (buf[0] << 8) + buf[1];
		read_len = (buf[2] << 8) + buf[3] + 1;
		break;
	default:
		return -EINVAL;
	}

	while (read_pos < read_len && buf_pos < buf_size) {
		int block = read_len - read_pos > 0xFE ? 0xFE :
				read_len - read_pos;
		if (buf_pos + block > buf_size)
			block = buf_size - buf_pos;

		cmd_len = 0;
		cmd[cmd_len++] = 0x00;
		cmd[cmd_len++] = 0xB0;
		cmd[cmd_len++] = read_pos >> 8 & 0xFF;
		cmd[cmd_len++] = read_pos & 0xFF;
		cmd[cmd_len++] = block;

		ret = smartcard_command(smartcard, cmd, cmd_len, &buf[buf_pos],
				buf_size - buf_pos);
		if (ret < 0)
			return ret;

		read_pos += ret;
		buf_pos += ret;
	}

	return buf_pos;
}

#define GZIP_HEADER_LEN 10
#define GZIP_FOOTER_LEN 8

static ssize_t smartcard_gunzip(unsigned char *src, int src_len,
		unsigned char *dst, int dst_len)
{
	unsigned long crc, val;
	z_stream strm;
	ssize_t ret;

	if (src_len < GZIP_HEADER_LEN + GZIP_FOOTER_LEN)
		return -EINVAL;
	if (src[0] != 0x1F || src[1] != 0x8B || src[2] != 0x08 ||
			src[3] != 0x00 || src[8] != 0x00)
		return -EINVAL;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;

	ret = inflateInit2(&strm, -15);
	if (ret != Z_OK)
		return ret;

	strm.avail_in = src_len - GZIP_HEADER_LEN;
	strm.next_in = &src[GZIP_HEADER_LEN];
	strm.avail_out = dst_len;
	strm.next_out = dst;

	ret = inflate(&strm, Z_NO_FLUSH);
	inflateEnd(&strm);

	if (ret != Z_STREAM_END) {
		pr_debug("smartcard_gunzip failed with %d", ret);
		return -EIO;
	}
	ret = dst_len - strm.avail_out;

	if (strm.avail_in < GZIP_FOOTER_LEN) {
		pr_debug("smartcard_gunzip missing crc and size (%d)",
				strm.avail_in);
		return -EIO;
	}

	crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, dst, ret);
	val = strm.next_in[0] | strm.next_in[1] << 8 | strm.next_in[2] << 16 |
			strm.next_in[3] << 24;
	if (crc != val) {
		pr_debug("smartcard_gunzip wrong crc 0x%08lx (expected 0x%08lx)",
				crc, val);
		return -EIO;
	}

	val = strm.next_in[4] | strm.next_in[5] << 8 | strm.next_in[6] << 16 |
			strm.next_in[7] << 24;
	if (ret != val) {
		pr_debug("smartcard_gunzip wrong length %ld (expected %ld)",
				(unsigned long)ret, val);
		return -EIO;
	}

	return ret;
}

static char *get_node_content(xmlNodePtr rootnode, const char **nodenames)
{
	xmlChar *nodename = BAD_CAST(*nodenames);
	xmlNodePtr node = rootnode;

	if (node == NULL)
		return NULL;

	while (node != NULL) {
		if (xmlStrcmp(node->name, nodename)) {
			node = node->next;
			continue;
		}
		nodenames++;
		if (*nodenames)
			return get_node_content(node->children, nodenames);
		return (char *)xmlStrdup(node->children->content);
	}

	return NULL;
}

const char *EGK_EF_PD_FIRST_NAME[] = { "UC_PersoenlicheVersichertendatenXML",
	"Versicherter", "Person", "Vorname", NULL };
const char *EGK_EF_PD_LAST_NAME[] = { "UC_PersoenlicheVersichertendatenXML",
	"Versicherter", "Person", "Nachname", NULL };
const char *EGK_EF_PD_DATE_OF_BIRTH[] = { "UC_PersoenlicheVersichertendatenXML",
	"Versicherter", "Person", "Geburtsdatum", NULL };
const char *EGK_EF_PD_GENDER[] = { "UC_PersoenlicheVersichertendatenXML",
	"Versicherter", "Person", "Geschlecht", NULL };
const char *EGK_EF_PD_ZIP_CODE[] = { "UC_PersoenlicheVersichertendatenXML",
	"Versicherter", "Person", "StrassenAdresse", "Postleitzahl", NULL };
const char *EGK_EF_PD_CITY[] = { "UC_PersoenlicheVersichertendatenXML",
	"Versicherter", "Person", "StrassenAdresse", "Ort", NULL };
const char *EGK_EF_PD_COUNTRY[] = { "UC_PersoenlicheVersichertendatenXML",
	"Versicherter", "Person", "StrassenAdresse", "Land",
	"Wohnsitzlaendercode", NULL };
const char *EGK_EF_PD_STREET[] = { "UC_PersoenlicheVersichertendatenXML",
	"Versicherter", "Person", "StrassenAdresse", "Strasse", NULL };
const char *EGK_EF_PD_STREET_NUMBER[] = { "UC_PersoenlicheVersichertendatenXML",
	"Versicherter", "Person", "StrassenAdresse", "Hausnummer", NULL };
const char *EGK_EF_PD_INSURANCE_ID[] = { "UC_PersoenlicheVersichertendatenXML",
	"Versicherter", "Versicherten_ID", NULL };

static int read_ef_pd(struct smartcard *smartcard, struct smartcard_info *data)
{
	unsigned char raw_data[0x0352 + sizeof(EGK_SUCCESS)];
	unsigned char file_data[0xFFF];
	xmlDocPtr doc = NULL;
	int ret;

	ret = smartcard_command(smartcard, EGK_SELECT_DF_HCA,
			sizeof(EGK_SELECT_DF_HCA), raw_data, sizeof(raw_data));
	if (ret < 0) {
		pr_debug("Select DF.HCA failed %d", ret);
		return ret;
	}

	ret = smartcard_read_file(smartcard, EGK_EF_PD_SHORT_ID,
			READ_FILE_MODE_LENGTH_0, raw_data, sizeof(raw_data));
	if (ret < 0) {
		pr_debug("Read EF.PD failed %d", ret);
		return ret;
	}

	ret = smartcard_gunzip(raw_data, ret, file_data, sizeof(file_data));
	if (ret < 0) {
		pr_debug("Unzip EF.PD failed %d", ret);
		return ret;
	}

	doc = xmlReadMemory((const char *)file_data, ret, NULL, NULL, 0);
	if (!doc) {
		pr_debug("Parse EF.PD failed");
		return -EIO;
	}
	data->first_name = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_PD_FIRST_NAME);
	data->last_name = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_PD_LAST_NAME);
	data->date_of_birth = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_PD_DATE_OF_BIRTH);
	data->gender = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_PD_GENDER);
	data->zip_code = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_PD_ZIP_CODE);
	data->city = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_PD_CITY);
	data->country = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_PD_COUNTRY);
	data->street = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_PD_STREET);
	data->street_number = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_PD_STREET_NUMBER);
	data->insurance_id = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_PD_INSURANCE_ID);

	xmlFreeDoc(doc);
	xmlCleanupParser();

	return 0;
}

const char *EGK_EF_VD_INSURER_ID[] = { "UC_AllgemeineVersicherungsdatenXML",
	"Versicherter", "Versicherungsschutz", "Kostentraeger",
	"Kostentraegerkennung", NULL };
const char *EGK_EF_VD_INSURER_COUNTRY[] = {
	"UC_AllgemeineVersicherungsdatenXML", "Versicherter",
	"Versicherungsschutz", "Kostentraeger", "Kostentraegerlaendercode",
	NULL };
const char *EGK_EF_VD_INSURER_NAME[] = { "UC_AllgemeineVersicherungsdatenXML",
	"Versicherter", "Versicherungsschutz", "Kostentraeger",
	"Name", NULL };
const char *EGK_EF_VD_BILLING_INSURER_ID[] = {
	"UC_AllgemeineVersicherungsdatenXML", "Versicherter",
	"Versicherungsschutz", "Kostentraeger", "AbrechnenderKostentraeger",
	"Kostentraegerkennung", NULL };
const char *EGK_EF_VD_BILLING_INSURER_NAME[] = {
	"UC_AllgemeineVersicherungsdatenXML", "Versicherter",
	"Versicherungsschutz", "Kostentraeger", "AbrechnenderKostentraeger",
	"Name", NULL };

static int read_ef_vd(struct smartcard *smartcard, struct smartcard_info *data)
{
	unsigned char raw_data[0x04E2 + sizeof(EGK_SUCCESS)];
	unsigned char file_data[0xFFF];
	xmlDocPtr doc = NULL;
	int ret;

	ret = smartcard_command(smartcard, EGK_SELECT_DF_HCA,
			sizeof(EGK_SELECT_DF_HCA), raw_data, sizeof(raw_data));
	if (ret < 0) {
		pr_debug("Select DF.HCA failed %d", ret);
		return ret;
	}

	ret = smartcard_read_file(smartcard, EGK_EF_VD_SHORT_ID,
			READ_FILE_MODE_OFFSET_0, raw_data, sizeof(raw_data));
	if (ret < 0) {
		pr_debug("Read EF.VD failed %d", ret);
		return ret;
	}

	ret = smartcard_gunzip(raw_data, ret, file_data, sizeof(file_data));
	if (ret < 0) {
		pr_debug("Unzip EF.VD failed %d", ret);
		return ret;
	}

	doc = xmlReadMemory((const char *)file_data, ret, NULL, NULL, 0);
	if (!doc) {
		pr_debug("Parse EF.VD failed");
		return -EIO;
	}
	data->insurer_id = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_VD_INSURER_ID);
	data->insurer_country = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_VD_INSURER_COUNTRY);
	data->insurer_name = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_VD_INSURER_NAME);
	data->billing_insurer_id = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_VD_BILLING_INSURER_ID);
	data->billing_insurer_name = get_node_content(xmlDocGetRootElement(doc),
			EGK_EF_VD_BILLING_INSURER_NAME);

	xmlFreeDoc(doc);
	xmlCleanupParser();

	return 0;
}

static int read_ef_gdo(struct smartcard *smartcard, struct smartcard_info *data)
{
	unsigned char raw_data[0x0C + sizeof(EGK_SUCCESS)];
	int i, ret;

	ret = smartcard_command(smartcard, EGK_SELECT_EGK_ROOT,
			sizeof(EGK_SELECT_EGK_ROOT), raw_data,
			sizeof(raw_data));
	if (ret < 0) {
		pr_debug("Select eGK root failed %d", ret);
		return ret;
	}

	ret = smartcard_command(smartcard, EGK_READ_EF_GDO,
			sizeof(EGK_READ_EF_GDO), raw_data, sizeof(raw_data));
	if (ret < 3) {
		pr_debug("Read EF.GDO failed %d", ret);
		return ret;
	}
	ret -= 2;

	data->card_id = malloc(ret * 2 + 1);
	if (!data->card_id)
		return -ENOMEM;

	for (i = 0; i < ret; i++)
		snprintf(&data->card_id[i * 2], 3, "%02X", raw_data[i + 2]);

	return 0;
}

static int read_ef_atr(struct smartcard *smartcard, struct smartcard_info *data)
{
	unsigned char raw_data[0xFF + sizeof(EGK_SUCCESS)];
	int i, ret;

	ret = smartcard_command(smartcard, EGK_SELECT_EGK_ROOT,
			sizeof(EGK_SELECT_EGK_ROOT), raw_data,
			sizeof(raw_data));
	if (ret < 0) {
		pr_debug("Select eGK root failed %d", ret);
		return ret;
	}

	ret = smartcard_command(smartcard, EGK_READ_EF_ATR,
			sizeof(EGK_READ_EF_ATR), raw_data, sizeof(raw_data));
	if (ret < 3) {
		pr_debug("Read EF.ATR failed %d", ret);
		return ret;
	}

	data->card_atr = malloc(ret * 2 + 1);
	if (!data->card_atr)
		return -ENOMEM;

	for (i = 0; i < ret; i++)
		snprintf(&data->card_atr[i * 2], 3, "%02X", raw_data[i]);

	return 0;
}

static int read_atr(struct smartcard *smartcard, struct smartcard_info *data)
{
	unsigned char raw_data[0xFF + sizeof(EGK_SUCCESS)];
	int i, ret;

	ret = smartcard_command(smartcard, EGK_REQUEST_ICC,
			sizeof(EGK_REQUEST_ICC), raw_data, sizeof(raw_data));
	if (ret < 0) {
		pr_debug("Request ICC failed %d", ret);
		return ret;
	}

	data->atr = malloc(ret * 2 + 1);
	if (!data->atr)
		return -ENOMEM;

	for (i = 0; i < ret; i++)
		snprintf(&data->atr[i * 2], 3, "%02X", raw_data[i]);

	return 0;
}

int smartcard_read_info(struct smartcard *smartcard,
		struct smartcard_info *data)
{
	memset(data, 0, sizeof(*data));

	if (read_atr(smartcard, data))
		return -ENOENT;

	read_ef_gdo(smartcard, data);
	read_ef_atr(smartcard, data);
	read_ef_pd(smartcard, data);
	read_ef_vd(smartcard, data);

	return 0;
}

void smartcard_read_info_free(struct smartcard_info *data)
{
	if (!data)
		return;

	free(data->first_name);
	free(data->last_name);
	free(data->date_of_birth);
	free(data->gender);
	free(data->zip_code);
	free(data->city);
	free(data->country);
	free(data->street);
	free(data->street_number);
	free(data->insurance_id);
	free(data->insurer_id);
	free(data->insurer_country);
	free(data->insurer_name);
	free(data->billing_insurer_id);
	free(data->billing_insurer_name);
	free(data->card_id);
	free(data->card_atr);
	free(data->atr);
}

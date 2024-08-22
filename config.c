#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <expat.h>
#include "buffer.h"
#include "channel.h"
#include "dupcontent.h"

enum {
	ELEM_UNKNOWN = -1,
	ELEM_CONFIG,
	ELEM_URL,
};

static const char *s_errstr = NULL;
static char s_errbuf[512];
static XML_Parser s_parser = NULL;
static struct buffer s_buffer = { 0 };

static int s_inside_config = 0;
static int s_inside_url = 0;

static FILE *find_cfg(void)
{
	FILE *f;
	char path[512];
	const char *prefix;

	prefix = getenv("XDG_CONFIG_HOME");
	if (prefix) {
		snprintf(path, sizeof(path), "%s", prefix);
		snprintf(&path[strlen(path)], sizeof(path) - strlen(path),
			 "/raisin/config.xml");
		f = fopen(path, "r");
		if (f) {
			return f;
		}
	}

	prefix = getenv("HOME");
	if (prefix) {
		snprintf(path, sizeof(path), "%s", prefix);
		snprintf(&path[strlen(path)], sizeof(path) - strlen(path),
			 "/.config/raisin/config.xml");
		f = fopen(path, "r");
		if (f) {
			return f;
		}
	}

	f = fopen("config.xml", "r");
	return f;
}

static char *read_cfg(FILE *f, size_t *len)
{
	char *data;
	size_t pos, r;

	fseek(f, 0, SEEK_END);
	*len = ftell(f);
	fseek(f, 0, SEEK_SET);

	data = malloc(*len);
	if (data == NULL) {
		*len = 0;
		return NULL;
	}

	pos = 0;
	while (pos < *len) {
		r = fread(data, 1, *len - pos, f);
		pos += r;
	}

	return data;
}

static int elem_type(const char *str)
{
	if (strcmp(str, "config") == 0)
		return ELEM_CONFIG;
	if (strcmp(str, "url") == 0)
		return ELEM_URL;

	return ELEM_UNKNOWN;
}

static void cancel_with_error(const char *fmt, ...)
{
	va_list args;
	vsnprintf(s_errbuf, sizeof(s_errbuf), fmt, args);
	s_errstr = s_errbuf;
	XML_StopParser(s_parser, XML_FALSE);
}

static XMLCALL void start_elem_handler(void *userdata, const XML_Char *name,
				       const XML_Char **attrs)
{
	(void)attrs;
	(void)userdata;

	int type = elem_type(name);

	if (type == ELEM_UNKNOWN) {
		cancel_with_error("invalid element type in configuration: %s",
				  name);
		return;
	}

	if (type != ELEM_CONFIG && !s_inside_config) {
		cancel_with_error("<config> element is not root");
		return;
	}

	switch (type) {
	case ELEM_CONFIG:
		s_inside_config = 1;
		break;
	case ELEM_URL:
		s_inside_url = 1;
		break;
	default:
		break;
	}
}

static XMLCALL void end_elem_handler(void *userdata, const XML_Char *name)
{
	(void)userdata;

	int type = elem_type(name);

	if (type == ELEM_UNKNOWN) {
		cancel_with_error("invalid element type in configuration: %s",
				  name);
		return;
	}

	switch (type) {
	case ELEM_CONFIG:
		s_inside_config = 0;
		break;
	case ELEM_URL:
		s_inside_url = 0;
		add_channel(dupcontent(s_buffer.data, s_buffer.size));
		buf_clear(&s_buffer);
		break;
	default:
		break;
	}
}

static XMLCALL void char_data_handler(void *userdata, const XML_Char *s,
				      int len)
{
	(void)userdata;
	buf_append(&s_buffer, s, len);
}

static int parse_cfg(char *data, size_t len)
{
	enum XML_Status status;
	enum XML_Error err;

	s_parser = XML_ParserCreate(NULL);
	XML_SetStartElementHandler(s_parser, start_elem_handler);
	XML_SetEndElementHandler(s_parser, end_elem_handler);
	XML_SetCharacterDataHandler(s_parser, char_data_handler);

	status = XML_Parse(s_parser, data, len, XML_TRUE);

	if (status != XML_STATUS_OK) {
		if (!s_errstr) {
			err = XML_GetErrorCode(s_parser);
			s_errstr = XML_ErrorString(err);
		}
		return -1;
	}

	return 0;
}

int load_cfg(void)
{
	FILE *f;
	char *data;
	size_t len;

	s_errstr = NULL;

	f = find_cfg();
	if (f == NULL) {
		s_errstr = "unable to find config.xml";
		return -1;
	}

	data = read_cfg(f, &len);
	fclose(f);
	if (data == NULL) {
		s_errstr = "unable to read config.xml";
		return -1;
	}

	return parse_cfg(data, len);
}

const char *load_cfg_errstr(void)
{
	return s_errstr;
}

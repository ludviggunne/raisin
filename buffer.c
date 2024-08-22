#include "buffer.h"
#include "mem.h"
#include <string.h>

#define INIT_CAPAC 32

struct buffer buf_init(void)
{
	struct buffer buf = { 0 };
	return buf;
}

void buf_append(struct buffer *buf, const char *data, int len)
{
	if (buf->data == NULL) {
		buf->data = malloc(INIT_CAPAC);
		buf->cap = INIT_CAPAC;
	}

	if (len < 0) {
		len = strlen(data);
	}

	if (buf->size + len > buf->cap) {
		while (buf->size + len > buf->cap) {
			buf->cap *= 2;
		}

		buf->data = realloc(buf->data, buf->cap);
	}

	memcpy(&buf->data[buf->size], data, (size_t)len);
	buf->size += (size_t)len;
}

void buf_putc(struct buffer *buf, char c)
{
	if (buf->data == NULL) {
		buf->data = malloc(INIT_CAPAC);
		buf->cap = INIT_CAPAC;
	}

	if (buf->size == buf->cap) {
		buf->cap *= 2;
		buf->data = realloc(buf->data, buf->cap);
	}

	buf->data[buf->size++] = c;
}

void buf_clear(struct buffer *buf)
{
	buf->size = 0;
}

void buf_free(struct buffer *buf)
{
	free(buf->data);
	memset(buf, 0, sizeof(*buf));
}

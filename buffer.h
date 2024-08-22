#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <stddef.h>

#define BUFFER_INITIAL_CAPACITY 64

struct buffer {
	char *data;
	size_t size;
	size_t cap;
};

/*
 * Creates a new buffer.
 */
struct buffer buf_init(void);

/*
 * Appends data to end of buffer.
 * If `len` is equal to -1, `data` is assumed to be
 * null terminated.
 */
void buf_append(struct buffer *buf, const char *data, int len);

/*
 * Appends a single character to the end of the buffer.
 */
void buf_putc(struct buffer *buf, char c);

/*
 * Clear buffer, retaining capacity.
 */
void buf_clear(struct buffer *buf);

/*
 * Frees memory associated with buffer.
 */
void buf_free(struct buffer *buf);

#endif

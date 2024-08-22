#ifndef DUPCONTENT_H_INCLUDED
#define DUPCONTENT_H_INCLUDED

#include <stddef.h>

/*
 * Duplicates a string, while also trimming leading/trailing
 * whitespace and substituting HTML entity codes.
 */
char *dupcontent(const char *str, size_t len);

#endif				/* DUPCONTENT_H_INCLUDED */

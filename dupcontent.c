#include "dupcontent.h"
#include "buffer.h"
#include <string.h>

#define ENTC_MAX_LEN 8

static inline int ws(int c)
{
	return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

char *dupcontent(const char *str, size_t len)
{
	size_t begin, end;

	begin = 0;
	while (ws(str[begin])) {
		begin++;
		if (begin == len)
			return NULL;
	}

	end = begin + 1;
	for (size_t i = begin; i < len; ++i) {
		if (!ws(str[i])) {
			end = i + 1;
		}
	}

	return strndup(&str[begin], end - begin);
}

#define CASE(str, pat, rep)\
     if (end == strlen(pat) && strncmp(str, pat, end) == 0) {\
             buf_append(buf, rep);\
             return end;\
     }

// static size_t parse_ent(struct buffer *buf, const char *str, size_t len)
// {
//      size_t i, end = 0;

//      for (i = 1; i < len && i < ENTC_MAX_LEN; ++i) {
//              if (str[i] == ';') {
//                      end = i;
//                      break;
//              }
//      }

//      if (end == 0) {
//              buf_putc(buf, '&');
//              return 1;
//      }
//      // CASE()
//      return 0;
// }

// static char *_dupcontent(const char *str, size_t len)
// {
//      struct buffer buf;
//      size_t begin, end, i;

//      (void)end;

//      begin = 0;
//      while (ws(str[begin])) {
//              begin++;
//              if ((size_t)begin == len) {
//                      return NULL;
//              }
//      }

//      len -= begin;
//      str += begin;

//      for (i = 0; (size_t)i < len; ++i) {
//              if (!ws(str[i])) {
//                      end = i + 1;
//              }

//              if (str[i] == '&') {
//                      i += parse_ent(&buf, &str[i], len - i);
//                      continue;
//              }

//              buf_putc(&buf, str[i]);
//      }
// }

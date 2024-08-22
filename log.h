#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include <stdio.h>
#include <unistd.h>

#define LOG(...)\
	{\
		if (!isatty(STDERR_FILENO))\
			fprintf(stderr, "[info]: " __VA_ARGS__);\
	}

#define LOGERR(...)\
	{\
		if (!isatty(STDERR_FILENO)) {\
			fprintf(stderr, "\x1b[31m[error]: " __VA_ARGS__);\
			fprintf(stderr, "\x1b[0m");\
		}\
	}

#endif

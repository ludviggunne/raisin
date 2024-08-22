#ifndef CHANNEL_H_INCLUDED
#define CHANNEL_H_INCLUDED

#include <stddef.h>
#include <curl/curl.h>
#include <pthread.h>

#include "buffer.h"

/*
 * The global channel list stays the same during the whole execution (from parsing configuration).
 * Only certain fields are updated.
 */
extern struct channel *g_channels;
extern size_t g_nchannels;

enum channel_status {
	CHSTAT_INIT = 0,
	CHSTAT_FETCHING,
	CHSTAT_FETCHED,
	CHSTAT_PARSING,
	CHSTAT_DONE,
	CHSTAT_ERROR,
};

struct ch_item {
	char *title;
	char *link;
	char *pubdate;
	char *descr;
};

struct channel {
	enum channel_status status;

	/* The URL of the channel. */
	char *url;

	/* The title of the channel, if status `CHSTAT_DONE`, otherwise `NULL` */
	char *title;

	/* An error string, if `status` is `CHSTAT_ERROR`, otherwise `NULL` */
	char *errstr;

	/* Error buffer filled in by curl */
	char errbuf[CURL_ERROR_SIZE];

	/* Download progress in percent */
	int prog;

	/* The lock must be aquired before access
	   to these fields:
	   * title
	   * errstr
	   * errbuf
	   * pdata
	   * xml
	   * items
	 */
	pthread_mutex_t lock;

	/* Raw XML received by libCurl */
	struct buffer xml;

	/* Parsing data */
	void *pdata;

	/* Number of channel items */
	size_t numitems;

	/* Channel items */
	struct ch_item *items;
};

void add_channel(char *url);
void free_channel_items(struct channel *ch);
// void add_channel_item(struct channel *ch);

#endif				/* CHANNEL_H_INCLUDED */

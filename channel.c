#include "channel.h"
#include "mem.h"
#include "pthread.h"
#include <string.h>

#define INIT_CAPAC 	8
#define ITEM_INIT_CAPAC 8

struct channel *g_channels = NULL;
size_t g_nchannels = 0;
size_t channel_cap = INIT_CAPAC;

void add_channel(char *url)
{
	struct channel channel;

	memset(&channel, 0, sizeof(channel));
	channel.url = url;
	pthread_mutex_init(&channel.lock, NULL);

	if (g_channels == NULL) {
		g_channels = malloc(INIT_CAPAC * sizeof(*g_channels));
	}

	if (g_nchannels == channel_cap) {
		channel_cap *= 2;
		g_channels =
		    realloc(g_channels, channel_cap * sizeof(*g_channels));
	}

	g_channels[g_nchannels++] = channel;
}

void free_channel_items(struct channel *ch)
{
	size_t i;
	struct ch_item *it;

	if (ch->items == NULL)
		return;

	for (i = 0; i < ch->numitems; ++i) {
		it = &ch->items[i];
		free(it->title);
		free(it->link);
		free(it->pubdate);
		free(it->descr);
	}
	free(ch->items);
	ch->items = NULL;
	ch->numitems = 0;
}

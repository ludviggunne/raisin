#include "mem.h"
#include "parse.h"
#include "thpool.h"
#include "dupcontent.h"
#include "log.h"
#include <expat.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define INIT_CAP 8

static int s_initialized = 0;
static threadpool s_thpool = NULL;

struct pdata {
	XML_Parser p;
	char *title;
	struct ch_item *items;
	size_t nitems;
	size_t itcap;
	struct ch_item *curritem;
	int currelem;
	struct buffer buf;
	int in_rss;
	int in_channel;
	int in_item;
};

enum {
	ELEM_SKIP = 0,		/* must be 0 */
	ELEM_RSS,
	ELEM_CHANNEL,
	ELEM_ITEM,
	ELEM_TITLE,
	ELEM_LINK,
	ELEM_PUBDATE,
	ELEM_DESCR,
};

static void init(void)
{
	int numthreads;

	numthreads = sysconf(_SC_NPROCESSORS_ONLN);
	if (numthreads < 0)
		numthreads = 4;

	LOG("initializing threadpool with %d threads\n", numthreads);
	assert((s_thpool = thpool_init(numthreads)));
	s_initialized = 1;
}

static void add_item(struct pdata *pdata)
{
	if (pdata->items == NULL) {
		pdata->items = malloc(sizeof(*pdata->items) * INIT_CAP);
		pdata->itcap = INIT_CAP;
	}

	if (pdata->nitems == pdata->itcap) {
		pdata->itcap *= 2;
		pdata->items =
		    realloc(pdata->items, sizeof(*pdata->items) * pdata->itcap);
	}

	pdata->curritem = &pdata->items[pdata->nitems];
	memset(pdata->curritem, 0, sizeof(*pdata->curritem));
	pdata->nitems++;
}

static int elem_type(const char *str)
{
	if (strcmp(str, "rss") == 0)
		return ELEM_RSS;
	if (strcmp(str, "channel") == 0)
		return ELEM_CHANNEL;
	if (strcmp(str, "item") == 0)
		return ELEM_ITEM;
	if (strcmp(str, "title") == 0)
		return ELEM_TITLE;
	if (strcmp(str, "link") == 0)
		return ELEM_LINK;
	if (strcmp(str, "pubDate") == 0)
		return ELEM_PUBDATE;
	if (strcmp(str, "description") == 0)
		return ELEM_DESCR;
	return ELEM_SKIP;
}

static void cancel_with_error(struct channel *ch, const char *fmt, ...)
{
	va_list args;
	struct pdata *pd;

	pthread_mutex_lock(&ch->lock);

	ch->status = CHSTAT_ERROR;
	vsnprintf(ch->errbuf, sizeof(ch->errbuf), fmt, args);
	ch->errstr = strdup(ch->errbuf);

	pd = (struct pdata *)ch->pdata;
	XML_StopParser(pd->p, XML_FALSE);

	pthread_mutex_unlock(&ch->lock);
}

static XMLCALL void start_elem_handler(void *userdata, const XML_Char *name,
				       const XML_Char **attrs)
{
	struct channel *ch;
	struct pdata *pd;
	(void)attrs;

	ch = (struct channel *)userdata;
	pd = (struct pdata *)ch->pdata;

	assert(pd);

	pd->currelem = elem_type(name);

	if (pd->currelem != ELEM_RSS && !pd->in_rss) {
		cancel_with_error(ch, "<rss> element is not root");
	}

	if (pd->currelem == ELEM_RSS) {
		pd->in_rss = 1;
	}

	if (pd->currelem == ELEM_ITEM) {
		add_item(pd);
	}
}

static XMLCALL void end_elem_handler(void *userdata, const XML_Char *name)
{
	struct channel *ch;
	struct pdata *pd;
	(void)name;

	ch = (struct channel *)userdata;
	pd = (struct pdata *)ch->pdata;

	assert(pd);

	switch (pd->currelem) {
	case ELEM_RSS:
		pd->in_rss = 0;
		break;
	case ELEM_ITEM:
		pd->curritem = NULL;
		break;
	case ELEM_TITLE:
		if (pd->curritem) {
			pd->curritem->title =
			    dupcontent(pd->buf.data, pd->buf.size);
		} else {
			pd->title = dupcontent(pd->buf.data, pd->buf.size);
		}
		break;
	case ELEM_LINK:
		if (pd->curritem) {
			pd->curritem->link =
			    dupcontent(pd->buf.data, pd->buf.size);
		}
		break;
	case ELEM_PUBDATE:
		if (pd->curritem) {
			pd->curritem->pubdate =
			    dupcontent(pd->buf.data, pd->buf.size);
		}
		break;
	case ELEM_DESCR:
		if (pd->curritem) {
			pd->curritem->descr =
			    dupcontent(pd->buf.data, pd->buf.size);
		}
		break;
	default:
		break;
	}

	pd->currelem = ELEM_SKIP;
	buf_clear(&pd->buf);
}

static XMLCALL void char_data_handler(void *userdata, const XML_Char *s,
				      int len)
{
	struct channel *ch;
	struct pdata *pd;

	ch = (struct channel *)userdata;
	pd = (struct pdata *)ch->pdata;

	if (pd->currelem == ELEM_SKIP)
		return;

	buf_append(&pd->buf, s, len);
}

static void parse_fn(void *ptr)
{
	struct channel *ch;
	XML_Parser p;
	enum XML_Status status;
	enum XML_Error ec;
	struct pdata *pdata;

	ch = (struct channel *)ptr;
	pdata = (struct pdata *)ch->pdata;

	pthread_mutex_lock(&ch->lock);
	ch->status = CHSTAT_PARSING;
	pthread_mutex_unlock(&ch->lock);

	assert((p = XML_ParserCreate(NULL)));
	XML_SetStartElementHandler(p, start_elem_handler);
	XML_SetEndElementHandler(p, end_elem_handler);
	XML_SetCharacterDataHandler(p, char_data_handler);
	XML_SetUserData(p, (void *)ch);
	pdata->p = p;

	LOG("begin parse of %s\n", ch->url);
	status = XML_Parse(p, ch->xml.data, ch->xml.size, XML_TRUE);
	LOG("done parsing %s\n", ch->url);

	pthread_mutex_lock(&ch->lock);
	{
		if (status != XML_STATUS_OK) {
			/* there might already be a custom error */
			if (ch->errstr == NULL) {
				ec = XML_GetErrorCode(p);
				ch->errstr = strdup(XML_ErrorString(ec));
			}
			ch->status = CHSTAT_ERROR;
			LOGERR("parsing of %s failed: %s\n", ch->url,
			       ch->errstr);
		} else {
			ch->status = CHSTAT_DONE;
			ch->items = pdata->items;
			ch->numitems = pdata->nitems;
			ch->title = pdata->title;
			LOG("parsing succeeded: %s has title %s\n", ch->url,
			    ch->title);
		}

		buf_free(&pdata->buf);
		free(pdata);
		ch->pdata = NULL;
	}
	pthread_mutex_unlock(&ch->lock);
}

void parse(struct channel *ch)
{
	if (!s_initialized) {
		init();
	}

	pthread_mutex_lock(&ch->lock);
	{
		free_channel_items(ch);
		free(ch->title);
		free(ch->errstr);
		free(ch->pdata);
		ch->title = NULL;
		ch->errstr = NULL;
		ch->prog = 0;

		ch->pdata = calloc(sizeof(struct pdata), 1);

		assert(thpool_add_work(s_thpool, parse_fn, ch) == 0);
	}
	pthread_mutex_unlock(&ch->lock);
}

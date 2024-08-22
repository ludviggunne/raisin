#include <curl/curl.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "fetch.h"
#include "channel.h"
#include "parse.h"
#include "log.h"

/* TODO: handle curl error codes */

#define TIMEOUT 10

static int s_initialized = 0;
static int s_running = 0;
static CURLM *s_curlm = NULL;

static void init(void)
{
	LOG("initializing fetch API\n");
	s_curlm = curl_multi_init();
	s_initialized = 1;
}

static size_t writefunction(char *ptr, size_t size, size_t nmemb,
			    void *userdata)
{
	struct channel *ch = (struct channel *)userdata;
	size *= nmemb;
	pthread_mutex_lock(&ch->lock);
	buf_append(&ch->xml, ptr, (int)size);
	pthread_mutex_unlock(&ch->lock);
	return size;
}

static int xferinfofunction(void *clientp, curl_off_t dltotal,
			    curl_off_t dlnow, curl_off_t ultotal,
			    curl_off_t ulnow)
{
	(void)ultotal;
	(void)ulnow;

	struct channel *ch = (struct channel *)clientp;

	pthread_mutex_lock(&ch->lock);
	if (dltotal == 0) {
		ch->prog = 0;
		pthread_mutex_unlock(&ch->lock);
		return 0;
	}

	ch->prog = (int)(100.0f * (float)dlnow / (float)dltotal);
	pthread_mutex_unlock(&ch->lock);
	return 0;
}

void fetch_begin(int opts)
{
	size_t i;
	// int _running_handles;
	CURL *curl;
	struct channel *ch;

	if (!s_initialized) {
		init();
	}

	assert(!s_running);
	s_running = 1;

	for (i = 0; i < g_nchannels; ++i) {

		ch = &g_channels[i];
		pthread_mutex_lock(&ch->lock);

		if (ch->status == CHSTAT_DONE && !(opts & FETCHOPT_FORCE)) {
			pthread_mutex_unlock(&ch->lock);
			continue;
		}

		curl = curl_easy_init();

		curl_easy_setopt(curl, CURLOPT_URL, ch->url);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, ch->errbuf);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
				 xferinfofunction);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunction);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT);

		/* The channel pointer is passed as user data to all
		   libCurl callbacks. CURLOPT_PRIVATE is used to retrieve
		   a corresponding channel from a finsihed transfer. */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)ch);
		curl_easy_setopt(curl, CURLOPT_XFERINFODATA, (void *)ch);
		curl_easy_setopt(curl, CURLOPT_PRIVATE, (void *)ch);

		curl_multi_add_handle(s_curlm, curl);

		free(ch->title);
		free(ch->errstr);
		free(ch->items);
		free(ch->pdata);
		ch->status = CHSTAT_FETCHING;
		ch->title = NULL;
		ch->errstr = NULL;
		ch->pdata = NULL;
		ch->prog = 0;
		ch->errbuf[0] = 0;
		buf_clear(&ch->xml);

		LOG("downloading %s\n", ch->url);

		pthread_mutex_unlock(&ch->lock);
	}
}

int fetch_update(void)
{
	CURLMsg *cm;
	struct channel *ch;
	int _count, running;

	assert(s_running);

	curl_multi_perform(s_curlm, &running);

	while ((cm = curl_multi_info_read(s_curlm, &_count))) {

		curl_easy_getinfo(cm->easy_handle, CURLINFO_PRIVATE,
				  (void *)&ch);
		pthread_mutex_lock(&ch->lock);

		if (cm->msg == CURLMSG_DONE) {
			if (cm->data.result == CURLE_OK) {
				LOG("%s succeeded\n", ch->url);

				pthread_mutex_unlock(&ch->lock);
				{
					ch->status = CHSTAT_FETCHED;
					parse(ch);
				}
				pthread_mutex_lock(&ch->lock);
			} else {
				ch->status = CHSTAT_ERROR;
				ch->errstr = strdup(ch->errbuf);
				LOGERR("%s failed: %s\n", ch->url, ch->errstr);
			}

			curl_easy_cleanup(cm->easy_handle);
			curl_multi_remove_handle(s_curlm, cm->easy_handle);
		}
		pthread_mutex_unlock(&ch->lock);
	}

	if (running == 0) {
		s_running = 0;
		return FETCH_UPD_DONE;
	}

	return FETCH_UPD_RUNNING;
}

int fetch_poll(void)
{
	int numfds, mask;
	struct curl_waitfd cwf;

	if (!s_initialized) {
		init();
	}

	cwf.fd = STDIN_FILENO;
	cwf.events = CURL_WAIT_POLLIN;
	cwf.revents = 0;

	curl_multi_poll(s_curlm, &cwf, 1, 500, &numfds);

	mask = 0;

	if (numfds > 0) {
		mask |= POLL_FETCH_AVAIL;
	}

	if (cwf.revents) {
		mask |= POLL_STDIN_AVAIL;
	}

	return mask;
}

int fetch_is_running(void)
{
	return s_running;
}

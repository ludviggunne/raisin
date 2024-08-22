#ifndef FETCH_H_INCLUDED
#define FETCH_H_INCLUDED

enum {
	POLL_STDIN_AVAIL = 1,
	POLL_FETCH_AVAIL = 2,
};

enum {
	FETCH_UPD_RUNNING = 0,
	FETCH_UPD_DONE,
};

enum {
	FETCHOPT_NONE = 0, FETCHOPT_FORCE = 1,
};

/*
 * Start a new transfer.
 */
void fetch_begin(int opts);

/*
 * Check for transfer events and update `g_channels`
 * accordingly.
 */
int fetch_update(void);

/*
 * Wether channel content is currently being transfered.
 */
int fetch_is_running(void);

/*
 * Polling is done through the fetch interface because it relies on
 * `curl_multi_poll` in order to get updates from transfers.
 * Returns a bitmask of the POLL_(..)_AVAIL flags.
 * This run through the whole main loop, even when `fetch_is_running`
 * returns 0.
 */
int fetch_poll(void);

#endif				/* FETCH_H_INCLUDED */

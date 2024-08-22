#include <stdio.h>
#include <stdlib.h>
#include "fetch.h"
#include "tui.h"
#include "config.h"
// #include "log.h"

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	if (load_cfg() < 0) {
		fprintf(stderr, "error: %s\n", load_cfg_errstr());
		return 1;
	}

	tui_init();
	atexit(tui_end);
	tui_refresh();

	while (!tui_want_quit()) {

		int mask = fetch_poll();

		if ( /*(mask & POLL_FETCH_AVAIL) && */ fetch_is_running()) {
			fetch_update();
		}

		if (mask & POLL_STDIN_AVAIL) {
			tui_handle_input();
		}

		tui_refresh();
	}
}

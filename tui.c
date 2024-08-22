#include "tui.h"
#include "channel.h"
#include "fetch.h"
#include <assert.h>

#include <wchar.h>
extern int wcwidth(wchar_t c);
#define TB_IMPL
#include "termbox2.h"

enum {
	VIEW_ALL,
	VIEW_CHANNEL,
	VIEW_ITEM,
	VIEW_HELP,
};

static size_t s_selected_chan = 0;
static size_t s_selected_item = 0;
static size_t s_scroll_offset = 0;
static int s_view = VIEW_ALL;
static int s_want_quit = 0;

void tui_init(void)
{
	tb_init();
	tb_hide_cursor();
	tb_clear();
}

void tui_end(void)
{
	tb_shutdown();
}

static int print_wrapped(int y, uintattr_t fg, uintattr_t bg, const char *str)
{
	// int w = tb_width();
	// int len = strlen(str);

	// for (;;) {
	//      if (len > w) {
	//              tb_printf(0, y, fg, bg, "%.*s", w, str);
	//              len -= w;
	//              str += w;
	//              y++;
	//              continue;
	//      }

	//      tb_print(0, y, fg, bg, str);
	//      break;
	// }

	// return y;

	int w = tb_width();
	uint32_t c;
	int x = 0, cw;

	while (*str) {
		cw = tb_utf8_char_to_unicode(&c, str);
		tb_set_cell(x, y, c, fg, bg);
		str += cw;

		x++;
		if (x == w) {
			x = 0;
			y++;
		}
	}

	return 0;
}

static void view_all(void)
{
	size_t i;
	for (i = 0; i < g_nchannels; ++i) {
		if (i < s_scroll_offset)
			continue;
		struct channel *ch = &g_channels[i];
		pthread_mutex_lock(&ch->lock);

		int x = 0, y = i;

		if (i == s_selected_chan) {
			tb_print(0, y, TB_DEFAULT, TB_DEFAULT, ">> ");
		} else {
			tb_print(0, y, TB_DEFAULT, TB_DEFAULT, "   ");
		}

		x = 3;

		switch (ch->status) {
		case CHSTAT_INIT:
			tb_printf(x, y, TB_DEFAULT, TB_DEFAULT, "%s", ch->url);
			break;
		case CHSTAT_FETCHING:
			tb_printf(x, y, TB_DEFAULT, TB_DEFAULT,
				  "%s: downloading: %d%%", ch->url, ch->prog);
			break;
		case CHSTAT_FETCHED:
			tb_printf(x, y, TB_DEFAULT, TB_DEFAULT,
				  "%s: downloaded", ch->url);
			break;
		case CHSTAT_PARSING:
			tb_printf(x, y, TB_DEFAULT, TB_DEFAULT,
				  "%s: parsing...", ch->url);
			break;
		case CHSTAT_DONE:
			tb_printf(x, y, TB_DEFAULT, TB_DEFAULT, "%s",
				  ch->title);
			break;
		case CHSTAT_ERROR:
			tb_printf(x, y, TB_RED, TB_DEFAULT, "%s: error: %s",
				  ch->url, ch->errstr);
			break;
		}

		pthread_mutex_unlock(&ch->lock);
	}
}

void view_channel(void)
{
	size_t i;
	struct channel *ch;
	struct ch_item *it;
	int x, y;

	ch = &g_channels[s_selected_chan];
	pthread_mutex_lock(&ch->lock);
	{
		x = 0;
		y = 0;
		tb_print(x, y, TB_DEFAULT, TB_DEFAULT, ch->title);

		for (i = 0; i < ch->numitems; ++i) {
			it = &ch->items[i];
			x = 0;
			y = i + 1;

			if (i == s_selected_item) {
				tb_print(x, y, TB_DEFAULT, TB_DEFAULT, ">> ");
			} else {
				tb_print(x, y, TB_DEFAULT, TB_DEFAULT, "   ");
			}

			x = 3;
			tb_print(x, y, TB_DEFAULT, TB_DEFAULT, it->title);
		}
	}
	pthread_mutex_unlock(&ch->lock);
}

static void view_item(void)
{
	struct channel *ch;
	struct ch_item *it;

	ch = &g_channels[s_selected_chan];
	it = &ch->items[s_selected_item];

	tb_print(0, 0, TB_DEFAULT, TB_DEFAULT,
		 it->title ? it->title : "no title");
	tb_print(0, 1, TB_DEFAULT, TB_DEFAULT, it->link ? it->link : "no link");
	tb_print(0, 2, TB_DEFAULT, TB_DEFAULT,
		 it->pubdate ? it->pubdate : "no publication date");
	print_wrapped(4, TB_DEFAULT, TB_DEFAULT,
		      it->descr ? it->descr : "no description");
}

void tui_refresh(void)
{
	tb_clear();
	switch (s_view) {
	case VIEW_ALL:
		view_all();
		break;
	case VIEW_CHANNEL:
		view_channel();
		break;
	case VIEW_ITEM:
		view_item();
		break;
	default:
		assert(0);
	}
	tb_present();
}

static void down(void)
{
	struct channel *ch;
	switch (s_view) {
	case VIEW_ALL:
		if (s_selected_chan < g_nchannels - 1) {
			s_selected_chan++;
		}
		break;
	case VIEW_CHANNEL:
		ch = &g_channels[s_selected_chan];
		if (s_selected_item < ch->numitems - 1) {
			s_selected_item++;
		}
		break;
	default:
		break;
	}
}

static void up(void)
{
	switch (s_view) {
	case VIEW_ALL:
		if (s_selected_chan > 0) {
			s_selected_chan--;
		}
		break;
	case VIEW_CHANNEL:
		if (s_selected_item > 0) {
			s_selected_item--;
		}
		break;
	default:
		break;
	}
}

static void quit(void)
{
	switch (s_view) {
	case VIEW_ALL:
		s_want_quit = 1;
		break;
	case VIEW_CHANNEL:
		s_view = VIEW_ALL;
		break;
	case VIEW_ITEM:
		s_view = VIEW_CHANNEL;
		break;
	default:
		break;

	}
}

static void reload(void)
{
	if (s_view == VIEW_ALL && !fetch_is_running()) {
		fetch_begin(FETCHOPT_NONE);
	}
}

static void reload_force(void)
{
	if (s_view == VIEW_ALL && !fetch_is_running()) {
		fetch_begin(FETCHOPT_FORCE);
	}
}

static void open_(void)
{
	struct channel *ch;
	switch (s_view) {
	case VIEW_ALL:
		ch = &g_channels[s_selected_chan];
		pthread_mutex_lock(&ch->lock);
		if (ch->status == CHSTAT_DONE) {
			s_view = VIEW_CHANNEL;
			s_selected_item = 0;
		}
		pthread_mutex_unlock(&ch->lock);
		break;
	case VIEW_CHANNEL:
		s_view = VIEW_ITEM;
		break;
	default:
		break;
	}
}

void tui_handle_input(void)
{
	struct tb_event e;

	tb_poll_event(&e);

	if (e.type == TB_EVENT_KEY) {
		switch (e.key ^ e.ch) {
		case 'j':
		case TB_KEY_ARROW_DOWN:
			down();
			break;
		case 'k':
		case TB_KEY_ARROW_UP:
			up();
			break;
		case 'q':
		case TB_KEY_ESC:
			quit();
			break;
		case 'h':
			if (s_view != VIEW_ALL)
				quit();
			break;
		case 'R':
			reload_force();
			break;
		case 'r':
			reload();
			break;
		case 'o':
		case 'l':
		case TB_KEY_ENTER:
			open_();
			break;
		default:
			break;
		}
	}
}

int tui_want_quit(void)
{
	return s_want_quit;
}

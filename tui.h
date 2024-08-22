#ifndef TUI_H_INCLUDED
#define TUI_H_INCLUDED

void tui_init(void);
void tui_refresh(void);
void tui_handle_input(void);
void tui_end(void);
int tui_want_quit(void);
const char *tui_errstr(void);

#endif

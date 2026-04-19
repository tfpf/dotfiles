#ifndef FOCUS_UTILS_HH_
#define FOCUS_UTILS_HH_

/*
 * On Windows, obtain the ID of the active window. On Linux and macOS, do
 * nothing.
 *
 * @return Active window ID or unspecified value (as described above).
 */
long long unsigned get_active_wid(void);

/*
 * On Linux and macOS, check whether the terminal has GUI focus. On Windows, do
 * nothing.
 *
 * @return Focus state or unspecified value (as described above).
 */
bool terminal_has_focus(void);

#endif

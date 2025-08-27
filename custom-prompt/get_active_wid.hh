#ifndef GET_ACTIVE_WID_HH_
#define GET_ACTIVE_WID_HH_

#ifdef __APPLE__
/*
 * Get the ID of the currently-focused window.
 *
 * @return Active window ID. If no topmost window is found, 0 is returned.
 */
extern "C" long long unsigned get_active_wid(void);
#else
/*
 * Get the ID of the currently-focused window.
 *
 * @return Active window ID. On Linux, if there is no X display running, 0 is
 * returned.
 */
long long unsigned get_active_wid(void);
#endif

#endif

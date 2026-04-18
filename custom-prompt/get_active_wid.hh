#ifndef GET_ACTIVE_WID_HH_
#define GET_ACTIVE_WID_HH_

/*
 * Get the ID of the currently-focused window.
 *
 * On Linux, this always returns 0.
 *
 * @return Active window ID.
 */
long long unsigned get_active_wid(void);

#endif

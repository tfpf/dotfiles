#ifndef GET_ACTIVE_WID_HH_
#define GET_ACTIVE_WID_HH_

// clang-format off

#ifdef __APPLE__
extern "C" {
#endif

/*
 * On Linux, return the ID of the active window if X is running, else 0.
 *
 * On macOS, return the ID of the topmost window if one is found, else 0.
 *
 * On Windows, return the ID of the foreground window.
 *
 * @return Active window ID.
 */
long long unsigned get_active_wid(void);

#ifdef __APPLE__
}
#endif

#endif

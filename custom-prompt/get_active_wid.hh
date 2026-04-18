#ifndef GET_ACTIVE_WID_HH_
#define GET_ACTIVE_WID_HH_

// clang-format off
#ifdef __cplusplus
extern "C"
{
#endif

/*
 * Get the ID of the currently-focused window.
 *
 * On Linux, this always returns 0.
 *
 * @return Active window ID.
 */
long long unsigned get_active_wid(void);

#ifdef __cplusplus
}
#endif

#endif

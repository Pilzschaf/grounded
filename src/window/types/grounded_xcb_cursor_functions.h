/*
 * This file uses the X-List pattern to define all required xcb cursor functions
 */

X(xcb_cursor_load_cursor, xcb_cursor_t, (xcb_cursor_context_t *ctx, const char *name))
X(xcb_cursor_context_new, int, (xcb_connection_t *conn, xcb_screen_t *screen, xcb_cursor_context_t **ctx))
X(xcb_cursor_context_free, void, (xcb_cursor_context_t *ctx))

/*
 * This file uses the X-List pattern to define all required wayland cursor functions
 */

X(wl_cursor_theme_load, struct wl_cursor_theme*, (const char *name, int size, struct wl_shm *shm))
X(wl_cursor_theme_destroy, void, (struct wl_cursor_theme *theme))
X(wl_cursor_theme_get_cursor, struct wl_cursor*, (struct wl_cursor_theme* theme, const char *name))
X(wl_cursor_image_get_buffer, struct wl_buffer*, (struct wl_cursor_image* image))

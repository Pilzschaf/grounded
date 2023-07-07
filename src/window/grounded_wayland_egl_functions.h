/*
 * This file uses the X-List pattern to define all required wayland egl functions
 */

X(wl_egl_window_create, struct wl_egl_window*, (struct wl_surface *surface, int width, int height))
X(wl_egl_window_resize, void, (struct wl_egl_window *egl_window, int width, int height, int dx, int dy))
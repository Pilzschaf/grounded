/*
 * This file uses the X-List pattern to define all required wayland egl functions
 */

X(wl_egl_window_create, struct wl_egl_window*, (struct wl_surface *surface, int width, int height))

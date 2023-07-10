/*
 * This file uses the X-List pattern to define all required wayland functions
 */

X(wl_display_connect, struct wl_display*, (const char *name))
X(wl_display_get_fd, int, (struct wl_display* display))
X(wl_proxy_marshal_constructor, struct wl_proxy*, (struct wl_proxy *proxy, uint32_t opcode, const struct wl_interface *interface, ...))
X(wl_proxy_add_listener, int, (struct wl_proxy* proxy, void (**implementation)(void), void *data))
X(wl_proxy_marshal_constructor_versioned, struct wl_proxy*, (struct wl_proxy *proxy, uint32_t opcode, const struct wl_interface *interface, uint32_t version, ...))
X(wl_display_roundtrip, int, (struct wl_display *display))
X(wl_display_dispatch_pending, int, (struct wl_display *display))
X(wl_proxy_marshal, void, (struct wl_proxy *p, uint32_t opcode, ...))
X(wl_proxy_destroy, void, (struct wl_proxy *proxy))
X(wl_proxy_set_user_data, void, (struct wl_proxy *proxy, void *user_data))
X(wl_proxy_get_version, uint32_t, (struct wl_proxy *proxy))
X(wl_proxy_get_user_data, void*, (struct wl_proxy *proxy))
X(wl_display_dispatch, int, (struct wl_display *display))
X(wl_proxy_marshal_flags, struct wl_proxy*, (struct wl_proxy *proxy, uint32_t opcode, const struct wl_interface *interface, uint32_t version, uint32_t flags, ...))
X(wl_display_prepare_read, int, (struct wl_display *display))
X(wl_display_flush, int, (struct wl_display *display))

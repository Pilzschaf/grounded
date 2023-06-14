// Dummy file so the created wayland definitions do not rely on system header files

#ifndef WAYLAND_UTIL_H
#define WAYLAND_UTIL_H

struct wl_interface {
	/** Interface name */
	const char *name;
	/** Interface version */
	int version;
	/** Number of methods (requests) */
	int method_count;
	/** Method (request) signatures */
	const struct wl_message *methods;
	/** Number of events */
	int event_count;
	/** Event signatures */
	const struct wl_message *events;
};

struct wl_message {
	/** Message name */
	const char *name;
	/** Message signature */
	const char *signature;
	/** Object argument interfaces */
	const struct wl_interface **types;
};

#endif // WAYLAND_UTIL_H
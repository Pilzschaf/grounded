
#define WL_DISPLAY_GET_REGISTRY 1
#define WL_COMPOSITOR_CREATE_SURFACE 0
#define WL_REGISTRY_BIND 0

#define WL_SURFACE_DESTROY 0
#define WL_SURFACE_ATTACH 1
#define WL_SURFACE_DAMAGE 2
#define WL_SURFACE_COMMIT 6
#define WL_SURFACE_SET_BUFFER_SCALE 8

#define WL_SEAT_GET_POINTER 0
#define WL_SEAT_GET_KEYBOARD 1
#define WL_SEAT_GET_TOUCH 2
#define WL_SEAT_RELEASE 3

#define WL_POINTER_SET_CURSOR 0

#define WL_MARSHAL_FLAG_DESTROY (1 << 0)

#define wl_array_for_each(pos, array)					\
	for (pos = (array)->data;					\
	     (const char *) pos < ((const char *) (array)->data + (array)->size); \
	     (pos)++)

typedef int32_t wl_fixed_t;

struct wl_registry_listener {
	void (*global)(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version);
	void (*global_remove)(void *data, struct wl_registry *wl_registry, uint32_t name);
};

struct wl_seat_listener {
	void (*capabilities)(void *data, struct wl_seat *wl_seat, uint32_t capabilities);
	void (*name)(void *data, struct wl_seat *wl_seat, const char *name);
};

struct wl_keyboard_listener {
	void (*keymap)(void *data, struct wl_keyboard *wl_keyboard, uint32_t format, int32_t fd, uint32_t size);
	void (*enter)(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys);
	void (*leave)(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface);
	void (*key)(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
	void (*modifiers)(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
	void (*repeat_info)(void *data, struct wl_keyboard *wl_keyboard, int32_t rate, int32_t delay);
};

struct wl_pointer_listener {
	void (*enter)(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
	void (*leave)(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface);
	void (*motion)(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);
	void (*button)(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
	void (*axis)(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
	void (*frame)(void *data, struct wl_pointer *wl_pointer);
	void (*axis_source)(void *data, struct wl_pointer *wl_pointer, uint32_t axis_source);
	void (*axis_stop)(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis);
	void (*axis_discrete)(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t discrete);
	void (*axis_value120)(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t value120);
};

enum wl_keyboard_key_state {
	WL_KEYBOARD_KEY_STATE_RELEASED = 0,
	WL_KEYBOARD_KEY_STATE_PRESSED = 1,
};

enum wl_seat_capability {
	WL_SEAT_CAPABILITY_POINTER = 1,
	WL_SEAT_CAPABILITY_KEYBOARD = 2,
	WL_SEAT_CAPABILITY_TOUCH = 4,
};

static inline struct wl_registry* wl_display_get_registry(struct wl_display *wl_display) {
	struct wl_proxy *registry;

	registry = wl_proxy_marshal_constructor((struct wl_proxy *) wl_display, WL_DISPLAY_GET_REGISTRY, wl_registry_interface, 0);

	return (struct wl_registry *) registry;
}

static inline struct wl_surface* wl_compositor_create_surface(struct wl_compositor *wl_compositor) {
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) wl_compositor, WL_COMPOSITOR_CREATE_SURFACE, wl_surface_interface, 0);

	return (struct wl_surface *) id;
}

static inline int wl_registry_add_listener(struct wl_registry *wl_registry, const struct wl_registry_listener *listener, void *data) {
	return wl_proxy_add_listener((struct wl_proxy *) wl_registry, (void (**)(void)) listener, data);
}

static inline void* wl_registry_bind(struct wl_registry *wl_registry, uint32_t name, const struct wl_interface *interface, uint32_t version) {
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor_versioned((struct wl_proxy *) wl_registry, WL_REGISTRY_BIND, interface, version, name, interface->name, version, 0);

	return (void *) id;
}

static inline void wl_surface_destroy(struct wl_surface *wl_surface) {
	wl_proxy_marshal((struct wl_proxy *) wl_surface, WL_SURFACE_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) wl_surface);
}

static inline void wl_surface_commit(struct wl_surface *wl_surface) {
	wl_proxy_marshal((struct wl_proxy *) wl_surface, WL_SURFACE_COMMIT);
}

static inline int wl_keyboard_add_listener(struct wl_keyboard *wl_keyboard, const struct wl_keyboard_listener *listener, void *data) {
	return wl_proxy_add_listener((struct wl_proxy *) wl_keyboard, (void (**)(void)) listener, data);
}

static inline void wl_keyboard_destroy(struct wl_keyboard *wl_keyboard) {
	wl_proxy_destroy((struct wl_proxy *) wl_keyboard);
}

static inline int wl_pointer_add_listener(struct wl_pointer *wl_pointer, const struct wl_pointer_listener *listener, void *data) {
	return wl_proxy_add_listener((struct wl_proxy *) wl_pointer, (void (**)(void)) listener, data);
}

#if 0
static inline struct wl_keyboard * wl_seat_get_keyboard(struct wl_seat *wl_seat) {
	struct wl_proxy *id;
	id = wl_proxy_marshal_flags((struct wl_proxy *) wl_seat, WL_SEAT_GET_KEYBOARD, &wl_keyboard_interface, wl_proxy_get_version((struct wl_proxy *) wl_seat), 0, NULL);
	return (struct wl_keyboard *) id;
}

static inline struct wl_pointer * wl_seat_get_pointer(struct wl_seat *wl_seat) {
	struct wl_proxy *id;
	id = wl_proxy_marshal_flags((struct wl_proxy *) wl_seat, WL_SEAT_GET_POINTER, &wl_pointer_interface, wl_proxy_get_version((struct wl_proxy *) wl_seat), 0, NULL);
	return (struct wl_pointer *) id;
}
#else
static inline struct wl_pointer * wl_seat_get_pointer(struct wl_seat *wl_seat) {
	struct wl_proxy *id;
	id = wl_proxy_marshal_constructor((struct wl_proxy *) wl_seat, WL_SEAT_GET_POINTER, wl_pointer_interface, 0);
	return (struct wl_pointer *) id;
}
static inline struct wl_keyboard* wl_seat_get_keyboard(struct wl_seat *wl_seat) {
	struct wl_proxy *id;
	id = wl_proxy_marshal_constructor((struct wl_proxy *) wl_seat, WL_SEAT_GET_KEYBOARD, wl_keyboard_interface, 0);
	return (struct wl_keyboard *) id;
}
#endif

static inline void wl_pointer_destroy(struct wl_pointer *wl_pointer) {
	wl_proxy_destroy((struct wl_proxy *) wl_pointer);
}

static inline int wl_seat_add_listener(struct wl_seat *wl_seat, const struct wl_seat_listener *listener, void *data) {
	return wl_proxy_add_listener((struct wl_proxy *) wl_seat, (void (**)(void)) listener, data);
}

// https://wayland.dpldocs.info/source/wayland.native.util.d.html#L576
static inline int wl_fixed_to_int(wl_fixed_t f){
    return f / 256;
}

wl_fixed_t wl_fixed_from_int(int i) {
    return i * 256;
}

wl_fixed_t wl_fixed_from_double(double d) {
    union di {
        double d;
        long i;
    };
    union di u;
    u.d = d + (3L << (51 - 8));
    return (wl_fixed_t)u.i;
}

double wl_fixed_to_double (wl_fixed_t f){
    union di {
        double d;
        long i;
    };
    union di u;
    u.i = ((1023L + 44L) << 52) + (1L << 51) + f;
    return u.d - (3L << 43);
}

static inline void wl_pointer_set_cursor(struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, int32_t hotspot_x, int32_t hotspot_y) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_pointer, WL_POINTER_SET_CURSOR, NULL, wl_proxy_get_version((struct wl_proxy *) wl_pointer), 0, serial, surface, hotspot_x, hotspot_y);
}

static inline void wl_surface_set_buffer_scale(struct wl_surface *wl_surface, int32_t scale) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_surface, WL_SURFACE_SET_BUFFER_SCALE, NULL, wl_proxy_get_version((struct wl_proxy *) wl_surface), 0, scale);
}

static inline void wl_surface_attach(struct wl_surface *wl_surface, struct wl_buffer *buffer, int32_t x, int32_t y) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_surface, WL_SURFACE_ATTACH, NULL, wl_proxy_get_version((struct wl_proxy *) wl_surface), 0, buffer, x, y);
}

static inline void wl_surface_damage(struct wl_surface *wl_surface, int32_t x, int32_t y, int32_t width, int32_t height) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_surface, WL_SURFACE_DAMAGE, NULL, wl_proxy_get_version((struct wl_proxy *) wl_surface), 0, x, y, width, height);
}

struct wl_cursor_image {
	/** Actual width */
	uint32_t width;

	/** Actual height */
	uint32_t height;

	/** Hot spot x (must be inside image) */
	uint32_t hotspot_x;

	/** Hot spot y (must be inside image) */
	uint32_t hotspot_y;

	/** Animation delay to next frame (ms) */
	uint32_t delay;
};

struct wl_cursor {
	/** How many images there are in this cursor’s animation */
	unsigned int image_count;

	/** The array of still images composing this animation */
	struct wl_cursor_image **images;

	/** The name of this cursor */
	char *name;
};

struct wl_array {
	/** Array size */
	size_t size;
	/** Allocated space */
	size_t alloc;
	/** Array data */
	void *data;
};

struct wl_message {
	/** Message name */
	const char *name;
	/** Message signature */
	const char *signature;
	/** Object argument interfaces */
	const struct wl_interface **types;
};
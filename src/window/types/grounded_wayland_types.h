
#define WL_DISPLAY_GET_REGISTRY 1

#define WL_REGISTRY_BIND 0

#define WL_COMPOSITOR_CREATE_SURFACE 0
#define WL_COMPOSITOR_CREATE_REGION 1

#define WL_REGION_DESTROY 0
#define WL_REGION_ADD 1

#define WL_SURFACE_DESTROY 0
#define WL_SURFACE_ATTACH 1
#define WL_SURFACE_DAMAGE 2
#define WL_SURFACE_SET_INPUT_REGION 5
#define WL_SURFACE_COMMIT 6
#define WL_SURFACE_SET_BUFFER_SCALE 8

#define WL_SEAT_GET_POINTER 0
#define WL_SEAT_GET_KEYBOARD 1
#define WL_SEAT_GET_TOUCH 2
#define WL_SEAT_RELEASE 3

#define WL_POINTER_SET_CURSOR 0

#define WL_MARSHAL_FLAG_DESTROY (1 << 0)

#define WL_SHM_CREATE_POOL 0
#define WL_SHM_POOL_CREATE_BUFFER 0
#define WL_SHM_POOL_DESTROY 1

#define WL_DATA_DEVICE_MANAGER_CREATE_DATA_SOURCE 0
#define WL_DATA_DEVICE_MANAGER_GET_DATA_DEVICE 1

#define WL_DATA_DEVICE_START_DRAG 0

#define WL_DATA_OFFER_ACCEPT 0
#define WL_DATA_OFFER_RECEIVE 1
#define WL_DATA_OFFER_DESTROY 2
#define WL_DATA_OFFER_FINISH 3

#define WL_DATA_SOURCE_OFFER 0
#define WL_DATA_SOURCE_DESTROY 1
#define WL_DATA_SOURCE_SET_ACTIONS 2

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

struct wl_data_device_listener {
	void (*data_offer)(void *data, struct wl_data_device *wl_data_device, struct wl_data_offer *id);
	void (*enter)(void *data, struct wl_data_device *wl_data_device, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer *id);
	void (*leave)(void *data, struct wl_data_device *wl_data_device);
	void (*motion)(void *data, struct wl_data_device *wl_data_device, uint32_t time, wl_fixed_t x, wl_fixed_t y);
	void (*drop)(void *data, struct wl_data_device *wl_data_device);
	void (*selection)(void *data, struct wl_data_device *wl_data_device, struct wl_data_offer *id);
};

struct wl_data_offer_listener {
	void (*offer)(void *data, struct wl_data_offer *wl_data_offer, const char *mime_type);
	void (*source_actions)(void *data, struct wl_data_offer *wl_data_offer, uint32_t source_actions);
	void (*action)(void *data, struct wl_data_offer *wl_data_offer, uint32_t dnd_action);
};

struct wl_data_source_listener {
	void (*target)(void *data, struct wl_data_source *wl_data_source, const char *mime_type);
	void (*send)(void *data, struct wl_data_source *wl_data_source, const char *mime_type, int32_t fd);
	void (*cancelled)(void *data, struct wl_data_source *wl_data_source);
	void (*dnd_drop_performed)(void *data, struct wl_data_source *wl_data_source);
	void (*dnd_finished)(void *data, struct wl_data_source *wl_data_source);
	void (*action)(void *data, struct wl_data_source *wl_data_source, uint32_t dnd_action);
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

enum wl_data_device_manager_dnd_action {
	WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE = 0,
	WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY = 1,
	WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE = 2,
	WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK = 4,
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

static inline struct wl_region* wl_compositor_create_region(struct wl_compositor *wl_compositor) {
	struct wl_proxy *id;

	id = wl_proxy_marshal_flags((struct wl_proxy *) wl_compositor,
			 WL_COMPOSITOR_CREATE_REGION, wl_region_interface, wl_proxy_get_version((struct wl_proxy *) wl_compositor), 0, NULL);

	return (struct wl_region *) id;
}

static inline void wl_region_add(struct wl_region *wl_region, int32_t x, int32_t y, int32_t width, int32_t height) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_region,
			 WL_REGION_ADD, NULL, wl_proxy_get_version((struct wl_proxy *) wl_region), 0, x, y, width, height);
}

static inline void wl_region_destroy(struct wl_region *wl_region) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_region,
			 WL_REGION_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *) wl_region), WL_MARSHAL_FLAG_DESTROY);
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

static inline void wl_surface_set_user_data(struct wl_surface *wl_surface, void *user_data) {
	wl_proxy_set_user_data((struct wl_proxy *) wl_surface, user_data);
}

static inline void* wl_surface_get_user_data(struct wl_surface *wl_surface) {
	return wl_proxy_get_user_data((struct wl_proxy *) wl_surface);
}

static inline void wl_surface_set_input_region(struct wl_surface *wl_surface, struct wl_region *region) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_surface,
			 WL_SURFACE_SET_INPUT_REGION, NULL, wl_proxy_get_version((struct wl_proxy *) wl_surface), 0, region);
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

static inline struct wl_shm_pool* wl_shm_create_pool(struct wl_shm *wl_shm, int32_t fd, int32_t size) {
	struct wl_proxy *id;

	id = wl_proxy_marshal_flags((struct wl_proxy *) wl_shm,
			 WL_SHM_CREATE_POOL, wl_shm_pool_interface, wl_proxy_get_version((struct wl_proxy *) wl_shm), 0, NULL, fd, size);

	return (struct wl_shm_pool *) id;
}

static inline struct wl_buffer* wl_shm_pool_create_buffer(struct wl_shm_pool *wl_shm_pool, int32_t offset, int32_t width, int32_t height, int32_t stride, uint32_t format){
	struct wl_proxy *id;

	id = wl_proxy_marshal_flags((struct wl_proxy *) wl_shm_pool,
			 WL_SHM_POOL_CREATE_BUFFER, wl_buffer_interface, wl_proxy_get_version((struct wl_proxy *) wl_shm_pool), 0, NULL, offset, width, height, stride, format);

	return (struct wl_buffer *) id;
}

static inline void wl_shm_pool_destroy(struct wl_shm_pool *wl_shm_pool) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_shm_pool, WL_SHM_POOL_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *) wl_shm_pool), WL_MARSHAL_FLAG_DESTROY);
}

static inline struct wl_data_source * wl_data_device_manager_create_data_source(struct wl_data_device_manager *wl_data_device_manager) {
	struct wl_proxy *id;

	id = wl_proxy_marshal_flags((struct wl_proxy *) wl_data_device_manager,
			 WL_DATA_DEVICE_MANAGER_CREATE_DATA_SOURCE, wl_data_source_interface, wl_proxy_get_version((struct wl_proxy *) wl_data_device_manager), 0, NULL);

	return (struct wl_data_source *) id;
}

static inline struct wl_data_device * wl_data_device_manager_get_data_device(struct wl_data_device_manager *wl_data_device_manager, struct wl_seat *seat) {
	struct wl_proxy *id;

	id = wl_proxy_marshal_flags((struct wl_proxy *) wl_data_device_manager,
			 WL_DATA_DEVICE_MANAGER_GET_DATA_DEVICE, wl_data_device_interface, wl_proxy_get_version((struct wl_proxy *) wl_data_device_manager), 0, NULL, seat);

	return (struct wl_data_device *) id;
}

static inline void wl_data_device_start_drag(struct wl_data_device *wl_data_device, struct wl_data_source *source, struct wl_surface *origin, struct wl_surface *icon, uint32_t serial) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_data_device,
			 WL_DATA_DEVICE_START_DRAG, NULL, wl_proxy_get_version((struct wl_proxy *) wl_data_device), 0, source, origin, icon, serial);
}

static inline int wl_data_device_add_listener(struct wl_data_device *wl_data_device, const struct wl_data_device_listener *listener, void *data) {
	return wl_proxy_add_listener((struct wl_proxy *) wl_data_device,
				     (void (**)(void)) listener, data);
}

static inline void wl_data_offer_accept(struct wl_data_offer *wl_data_offer, uint32_t serial, const char *mime_type) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_data_offer,
			 WL_DATA_OFFER_ACCEPT, NULL, wl_proxy_get_version((struct wl_proxy *) wl_data_offer), 0, serial, mime_type);
}

static inline int wl_data_offer_add_listener(struct wl_data_offer *wl_data_offer, const struct wl_data_offer_listener *listener, void *data) {
	return wl_proxy_add_listener((struct wl_proxy *) wl_data_offer, (void (**)(void)) listener, data);
}

static inline void wl_data_offer_receive(struct wl_data_offer *wl_data_offer, const char *mime_type, int32_t fd) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_data_offer,
			 WL_DATA_OFFER_RECEIVE, NULL, wl_proxy_get_version((struct wl_proxy *) wl_data_offer), 0, mime_type, fd);
}

static inline void wl_data_offer_finish(struct wl_data_offer *wl_data_offer) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_data_offer,
			 WL_DATA_OFFER_FINISH, NULL, wl_proxy_get_version((struct wl_proxy *) wl_data_offer), 0);
}

static inline void wl_data_offer_destroy(struct wl_data_offer *wl_data_offer) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_data_offer, WL_DATA_OFFER_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *) wl_data_offer), WL_MARSHAL_FLAG_DESTROY);
}

static inline void * wl_data_offer_get_user_data(struct wl_data_offer *wl_data_offer) {
	return wl_proxy_get_user_data((struct wl_proxy *) wl_data_offer);
}


static inline int wl_data_source_add_listener(struct wl_data_source *wl_data_source, const struct wl_data_source_listener *listener, void *data) {
	return wl_proxy_add_listener((struct wl_proxy *) wl_data_source, (void (**)(void)) listener, data);
}

static inline void wl_data_source_offer(struct wl_data_source *wl_data_source, const char *mime_type) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_data_source, WL_DATA_SOURCE_OFFER, NULL, wl_proxy_get_version((struct wl_proxy *) wl_data_source), 0, mime_type);
}

static inline void wl_data_source_destroy(struct wl_data_source *wl_data_source) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_data_source, WL_DATA_SOURCE_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *) wl_data_source), WL_MARSHAL_FLAG_DESTROY);
}

static inline void wl_data_source_set_actions(struct wl_data_source *wl_data_source, uint32_t dnd_actions) {
	wl_proxy_marshal_flags((struct wl_proxy *) wl_data_source, WL_DATA_SOURCE_SET_ACTIONS, NULL, wl_proxy_get_version((struct wl_proxy *) wl_data_source), 0, dnd_actions);
}

#ifndef WL_SHM_FORMAT_ENUM
#define WL_SHM_FORMAT_ENUM
/**
 * @ingroup iface_wl_shm
 * pixel formats
 *
 * This describes the memory layout of an individual pixel.
 *
 * All renderers should support argb8888 and xrgb8888 but any other
 * formats are optional and may not be supported by the particular
 * renderer in use.
 *
 * The drm format codes match the macros defined in drm_fourcc.h, except
 * argb8888 and xrgb8888. The formats actually supported by the compositor
 * will be reported by the format event.
 *
 * For all wl_shm formats and unless specified in another protocol
 * extension, pre-multiplied alpha is used for pixel values.
 */
enum wl_shm_format {
	/**
	 * 32-bit ARGB format, [31:0] A:R:G:B 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_ARGB8888 = 0,
	/**
	 * 32-bit RGB format, [31:0] x:R:G:B 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_XRGB8888 = 1,
	/**
	 * 8-bit color index format, [7:0] C
	 */
	WL_SHM_FORMAT_C8 = 0x20203843,
	/**
	 * 8-bit RGB format, [7:0] R:G:B 3:3:2
	 */
	WL_SHM_FORMAT_RGB332 = 0x38424752,
	/**
	 * 8-bit BGR format, [7:0] B:G:R 2:3:3
	 */
	WL_SHM_FORMAT_BGR233 = 0x38524742,
	/**
	 * 16-bit xRGB format, [15:0] x:R:G:B 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_XRGB4444 = 0x32315258,
	/**
	 * 16-bit xBGR format, [15:0] x:B:G:R 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_XBGR4444 = 0x32314258,
	/**
	 * 16-bit RGBx format, [15:0] R:G:B:x 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_RGBX4444 = 0x32315852,
	/**
	 * 16-bit BGRx format, [15:0] B:G:R:x 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_BGRX4444 = 0x32315842,
	/**
	 * 16-bit ARGB format, [15:0] A:R:G:B 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_ARGB4444 = 0x32315241,
	/**
	 * 16-bit ABGR format, [15:0] A:B:G:R 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_ABGR4444 = 0x32314241,
	/**
	 * 16-bit RBGA format, [15:0] R:G:B:A 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_RGBA4444 = 0x32314152,
	/**
	 * 16-bit BGRA format, [15:0] B:G:R:A 4:4:4:4 little endian
	 */
	WL_SHM_FORMAT_BGRA4444 = 0x32314142,
	/**
	 * 16-bit xRGB format, [15:0] x:R:G:B 1:5:5:5 little endian
	 */
	WL_SHM_FORMAT_XRGB1555 = 0x35315258,
	/**
	 * 16-bit xBGR 1555 format, [15:0] x:B:G:R 1:5:5:5 little endian
	 */
	WL_SHM_FORMAT_XBGR1555 = 0x35314258,
	/**
	 * 16-bit RGBx 5551 format, [15:0] R:G:B:x 5:5:5:1 little endian
	 */
	WL_SHM_FORMAT_RGBX5551 = 0x35315852,
	/**
	 * 16-bit BGRx 5551 format, [15:0] B:G:R:x 5:5:5:1 little endian
	 */
	WL_SHM_FORMAT_BGRX5551 = 0x35315842,
	/**
	 * 16-bit ARGB 1555 format, [15:0] A:R:G:B 1:5:5:5 little endian
	 */
	WL_SHM_FORMAT_ARGB1555 = 0x35315241,
	/**
	 * 16-bit ABGR 1555 format, [15:0] A:B:G:R 1:5:5:5 little endian
	 */
	WL_SHM_FORMAT_ABGR1555 = 0x35314241,
	/**
	 * 16-bit RGBA 5551 format, [15:0] R:G:B:A 5:5:5:1 little endian
	 */
	WL_SHM_FORMAT_RGBA5551 = 0x35314152,
	/**
	 * 16-bit BGRA 5551 format, [15:0] B:G:R:A 5:5:5:1 little endian
	 */
	WL_SHM_FORMAT_BGRA5551 = 0x35314142,
	/**
	 * 16-bit RGB 565 format, [15:0] R:G:B 5:6:5 little endian
	 */
	WL_SHM_FORMAT_RGB565 = 0x36314752,
	/**
	 * 16-bit BGR 565 format, [15:0] B:G:R 5:6:5 little endian
	 */
	WL_SHM_FORMAT_BGR565 = 0x36314742,
	/**
	 * 24-bit RGB format, [23:0] R:G:B little endian
	 */
	WL_SHM_FORMAT_RGB888 = 0x34324752,
	/**
	 * 24-bit BGR format, [23:0] B:G:R little endian
	 */
	WL_SHM_FORMAT_BGR888 = 0x34324742,
	/**
	 * 32-bit xBGR format, [31:0] x:B:G:R 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_XBGR8888 = 0x34324258,
	/**
	 * 32-bit RGBx format, [31:0] R:G:B:x 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_RGBX8888 = 0x34325852,
	/**
	 * 32-bit BGRx format, [31:0] B:G:R:x 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_BGRX8888 = 0x34325842,
	/**
	 * 32-bit ABGR format, [31:0] A:B:G:R 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_ABGR8888 = 0x34324241,
	/**
	 * 32-bit RGBA format, [31:0] R:G:B:A 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_RGBA8888 = 0x34324152,
	/**
	 * 32-bit BGRA format, [31:0] B:G:R:A 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_BGRA8888 = 0x34324142,
	/**
	 * 32-bit xRGB format, [31:0] x:R:G:B 2:10:10:10 little endian
	 */
	WL_SHM_FORMAT_XRGB2101010 = 0x30335258,
	/**
	 * 32-bit xBGR format, [31:0] x:B:G:R 2:10:10:10 little endian
	 */
	WL_SHM_FORMAT_XBGR2101010 = 0x30334258,
	/**
	 * 32-bit RGBx format, [31:0] R:G:B:x 10:10:10:2 little endian
	 */
	WL_SHM_FORMAT_RGBX1010102 = 0x30335852,
	/**
	 * 32-bit BGRx format, [31:0] B:G:R:x 10:10:10:2 little endian
	 */
	WL_SHM_FORMAT_BGRX1010102 = 0x30335842,
	/**
	 * 32-bit ARGB format, [31:0] A:R:G:B 2:10:10:10 little endian
	 */
	WL_SHM_FORMAT_ARGB2101010 = 0x30335241,
	/**
	 * 32-bit ABGR format, [31:0] A:B:G:R 2:10:10:10 little endian
	 */
	WL_SHM_FORMAT_ABGR2101010 = 0x30334241,
	/**
	 * 32-bit RGBA format, [31:0] R:G:B:A 10:10:10:2 little endian
	 */
	WL_SHM_FORMAT_RGBA1010102 = 0x30334152,
	/**
	 * 32-bit BGRA format, [31:0] B:G:R:A 10:10:10:2 little endian
	 */
	WL_SHM_FORMAT_BGRA1010102 = 0x30334142,
	/**
	 * packed YCbCr format, [31:0] Cr0:Y1:Cb0:Y0 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_YUYV = 0x56595559,
	/**
	 * packed YCbCr format, [31:0] Cb0:Y1:Cr0:Y0 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_YVYU = 0x55595659,
	/**
	 * packed YCbCr format, [31:0] Y1:Cr0:Y0:Cb0 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_UYVY = 0x59565955,
	/**
	 * packed YCbCr format, [31:0] Y1:Cb0:Y0:Cr0 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_VYUY = 0x59555956,
	/**
	 * packed AYCbCr format, [31:0] A:Y:Cb:Cr 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_AYUV = 0x56555941,
	/**
	 * 2 plane YCbCr Cr:Cb format, 2x2 subsampled Cr:Cb plane
	 */
	WL_SHM_FORMAT_NV12 = 0x3231564e,
	/**
	 * 2 plane YCbCr Cb:Cr format, 2x2 subsampled Cb:Cr plane
	 */
	WL_SHM_FORMAT_NV21 = 0x3132564e,
	/**
	 * 2 plane YCbCr Cr:Cb format, 2x1 subsampled Cr:Cb plane
	 */
	WL_SHM_FORMAT_NV16 = 0x3631564e,
	/**
	 * 2 plane YCbCr Cb:Cr format, 2x1 subsampled Cb:Cr plane
	 */
	WL_SHM_FORMAT_NV61 = 0x3136564e,
	/**
	 * 3 plane YCbCr format, 4x4 subsampled Cb (1) and Cr (2) planes
	 */
	WL_SHM_FORMAT_YUV410 = 0x39565559,
	/**
	 * 3 plane YCbCr format, 4x4 subsampled Cr (1) and Cb (2) planes
	 */
	WL_SHM_FORMAT_YVU410 = 0x39555659,
	/**
	 * 3 plane YCbCr format, 4x1 subsampled Cb (1) and Cr (2) planes
	 */
	WL_SHM_FORMAT_YUV411 = 0x31315559,
	/**
	 * 3 plane YCbCr format, 4x1 subsampled Cr (1) and Cb (2) planes
	 */
	WL_SHM_FORMAT_YVU411 = 0x31315659,
	/**
	 * 3 plane YCbCr format, 2x2 subsampled Cb (1) and Cr (2) planes
	 */
	WL_SHM_FORMAT_YUV420 = 0x32315559,
	/**
	 * 3 plane YCbCr format, 2x2 subsampled Cr (1) and Cb (2) planes
	 */
	WL_SHM_FORMAT_YVU420 = 0x32315659,
	/**
	 * 3 plane YCbCr format, 2x1 subsampled Cb (1) and Cr (2) planes
	 */
	WL_SHM_FORMAT_YUV422 = 0x36315559,
	/**
	 * 3 plane YCbCr format, 2x1 subsampled Cr (1) and Cb (2) planes
	 */
	WL_SHM_FORMAT_YVU422 = 0x36315659,
	/**
	 * 3 plane YCbCr format, non-subsampled Cb (1) and Cr (2) planes
	 */
	WL_SHM_FORMAT_YUV444 = 0x34325559,
	/**
	 * 3 plane YCbCr format, non-subsampled Cr (1) and Cb (2) planes
	 */
	WL_SHM_FORMAT_YVU444 = 0x34325659,
	/**
	 * [7:0] R
	 */
	WL_SHM_FORMAT_R8 = 0x20203852,
	/**
	 * [15:0] R little endian
	 */
	WL_SHM_FORMAT_R16 = 0x20363152,
	/**
	 * [15:0] R:G 8:8 little endian
	 */
	WL_SHM_FORMAT_RG88 = 0x38384752,
	/**
	 * [15:0] G:R 8:8 little endian
	 */
	WL_SHM_FORMAT_GR88 = 0x38385247,
	/**
	 * [31:0] R:G 16:16 little endian
	 */
	WL_SHM_FORMAT_RG1616 = 0x32334752,
	/**
	 * [31:0] G:R 16:16 little endian
	 */
	WL_SHM_FORMAT_GR1616 = 0x32335247,
	/**
	 * [63:0] x:R:G:B 16:16:16:16 little endian
	 */
	WL_SHM_FORMAT_XRGB16161616F = 0x48345258,
	/**
	 * [63:0] x:B:G:R 16:16:16:16 little endian
	 */
	WL_SHM_FORMAT_XBGR16161616F = 0x48344258,
	/**
	 * [63:0] A:R:G:B 16:16:16:16 little endian
	 */
	WL_SHM_FORMAT_ARGB16161616F = 0x48345241,
	/**
	 * [63:0] A:B:G:R 16:16:16:16 little endian
	 */
	WL_SHM_FORMAT_ABGR16161616F = 0x48344241,
	/**
	 * [31:0] X:Y:Cb:Cr 8:8:8:8 little endian
	 */
	WL_SHM_FORMAT_XYUV8888 = 0x56555958,
	/**
	 * [23:0] Cr:Cb:Y 8:8:8 little endian
	 */
	WL_SHM_FORMAT_VUY888 = 0x34325556,
	/**
	 * Y followed by U then V, 10:10:10. Non-linear modifier only
	 */
	WL_SHM_FORMAT_VUY101010 = 0x30335556,
	/**
	 * [63:0] Cr0:0:Y1:0:Cb0:0:Y0:0 10:6:10:6:10:6:10:6 little endian per 2 Y pixels
	 */
	WL_SHM_FORMAT_Y210 = 0x30313259,
	/**
	 * [63:0] Cr0:0:Y1:0:Cb0:0:Y0:0 12:4:12:4:12:4:12:4 little endian per 2 Y pixels
	 */
	WL_SHM_FORMAT_Y212 = 0x32313259,
	/**
	 * [63:0] Cr0:Y1:Cb0:Y0 16:16:16:16 little endian per 2 Y pixels
	 */
	WL_SHM_FORMAT_Y216 = 0x36313259,
	/**
	 * [31:0] A:Cr:Y:Cb 2:10:10:10 little endian
	 */
	WL_SHM_FORMAT_Y410 = 0x30313459,
	/**
	 * [63:0] A:0:Cr:0:Y:0:Cb:0 12:4:12:4:12:4:12:4 little endian
	 */
	WL_SHM_FORMAT_Y412 = 0x32313459,
	/**
	 * [63:0] A:Cr:Y:Cb 16:16:16:16 little endian
	 */
	WL_SHM_FORMAT_Y416 = 0x36313459,
	/**
	 * [31:0] X:Cr:Y:Cb 2:10:10:10 little endian
	 */
	WL_SHM_FORMAT_XVYU2101010 = 0x30335658,
	/**
	 * [63:0] X:0:Cr:0:Y:0:Cb:0 12:4:12:4:12:4:12:4 little endian
	 */
	WL_SHM_FORMAT_XVYU12_16161616 = 0x36335658,
	/**
	 * [63:0] X:Cr:Y:Cb 16:16:16:16 little endian
	 */
	WL_SHM_FORMAT_XVYU16161616 = 0x38345658,
	/**
	 * [63:0]   A3:A2:Y3:0:Cr0:0:Y2:0:A1:A0:Y1:0:Cb0:0:Y0:0  1:1:8:2:8:2:8:2:1:1:8:2:8:2:8:2 little endian
	 */
	WL_SHM_FORMAT_Y0L0 = 0x304c3059,
	/**
	 * [63:0]   X3:X2:Y3:0:Cr0:0:Y2:0:X1:X0:Y1:0:Cb0:0:Y0:0  1:1:8:2:8:2:8:2:1:1:8:2:8:2:8:2 little endian
	 */
	WL_SHM_FORMAT_X0L0 = 0x304c3058,
	/**
	 * [63:0]   A3:A2:Y3:Cr0:Y2:A1:A0:Y1:Cb0:Y0  1:1:10:10:10:1:1:10:10:10 little endian
	 */
	WL_SHM_FORMAT_Y0L2 = 0x324c3059,
	/**
	 * [63:0]   X3:X2:Y3:Cr0:Y2:X1:X0:Y1:Cb0:Y0  1:1:10:10:10:1:1:10:10:10 little endian
	 */
	WL_SHM_FORMAT_X0L2 = 0x324c3058,
	WL_SHM_FORMAT_YUV420_8BIT = 0x38305559,
	WL_SHM_FORMAT_YUV420_10BIT = 0x30315559,
	WL_SHM_FORMAT_XRGB8888_A8 = 0x38415258,
	WL_SHM_FORMAT_XBGR8888_A8 = 0x38414258,
	WL_SHM_FORMAT_RGBX8888_A8 = 0x38415852,
	WL_SHM_FORMAT_BGRX8888_A8 = 0x38415842,
	WL_SHM_FORMAT_RGB888_A8 = 0x38413852,
	WL_SHM_FORMAT_BGR888_A8 = 0x38413842,
	WL_SHM_FORMAT_RGB565_A8 = 0x38413552,
	WL_SHM_FORMAT_BGR565_A8 = 0x38413542,
	/**
	 * non-subsampled Cr:Cb plane
	 */
	WL_SHM_FORMAT_NV24 = 0x3432564e,
	/**
	 * non-subsampled Cb:Cr plane
	 */
	WL_SHM_FORMAT_NV42 = 0x3234564e,
	/**
	 * 2x1 subsampled Cr:Cb plane, 10 bit per channel
	 */
	WL_SHM_FORMAT_P210 = 0x30313250,
	/**
	 * 2x2 subsampled Cr:Cb plane 10 bits per channel
	 */
	WL_SHM_FORMAT_P010 = 0x30313050,
	/**
	 * 2x2 subsampled Cr:Cb plane 12 bits per channel
	 */
	WL_SHM_FORMAT_P012 = 0x32313050,
	/**
	 * 2x2 subsampled Cr:Cb plane 16 bits per channel
	 */
	WL_SHM_FORMAT_P016 = 0x36313050,
	/**
	 * [63:0] A:x:B:x:G:x:R:x 10:6:10:6:10:6:10:6 little endian
	 */
	WL_SHM_FORMAT_AXBXGXRX106106106106 = 0x30314241,
	/**
	 * 2x2 subsampled Cr:Cb plane
	 */
	WL_SHM_FORMAT_NV15 = 0x3531564e,
	WL_SHM_FORMAT_Q410 = 0x30313451,
	WL_SHM_FORMAT_Q401 = 0x31303451,
	/**
	 * [63:0] x:R:G:B 16:16:16:16 little endian
	 */
	WL_SHM_FORMAT_XRGB16161616 = 0x38345258,
	/**
	 * [63:0] x:B:G:R 16:16:16:16 little endian
	 */
	WL_SHM_FORMAT_XBGR16161616 = 0x38344258,
	/**
	 * [63:0] A:R:G:B 16:16:16:16 little endian
	 */
	WL_SHM_FORMAT_ARGB16161616 = 0x38345241,
	/**
	 * [63:0] A:B:G:R 16:16:16:16 little endian
	 */
	WL_SHM_FORMAT_ABGR16161616 = 0x38344241,
};
#endif /* WL_SHM_FORMAT_ENUM */

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
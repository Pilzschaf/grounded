#include <grounded/window/grounded_window.h>
#include <grounded/threading/grounded_threading.h>

#include <dlfcn.h>
// Required for key mapping
#include <linux/input.h>
#include <time.h>
#include <errno.h>

#include <EGL/egl.h>

struct wl_interface {
	const char *name;
	int version;
	int method_count;
	const struct wl_message *methods;
	int event_count;
	const struct wl_message *events;
};
struct wl_compositor;
struct wl_registry_listener;
struct wl_registry;
struct wl_array;
struct wl_keyboard;
struct wl_seat;

typedef struct GroundedWaylandWindow {
    struct wl_surface* surface;
    struct xdg_surface* xdgSurface;
    struct xdg_toplevel* xdgToplevel;
    u32 width;
    u32 height;

    // OpenGL
    EGLSurface eglSurface;
    EGLContext eglContext;
    struct wl_egl_window* eglWindow;
} GroundedWaylandWindow;
#define MAX_XCB_WINDOWS 64
GroundedWaylandWindow waylandWindowSlots[MAX_XCB_WINDOWS];

// wayland function types
#define X(N, R, P) typedef R grounded_wayland_##N P;
#include "grounded_wayland_functions.h"
#undef X

// wayland function pointers
#define X(N, R, P) static grounded_wayland_##N * N = 0;
#include "grounded_wayland_functions.h"
#undef X

// wayland-egl function types
#define X(N, R, P) typedef R grounded_wayland_##N P;
#include "grounded_wayland_egl_functions.h"
#undef X

// wayland-egl function pointers
#define X(N, R, P) static grounded_wayland_##N * N = 0;
#include "grounded_wayland_egl_functions.h"
#undef X

#if 1
const struct wl_interface* wl_registry_interface;
const struct wl_interface* wl_surface_interface;
const struct wl_interface* wl_compositor_interface;
const struct wl_interface* wl_seat_interface;
const struct wl_interface* wl_output_interface;
const struct wl_interface* wl_keyboard_interface;
const struct wl_interface* wl_pointer_interface;
#else
extern const struct wl_interface wl_registry_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface wl_compositor_interface;
extern const struct wl_interface wl_seat_interface;
extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_keyboard_interface;
extern const struct wl_interface wl_pointer_interface;
#endif

struct wl_compositor* compositor;
struct wl_display* waylandDisplay;
struct xdg_wm_base* xdgWmBase;
struct wl_keyboard* keyboard;
struct wl_pointer* pointer;
struct zxdg_decoration_manager_v1* decorationManager;

GroundedKeyboardState waylandKeyState;
MouseState waylandMouseState;

#include "grounded_wayland_types.h"

#include "wayland_protocols/xdg_shell.h"
#include "wayland_protocols/xdg-decoration-unstable-v1.h"

static void reportWaylandError(const char* message) {
    printf("Error: %s\n", message);
}

static void keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size) {
    GROUNDED_LOG_VERBOSE("Keyboard keymap");
}

static void keyboard_handle_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
    GROUNDED_LOG_VERBOSE("Keyboard gained focus");
}

static void keyboard_handle_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) {
    GROUNDED_LOG_VERBOSE("Keyboard lost focus");
}

static u8 translateWaylandKeycode(u32 key) {
    u8 result = 0;
    switch(key) {
        case KEY_0:
        result = GROUNDED_KEY_0;
        break;
        case KEY_1:
        result = GROUNDED_KEY_1;
        break;
        case KEY_2:
        result = GROUNDED_KEY_2;
        break;
        case KEY_3:
        result = GROUNDED_KEY_3;
        break;
        case KEY_4:
        result = GROUNDED_KEY_4;
        break;
        case KEY_5:
        result = GROUNDED_KEY_5;
        break;
        case KEY_6:
        result = GROUNDED_KEY_6;
        break;
        case KEY_7:
        result = GROUNDED_KEY_7;
        break;
        case KEY_8:
        result = GROUNDED_KEY_8;
        break;
        case KEY_9:
        result = GROUNDED_KEY_9;
        break;
        case KEY_A:
        result = GROUNDED_KEY_A;
        break;
        case KEY_B:
        result = GROUNDED_KEY_B;
        break;
        case KEY_C:
        result = GROUNDED_KEY_C;
        break;
        case KEY_D:
        result = GROUNDED_KEY_D;
        break;
        case KEY_E:
        result = GROUNDED_KEY_E;
        break;
        case KEY_F:
        result = GROUNDED_KEY_F;
        break;
        case KEY_G:
        result = GROUNDED_KEY_G;
        break;
        case KEY_H:
        result = GROUNDED_KEY_H;
        break;
        case KEY_I:
        result = GROUNDED_KEY_I;
        break;
        case KEY_J:
        result = GROUNDED_KEY_J;
        break;
        case KEY_K:
        result = GROUNDED_KEY_K;
        break;
        case KEY_L:
        result = GROUNDED_KEY_L;
        break;
        case KEY_M:
        result = GROUNDED_KEY_M;
        break;
        case KEY_N:
        result = GROUNDED_KEY_N;
        break;
        case KEY_O:
        result = GROUNDED_KEY_O;
        break;
        case KEY_P:
        result = GROUNDED_KEY_P;
        break;
        case KEY_Q:
        result = GROUNDED_KEY_Q;
        break;
        case KEY_R:
        result = GROUNDED_KEY_R;
        break;
        case KEY_S:
        result = GROUNDED_KEY_S;
        break;
        case KEY_T:
        result = GROUNDED_KEY_T;
        break;
        case KEY_U:
        result = GROUNDED_KEY_U;
        break;
        case KEY_V:
        result = GROUNDED_KEY_V;
        break;
        case KEY_W:
        result = GROUNDED_KEY_W;
        break;
        case KEY_X:
        result = GROUNDED_KEY_X;
        break;
        case KEY_Y:
        result = GROUNDED_KEY_Y;
        break;
        case KEY_Z:
        result = GROUNDED_KEY_Z;
        break;
        case KEY_F1:
        result = GROUNDED_KEY_F1;
        break;
        case KEY_F2:
        result = GROUNDED_KEY_F2;
        break;
        case KEY_F3:
        result = GROUNDED_KEY_F3;
        break;
        case KEY_F4:
        result = GROUNDED_KEY_F4;
        break;
        case KEY_F5:
        result = GROUNDED_KEY_F5;
        break;
        case KEY_F6:
        result = GROUNDED_KEY_F6;
        break;
        case KEY_F7:
        result = GROUNDED_KEY_F7;
        break;
        case KEY_F8:
        result = GROUNDED_KEY_F8;
        break;
        case KEY_F9:
        result = GROUNDED_KEY_F9;
        break;
        case KEY_F10:
        result = GROUNDED_KEY_F10;
        break;
        case KEY_F11:
        result = GROUNDED_KEY_F11;
        break;
        case KEY_F12:
        result = GROUNDED_KEY_F12;
        break;
        case KEY_F13:
        result = GROUNDED_KEY_F13;
        break;
        case KEY_F14:
        result = GROUNDED_KEY_F14;
        break;
        case KEY_F15:
        result = GROUNDED_KEY_F15;
        break;
        case KEY_F16:
        result = GROUNDED_KEY_F16;
        break;
        case KEY_SPACE:
        result = GROUNDED_KEY_SPACE;
        break;
        case KEY_LEFTSHIFT:
        result = GROUNDED_KEY_LSHIFT;
        break;
        case KEY_RIGHTSHIFT:
        result = GROUNDED_KEY_RSHIFT;
        break;
        default:
        GROUNDED_LOG_WARNING("Unknown keycode");
        break;
    }
    return result;
}

static void keyboard_handle_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    if(state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        waylandKeyState.keys[translateWaylandKeycode(key)] = true;
    } else if(state == WL_KEYBOARD_KEY_STATE_RELEASED) {
        waylandKeyState.keys[translateWaylandKeycode(key)] = false;
    }
    //fprintf(stderr, "Key is %d state is %d\n", key, state);
}

static void keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
    //fprintf(stderr, "Modifiers depressed %d, latched %d, locked %d, group %d\n",
    //        mods_depressed, mods_latched, mods_locked, group);
}

static const struct wl_keyboard_listener keyboard_listener = {
    keyboard_handle_keymap,
    keyboard_handle_enter,
    keyboard_handle_leave,
    keyboard_handle_key,
    keyboard_handle_modifiers,
};


static void pointerHandleEnter(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {

}

static void pointerHandleLeave(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface) {

}

static void pointerHandleMotion(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    s32 posX = wl_fixed_to_int(surface_x);
    s32 posY = wl_fixed_to_int(surface_y);
    waylandMouseState.x = posX;
    waylandMouseState.y = posY;
}

static void pointerHandleButton(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
    bool pressed = state == 1;
    // button code is platform specific. See linux/input-event-codes.h. Want to use BTN_LEFT, BTN_RIGHT, BTN_MIDDLE
    if(button == BTN_LEFT) {
        waylandMouseState.buttons[GROUNDED_MOUSE_BUTTON_LEFT] = pressed;
    } else if(button == BTN_RIGHT) {
        waylandMouseState.buttons[GROUNDED_MOUSE_BUTTON_RIGHT] = pressed;
    } else if(button == BTN_MIDDLE) {
        waylandMouseState.buttons[GROUNDED_MOUSE_BUTTON_MIDDLE] = pressed;
    }
}

static void pointerHandleAxis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
    bool vertical = axis == 0;
    s32 amount = value;
    if(vertical) {
        waylandMouseState.scrollDelta = amount / -5000.f;
    } else {
        waylandMouseState.horizontalScrollDelta = amount / -5000.f;
    }
}

static const struct wl_pointer_listener pointerListener = {
    pointerHandleEnter,
    pointerHandleLeave,
    pointerHandleMotion,
    pointerHandleButton,
    pointerHandleAxis,
};

static void seatHandleCapabilities(void *data, struct wl_seat *seat, u32 c) {
    enum wl_seat_capability caps = (enum wl_seat_capability) c;
    if (caps & WL_SEAT_CAPABILITY_POINTER) {
        // Mouse input
        pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer, &pointerListener, 0);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && pointer) {
        wl_pointer_destroy(pointer);
    }
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        keyboard = wl_seat_get_keyboard(seat);
	    wl_keyboard_add_listener(keyboard, &keyboard_listener, 0);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
	    wl_keyboard_destroy(keyboard);
	    keyboard = 0;
    }
}

static const struct wl_seat_listener seatListener = {
    seatHandleCapabilities,
};


static void handle_ping_xdg_wm_base(void *data, struct xdg_wm_base *xdg, uint32_t serial) {
    xdg_wm_base_pong(xdg, serial);
}

static const struct xdg_wm_base_listener xdgWmBaseListener = {
    handle_ping_xdg_wm_base
};


static void registry_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    GROUNDED_LOG_INFO(interface);
    if (strcmp(interface, "wl_compositor") == 0) {
        // Wayland compositor is required for creating surfaces
        compositor = (struct wl_compositor*)wl_registry_bind(registry, id, wl_compositor_interface, 4);
        ASSERT(compositor);
    } else if(strcmp(interface, "xdg_wm_base") == 0) {
        xdgWmBase = (struct xdg_wm_base*)wl_registry_bind(registry, id, &xdg_wm_base_interface, 3);
        ASSERT(xdgWmBase);
        xdg_wm_base_add_listener(xdgWmBase, &xdgWmBaseListener, 0);
    } else if(strcmp(interface,"wl_seat") == 0) {
        // Seats are input devices like keyboards mice etc.
        struct wl_seat* seat = (struct wl_seat*)wl_registry_bind(registry, id, wl_seat_interface, 1);
        ASSERT(seat);
        wl_seat_add_listener(seat, &seatListener, 0);
    } else if(strcmp(interface, "zxdg_decoration_manager_v1") == 0) {
        // Client and server side decoration negotiation
        decorationManager = (struct zxdg_decoration_manager_v1*)wl_registry_bind(registry, id, &zxdg_decoration_manager_v1_interface, 1);
    }
}

static void registry_global_remove(void *a, struct wl_registry *b, uint32_t c) {}

static const struct wl_registry_listener registryListener = {
    .global = registry_global,
    .global_remove = registry_global_remove
};


static void xdgToplevelHandleConfigure(void* data,  struct xdg_toplevel* toplevel,  int32_t width, int32_t height, struct wl_array* states) {
    GroundedWaylandWindow* window = (GroundedWaylandWindow*)data;

    // States array tells us in which state the new window is
    /*u32* state;
    wl_array_for_each(state, states) {
        switch(*state) {
            case XDG_TOPLEVEL_STATE_MAXIMIZED:{

            } break;
            case XDG_TOPLEVEL_STATE_FULLSCREEN:{

            } break;
            case XDG_TOPLEVEL_STATE_RESIZING:{

            } break;
            case XDG_TOPLEVEL_STATE_ACTIVATED:{

            } break;
        }
    }*/

    // Here we get a new width and height
    if(width && height && (width != window->width || height != window->height)) {
        eventQueue[eventQueueIndex++] = (GroundedEvent){
            .type = GROUNDED_EVENT_TYPE_RESIZE,
            .resize.window = window,
            .resize.width = width,
            .resize.height = height,
        };
    }
    if(width) {
        window->width = width;
    }
    if(height) {
        window->height = height;
    }
}

static void xdgToplevelHandleClose(void* data, struct xdg_toplevel* toplevel) {
    GroundedWaylandWindow* window = (GroundedWaylandWindow*)data;
    eventQueue[eventQueueIndex++] = (GroundedEvent){
        .type = GROUNDED_EVENT_TYPE_CLOSE_REQUEST,
        .closeRequest.window = window,
    };
}

static const struct xdg_toplevel_listener xdgToplevelListener = {
    xdgToplevelHandleConfigure,
    xdgToplevelHandleClose
};

static void xdgSurfaceHandleConfigure(void* data, struct xdg_surface* surface, uint32_t serial) {
    xdg_surface_ack_configure(surface, serial);

    GroundedWaylandWindow* window = (GroundedWaylandWindow*) data;
    //struct xdg_surface_state *state = xdg_surface_get_pending(surface);
    

    //wl_buffer* buffer = create_buffer(640, 480);
    //wl_surface_attach(window->surface, buffer, 0, 0);
    //wl_surface_commit(window->surface);
    //xdg_surface_commit();
}

static const struct xdg_surface_listener xdgSurfaceListener = {
    xdgSurfaceHandleConfigure
};

static void handleConfigureZxdgDecoration(void *data, struct zxdg_toplevel_decoration_v1 *zxdg_toplevel_decoration_v1, uint32_t mode) {
    GROUNDED_LOG_DEBUG("Toplevel decoration configure");
    if (mode == ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE) {
        GROUNDED_LOG_INFO("CSD");
    } else {
        GROUNDED_LOG_INFO("SSD");
    }
}

static const struct zxdg_toplevel_decoration_v1_listener decorationListener = {
    handleConfigureZxdgDecoration
};

static bool initWayland() {
    const char* error = 0;

    void* waylandLibrary = dlopen("libwayland-client.so", RTLD_LAZY | RTLD_LOCAL);
    if(!waylandLibrary) {
        error = "No wayland library found";
    }

    if(!error) { // Load function pointers
        const char* firstMissingFunctionName = 0;
        #define X(N, R, P) N = (grounded_wayland_##N *) dlsym(waylandLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
        #include "grounded_wayland_functions.h"
        #undef X
        if(firstMissingFunctionName) {
            printf("Could not load wayland function: %s\n", firstMissingFunctionName);
            error = "Could not load all wayland functions. Your wayland version is incompatible";
        }
    }

    if(!error) { // Load wayland interfaces
        const char* firstMissingInterfaceName = 0;
        #define LOAD_WAYLAND_INTERFACE(N) N = dlsym(waylandLibrary, #N); if(!N && !firstMissingInterfaceName) {firstMissingInterfaceName = #N ;}
        LOAD_WAYLAND_INTERFACE(wl_registry_interface);
        LOAD_WAYLAND_INTERFACE(wl_surface_interface);
        LOAD_WAYLAND_INTERFACE(wl_compositor_interface);
        LOAD_WAYLAND_INTERFACE(wl_seat_interface);
        LOAD_WAYLAND_INTERFACE(wl_output_interface);
        LOAD_WAYLAND_INTERFACE(wl_keyboard_interface);
        LOAD_WAYLAND_INTERFACE(wl_pointer_interface);
        if(firstMissingInterfaceName) {
            error = "Could not load all wayland interfaces. Your wayland version is incompatible";
        }
    }

    if(!error) {
        waylandDisplay = wl_display_connect(0);
        if(!waylandDisplay) {
            error = "Could not connect to wayland display. If you are not using Wayland this is expected";
        }
    }

    struct wl_registry* registry;
    if(!error) {
        registry = wl_display_get_registry(waylandDisplay);
        if(!registry) {
            error = "Could not get wayland registry";
        }
        wl_registry_add_listener(registry, &registryListener, 0);

        // Roundtrip to retrieve all registry objects
        wl_display_roundtrip(waylandDisplay);

        // Roundtrip to retrieve all initial output events
        wl_display_roundtrip(waylandDisplay);
    }

    if(error) {
        reportWaylandError(error);
        return false;
    }

    return true;
}

static void shutdownWayland() {
    //TODO: Release resources
}

static void waylandSetWindowTitle(GroundedWaylandWindow* window, String8 title) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    xdg_toplevel_set_title(window->xdgToplevel, str8GetCstr(scratch, title));

    arenaEndTemp(temp);
}

static void waylandWindowSetFullsreen(GroundedWaylandWindow* window, bool fullscreen) {
    if(fullscreen) {
        xdg_toplevel_set_fullscreen(window->xdgToplevel, 0);
    } else {
        xdg_toplevel_unset_fullscreen(window->xdgToplevel);
    }
}

static void waylandWindowSetBorderless(GroundedWaylandWindow* window, bool borderless) {
    ASSERT(false);
}

static void waylandWindowSetHidden(GroundedWaylandWindow* window, bool hidden) {
    ASSERT(false);
}

static GroundedWindow* waylandCreateWindow(struct GroundedWindowCreateParameters* parameters) {
    if(!parameters) {
        static struct GroundedWindowCreateParameters defaultParameters = {0};
        parameters = &defaultParameters;
    }
    MemoryArena* tempArena = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(tempArena);

    GroundedWaylandWindow* window = &waylandWindowSlots[0];
    window->width = parameters->width;
    window->height = parameters->height;
    if(!window->width) window->width = 1920;
    if(!window->height) window->height = 1080;
    window->surface = wl_compositor_create_surface(compositor);
    window->xdgSurface = xdg_wm_base_get_xdg_surface(xdgWmBase, window->surface);
    ASSERT(window->xdgSurface);
    xdg_surface_add_listener(window->xdgSurface, &xdgSurfaceListener, window);
    window->xdgToplevel = xdg_surface_get_toplevel(window->xdgSurface);
    xdg_toplevel_add_listener(window->xdgToplevel, &xdgToplevelListener, window);
    ASSERT(window->xdgToplevel);
    
    // Window Title
    if(parameters->title.size > 0) {
        waylandSetWindowTitle(window, parameters->title);
    }
    if(parameters->minWidth || parameters->minHeight) {
        xdg_toplevel_set_min_size(window->xdgToplevel, parameters->minWidth, parameters->minHeight);
    }
    if(parameters->maxWidth || parameters->maxHeight) {
        // 0 means no max Size in that dimension
        xdg_toplevel_set_max_size(window->xdgToplevel, parameters->maxWidth, parameters->maxHeight);
    }

    // Set server side decorations
    struct zxdg_toplevel_decoration_v1* decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(decorationManager, window->xdgToplevel);
    zxdg_toplevel_decoration_v1_add_listener(decoration, &decorationListener, window);
    zxdg_toplevel_decoration_v1_set_mode(decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);

    wl_surface_commit(window->surface);

    wl_display_roundtrip(waylandDisplay);

    arenaEndTemp(temp);

    return window;
}

static void waylandDestroyWindow(GroundedWaylandWindow* window) {
    wl_surface_destroy(window->surface);
    wl_display_roundtrip(waylandDisplay);
}

static u32 waylandGetWindowWidth(GroundedWaylandWindow* window) {
    return window->width;
}

static u32 waylandGetWindowHeight(GroundedWaylandWindow* window) {
    return window->height;
}

// Returns true when poll was successful. False on timeout
static bool waylandPoll(u32 maxWaitingTimeInMs) {
	int ret;
	struct pollfd pfd[1];
    int timeout = (int)maxWaitingTimeInMs;

    if(!timeout) {
        // negative is infinite block
        timeout = -1;
    }
    struct timespec ts = {0};
    ts.tv_sec = timeout / 1000;
    ts.tv_nsec = (timeout * 1000000)%1000000000;
	pfd[0].fd = wl_display_get_fd(waylandDisplay);
	pfd[0].events = POLLIN;
	do {
		ret = ppoll(pfd, 1, timeout < 0 ? 0 : &ts, 0);
	} while (ret == -1 && errno == EINTR); // A signal occured before

	if (ret == 0) {
        // Timed out
		return false;
    } else if(ret < 0) {
        // Error
        return false;
    }

	return true;
}


static GroundedEvent* waylandGetEvents(u32* eventCount, u32 maxWaitingTimeInMs) {
    eventQueueIndex = 0;

    if(!maxWaitingTimeInMs) {
        // Sends out pending requests to all event queues
        wl_display_roundtrip(waylandDisplay);
    }

    if(waylandPoll(maxWaitingTimeInMs)) {
        wl_display_dispatch(waylandDisplay);
    }

    *eventCount = eventQueueIndex;
    return eventQueue;
}

static GroundedEvent* waylandPollEvents(u32* eventCount) {
    eventQueueIndex = 0;

    // Sends out pending requests to all event queues
    wl_display_roundtrip(waylandDisplay);

    //wl_display_dispatch_pending(waylandDisplay);

    *eventCount = eventQueueIndex;
    return eventQueue;
}

static void waylandFetchKeyboardState(GroundedKeyboardState* keyboardState) {
    *keyboardState = waylandKeyState;
}

static void waylandFetchMouseState(GroundedWaylandWindow* window, MouseState* mouseState) {
    mouseState->x = waylandMouseState.x;
    mouseState->y = waylandMouseState.y;

    // Set mouse button state
    memcpy(mouseState->buttons, waylandMouseState.buttons, sizeof(mouseState->buttons));

    mouseState->scrollDelta = waylandMouseState.scrollDelta;
    mouseState->horizontalScrollDelta = waylandMouseState.horizontalScrollDelta;
    mouseState->deltaX = mouseState->x - mouseState->lastX;
    mouseState->deltaY = mouseState->y - mouseState->lastY;

    waylandMouseState.scrollDelta = 0.0f;
    waylandMouseState.horizontalScrollDelta = 0.0f;
}


//*************
// OpenGL stuff
#ifdef GROUNDED_OPENGL_SUPPORT
EGLDisplay waylandEglDisplay;
GROUNDED_FUNCTION bool waylandCreateOpenGLContext(GroundedWaylandWindow* window, u32 flags, GroundedWaylandWindow* windowContextToShareResources) {
    { // Load egl function pointers
        void* eglLibrary = dlopen("libwayland-egl.so", RTLD_LAZY | RTLD_LOCAL);
        if(!eglLibrary) {
            const char* error = "No wayland-egl library found";
            GROUNDED_LOG_ERROR(error);
        } else {
            const char* firstMissingFunctionName = 0;
            #define X(N, R, P) N = (grounded_wayland_##N *) dlsym(eglLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "grounded_wayland_egl_functions.h"
            #undef X
            if(firstMissingFunctionName) {
                printf("Could not load wayland function: %s\n", firstMissingFunctionName);
                const char* error = "Could not load all wayland-egl functions. Your wayland-egl version is incompatible";
                GROUNDED_LOG_ERROR(error);
            }
        }
    }

    waylandEglDisplay = eglGetDisplay((EGLNativeDisplayType)waylandDisplay);
    if(waylandEglDisplay == EGL_NO_DISPLAY) {
        GROUNDED_LOG_ERROR("Error obtaining EGL display for wayland display");
        return false;
    }

    int eglVersionMajor = 0;
    int eglVersionMinor = 0;
    if(!eglInitialize(waylandEglDisplay, &eglVersionMajor, &eglVersionMinor)) {
        GROUNDED_LOG_ERROR("Error initializing EGL display");
        eglTerminate(waylandEglDisplay);
        return false;
    }
    //LOG_INFO("Using EGL version ", eglVersionMajor, ".", eglVersionMinor);

    // OPENGL_ES available instead of EGL_OPENGL_API
    if(!eglBindAPI(EGL_OPENGL_API)) {
        GROUNDED_LOG_ERROR("Error binding OpenGL API");
        return false;
    }

    // if config is 0 the total number of configs is returned
    EGLConfig config;
    int numConfigs = 0;
    int attribList[] = {EGL_RED_SIZE, 1, EGL_GREEN_SIZE, 1, EGL_BLUE_SIZE, 1, EGL_NONE};
    if(!eglChooseConfig(waylandEglDisplay, attribList, &config, 1, &numConfigs) || numConfigs <= 0) {
        GROUNDED_LOG_ERROR("Error choosing OpenGL config");
        return false;
    }

    window->eglContext = eglCreateContext(waylandEglDisplay, config, EGL_NO_CONTEXT, 0);
    if(window->eglContext == EGL_NO_CONTEXT) {
        GROUNDED_LOG_ERROR("Error creating EGL Context");
        return false;
    }

    window->eglWindow = wl_egl_window_create(window->surface, window->width, window->height);
    if(!window->eglWindow) {
        GROUNDED_LOG_ERROR("Error creating wayland EGL window");
        return false;
    }
    window->eglSurface = eglCreateWindowSurface(waylandEglDisplay, config, (EGLNativeWindowType)window->eglWindow, 0);
    if(window->eglSurface == EGL_NO_SURFACE) {
        GROUNDED_LOG_ERROR("Error creating EGL surface");
        return false;
    }

    return true;
}

GROUNDED_FUNCTION void waylandOpenGLMakeCurrent(GroundedWaylandWindow* window) {
    if(!eglMakeCurrent(waylandEglDisplay, window->eglSurface, window->eglSurface, window->eglContext)) {
        //LOG_INFO("Error: ", eglGetError());
        GROUNDED_LOG_ERROR("Error making OpenGL context current");
        //return false;
    }
}

GROUNDED_FUNCTION void waylandWindowGlSwapBuffers(GroundedWaylandWindow* window) {
    eglSwapBuffers(waylandEglDisplay, window->eglSurface);
}
#endif // GROUNDED_OPENGL_SUPPORT


//*************
// Vulkan stuff
#ifdef GROUNDED_VULKAN_SUPPORT
typedef VkFlags VkWaylandSurfaceCreateFlagsKHR;
typedef struct VkWaylandSurfaceCreateInfoKHR {
    VkStructureType                   sType;
    const void*                       pNext;
    VkWaylandSurfaceCreateFlagsKHR    flags;
    struct wl_display*                display;
    struct wl_surface*                surface;
} VkWaylandSurfaceCreateInfoKHR;

typedef VkResult (VKAPI_PTR *PFN_vkCreateWaylandSurfaceKHR)(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
typedef VkBool32 (VKAPI_PTR *PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, struct wl_display* display);

static VkSurfaceKHR waylandGetVulkanSurface(GroundedWaylandWindow* window, VkInstance instance) {
    const char* error = 0;
    VkSurfaceKHR surface = 0;
    static PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR = 0;
    if(!vkCreateWaylandSurfaceKHR) {
        vkCreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR");    
    }
    if(!vkCreateWaylandSurfaceKHR) {
        error = "Could not load vkCreateWaylandSurfaceKHR. Is the wayland instance extension enabled and available?";
    }

    if(!error) {
        VkWaylandSurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
        createInfo.display = waylandDisplay;
        createInfo.surface = window->surface;
        VkResult result = vkCreateWaylandSurfaceKHR(instance, &createInfo, 0, &surface);
    }

    if(error) {
        printf("Error creating vulkan surface: %s\n", error);
    }
    
    return surface;
}
#endif // GROUNDED_VULKAN_SUPPORT

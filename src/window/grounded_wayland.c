#include <grounded/window/grounded_window.h>
#include <grounded/threading/grounded_threading.h>
#include <grounded/file/grounded_file.h>
#include <grounded/memory/grounded_arena.h>
#include <grounded/memory/grounded_memory.h>

#include <dlfcn.h>
// Required for key mapping
#include <linux/input.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h> // For shared memory files
#include <sys/mman.h> // For shared memory files
#include <unistd.h>

#ifdef GROUNDED_OPENGL_SUPPORT
//#include <EGL/egl.h>
#endif

//#include <wayland-cursor.h>
//#include <wayland-client.h>

// Wayland icons
// Wayland requires .desktop specification for icon support
// .desktop can be installed in ~/.local/share/applications
// icons can be installed in ~/.local/share/icons
// Actually path is $XDG_DATA_DIRS/icons
// Old variant was in ~/.icons

// Because of wayland API design we need to store the current active drag offer. This should be done per datadevice which is per seat eg. per user
struct WaylandDataOffer* dragOffer;

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
struct wl_shm;
struct wl_buffer;
struct wl_cursor_image;
struct wl_region;
struct wl_data_source;
struct wl_data_offer;

typedef struct GroundedWaylandWindow {
    struct wl_surface* surface;
    struct xdg_surface* xdgSurface;
    struct xdg_toplevel* xdgToplevel;
    u32 width;
    u32 height;
    void* userData;
    GroundedWindowCustomTitlebarCallback* customTitlebarCallback;
    MouseState mouseState;

    GroundedWindowDndCallback* dndCallback;

#ifdef GROUNDED_OPENGL_SUPPORT
    EGLSurface eglSurface;
    struct wl_egl_window* eglWindow;
#endif
} GroundedWaylandWindow;

#ifdef GROUNDED_OPENGL_SUPPORT
static void waylandResizeEglSurface(GroundedWaylandWindow* window);
#endif

// wayland function types
#define X(N, R, P) typedef R grounded_wayland_##N P;
#include "types/grounded_wayland_functions.h"
#undef X

// wayland function pointers
#define X(N, R, P) static grounded_wayland_##N * N = 0;
#include "types/grounded_wayland_functions.h"
#undef X

// wayland-egl function types
#define X(N, R, P) typedef R grounded_wayland_##N P;
#include "types/grounded_wayland_egl_functions.h"
#undef X

// wayland-egl function pointers
#define X(N, R, P) static grounded_wayland_##N * N = 0;
#include "types/grounded_wayland_egl_functions.h"
#undef X

// wayland-cursor function types
#define X(N, R, P) typedef R grounded_wayland_##N P;
#include "types/grounded_wayland_cursor_functions.h"
#undef X

// wayland-cursor function pointers
#define X(N, R, P) static grounded_wayland_##N * N = 0;
#include "types/grounded_wayland_cursor_functions.h"
#undef X

#if 1
const struct wl_interface* wl_registry_interface;
const struct wl_interface* wl_surface_interface;
const struct wl_interface* wl_compositor_interface;
const struct wl_interface* wl_seat_interface;
const struct wl_interface* wl_output_interface;
const struct wl_interface* wl_keyboard_interface;
const struct wl_interface* wl_pointer_interface;
const struct wl_interface* wl_shm_interface;
const struct wl_interface* wl_buffer_interface;
const struct wl_interface* wl_shm_pool_interface;
const struct wl_interface* wl_region_interface;
const struct wl_interface* wl_data_device_manager_interface;
const struct wl_interface* wl_data_source_interface;
const struct wl_interface* wl_data_device_interface;
#else
extern const struct wl_interface wl_registry_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface wl_compositor_interface;
extern const struct wl_interface wl_seat_interface;
extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_keyboard_interface;
extern const struct wl_interface wl_pointer_interface;
#endif

struct wl_data_device_manager* dataDeviceManager;
u32 dataDeviceManagerVersion;

struct wl_compositor* compositor;
struct wl_display* waylandDisplay;
struct xdg_wm_base* xdgWmBase;
struct wl_keyboard* keyboard;
struct wl_pointer* pointer;
u32 lastPointerSerial;
struct wl_seat* pointerSeat;
struct wl_data_device* dataDevice; // Data device tied to pointerSeat
u32 pointerEnterSerial;
struct zxdg_decoration_manager_v1* decorationManager;
struct wl_shm* waylandShm; // Shared memory interface to compositor
struct wl_cursor_theme* cursorTheme;
struct wl_surface* cursorSurface;
bool waylandCursorLibraryPresent;
GroundedMouseCursor waylandCurrentCursorType = GROUNDED_MOUSE_CURSOR_DEFAULT;
GroundedMouseCursor waylandCursorTypeOverwrite = GROUNDED_MOUSE_CURSOR_COUNT;
GroundedWaylandWindow* activeWindow; // The window (if any) the mouse cursor is currently hovering

GroundedKeyboardState waylandKeyState;

struct WaylandDataOffer {
    String8List availableMimeTypeList;
    //TODO: Need to create a flat array for the mime types for easier API handling
    MemoryArena arena;
    bool dnd; // True for dnd data, false for clipboard data
    GroundedWaylandWindow* window; // 0 for clipboard data
    u32 enterSerial;
    struct wl_data_offer* offer;
    u32 allowedActions;
    u32 selectedAction;

    u32 mimeTypeCount;
    u32 lastAcceptedMimeIndex;
    String8* mimeTypes;
    GroundedWindowDndDropCallback* dropCallback;
    s32 x;
    s32 y;
};

#include "types/grounded_wayland_types.h"

#include "wayland_protocols/xdg_shell.h"
#include "wayland_protocols/xdg-decoration-unstable-v1.h"

static void waylandWindowSetMaximized(GroundedWaylandWindow* window, bool maximized);
GROUNDED_FUNCTION void waylandSetCursorType(enum GroundedMouseCursor cursorType);

static void reportWaylandError(const char* message) {
    printf("Error: %s\n", message);
}

static void randname(char *buf) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	long r = ts.tv_nsec;
	for (int i = 0; i < 6; ++i) {
		buf[i] = 'A'+(r&15)+(r&16)*2;
		r >>= 5;
	}
}

static int createSharedMemoryFile(u64 size) {
    int fd = -1;
    {
        int retries = 100;
        do {
            char name[] = "/wl_shm-XXXXXX";
            randname(name + sizeof(name) - 7);
            --retries;
            fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
            if (fd >= 0) {
                shm_unlink(name);
                break;
            }
        } while (retries > 0 && errno == EEXIST);
    }

	if (fd < 0) {
		return -1;
    }
	int ret;
	do {
		ret = ftruncate(fd, size);
	} while (ret < 0 && errno == EINTR);
	if (ret < 0) {
		close(fd);
		return -1;
	}
	return fd;
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
        case KEY_ESC:
        result = GROUNDED_KEY_ESCAPE;
        break;
        default:
        GROUNDED_LOG_WARNING("Unknown keycode");
        break;
    }
    return result;
}

static void keyboard_handle_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    u8 keycode = translateWaylandKeycode(key);
    if(state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        waylandKeyState.keys[keycode] = true;
        eventQueue[eventQueueIndex++] = (GroundedEvent){
            .type = GROUNDED_EVENT_TYPE_KEY_DOWN,
            .keyDown.keycode = keycode,
            .keyDown.modifiers = 0, //TODO: Modifiers
        };
    } else if(state == WL_KEYBOARD_KEY_STATE_RELEASED) {
        waylandKeyState.keys[keycode] = false;
        eventQueue[eventQueueIndex++] = (GroundedEvent){
            .type = GROUNDED_EVENT_TYPE_KEY_UP,
            .keyUp.keycode = keycode,
        };
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
    ASSERT(wl_pointer == pointer);

    GroundedWaylandWindow* window = (GroundedWaylandWindow*)wl_surface_get_user_data(surface);
    activeWindow = window;
    lastPointerSerial = serial;

    if(waylandCurrentCursorType == GROUNDED_MOUSE_CURSOR_CUSTOM) {
        wl_pointer_set_cursor(pointer, serial, cursorSurface, 0, 0);
    } else {
        waylandSetCursorType(waylandCurrentCursorType);
    }
    printf("Enter\n");

    pointerEnterSerial = serial;
}

static void pointerHandleLeave(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface) {
    printf("Pointer Leave\n");
    if(activeWindow) {
        // Make sure mouse position is outisde screen
        activeWindow->mouseState.x = -1;
        activeWindow->mouseState.y = -1;

        // Reset buttons to non-pressed state
        MEMORY_CLEAR_ARRAY(activeWindow->mouseState.buttons);
        //TODO: Should probably also send button up events for all buttons that were pressed upon leave. Or maybe we send a cursor leave event and let the applicatio handle it
    }
    lastPointerSerial = serial;
    activeWindow = 0;
    if(waylandCursorTypeOverwrite < GROUNDED_MOUSE_CURSOR_COUNT) {
        // Remove cursor overwrite
        GroundedMouseCursor prevCursor = waylandCursorTypeOverwrite;
        waylandCursorTypeOverwrite = GROUNDED_MOUSE_CURSOR_COUNT;
        groundedSetCursorType(prevCursor);
    }
}

static bool isCursorOverwriteActive() {
    return waylandCursorTypeOverwrite < GROUNDED_MOUSE_CURSOR_COUNT;
}

static void removeCursorOverwrite() {
    GroundedMouseCursor prevCursor = waylandCursorTypeOverwrite;
    waylandCursorTypeOverwrite = GROUNDED_MOUSE_CURSOR_COUNT;
    groundedSetCursorType(prevCursor);
}

static void setCursorOverwrite(GroundedMouseCursor newCursor) {
    if(isCursorOverwriteActive()) {
        removeCursorOverwrite();
    }
    GroundedMouseCursor prevCursor = waylandCurrentCursorType;
    groundedSetCursorType(newCursor);
    waylandCursorTypeOverwrite = prevCursor;
}

static void pointerHandleMotion(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    s32 posX = wl_fixed_to_int(surface_x);
    s32 posY = wl_fixed_to_int(surface_y);
    if(activeWindow) {
        activeWindow->mouseState.x = posX;
        activeWindow->mouseState.y = posY;
        if(activeWindow->customTitlebarCallback) {
            GroundedWindowCustomTitlebarHit hit = activeWindow->customTitlebarCallback((GroundedWindow*)activeWindow, activeWindow->mouseState.x, activeWindow->mouseState.y);
            if(hit == GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_BORDER) {
                float x = posX;
                float y = posY;
                float width = groundedWindowGetWidth((GroundedWindow*)activeWindow);
                float height = groundedWindowGetHeight((GroundedWindow*)activeWindow);
                float offset = 27.0f;
                u32 edges = 0;
                if(x < offset) {
                    edges |= XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
                } else if(x > width - offset) {
                    edges |= XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;
                }
                if(y < offset) {
                    edges |= XDG_TOPLEVEL_RESIZE_EDGE_TOP;
                } else if(y > height - offset) {
                    edges |= XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;
                }
                GroundedMouseCursor cursorType = GROUNDED_MOUSE_CURSOR_DEFAULT;
                switch(edges) {
                    case XDG_TOPLEVEL_RESIZE_EDGE_TOP: cursorType = GROUNDED_MOUSE_CURSOR_UPDOWN; break;
                    case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM: cursorType = GROUNDED_MOUSE_CURSOR_UPDOWN; break;
                    case XDG_TOPLEVEL_RESIZE_EDGE_LEFT: cursorType = GROUNDED_MOUSE_CURSOR_LEFTRIGHT; break;
                    case XDG_TOPLEVEL_RESIZE_EDGE_RIGHT: cursorType = GROUNDED_MOUSE_CURSOR_LEFTRIGHT; break;
                    case XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT: cursorType = GROUNDED_MOUSE_CURSOR_UPRIGHT; break;
                    case XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT: cursorType = GROUNDED_MOUSE_CURSOR_UPLEFT; break;
                    case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT: cursorType = GROUNDED_MOUSE_CURSOR_DOWNRIGHT; break;
                    case XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT: cursorType = GROUNDED_MOUSE_CURSOR_DOWNLEFT; break;
                }
                setCursorOverwrite(cursorType);
            } else if(isCursorOverwriteActive()) {
                removeCursorOverwrite();
            }
        }
    }
}

static void pointerHandleButton(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
    bool pressed = (state == 1);
    // button code is platform specific. See linux/input-event-codes.h. Want to use BTN_LEFT, BTN_RIGHT, BTN_MIDDLE
    u32 buttonCode = GROUNDED_MOUSE_BUTTON_LEFT;
    if(button == BTN_RIGHT) {
        buttonCode = GROUNDED_MOUSE_BUTTON_RIGHT;
    } else if(button == BTN_MIDDLE) {
        buttonCode = GROUNDED_MOUSE_BUTTON_MIDDLE;
    }
    lastPointerSerial = serial;
    //printf("Click serial: %u\n", lastPointerSerial);
    if(pressed && activeWindow && activeWindow->customTitlebarCallback) {
        GroundedWindowCustomTitlebarHit hit = activeWindow->customTitlebarCallback((GroundedWindow*)activeWindow, activeWindow->mouseState.x, activeWindow->mouseState.y);
        if(hit == GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_BAR) {
            /*struct wl_region* region = wl_compositor_create_region(compositor);
            wl_region_add(region, 0, 0, 0, 0);
            wl_surface_set_input_region(activeWindow->surface, region);
            wl_surface_commit(activeWindow->surface);
            wl_region_destroy(region);*/
            xdg_toplevel_move(activeWindow->xdgToplevel, pointerSeat, serial);
            // Do not process this event further
            return;
        } else if(hit == GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_BORDER) {
            u32 edges = 0;
            float x = activeWindow->mouseState.x;
            float y = activeWindow->mouseState.y;
            float width = groundedWindowGetWidth((GroundedWindow*)activeWindow);
            float height = groundedWindowGetHeight((GroundedWindow*)activeWindow);
            float offset = 20.0f;
            if(x < offset) {
                edges |= XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
            } else if(x > width - offset) {
                edges |= XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;
            }
            if(y < offset) {
                edges |= XDG_TOPLEVEL_RESIZE_EDGE_TOP;
            } else if(y > height - offset) {
                edges |= XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;
            }
            xdg_toplevel_resize(activeWindow->xdgToplevel, pointerSeat, serial, edges);
            // Do not process this event further
            return;
        }
    }
    
    if(activeWindow) {
        activeWindow->mouseState.buttons[buttonCode] = pressed;
    }
    if(pressed) {
        eventQueue[eventQueueIndex++] = (GroundedEvent){
            .type = GROUNDED_EVENT_TYPE_BUTTON_DOWN,
            .buttonDown.button = buttonCode,
            .buttonDown.window = (GroundedWindow*)activeWindow,
            .buttonDown.mousePositionX = activeWindow->mouseState.x,
            .buttonDown.mousePositionY = activeWindow->mouseState.y,
        };
    } else {
        eventQueue[eventQueueIndex++] = (GroundedEvent){
            .type = GROUNDED_EVENT_TYPE_BUTTON_UP,
            .buttonUp.button = buttonCode,
            .buttonUp.window = (GroundedWindow*)activeWindow,
            .buttonUp.mousePositionX = activeWindow->mouseState.x,
            .buttonUp.mousePositionY = activeWindow->mouseState.y,
        };
    }
}

static void pointerHandleAxis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
    bool vertical = axis == 0;
    s32 amount = value;
    if(activeWindow) {
        if(vertical) {
            activeWindow->mouseState.scrollDelta = amount / -5000.f;
        } else {
            activeWindow->mouseState.horizontalScrollDelta = amount / -5000.f;
        }
    }
}

static const struct wl_pointer_listener pointerListener = {
    pointerHandleEnter,
    pointerHandleLeave,
    pointerHandleMotion,
    pointerHandleButton,
    pointerHandleAxis,
};



// Read by using two alternating arenas. One persisting the other scratch.
static String8 readIntoBuffer(MemoryArena* arena, int fd) {
    String8 result = {0};

    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    MemoryArena* arenas[2] = {arena, scratch};
    ArenaMarker resetMarkers[2] = {arenaCreateMarker(arena), arenaCreateMarker(scratch)};
    u64 currentSize = 256;
    u32 currentArenaIndex = 1; // We start with the scratch arena as we have to call read twice to determine end of data (return value <= 0)
    u8* currentBuffer = 0;
    u64 actualSize = 0;

    while(true) {
        ASSERT(actualSize <= currentSize);
        arenaResetToMarker(resetMarkers[currentArenaIndex]);
        u8* nextBuffer = ARENA_PUSH_ARRAY_NO_CLEAR(arenas[currentArenaIndex], currentSize, u8);
        if(actualSize > 0) {
            MEMORY_COPY(nextBuffer, currentBuffer, actualSize);
        }
        currentBuffer = nextBuffer;
        
        ssize_t n = read(fd, currentBuffer + actualSize, currentSize - actualSize);
        if(n <= 0) {
            break;
        } else {
            actualSize += n;
        }

        currentSize = actualSize * 2;
        currentSize = CLAMP_TOP(currentSize, INT32_MAX);
        currentArenaIndex = (currentArenaIndex + 1) % 2;
    }

    if(!actualSize) {
        // Error so release allocated persisting memory
        arenaResetToMarker(resetMarkers[0]);
    } else {
        if(currentArenaIndex != 0) {
            // Copy final size to persisting arena
            arenaResetToMarker(resetMarkers[0]);
            u8* buffer = ARENA_PUSH_ARRAY_NO_CLEAR(arenas[0], actualSize, u8);
            MEMORY_COPY(buffer, currentBuffer, actualSize);
            result.base = buffer;
        } else {
            // Release overallocation on persisting arena
            result.base = currentBuffer;
            arenaPopTo(arenas[0], currentBuffer + actualSize);
        }
    }

    result.size = actualSize;
    arenaEndTemp(temp);
    return result;
}

extern const struct wl_data_device_listener dataDeviceListener;

static void seatHandleCapabilities(void *data, struct wl_seat *seat, u32 c) {
    enum wl_seat_capability caps = (enum wl_seat_capability) c;
    if (caps & WL_SEAT_CAPABILITY_POINTER) {
        // Mouse input
        pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer, &pointerListener, 0);
        pointerSeat = seat;
        dataDevice = wl_data_device_manager_get_data_device(dataDeviceManager, seat);
        wl_data_device_add_listener(dataDevice, &dataDeviceListener, 0);
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
    } else if(strcmp(interface, "wp_cursor_shape_manager_v1") == 0) {
        // Does not seem to be supported right now...
    } else if(strcmp(interface, "wl_data_device_manager") == 0) {
        // For drag and drop and clipboard support
        u32 compositorSupportedVersion = version;
        u32 requestedVersion = MIN(version, 1); // We support up to version 3
        dataDeviceManager = (struct wl_data_device_manager*)wl_registry_bind(registry, id, wl_data_device_manager_interface, requestedVersion);
        if(dataDeviceManager) {
            dataDeviceManagerVersion = requestedVersion;
        }
    } else if(strcmp(interface, "wl_shm") == 0) {
        // Shared memory. Needed for custom cursor themes and framebuffers - TODO: might have been replaced by drm (Drm is not particular useful for software rendering)
        waylandShm = (struct wl_shm*)wl_registry_bind(registry, id, wl_shm_interface, 1);
        ASSERT(waylandShm);
        //TODO: We assume that the compositor always comes before wl_shm. I do not see any guarantee by wayland that would actually suggest this so this is probably very unstable!
        ASSERT(compositor);
        if(waylandCursorLibraryPresent) {
            // Load default cursor theme in default size
            const char* sizeText = getenv("XCURSOR_SIZE");
            int cursorSize = 0;
            if(sizeText) {
                cursorSize = (int)strtol(sizeText, 0, 10);
            }
            if(!cursorSize) {
                // Fallback to size 24
                cursorSize = 24;
            }
            const char* cursorThemeName = getenv("XCURSOR_THEME");
            cursorTheme = wl_cursor_theme_load(cursorThemeName, cursorSize, waylandShm);
        }
        if(cursorTheme) {
            cursorSurface = wl_compositor_create_surface(compositor);
        }
        //wl_shm_add_listener(waylandShm, &shmListener, 0);
    }
}

static void registry_global_remove(void *a, struct wl_registry *b, uint32_t c) {}

static const struct wl_registry_listener registryListener = {
    .global = registry_global,
    .global_remove = registry_global_remove
};

//TODO: wl_surface.callback for frame draw callbacks https://wayland-book.com/surfaces-in-depth/frame-callbacks.html
// struct wl_callback *cb = wl_surface_frame(state.wl_surface);
// wl_callback_add_listener(cb, &wl_surface_frame_listener, &state);

//TODO: Should do set_opaque_region for the window if the application window has no transparent bits

//TODO: High-dpi: https://wayland-book.com/surfaces-in-depth/hidpi.html

// This might be called multiple times to accumulate new configuration options. Client should handle those and then respond to the xdgSurface configure witha ack_configure
static void xdgToplevelHandleConfigure(void* data,  struct xdg_toplevel* toplevel,  int32_t width, int32_t height, struct wl_array* states) {
    GroundedWaylandWindow* window = (GroundedWaylandWindow*)data;

    //printf("Configure\n");

    // States array tells us in which state the new window is
    u32* state;
    wl_array_for_each(state, states) {
        switch(*state) {
            case XDG_TOPLEVEL_STATE_MAXIMIZED:{
            } break;
            case XDG_TOPLEVEL_STATE_FULLSCREEN:{

            } break;
            case XDG_TOPLEVEL_STATE_RESIZING:{
                // Nothing additional to do
            } break;
            case XDG_TOPLEVEL_STATE_ACTIVATED:{

            } break;
        }
    }

    // Here we get a new width and height
    if(width && height && (width != window->width || height != window->height)) {
        eventQueue[eventQueueIndex++] = (GroundedEvent){
            .type = GROUNDED_EVENT_TYPE_RESIZE,
            .resize.window = (GroundedWindow*)window,
            .resize.width = width,
            .resize.height = height,
        };
        window->width = width;
        window->height = height;
        #ifdef GROUNDED_OPENGL_SUPPORT
        waylandResizeEglSurface(window);
        #endif
    }
}

static void xdgToplevelHandleClose(void* data, struct xdg_toplevel* toplevel) {
    GroundedWaylandWindow* window = (GroundedWaylandWindow*)data;
    eventQueue[eventQueueIndex++] = (GroundedEvent){
        .type = GROUNDED_EVENT_TYPE_CLOSE_REQUEST,
        .closeRequest.window = (GroundedWindow*)window,
    };
}

static const struct xdg_toplevel_listener xdgToplevelListener = {
    xdgToplevelHandleConfigure,
    xdgToplevelHandleClose
};

// The idea in wayland is that every frame is perfect. So the surface is only redrawn when all state changes have been applied
// This is done by the server sending state changes followed by a configure event where the client should apply the pending state changes and then ack_configure
// See https://wayland-book.com/xdg-shell-basics/xdg-surface.html
static void xdgSurfaceHandleConfigure(void* data, struct xdg_surface* surface, uint32_t serial) {
    xdg_surface_ack_configure(surface, serial);

    GroundedWaylandWindow* window = (GroundedWaylandWindow*) data;
    
    //struct xdg_surface_state *state = xdg_surface_get_pending(surface);
    
    /*if(window.maximized != maximizePending) {

    }*/

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
        #include "types/grounded_wayland_functions.h"
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
        LOAD_WAYLAND_INTERFACE(wl_shm_interface);
        LOAD_WAYLAND_INTERFACE(wl_buffer_interface);
        LOAD_WAYLAND_INTERFACE(wl_shm_pool_interface);
        LOAD_WAYLAND_INTERFACE(wl_region_interface);
        LOAD_WAYLAND_INTERFACE(wl_data_device_manager_interface);
        LOAD_WAYLAND_INTERFACE(wl_data_source_interface);
        LOAD_WAYLAND_INTERFACE(wl_data_device_interface);
        if(firstMissingInterfaceName) {
            error = "Could not load all wayland interfaces. Your wayland version is incompatible";
        }
    }

    if(!error) { // Load wayland cursor function pointers
        void* waylandCursorLibrary = dlopen("libwayland-cursor.so", RTLD_LAZY | RTLD_LOCAL);
        if(waylandCursorLibrary) {
            const char* firstMissingFunctionName = 0;
            #define X(N, R, P) N = (grounded_wayland_##N *) dlsym(waylandCursorLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "types/grounded_wayland_cursor_functions.h"
            #undef X
            if(firstMissingFunctionName) {
                printf("Could not load wayland cursor function: %s\n", firstMissingFunctionName);
                GROUNDED_LOG_WARNING("Could not load all wayland cursor functions. Wayland cursor support might be limited");
            } else {
                waylandCursorLibraryPresent = true;
            }
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
    //if(cursorTheme) wl_cursor_theme_destroy
    //if(cursor) ... free
    if(dragOffer) {
        wl_data_offer_destroy(dragOffer->offer);
        arenaRelease(&dragOffer->arena);
        dragOffer = 0;
    }
    if(dataDevice) {
        wl_data_device_destroy(dataDevice);
    }
    if(dataDeviceManager) {
        wl_data_device_manager_destroy(dataDeviceManager);
    }
    if(waylandDisplay) {
        wl_display_flush(waylandDisplay);
        wl_display_disconnect(waylandDisplay);
    }
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

static void waylandWindowSetMaximized(GroundedWaylandWindow* window, bool maximized) {
    if(maximized) {
        xdg_toplevel_set_maximized(window->xdgToplevel);
    } else {
        xdg_toplevel_unset_maximized(window->xdgToplevel);
    }
}

static void waylandSetBorderless(GroundedWaylandWindow* window, bool borderless) {
    if(decorationManager) {
        ASSERT(window->xdgToplevel);
        if(window->xdgToplevel) {
            struct zxdg_toplevel_decoration_v1* decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(decorationManager, window->xdgToplevel);
            if(borderless) {
                zxdg_toplevel_decoration_v1_set_mode(decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
            } else {
                zxdg_toplevel_decoration_v1_set_mode(decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
            }
        }
    }
}

static void waylandWindowSetUserData(GroundedWaylandWindow* window, void* userData) {
    window->userData = userData;
}

static void* waylandWindowGetUserData(GroundedWaylandWindow* window) {
    return window->userData;
}

static GroundedWindow* waylandCreateWindow(MemoryArena* arena, struct GroundedWindowCreateParameters* parameters) {
    if(!parameters) {
        static struct GroundedWindowCreateParameters defaultParameters = {0};
        parameters = &defaultParameters;
    }
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    GroundedWaylandWindow* window = ARENA_PUSH_STRUCT(arena, GroundedWaylandWindow);
    window->width = parameters->width;
    window->height = parameters->height;
    if(!window->width) window->width = 1920;
    if(!window->height) window->height = 1080;
    window->surface = wl_compositor_create_surface(compositor);
    wl_surface_set_user_data(window->surface, window);
    window->xdgSurface = xdg_wm_base_get_xdg_surface(xdgWmBase, window->surface);
    ASSERT(window->xdgSurface);
    xdg_surface_add_listener(window->xdgSurface, &xdgSurfaceListener, window);
    //xdg_surface_set_window_geometry(window->xdgSurface, x, y, window->width, window->height)
    window->xdgToplevel = xdg_surface_get_toplevel(window->xdgSurface);
    xdg_toplevel_add_listener(window->xdgToplevel, &xdgToplevelListener, window);
    if(!str8IsEmpty(parameters->applicationId)) {
        xdg_toplevel_set_app_id(window->xdgToplevel, str8GetCstr(scratch, parameters->applicationId));
    }
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

    // Set decorations
    if(parameters->customTitlebarCallback) {
        window->customTitlebarCallback = parameters->customTitlebarCallback;
        waylandSetBorderless(window, true);
    } else {
        waylandSetBorderless(window, parameters->borderless);
    }
    //struct zxdg_toplevel_decoration_v1* decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(decorationManager, window->xdgToplevel);
    //zxdg_toplevel_decoration_v1_add_listener(decoration, &decorationListener, window);
    //zxdg_toplevel_decoration_v1_set_mode(decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);

    window->dndCallback = parameters->dndCallback;

    wl_surface_commit(window->surface);

    wl_display_roundtrip(waylandDisplay);

    arenaEndTemp(temp);

    return (GroundedWindow*)window;
}

static void waylandDestroyWindow(GroundedWaylandWindow* window) {
    //TODO: Exception here after window close! Surface is probably null
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

static GroundedEvent* waylandPollEvents(u32* eventCount) {
    eventQueueIndex = 0;

    // Sends out pending requests to all event queues
    wl_display_roundtrip(waylandDisplay);

    // Dispatches pending events from server
    wl_display_dispatch_pending(waylandDisplay);

    *eventCount = eventQueueIndex;
    return eventQueue;
}

static GroundedEvent* waylandGetEvents(u32* eventCount, u32 maxWaitingTimeInMs) {
    eventQueueIndex = 0;
    static bool firstInvocation = true;

    // In the first invovation we just want to poll as this might otherwise block applications
    // which will never get to present anyhing to the surface and therefore the surface never 
    // gets visible and never receives events
    if(firstInvocation) {
        firstInvocation = false;
        return waylandPollEvents(eventCount);
    }

    /*while(wl_display_prepare_read(waylandDisplay) != 0) {
        wl_display_dispatch_pending(waylandDisplay);
    }

    // Flush display
    while(wl_display_flush(waylandDisplay) == -1) {
        if(errno != EAGAIN) {
            return 0;
        }
        struct pollfd fd = {wl_display_get_fd(waylandDisplay), POLLOUT};
        while(poll(&fd, 1, -1) == -1) {
            if(errno != EINTR && errno != EAGAIN) {
                return 0;
            }
        }
    }*/

    if(waylandPoll(maxWaitingTimeInMs)) {
        wl_display_dispatch(waylandDisplay);
    }

    *eventCount = eventQueueIndex;
    return eventQueue;
}

static void waylandFetchKeyboardState(GroundedKeyboardState* keyboardState) {
    *keyboardState = waylandKeyState;
}

static void waylandFetchMouseState(GroundedWaylandWindow* window, MouseState* mouseState) {
    mouseState->x = window->mouseState.x;
    mouseState->y = window->mouseState.y;

    // Set mouse button state
    memcpy(mouseState->buttons, window->mouseState.buttons, sizeof(mouseState->buttons));

    mouseState->scrollDelta = window->mouseState.scrollDelta;
    mouseState->horizontalScrollDelta = window->mouseState.horizontalScrollDelta;
    mouseState->deltaX = mouseState->x - mouseState->lastX;
    mouseState->deltaY = mouseState->y - mouseState->lastY;

    window->mouseState.scrollDelta = 0.0f;
    window->mouseState.horizontalScrollDelta = 0.0f;
}

GROUNDED_FUNCTION void waylandSetIcon(u8* data, u32 width, u32 height) {
    /*
     * This is probably the WORST part about the wayland API. Wayland expects that all applications are shipped with corresponding
     * .desktop files that describe what icon to use. As I want to allow applications to ship as a single executable file and still
     * have an icon I have to do some really nasty tricks to make it work. (There are also unofficial ways with DBUS etc. to achieve
     * this but those only work on specific window managers)
     * .desktop files traditionally live in /usr/share/applications/app_id.desktop. As this directory typically belongs to root, we can
     * not create a desktop file there without running as root which we obviously dont want to do. But we can check wheter the corresponding
     * files exist there. If so the application has probably been installed via a system specific way (typically a package manager) and we do
     * not have to set the icon specifically. If the application wanted to set a different icon it is silently ignored here but I give the system
     * specified icon precedence.
     * The other directory is ~/.local/share/applciations/app_id.desktop this has precedence over /usr/share. However I do not really want to
     * create persisting files in the home directory of the user. If he would install the application later using a package manager the created
     * desktop file in his user directory would overwrite the system .desktop file and the application might not work or use a wrong version.
     * So the idea is to create the .desktop file temporarily. But how do we do this in a robust way so that it is cleaned even if the application crashes?
     * We can mark it as being generated with a comment.
     * See also here for why icon support in wayland is definately not required. https://gitlab.freedesktop.org/wayland/wayland-protocols/-/issues/52
     */

    // TODO: Problem is that we have to save the image as png
}

GROUNDED_FUNCTION void waylandSetCursorType(enum GroundedMouseCursor cursorType) {
    const char* error = 0;
    if(waylandCursorTypeOverwrite < GROUNDED_MOUSE_CURSOR_COUNT) {
        waylandCursorTypeOverwrite = cursorType;
    } else if(waylandCursorLibraryPresent && cursorTheme) {
        struct wl_cursor* cursor = 0;
        struct wl_cursor_image* cursorImage = 0;
        struct wl_buffer* cursorBuffer = 0;
        int scale = 1;

        u64 candidateCount = 0;
        const char** candidateNames = getCursorNameCandidates(cursorType, &candidateCount);

        for(u64 i = 0; i < candidateCount; ++i) {
            cursor = wl_cursor_theme_get_cursor(cursorTheme, candidateNames[i]);
            if(cursor) {
                break;
            }
        }
        if(!cursor) {
            error = "Could not find cursor";
        } else {
            cursorImage = cursor->images[0];
            if(!cursorImage) {
                error = "Could not load cursor image";
            }
        }
        if(!error) {
            cursorBuffer = wl_cursor_image_get_buffer(cursorImage);
            if(!cursorBuffer) {
                error = "Could not get cursor buffer";
            }
        }
        if(!error) {
            wl_pointer_set_cursor(pointer, pointerEnterSerial, cursorSurface, cursorImage->hotspot_x / scale, cursorImage->hotspot_y / scale);
            wl_surface_set_buffer_scale(cursorSurface, scale);
            wl_surface_attach(cursorSurface, cursorBuffer, 0, 0);
            wl_surface_damage(cursorSurface, 0, 0, cursorImage->width, cursorImage->height);
            wl_surface_commit(cursorSurface);
            waylandCurrentCursorType = cursorType;
        }
    } else {
        error = "Wayland compositor does not support required cursor interface";
    }
    if(error) {
        GROUNDED_LOG_WARNING("Could not satisfy cursor request");
    }
}

GROUNDED_FUNCTION void waylandSetCustomCursor(u8* data, u32 width, u32 height) {
    u64 imageSize = width * height * 4;
    int fd = createSharedMemoryFile(imageSize);
    void* shmData = mmap(0, imageSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    memcpy(shmData, data, imageSize);
    int scale = 1;

    struct wl_buffer* cursorBuffer = 0;
    struct wl_shm_pool* pool = wl_shm_create_pool(waylandShm, fd, imageSize);
    cursorBuffer = wl_shm_pool_create_buffer(pool, 0, width, height, width*4, WL_SHM_FORMAT_ARGB8888);

    s32 hotspotX = 0;
    s32 hotspotY = 0;
    wl_pointer_set_cursor(pointer, pointerEnterSerial, cursorSurface, hotspotX / scale, hotspotY / scale);
    wl_surface_set_buffer_scale(cursorSurface, scale);
    wl_surface_attach(cursorSurface, cursorBuffer, 0, 0);
    wl_surface_damage(cursorSurface, 0, 0, width, height);
    wl_surface_commit(cursorSurface);

    // Now that the new surface is commited we can do cleanup
    close(fd);
    wl_shm_pool_destroy(pool);

    waylandCurrentCursorType = GROUNDED_MOUSE_CURSOR_CUSTOM;
}


//*************
// OpenGL stuff
#ifdef GROUNDED_OPENGL_SUPPORT
static EGLDisplay waylandEglDisplay;
static void* waylandEglLibrary;
//TODO: We seem to load the function pointers multiple times here
GROUNDED_FUNCTION GroundedOpenGLContext* waylandCreateOpenGLContext(MemoryArena* arena, GroundedOpenGLContext* contextToShareResources) {
    GroundedOpenGLContext* result = ARENA_PUSH_STRUCT(arena, GroundedOpenGLContext);
    if(!waylandEglLibrary) { // Load egl function pointers
        waylandEglLibrary = dlopen("libwayland-egl.so", RTLD_LAZY | RTLD_LOCAL);
        if(!waylandEglLibrary) {
            const char* error = "No wayland-egl library found";
            GROUNDED_LOG_ERROR(error);
            return false;
        } else {
            const char* firstMissingFunctionName = 0;
            #define X(N, R, P) N = (grounded_wayland_##N *) dlsym(waylandEglLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "types/grounded_wayland_egl_functions.h"
            #undef X
            if(firstMissingFunctionName) {
                printf("Could not load wayland function: %s\n", firstMissingFunctionName);
                const char* error = "Could not load all wayland-egl functions. Your wayland-egl version is incompatible";
                GROUNDED_LOG_ERROR(error);
                dlclose(waylandEglLibrary);
                waylandEglLibrary = 0;
                return false;
            }
        }
    }

    if(!waylandEglDisplay) {
        waylandEglDisplay = eglGetDisplay((EGLNativeDisplayType)waylandDisplay);
        if(waylandEglDisplay == EGL_NO_DISPLAY) {
            GROUNDED_LOG_ERROR("Error obtaining EGL display for wayland display");
            dlclose(waylandEglLibrary);
            waylandEglLibrary = 0;
            waylandEglDisplay = 0;
            return false;
        }
        int eglVersionMajor = 0;
        int eglVersionMinor = 0;
        if(!eglInitialize(waylandEglDisplay, &eglVersionMajor, &eglVersionMinor)) {
            GROUNDED_LOG_ERROR("Error initializing EGL display");
            eglTerminate(waylandEglDisplay);
            dlclose(waylandEglLibrary);
            waylandEglLibrary = 0;
            waylandEglDisplay = 0;
            return false;
        }
        //LOG_INFO("Using EGL version ", eglVersionMajor, ".", eglVersionMinor);

        
    }

    // OPENGL_ES available instead of EGL_OPENGL_API
    if(!eglBindAPI(EGL_OPENGL_API)) {
        GROUNDED_LOG_ERROR("Error binding OpenGL API");
        eglTerminate(waylandEglDisplay);
        dlclose(waylandEglLibrary);
        waylandEglLibrary = 0;
        waylandEglDisplay = 0;
        return false;
    }

    //TODO: Config is required when creating the context and when creating the window
    EGLConfig config;
    int numConfigs = 0;
    int attribList[] = {EGL_RED_SIZE, 1, EGL_GREEN_SIZE, 1, EGL_BLUE_SIZE, 1, EGL_NONE};
    if(!eglChooseConfig(waylandEglDisplay, attribList, &config, 1, &numConfigs) || numConfigs <= 0) {
        GROUNDED_LOG_ERROR("Error choosing OpenGL config");
        return false;
    }

    EGLContext shareContext = contextToShareResources ? contextToShareResources->eglContext : EGL_NO_CONTEXT;
    result->eglContext = eglCreateContext(waylandEglDisplay, config, shareContext, 0);
    if(result->eglContext == EGL_NO_CONTEXT) {
        GROUNDED_LOG_ERROR("Error creating EGL Context");
        return false;
    }
    return result;
}

static bool createEglSurface(GroundedWaylandWindow* window) {
    // if config is 0 the total number of configs is returned
    EGLConfig config;
    int numConfigs = 0;
    int attribList[] = {EGL_RED_SIZE, 1, EGL_GREEN_SIZE, 1, EGL_BLUE_SIZE, 1, EGL_NONE};
    if(!eglChooseConfig(waylandEglDisplay, attribList, &config, 1, &numConfigs) || numConfigs <= 0) {
        GROUNDED_LOG_ERROR("Error choosing OpenGL config");
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

static void waylandResizeEglSurface(GroundedWaylandWindow* window) {
    ASSERT(window->eglWindow);
    ASSERT(window->eglSurface);
    
    wl_egl_window_resize(window->eglWindow, window->width, window->height, 0, 0);
}

GROUNDED_FUNCTION void waylandOpenGLMakeCurrent(GroundedWaylandWindow* window, GroundedOpenGLContext* context) {
    if(!window->eglWindow || !window->eglSurface) {
        createEglSurface(window);
    }
    if(!eglMakeCurrent(waylandEglDisplay, window->eglSurface, window->eglSurface, context->eglContext)) {
        //LOG_INFO("Error: ", eglGetError());
        GROUNDED_LOG_ERROR("Error making OpenGL context current");
        //return false;
    }
}

GROUNDED_FUNCTION void waylandWindowGlSwapBuffers(GroundedWaylandWindow* window) {
    eglSwapBuffers(waylandEglDisplay, window->eglSurface);
}

GROUNDED_FUNCTION void waylandWindowSetGlSwapInterval(int interval) {
    eglSwapInterval(waylandEglDisplay, interval);
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





































//#include "wayland_data_device.c"

/*
 * Encountered mime types:
 * text/plain;charset=utf-8
 * text/plain
 * chromium/x-web-custom-data   Some custom chromium data. Probably not interesting
 * text/html
 * text/plain
 * text/uri-list    This is a list of files/uris
 * application/x-kde4-urilist
 * application/vnd.portal.filetransfer Special type for FileTransfer protocol which seems to be a gtk thing
 * text/x-moz-url
 * _NETSCAPE_URL
 * text/x-moz-url-data
 * text/x-moz-url-desc
 * application/x-moz-custom-clipdata
 * text/_moz_htmlcontext
 * text/_moz_htmlinfo
 * application/x-moz-nativeimage
 * image/png
 * image/jpeg
 * image/jpg
 * image/gif
 * application/x-moz-file-promise
 * application/x-moz-file-promise-url
 * text/x-uri
 * application/x-moz-file-promise-dest-filename
 * application/x-kde-suggestedfilename
 */

static void dataOfferHandleOffer(void* userData, struct wl_data_offer* offer, const char* mimeType) {
    // Sent immediately after creating a wl_data_offer object once for every mime type.
    
    struct WaylandDataOffer* waylandOffer = (struct WaylandDataOffer*)userData;
    // Basically mark the data offer with the type it has. Is called once for every available mime type of the offer
    // Typical stuff: text/plain;charset=utf-8, text/uri-list, etc.

    // The first mime types are not actual mime types. They are all caps-lock and maybe we want to filter them out

    // I do not know if a copy is really necessary but it defenitely feels safer
    str8ListPushCopyAndNullTerminate(&waylandOffer->arena, &waylandOffer->availableMimeTypeList, str8FromCstr(mimeType));
}

static void dataOfferHandleSourceActions(void* userData, struct wl_data_offer *wl_data_offer, uint32_t sourceActions) {
    ASSERT(dataDeviceManagerVersion >= 3);
    // This gives the actions that are supported by the source side. Called directly after the mime types and when source changes available actions
    // We only receive this for dnd data offers

    struct WaylandDataOffer* waylandOffer = (struct WaylandDataOffer*)userData;
    waylandOffer->allowedActions = 0;

    printf("Data offer source actions\n");
    waylandOffer->allowedActions = sourceActions;
    // We do not have to do anything further upon receiving this event

    /*if(sourceActions & WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY) {
        waylandOffer->allowedActions = ;
    }
    if(sourceActions & WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE) {
        //waylandOffer->allowedActions = ;
    }
    if(sourceActions & WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK) {
        // Source wants us to show a dialog which action the user wants to perform on drop
        //waylandOffer->allowedActions = ;
    }*/
}

static void dataOfferHandleAction(void* userData, struct wl_data_offer* dataOffer, uint32_t dndAction) {
    // The action the compositor selected for us based on source and destination preferences. We simply ignore it for now...
    // Most recent action received is always the valid one
    //printf("Data offer action\n");
    ASSERT(dataDeviceManagerVersion >= 3);

    //TODO: Apparently action changes can still happen after drop when we set other action for example because of ask
    ASSERT(dragOffer);
    struct WaylandDataOffer* waylandOffer = (struct WaylandDataOffer*)userData;
    waylandOffer->selectedAction = dndAction;

    if(dndAction == WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK) {
        // If we receive ask we can select an action ourselves with wl_data_offer_set_action
        u32 preferredAction = WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE;
        wl_data_offer_set_actions(dataOffer, waylandOffer->allowedActions & (~WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK), preferredAction);
    }
}

static const struct wl_data_offer_listener dataOfferListener = {
    dataOfferHandleOffer,
    dataOfferHandleSourceActions,
    dataOfferHandleAction,
};



static void dataDeviceListenerOffer(void* data, struct wl_data_device* dataDevice, struct wl_data_offer* offer) {
    // Compositor announces a new data offer. Can be dnd or clipboard
    printf("DataDevice offer\n");

    struct WaylandDataOffer* waylandOffer = ARENA_BOOTSTRAP_PUSH_STRUCT(createGrowingArena(osGetMemorySubsystem(), KB(4)), struct WaylandDataOffer, arena);
    waylandOffer->offer = offer;
    waylandOffer->lastAcceptedMimeIndex = 0xFFFFFFFF;

    // Add listener which tells us, what data type the offer contains
    wl_data_offer_add_listener(offer, &dataOfferListener, waylandOffer);
}

static void updateWaylandDragPosition(GroundedWaylandWindow* window, struct WaylandDataOffer* waylandOffer, s32 posX, s32 posY) {
    waylandOffer->x = posX;
    waylandOffer->y = posY;
    u32 newMimeIndex = window->dndCallback(0, (GroundedWindow*)window, posX, posY, waylandOffer->mimeTypeCount, waylandOffer->mimeTypes, &waylandOffer->dropCallback);
    if(newMimeIndex != waylandOffer->lastAcceptedMimeIndex) {
        if(newMimeIndex < waylandOffer->mimeTypeCount) {
            wl_data_offer_accept(waylandOffer->offer, waylandOffer->enterSerial, (const char*)waylandOffer->mimeTypes[newMimeIndex].base);
        } else {
            wl_data_offer_accept(waylandOffer->offer, waylandOffer->enterSerial, 0);
        }
        waylandOffer->lastAcceptedMimeIndex = newMimeIndex;
    }
}

static void dataDeviceListenerEnter(void* data, struct wl_data_device* dataDevice, u32 serial, struct wl_surface* surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer* offer) {
    // At this point we know that the data offer is a drag offer
    struct WaylandDataOffer* waylandOffer = (struct WaylandDataOffer*)wl_data_offer_get_user_data(offer);
    ASSERT(waylandOffer);
    waylandOffer->dnd = true;
    waylandOffer->enterSerial = serial;

    GroundedWaylandWindow* window = 0;
    if(surface) {
        window = (GroundedWaylandWindow*)wl_surface_get_user_data(surface);
        ASSERT(window->surface == surface);
        waylandOffer->window = window;
        dragOffer = waylandOffer;
    }

    if(window && window->dndCallback) {
        u32 mimeCount = 0;
        for(String8Node* node = waylandOffer->availableMimeTypeList.first; node != 0; node = node->next) {
            mimeCount++;
        }
        waylandOffer->lastAcceptedMimeIndex = UINT32_MAX;
        if(mimeCount > 0) {
            waylandOffer->mimeTypeCount = mimeCount;
            waylandOffer->mimeTypes = ARENA_PUSH_ARRAY(&waylandOffer->arena, mimeCount, String8);
            u32 mimeIndex = 0;
            for(String8Node* node = waylandOffer->availableMimeTypeList.first; node != 0; node = node->next) {
                waylandOffer->mimeTypes[mimeIndex++] = node->string;
            }
        }

        // Now we do the same like in dataDeviceListenerMotion and ask client what it would accept
        s32 posX = wl_fixed_to_int(x);
        s32 posY = wl_fixed_to_int(y);
        updateWaylandDragPosition(window, waylandOffer, posX, posY);
    }

    printf("Data offer enter\n");
}

static void dataDeviceListenerLeave(void* data, struct wl_data_device* dataDevice) {
    // We have to estroy the offer
    ASSERT(dragOffer);
    ASSERT(dragOffer->dnd);
    wl_data_offer_destroy(dragOffer->offer);
    arenaRelease(&dragOffer->arena);
    dragOffer = 0;

    printf("Data offer leave\n");
}

static void dataDeviceListenerMotion(void* data, struct wl_data_device* dataDevice, u32 time, wl_fixed_t x, wl_fixed_t y) {
    printf("Data offer move\n");
    struct WaylandDataOffer* waylandOffer = dragOffer;
    GroundedWaylandWindow* window = 0;
    if(waylandOffer != 0) {
        window = waylandOffer->window;
    }

    if(window && window->dndCallback) {
        s32 posX = wl_fixed_to_int(x);
        s32 posY = wl_fixed_to_int(y);
        updateWaylandDragPosition(window, waylandOffer, posX, posY);
    }
}

static void dataDeviceListenerDrop(void* data, struct wl_data_device* dataDevice) {
    printf("Data offer drop\n");

    struct WaylandDataOffer* waylandOffer = dragOffer;
    ASSERT(waylandOffer);
    
    // We should honor last action received from dataOffer.action. If it is copy or move we can do receive requests. End transfer with wl_data_offer_finish()
    
    if(waylandOffer->lastAcceptedMimeIndex < waylandOffer->mimeTypeCount) {
        int fds[2];
	    pipe(fds);
        String8 mimeType = waylandOffer->mimeTypes[waylandOffer->lastAcceptedMimeIndex];
        // We know that mimetype is 0-terminated as we copy it with null termination when creating
        ASSERT(mimeType.base[mimeType.size] == '\0');
	    wl_data_offer_receive(waylandOffer->offer, (const char*)mimeType.base, fds[1]);
        close(fds[1]);

        // Roundtrip to receive data. Especially necessary if we are source and destination
        wl_display_roundtrip(waylandDisplay);

        // Read in data
        String8 data = readIntoBuffer(&waylandOffer->arena, fds[0]);
	    close(fds[0]);

        // Options to send data:
        // 1. Reuse existing window->dndCallback to send final data maybe with special flag specifying drop
        // 2. Add additional drop callback
        // 3. Add as an event to the event queue
        // First 2 options have way easier memory management as it can immediately be released here.
        // However to me it seems cleaner to handle it in the usual event queue. However this opens the question how the memory should be handled.
        // We do not want to require the client to have to release the data when it has finished as it might decide to not handle it at all.
        // A delete queue once the events are requested a second time? What if a client calls it multiple times in the hope for more events?
        // Could make this very explicit as the events are always provided as a list which is reused on the next call
        // Other idea: dndCallback specifies function which should be called upon drop. 
        // Allows for flexibility of different functions for different windows
        // Control and data flow is very clear. I decided to implement this
        if(waylandOffer->dropCallback) {
            waylandOffer->dropCallback(0, data, (GroundedWindow*)waylandOffer->window, waylandOffer->x, waylandOffer->y, mimeType);
        }
    }

    if(dataDeviceManagerVersion >= 3) {
        wl_data_offer_finish(waylandOffer->offer);
    }
    wl_data_offer_destroy(waylandOffer->offer);
    arenaRelease(&waylandOffer->arena);
    dragOffer = 0;
}

static void dataDeviceListenerSelection(void* data, struct wl_data_device* dataDevice, struct wl_data_offer* dataOffer) {
    // This is for copy and paste
    struct WaylandDataOffer* waylandOffer = (struct WaylandDataOffer*)wl_data_offer_get_user_data(dataOffer);
    ASSERT(waylandOffer);
    waylandOffer->dnd = false;
}

const struct wl_data_device_listener dataDeviceListener = {
    dataDeviceListenerOffer,
    dataDeviceListenerEnter,
    dataDeviceListenerLeave,
    dataDeviceListenerMotion,
    dataDeviceListenerDrop,
    dataDeviceListenerSelection,
};



struct WaylandDataSource {
    MemoryArena* arena;
    String8* mimeTypes;
    u64 mimeTypeCount;
    GroundedWindowDndSendCallback* sendCallback;
    GroundedWindowDndCancelCallback* cancelCallback;
    void* userData;
    //enum wl_data_device_manager_dnd_action last_dnd_action = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
};

static void dataSourceHandleTarget(void* data, struct wl_data_source* source, const char* mimeType) {
	if (mimeType && *mimeType) {
		printf("Destination would accept MIME type if dropped: %s\n", mimeType);
        setCursorOverwrite(GROUNDED_MOUSE_CURSOR_GRABBING);
	} else {
		printf("Destination would reject if dropped\n");
        setCursorOverwrite(GROUNDED_MOUSE_CURSOR_DND_NO_DROP);
	}
}

static void dataSourceHandleSend(void *data, struct wl_data_source *wl_data_source, const char* _mimeType, int32_t fd) {
    struct WaylandDataSource* waylandDataSource = (struct WaylandDataSource*) data;
    ASSERT(waylandDataSource);
    //ASSERT(waylandDataSource->sendCallback);
    printf("Target requests data\n");
    
    // Get index of mimeType
    String8 mimeType = str8FromCstr(_mimeType);
    u64 mimeTypeIndex = UINT64_MAX;
    if(waylandDataSource) {
        for(u64 i = 0; i < waylandDataSource->mimeTypeCount; ++i) {
            if(str8Compare(waylandDataSource->mimeTypes[i], mimeType)) {
                mimeTypeIndex = i;
                mimeType = waylandDataSource->mimeTypes[i];
                break;
            }
        }
    }

    if(mimeTypeIndex < waylandDataSource->mimeTypeCount && waylandDataSource->sendCallback) {
        //TODO: Cache data of this mimetype
        String8 data = waylandDataSource->sendCallback(waylandDataSource->arena, mimeType, mimeTypeIndex, waylandDataSource->userData);
        write(fd, data.base, data.size);
    }
	close(fd);
}

// Drop has been cancelled. Now we can release resources. Only gets called when replaced by a new data source
static void dataSourceHandleCancelled(void *data, struct wl_data_source * dataSource) {
    // If version is <= 2 this is only sent when the data source has been replaced by another source
    struct WaylandDataSource* waylandDataSource = (struct WaylandDataSource*) data;
    if(waylandDataSource->cancelCallback) {
        waylandDataSource->cancelCallback(waylandDataSource->arena, waylandDataSource->userData);
    }
    
    removeCursorOverwrite();
    wl_data_source_destroy(dataSource);
    arenaRelease(waylandDataSource->arena);
}

// Since version 3: Basically no useful information for us so we do nothing
static void dataSourceHandleDndDropPerformed(void *data, struct wl_data_source *wl_data_source) {}

// Since version 3: We are now allowed to free all resources as drop was successful
static void dataSourceHandleDndFinished(void *data, struct wl_data_source* dataSource) {
    struct WaylandDataSource* waylandDataSource = (struct WaylandDataSource*) data;
    removeCursorOverwrite();
    wl_data_source_destroy(dataSource);
    arenaRelease(waylandDataSource->arena);
}

static void dataSourceHandleAction(void *data, struct wl_data_source *source, u32 dnd_action) {
	//last_dnd_action = dnd_action;
	switch (dnd_action) {
	case WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE:
		printf("Destination would perform a move action if dropped\n");
        setCursorOverwrite(GROUNDED_MOUSE_CURSOR_GRABBING);
		break;
	case WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY:
		printf("Destination would perform a copy action if dropped\n");
        setCursorOverwrite(GROUNDED_MOUSE_CURSOR_GRABBING);
		break;
	case WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE:
		printf("Destination would reject the drag if dropped\n");
        setCursorOverwrite(GROUNDED_MOUSE_CURSOR_DND_NO_DROP);
		break;
	}
}

static struct wl_data_source_listener dataSourceListener = {
    dataSourceHandleTarget,
    dataSourceHandleSend,
    dataSourceHandleCancelled,
    dataSourceHandleDndDropPerformed,
    dataSourceHandleDndFinished,
    dataSourceHandleAction,
};

struct GroundedWindowDragPayloadDescription {
    MemoryArena arena;
    struct wl_surface* icon;
    u32 mimeTypeCount;
    String8* mimeTypes;
    GroundedWindowDndSendCallback* sendCallback;
    GroundedWindowDndCancelCallback* cancelCallback;
};

GROUNDED_FUNCTION GroundedWindowDragPayloadDescription* groundedWindowPrepareDragPayload(GroundedWindow* window) {
    GroundedWindowDragPayloadDescription* result = ARENA_BOOTSTRAP_PUSH_STRUCT(createGrowingArena(osGetMemorySubsystem(), KB(4)), GroundedWindowDragPayloadDescription, arena);
    return result;
}

GROUNDED_FUNCTION MemoryArena* groundedWindowDragPayloadGetArena(GroundedWindowDragPayloadDescription* desc) {
    return &desc->arena;
}

GROUNDED_FUNCTION void groundedWindowDragPayloadSetImage(GroundedWindowDragPayloadDescription* desc, u8* data, u32 width, u32 height) {
    // It is not allowed to set the image twice
    ASSERT(!desc->icon);
    
    if(!desc->icon) {
        desc->icon = wl_compositor_create_surface(compositor);
        if(desc->icon) {
            u64 imageSize = width * height * sizeof(u32);
            int fd = createSharedMemoryFile(imageSize);
            u8* poolData = mmap(0, imageSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            struct wl_shm_pool* pool = wl_shm_create_pool(waylandShm, fd, imageSize);

            struct wl_buffer* wlBuffer = wl_shm_pool_create_buffer(pool, 0, width, height, width * sizeof(u32), WL_SHM_FORMAT_XRGB8888);
            MEMORY_COPY(poolData, data, imageSize);

            wl_surface_attach(desc->icon, wlBuffer, 0, 0);
            wl_surface_damage(desc->icon, 0, 0, UINT32_MAX, UINT32_MAX);
            wl_surface_commit(desc->icon);
        }
    }
}

//TODO: Could create linked list of mime types. This would allow an API where the user can add mime types to a list. 
// Even different handling functions per mime type would be possible but is this actually desirable?
GROUNDED_FUNCTION void groundedWindowDragPayloadSetMimeTypes(GroundedWindowDragPayloadDescription* desc, u32 mimeTypeCount, String8* mimeTypes) {
    // Only allowed to set mime types once
    ASSERT(!desc->mimeTypeCount);
    ASSERT(!desc->mimeTypes);

    desc->mimeTypes = ARENA_PUSH_ARRAY_NO_CLEAR(&desc->arena, mimeTypeCount, String8);
    for(u64 i = 0; i < mimeTypeCount; ++i) {
        desc->mimeTypes[i] = str8CopyAndNullTerminate(&desc->arena, mimeTypes[i]);
    }
    desc->mimeTypeCount = mimeTypeCount;
}

GROUNDED_FUNCTION void groundedWindowDragPayloadSetSendCallback(GroundedWindowDragPayloadDescription* desc, GroundedWindowDndSendCallback* callback) {
    desc->sendCallback = callback;
}

GROUNDED_FUNCTION void groundedWindowDragPayloadSetCancelCallback(GroundedWindowDragPayloadDescription* desc, GroundedWindowDndCancelCallback* callback) {
    desc->cancelCallback = callback;
}

GROUNDED_FUNCTION void groundedWindowBeginDragAndDrop(GroundedWindowDragPayloadDescription* desc, void* userData) {
    // Serial is the last pointer serial. Should probably be pointer button serial
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    
    struct WaylandDataSource* waylandDataSource = ARENA_PUSH_STRUCT(&desc->arena, struct WaylandDataSource);
    waylandDataSource->arena = &desc->arena;
    waylandDataSource->sendCallback = desc->sendCallback;
    waylandDataSource->cancelCallback = desc->cancelCallback;
    waylandDataSource->userData = userData;
    waylandDataSource->mimeTypeCount = desc->mimeTypeCount;
    waylandDataSource->mimeTypes = desc->mimeTypes;

    struct wl_data_source* dataSource = wl_data_device_manager_create_data_source(dataDeviceManager);
    wl_data_source_add_listener(dataSource, &dataSourceListener, waylandDataSource);
    for(u64 i = 0; i < waylandDataSource->mimeTypeCount; ++i) {
        wl_data_source_offer(dataSource, str8GetCstr(scratch, waylandDataSource->mimeTypes[i]));
    }
    if(dataDeviceManagerVersion >= 3) {
        wl_data_source_set_actions(dataSource, WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE | WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
    }

    setCursorOverwrite(GROUNDED_MOUSE_CURSOR_GRABBING);

    struct wl_surface* icon = desc->icon;

    //ASSERT(activeWindow == desc->window);
    printf("Drag serial: %u\n", lastPointerSerial);
    wl_data_device_start_drag(dataDevice, dataSource, activeWindow->surface, desc->icon, lastPointerSerial);

    arenaEndTemp(temp);
}

#include <grounded/window/grounded_window.h>
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
#include <sys/timerfd.h> // For timerfd_settime

#ifdef GROUNDED_OPENGL_SUPPORT
//#include <EGL/egl.h>
#endif

#if 1
#define GROUNDED_WAYLAND_LOG_CALL(name)
#define GROUNDED_WAYLAND_LOG_HANDLER(name)
#else
#define GROUNDED_WAYLAND_LOG_CALL(name) GROUNDED_LOG_VERBOSE("> " name)
#define GROUNDED_WAYLAND_LOG_HANDLER(name) GROUNDED_LOG_VERBOSE("< " name)
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
struct wl_output;

typedef struct GroundedWaylandWindow {
    struct wl_surface* surface;
    struct xdg_surface* xdgSurface;
    struct xdg_toplevel* xdgToplevel;
    struct zwp_idle_inhibitor_v1* idleInhibitor;
    struct zwp_confined_pointer_v1* confinedPointer;
    u32 width, minWidth, maxWidth;
    u32 height, minHeight, maxHeight;
    void* userData;
    void* dndUserData;
    GroundedWindowCustomTitlebarCallback* customTitlebarCallback;
    MouseState mouseState;
    String8 applicationId;
    String8 title; // Guaranteed to be 0-terminated
    char titleBuffer[256];
    bool borderless, inhibitIdle, fullscreen;
    int shm; // Shared memory for framebuffers
    struct wl_buffer* waylandBuffers[2]; // Double buffering. Index 0 is the one currently drawn to
    void* framebufferPointers[2]; // Double buffering. Index 0 is the one currently drawn to
	u32 framebufferWidth; // Those values need to exist because the window width and height can change asynchronously
	u32 framebufferHeight;
    char* foreignHandle;

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
struct wl_registry* registry;
struct xdg_wm_base* xdgWmBase;
struct wl_keyboard* keyboard;
struct wl_pointer* pointer;
struct zwp_relative_pointer_v1* relativePointer;
u32 lastPointerSerial;
struct wl_seat* pointerSeat;
struct wl_data_device* dataDevice; // Data device tied to pointerSeat
u32 pointerEnterSerial;
struct WaylandDataSource* dragDataSource; // Data source of the current drag

struct zxdg_decoration_manager_v1* decorationManager;
struct zxdg_output_manager_v1* xdgOutputManager;
struct zwp_idle_inhibit_manager_v1* idleInhibitManager;
struct zwp_pointer_constraints_v1* pointerConstraints;
struct zwp_relative_pointer_manager_v1* relativePointerManager;

struct zxdg_exporter_v2* zxdgExporter;

struct wl_shm* waylandShm; // Shared memory interface to compositor
struct wl_cursor_theme* cursorTheme;
struct wl_surface* cursorSurface;
bool waylandCursorLibraryPresent;
GroundedMouseCursor waylandCurrentCursorType = GROUNDED_MOUSE_CURSOR_DEFAULT;
GroundedMouseCursor waylandCursorTypeOverwrite = GROUNDED_MOUSE_CURSOR_COUNT;
int keyRepeatTimer = -1;
u32 keyRepeatDelay = 500;
u32 keyRepeatRate = 25;
u32 keyRepeatKey;
GroundedWaylandWindow* activeWindow; // The window (if any) the mouse cursor is currently hovering

GroundedKeyboardState waylandKeyState;

struct WaylandDataOffer {
    String8List availableMimeTypeList;
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
    struct GroundedDragPayload payload;
    s32 x;
    s32 y;
};

struct WaylandScreen {
    GroundedWindowDisplay display;
    struct wl_output* output;
    struct zxdg_output_v1* xdgOutput;
    u32 registryId;
    float refreshRate;
    float scaleFactor;
    bool active;
    String8 description;
};

#define MAX_WAYLAND_SCREEN_COUNT 16
struct WaylandScreen waylandScreens[MAX_WAYLAND_SCREEN_COUNT];
u32 outputVersion; // Version of the wl_output interface
u32 xdgOutputVersion;
u32 waylandScreenCount;
STATIC_ASSERT(sizeof(waylandScreens) < KB(4)); // Make sure we require a sane amount of memory

#include "types/grounded_wayland_types.h"

#include "wayland_protocols/xdg-shell.h"
#include "wayland_protocols/xdg-decoration-unstable-v1.h"
#include "wayland_protocols/xdg-output-unstable-v1.h"
#include "wayland_protocols/idle-inhibit-unstable-v1.h"
#include "wayland_protocols/pointer-constraints-unstable-v1.h"
#include "wayland_protocols/relative-pointer-unstable-v1.h"
#include "wayland_protocols/xdg-foreign-unstable-v2.h"

static void waylandWindowSetMaximized(GroundedWaylandWindow* window, bool maximized);
GROUNDED_FUNCTION void waylandSetCursorType(enum GroundedMouseCursor cursorType);

static void reportWaylandError(const char* message) {
    GROUNDED_PUSH_ERRORF("Error: %s\n", message);
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

static void keyboardHandleKeymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size) {
    //GROUNDED_LOG_VERBOSE("Keyboard keymap");
    char* keymapData = 0;
    struct xkb_keymap* keymap = 0;
    ASSUME(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        if(!xkbContext) {
            ASSERT(false);
        } else {
            //TODO: Need to read format with xkb
            char* keymapData = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
            if(keymapData == MAP_FAILED) {
                keymapData = 0;
            }
            if(keymapData) {
                keymap = xkb_keymap_new_from_string(xkbContext, keymapData, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
                munmap(keymapData, size);
            }
        }
        if(keymap) {
            // Free old state if it exists
            if(xkbState) {
                xkb_state_unref(xkbState);
            }
            xkbState = xkb_state_new(keymap);
            if(xkbKeymap) {
                xkb_keymap_unref(xkbKeymap);
            }
            xkbKeymap = keymap;
        }
    }
    if(fd) {
        close(fd);
    }
}

static void keyboardHandleEnter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
    //GROUNDED_LOG_VERBOSE("Keyboard gained focus");
}

static void keyboardHandleLeave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) {
    GROUNDED_LOG_VERBOSE("Keyboard lost focus");

    // Stop keyrepeat timer
    struct itimerspec timer = {0};
    timerfd_settime(keyRepeatTimer, 0, &timer, 0);
    keyRepeatKey = 0;
}

u32 waylandKeycodeTranslationTable[256] = {0};

static void waylandInitKeycodeTranslationTable() {
    for(u32 i = 0; i < ARRAY_COUNT(waylandKeycodeTranslationTable); ++i) {
        waylandKeycodeTranslationTable[i] = 0;
    }
    waylandKeycodeTranslationTable[KEY_0] = GROUNDED_KEY_0; 
    waylandKeycodeTranslationTable[KEY_1] = GROUNDED_KEY_1; 
    waylandKeycodeTranslationTable[KEY_2] = GROUNDED_KEY_2; 
    waylandKeycodeTranslationTable[KEY_3] = GROUNDED_KEY_3; 
    waylandKeycodeTranslationTable[KEY_4] = GROUNDED_KEY_4; 
    waylandKeycodeTranslationTable[KEY_5] = GROUNDED_KEY_5; 
    waylandKeycodeTranslationTable[KEY_6] = GROUNDED_KEY_6; 
    waylandKeycodeTranslationTable[KEY_7] = GROUNDED_KEY_7; 
    waylandKeycodeTranslationTable[KEY_8] = GROUNDED_KEY_8;   
    waylandKeycodeTranslationTable[KEY_9] = GROUNDED_KEY_9; 
    waylandKeycodeTranslationTable[KEY_A] = GROUNDED_KEY_A;
    waylandKeycodeTranslationTable[KEY_B] = GROUNDED_KEY_B;
    waylandKeycodeTranslationTable[KEY_C] = GROUNDED_KEY_C;
    waylandKeycodeTranslationTable[KEY_D] = GROUNDED_KEY_D;
    waylandKeycodeTranslationTable[KEY_E] = GROUNDED_KEY_E;
    waylandKeycodeTranslationTable[KEY_F] = GROUNDED_KEY_F;
    waylandKeycodeTranslationTable[KEY_G] = GROUNDED_KEY_G;
    waylandKeycodeTranslationTable[KEY_H] = GROUNDED_KEY_H;
    waylandKeycodeTranslationTable[KEY_I] = GROUNDED_KEY_I;
    waylandKeycodeTranslationTable[KEY_J] = GROUNDED_KEY_J;
    waylandKeycodeTranslationTable[KEY_K] = GROUNDED_KEY_K;
    waylandKeycodeTranslationTable[KEY_L] = GROUNDED_KEY_L;
    waylandKeycodeTranslationTable[KEY_M] = GROUNDED_KEY_M;
    waylandKeycodeTranslationTable[KEY_N] = GROUNDED_KEY_N;
    waylandKeycodeTranslationTable[KEY_O] = GROUNDED_KEY_O;
    waylandKeycodeTranslationTable[KEY_P] = GROUNDED_KEY_P;
    waylandKeycodeTranslationTable[KEY_Q] = GROUNDED_KEY_Q;
    waylandKeycodeTranslationTable[KEY_R] = GROUNDED_KEY_R;
    waylandKeycodeTranslationTable[KEY_S] = GROUNDED_KEY_S;
    waylandKeycodeTranslationTable[KEY_T] = GROUNDED_KEY_T;
    waylandKeycodeTranslationTable[KEY_U] = GROUNDED_KEY_U;
    waylandKeycodeTranslationTable[KEY_V] = GROUNDED_KEY_V;
    waylandKeycodeTranslationTable[KEY_W] = GROUNDED_KEY_W;
    waylandKeycodeTranslationTable[KEY_X] = GROUNDED_KEY_X;
    waylandKeycodeTranslationTable[KEY_Y] = GROUNDED_KEY_Y;
    waylandKeycodeTranslationTable[KEY_Z] = GROUNDED_KEY_Z;
    waylandKeycodeTranslationTable[KEY_F1] = GROUNDED_KEY_F1;
    waylandKeycodeTranslationTable[KEY_F2] = GROUNDED_KEY_F2;
    waylandKeycodeTranslationTable[KEY_F3] = GROUNDED_KEY_F3;
    waylandKeycodeTranslationTable[KEY_F4] = GROUNDED_KEY_F4;
    waylandKeycodeTranslationTable[KEY_F5] = GROUNDED_KEY_F5;
    waylandKeycodeTranslationTable[KEY_F6] = GROUNDED_KEY_F6;
    waylandKeycodeTranslationTable[KEY_F7] = GROUNDED_KEY_F7;
    waylandKeycodeTranslationTable[KEY_F8] = GROUNDED_KEY_F8;
    waylandKeycodeTranslationTable[KEY_F9] = GROUNDED_KEY_F9;
    waylandKeycodeTranslationTable[KEY_F10] = GROUNDED_KEY_F10;
    waylandKeycodeTranslationTable[KEY_F11] = GROUNDED_KEY_F11;
    waylandKeycodeTranslationTable[KEY_F12] = GROUNDED_KEY_F12;
    waylandKeycodeTranslationTable[KEY_F13] = GROUNDED_KEY_F13;
    waylandKeycodeTranslationTable[KEY_F14] = GROUNDED_KEY_F14;
    waylandKeycodeTranslationTable[KEY_F15] = GROUNDED_KEY_F15;
    waylandKeycodeTranslationTable[KEY_F16] = GROUNDED_KEY_F16;
    waylandKeycodeTranslationTable[KEY_SPACE] = GROUNDED_KEY_SPACE;
    waylandKeycodeTranslationTable[KEY_LEFTSHIFT] = GROUNDED_KEY_LSHIFT;
    waylandKeycodeTranslationTable[KEY_RIGHTSHIFT] = GROUNDED_KEY_RSHIFT;
    waylandKeycodeTranslationTable[KEY_ESC] = GROUNDED_KEY_ESCAPE;
    waylandKeycodeTranslationTable[KEY_LEFT] = GROUNDED_KEY_LEFT;
    waylandKeycodeTranslationTable[KEY_RIGHT] = GROUNDED_KEY_RIGHT;
    waylandKeycodeTranslationTable[KEY_UP] = GROUNDED_KEY_UP;
    waylandKeycodeTranslationTable[KEY_DOWN] = GROUNDED_KEY_DOWN;
    waylandKeycodeTranslationTable[KEY_ENTER] = GROUNDED_KEY_ENTER;
    waylandKeycodeTranslationTable[KEY_BACKSPACE] = GROUNDED_KEY_BACKSPACE;
    waylandKeycodeTranslationTable[KEY_INSERT] = GROUNDED_KEY_INSERT;
    waylandKeycodeTranslationTable[KEY_HOME] = GROUNDED_KEY_HOME;
    waylandKeycodeTranslationTable[KEY_PAGEUP] = GROUNDED_KEY_PAGE_UP;
    waylandKeycodeTranslationTable[KEY_DELETE] = GROUNDED_KEY_DELETE;
    waylandKeycodeTranslationTable[KEY_END] = GROUNDED_KEY_END;
    waylandKeycodeTranslationTable[KEY_PAGEDOWN] = GROUNDED_KEY_PAGE_DOWN;
    waylandKeycodeTranslationTable[KEY_TAB] = GROUNDED_KEY_TAB;
    waylandKeycodeTranslationTable[KEY_COMMA] = GROUNDED_KEY_COMMA;
    waylandKeycodeTranslationTable[KEY_DOT] = GROUNDED_KEY_DOT;
    waylandKeycodeTranslationTable[KEY_SLASH] = GROUNDED_KEY_SLASH;
    waylandKeycodeTranslationTable[KEY_LEFTCTRL] = GROUNDED_KEY_LCTRL;
    waylandKeycodeTranslationTable[KEY_RIGHTCTRL] = GROUNDED_KEY_RCTRL;
}

static u8 translateWaylandKeycode(u32 key) {
    u8 result = 0;
    if(key < ARRAY_COUNT(waylandKeycodeTranslationTable)) {
        result = (u8)waylandKeycodeTranslationTable[key];
    }
    if(!result) {
        GROUNDED_LOG_WARNING("Unknown keycode");
    }
    return result;
}

static u32 translateInverseWaylandKeycode(u32 keycode) {
    u32 result = 0;
    for(u32 i = 0; i < ARRAY_COUNT(waylandKeycodeTranslationTable); ++i) {
        if(waylandKeycodeTranslationTable[i] == keycode) {
            result = i;
            break;
        }
    }
    return result;
}

static void keyboardHandleKey(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
    u8 keycode = translateWaylandKeycode(key);
    struct itimerspec timer = {0};
    if(state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        waylandKeyState.keys[keycode] = true;
        u32 modifiers = 0;
        u32 codepoint = 0;
        if(xkbContext && xkbState) {
            // Wayland keycodes are offset by 8
            xkb_keysym_t keysym = xkb_state_key_get_one_sym(xkbState, key + 8);
            codepoint = xkb_keysym_to_utf32(keysym);
            modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_SHIFT : 0;
            modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_CONTROL : 0;
            modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_ALT : 0;
            modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_WINDOWS : 0;
        } else {
            if(waylandKeyState.keys[GROUNDED_KEY_LSHIFT] || waylandKeyState.keys[GROUNDED_KEY_RSHIFT]) {
                modifiers |= GROUNDED_KEY_MODIFIER_SHIFT;
            }
        }
        eventQueue[eventQueueIndex++] = (GroundedEvent){
            .type = GROUNDED_EVENT_TYPE_KEY_DOWN,
            .keyDown.keycode = keycode,
            .keyDown.modifiers = modifiers,
            .keyDown.codepoint = codepoint,
        };
        //TODO: Maybe we want key repeats for all keys?
        if(xkb_keymap_key_repeats(xkbKeymap, key + 8) && keyRepeatRate > 0) {
            keyRepeatKey = key;
            if (keyRepeatRate > 1)
                timer.it_interval.tv_nsec = 1000000000 / keyRepeatRate;
            else
                timer.it_interval.tv_sec = 1;

            timer.it_value.tv_sec = keyRepeatDelay / 1000;
            timer.it_value.tv_nsec = (keyRepeatDelay % 1000) * 1000000;
        }
        
    } else if(state == WL_KEYBOARD_KEY_STATE_RELEASED) {
        waylandKeyState.keys[keycode] = false;
        eventQueue[eventQueueIndex++] = (GroundedEvent){
            .type = GROUNDED_EVENT_TYPE_KEY_UP,
            .keyUp.keycode = keycode,
        };
    }

    timerfd_settime(keyRepeatTimer, 0, &timer, 0);

    //fprintf(stderr, "Key is %d state is %d\n", key, state);
}

static void keyboardHandleModifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
    //fprintf(stderr, "Modifiers depressed %d, latched %d, locked %d, group %d\n",
    //        mods_depressed, mods_latched, mods_locked, group);
    if(xkbState) {
        xkb_state_update_mask(xkbState, mods_depressed, mods_latched, mods_locked, 0, 0, group);
    }
}

static void keyboardHandleRepeatInfo(void *data, struct wl_keyboard *wl_keyboard, int32_t rate, int32_t delay) {
    // Delay is the number of milliseconds a key should be held down before key repeat starts
    // Rate is the number of characters per second while repeat is active
    keyRepeatRate = rate < 0 ? 0 : rate;
    keyRepeatDelay = delay < 0 ? keyRepeatDelay : delay;
}

static const struct wl_keyboard_listener keyboardListener = {
    keyboardHandleKeymap,
    keyboardHandleEnter,
    keyboardHandleLeave,
    keyboardHandleKey,
    keyboardHandleModifiers,
    keyboardHandleRepeatInfo,
};


static void pointerHandleEnter(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    ASSERT(wl_pointer == pointer);

    // Can be called when surface has already been destroyed. So we check if we have valid surface first
    if(surface) {
        GroundedWaylandWindow* window = (GroundedWaylandWindow*)wl_surface_get_user_data(surface);
        activeWindow = window;
        lastPointerSerial = serial;
        pointerEnterSerial = serial;
        window->mouseState.mouseInWindow = true;

        if(waylandCurrentCursorType == GROUNDED_MOUSE_CURSOR_CUSTOM) {
            wl_pointer_set_cursor(pointer, serial, cursorSurface, 0, 0);
        } else {
            waylandSetCursorType(waylandCurrentCursorType);
        }
        //printf("Enter with serial %u\n", serial);

        pointerEnterSerial = serial;
    }
}

static void pointerHandleLeave(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface) {
    //printf("Pointer Leave\n");
    if(activeWindow) {
        // Make sure mouse position is outside screen
        activeWindow->mouseState.x = -1;
        activeWindow->mouseState.y = -1;
        activeWindow->mouseState.mouseInWindow = false;

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
    //float posXf = (float)wl_fixed_to_double(surface_x);
    //float posYf = (float)wl_fixed_to_double(surface_y);
    if(activeWindow) {
        activeWindow->mouseState.mouseInWindow = true;
        activeWindow->mouseState.x = posX;
        activeWindow->mouseState.y = posY;
        bool handled = false;
        if(activeWindow->customTitlebarCallback) {
            GroundedWindowCustomTitlebarHit hit = activeWindow->customTitlebarCallback((GroundedWindow*)activeWindow, activeWindow->mouseState.x, activeWindow->mouseState.y);
            if(hit == GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_BORDER) {
                float x = posX;
                float y = posY;
                float width = groundedWindowGetWidth((GroundedWindow*)activeWindow);
                float height = groundedWindowGetHeight((GroundedWindow*)activeWindow);
                float offset = 10.0f;
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
                handled = true;
            } else if(isCursorOverwriteActive()) {
                removeCursorOverwrite();
            }
        } 
        if(!handled) {
            eventQueue[eventQueueIndex++] = (GroundedEvent){
                .type = GROUNDED_EVENT_TYPE_MOUSE_MOVE,
                .window = (GroundedWindow*)activeWindow,
                .mouseMove.mousePositionX = activeWindow->mouseState.x,
                .mouseMove.mousePositionY = activeWindow->mouseState.y,
            };
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
            .window = (GroundedWindow*)activeWindow,
            .buttonDown.button = buttonCode,
            .buttonDown.mousePositionX = activeWindow->mouseState.x,
            .buttonDown.mousePositionY = activeWindow->mouseState.y,
        };
    } else {
        eventQueue[eventQueueIndex++] = (GroundedEvent){
            .type = GROUNDED_EVENT_TYPE_BUTTON_UP,
            .window = (GroundedWindow*)activeWindow,
            .buttonUp.button = buttonCode,
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

static void lockedPointerHandleLocked(void* userData,
                                      struct zwp_confined_pointer_v1* lockedPointer)
{
    printf("Lock enabled\n");
}

static void lockedPointerHandleUnlocked(void* userData,
                                        struct zwp_confined_pointer_v1* lockedPointer)
{
    printf("Lock disabled\n");
}

static const struct zwp_confined_pointer_v1_listener confinedPointerListener =
{
    lockedPointerHandleLocked,
    lockedPointerHandleUnlocked
};

// Surface must be focused upon request in order for the constrain to work.
//TODO: Should we add a flag and confine upon focus event?
GROUNDED_FUNCTION void groundedWindowConstrainPointer(GroundedWindow* opaqueWindow, bool constrain) {
    GroundedWaylandWindow* window = (GroundedWaylandWindow*)opaqueWindow;
    if(window && pointerConstraints) {
        if(constrain) {
            if(!window->confinedPointer) {
                u32 lifetime = ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT;
                lifetime = ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT;
                window->confinedPointer = zwp_pointer_constraints_v1_confine_pointer(pointerConstraints, window->surface, pointer, 0, lifetime);
                zwp_confined_pointer_v1_add_listener(window->confinedPointer, &confinedPointerListener, window);
            }
        } else {
            if(window->confinedPointer) {
                zwp_confined_pointer_v1_destroy(window->confinedPointer);
                window->confinedPointer = 0;
            }
        }
    }
}


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

static void relativePointerHandleMotion(void* userData,
                                        struct zwp_relative_pointer_v1* movedPointer, u32 utime_hi, u32 utime_lo, s32 dx, s32 dy, s32 dx_unaccel, s32 dy_unaccel){
    
    if(activeWindow) {
        float relativeX = (float)wl_fixed_to_double(dx);
        float relativeY = (float)wl_fixed_to_double(dy);
        activeWindow->mouseState.relativeX += relativeX;
        activeWindow->mouseState.relativeY += relativeY;
    }
}

static const struct zwp_relative_pointer_v1_listener relativePointerListener = {
    .relative_motion = relativePointerHandleMotion,
};

static void seatHandleCapabilities(void *data, struct wl_seat *seat, u32 c) {
    enum wl_seat_capability caps = (enum wl_seat_capability) c;
    if (caps & WL_SEAT_CAPABILITY_POINTER) {
        // Mouse input
        pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer, &pointerListener, 0);
        pointerSeat = seat;
        dataDevice = wl_data_device_manager_get_data_device(dataDeviceManager, seat);
        wl_data_device_add_listener(dataDevice, &dataDeviceListener, 0);
        if(relativePointerManager) {
            relativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(relativePointerManager, pointer);
            zwp_relative_pointer_v1_add_listener(relativePointer, &relativePointerListener, 0);
        }
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && pointer) {
        wl_pointer_destroy(pointer);
    }
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        keyboard = wl_seat_get_keyboard(seat);
	    wl_keyboard_add_listener(keyboard, &keyboardListener, 0);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
	    wl_keyboard_destroy(keyboard);
	    keyboard = 0;
    }
}

static void seatHandleName(void *data, struct wl_seat *wl_seat, const char *name) {
    return;
}

static const struct wl_seat_listener seatListener = {
    seatHandleCapabilities,
    seatHandleName,
};


static void handle_ping_xdg_wm_base(void *data, struct xdg_wm_base *xdg, uint32_t serial) {
    xdg_wm_base_pong(xdg, serial);
}

static const struct xdg_wm_base_listener xdgWmBaseListener = {
    handle_ping_xdg_wm_base
};

static void outputHandleGeometry(void* data, struct wl_output* wl_output, int32_t x, int32_t y, int32_t physicalWidth, int32_t physicalHeight, int32_t subpixel, const char *make, const char *model, int32_t transform) {
    struct WaylandScreen* screen = (struct WaylandScreen*) data;
    ASSERT(screen);
    if(screen) {
        screen->display.virtualX = x;
        screen->display.virtualY = y;
        screen->display.name = str8FromCstr(model);
        screen->display.manufacturer = str8FromCstr(make);
        if(screen == waylandScreens) {
            // Wayland has no inbuilt mechanism for primary monitors. We assume here that the first wl_output is also the primary one
            screen->display.primary = true;
        }
    }
}

static void outputHandleMode(void* data, struct wl_output* wl_output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
    struct WaylandScreen* screen = (struct WaylandScreen*) data;
    ASSERT(screen);
}

static void outputHandleDone(void* data, struct wl_output* wl_output) {
    // We now have all current info about this screen
    struct WaylandScreen* screen = (struct WaylandScreen*) data;
    ASSERT(screen);
    if(screen) {
        // If no scale factor given default to 1.0f
        if(!screen->scaleFactor) {
            screen->scaleFactor = 1.0f;
        }
    }
}

static void outputHandleScale(void* data, struct wl_output* wl_output, int32_t factor) {
    struct WaylandScreen* screen = (struct WaylandScreen*) data;
    ASSERT(screen);
    if(screen) {
        screen->scaleFactor = factor;
    }
}

static void outputHandleName(void* data, struct wl_output* wl_output, const char* name) {
    struct WaylandScreen* screen = (struct WaylandScreen*) data;
    ASSERT(screen);
    if(screen && str8IsEmpty(screen->display.name)) {
        // This is more the name the compositor gave this screen. For example DP-1
        screen->display.name = str8FromCstr(name);
    }
}

static void outputHandleDescription(void* data, struct wl_output* wl_output, const char* description) {
    struct WaylandScreen* screen = (struct WaylandScreen*) data;
    ASSERT(screen);
    if(screen) {
        if(str8IsEmpty(screen->description)) {
            screen->description = str8FromCstr(description);
        }
    }
}

static const struct wl_output_listener outputListener = {
    outputHandleGeometry,
    outputHandleMode,
    outputHandleDone,
    outputHandleScale,
    outputHandleName,
    outputHandleDescription,
};

static void xdgOutputHandleLogicalPosition(void* data, struct zxdg_output_v1* xdgOutput, int32_t x, int32_t y) {
    struct WaylandScreen* screen = (struct WaylandScreen*) data;
    ASSERT(screen);
    if(screen) {
        screen->display.virtualX = x;
        screen->display.virtualY = y;
    }
}

static void xdgOutputHandleLogicalSize(void* data, struct zxdg_output_v1* xdgOutput, int32_t width, int32_t height) {
    struct WaylandScreen* screen = (struct WaylandScreen*) data;
    ASSERT(screen);
    // This is the size in the compositor space. So it already accounts for translations and compositor side scaling
    if(screen) {
        screen->display.width = width;
        screen->display.height = height;
    }
}

static void xdgOutputHandleDone(void* data, struct zxdg_output_v1* xdgOutput) {
    // This event is deprecated since version 3. Instead, a wl_output.done event is sent
    ASSERT(xdgOutputVersion < 3);
}

static void xdgOutputHandleName(void* data, struct zxdg_output_v1* xdgOutput, const char* name) {
    // Deprecated as of wl_output v4
    // This is also the compositor assigned name eg. DP-1
    struct WaylandScreen* screen = (struct WaylandScreen*) data;
    ASSERT(screen);
    if(screen && outputVersion < 4 && str8IsEmpty(screen->display.name)) {
        // Assign this name if we have nothing else
        screen->display.name = str8FromCstr(name);
    }
}

static void xdgOutputHandleDescription(void* data, struct zxdg_output_v1* xdgOutput, const char* description) {
    struct WaylandScreen* screen = (struct WaylandScreen*) data;
    ASSERT(screen);
    // Deprecated as of wl_output v4. But still sent for compatability
    if(screen && outputVersion < 4 && str8IsEmpty(screen->description)) {
        screen->description = str8FromCstr(description);
    }
}

static const struct zxdg_output_v1_listener xdgOutputListener = {
    xdgOutputHandleLogicalPosition,
    xdgOutputHandleLogicalSize,
    xdgOutputHandleDone,
    xdgOutputHandleName,
    xdgOutputHandleDescription,
};

static void registry_global(void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version) {
    //GROUNDED_LOG_INFO(interface);
    if (strcmp(interface, "wl_compositor") == 0) {
        // Wayland compositor is required for creating surfaces
        compositor = (struct wl_compositor*)wl_registry_bind(registry, id, wl_compositor_interface, 4);
        ASSERT(compositor);
    } else if(strcmp(interface, "xdg_wm_base") == 0) {
        xdgWmBase = (struct xdg_wm_base*)wl_registry_bind(registry, id, &xdg_wm_base_interface, 3);
        ASSERT(xdgWmBase);
        if(xdgWmBase) {
            xdg_wm_base_add_listener(xdgWmBase, &xdgWmBaseListener, 0);
        }
    } else if(strcmp(interface,"wl_seat") == 0) {
        // Seats are input devices like keyboards mice etc. Actually a seat represents a single user
        u32 seatVersion = MIN(version, 4); // We prefer version 4 for keyboard key repeat info but can also handle older versions
        struct wl_seat* seat = (struct wl_seat*)wl_registry_bind(registry, id, wl_seat_interface, seatVersion);
        ASSERT(seat);
        if(seat) {
            wl_seat_add_listener(seat, &seatListener, 0);
        }
    } else if(strcmp(interface, "wl_output") == 0) {
        // Output can be used to get information about connected monitors and the virtual screen setup
        // We get one wl_output for each connected screen
        if(!outputVersion) {
            outputVersion = version;
        } else {
            ASSERT(outputVersion == version);
        }
        ASSERT(waylandScreenCount < ARRAY_COUNT(waylandScreens));
        if(waylandScreenCount < ARRAY_COUNT(waylandScreens)) {
            struct wl_output* output = (struct wl_output*)wl_registry_bind(registry, id, wl_output_interface, outputVersion);
            ASSERT(output);
            if(output) {
                struct WaylandScreen* screen = &waylandScreens[waylandScreenCount++];
                MEMORY_CLEAR_STRUCT(screen);
                screen->output = output;
                screen->active = true;
                screen->registryId = id;
                wl_output_add_listener(output, &outputListener, screen); // Last is custom data
                if(xdgOutputManager) {
                    screen->xdgOutput = zxdg_output_manager_v1_get_xdg_output(xdgOutputManager, screen->output);
                    zxdg_output_v1_add_listener(screen->xdgOutput, &xdgOutputListener, screen);
                }
            }
        }
    } else if(strcmp(interface, "zxdg_decoration_manager_v1") == 0) {
        // Client and server side decoration negotiation
        decorationManager = (struct zxdg_decoration_manager_v1*)wl_registry_bind(registry, id, &zxdg_decoration_manager_v1_interface, 1);
        ASSERT(decorationManager);
    } else if(strcmp(interface, "zxdg_output_manager_v1") == 0) {
        xdgOutputVersion = MIN(version, 3);
        xdgOutputManager = (struct zxdg_output_manager_v1*)wl_registry_bind(registry, id, &zxdg_output_manager_v1_interface, xdgOutputVersion);
        ASSERT(xdgOutputManager);
        // Initialize all existing WaylandScreens
        if(xdgOutputManager) {
            for(u32 i = 0; i < waylandScreenCount; ++i) {
                waylandScreens[i].xdgOutput = zxdg_output_manager_v1_get_xdg_output(xdgOutputManager, waylandScreens[i].output);
                ASSERT(waylandScreens[i].xdgOutput);
                if(waylandScreens[i].xdgOutput) {
                    zxdg_output_v1_add_listener(waylandScreens[i].xdgOutput, &xdgOutputListener, &waylandScreens[i]);
                }
            }
        }
    } else if(strcmp(interface, "zwp_idle_inhibit_manager_v1") == 0) {
        // Ability to prevent system from going into sleep mode
        idleInhibitManager = (struct zwp_idle_inhibit_manager_v1*)wl_registry_bind(registry, id, &zwp_idle_inhibit_manager_v1_interface, 1);
        ASSERT(idleInhibitManager);
    } else if(strcmp(interface, "zwp_pointer_constraints_v1") == 0) {
        pointerConstraints = (struct zwp_pointer_constraints_v1*)wl_registry_bind(registry, id, &zwp_pointer_constraints_v1_interface, 1);
        ASSERT(pointerConstraints);
    } else if(strcmp(interface, "zwp_relative_pointer_manager_v1") == 0) {
        relativePointerManager = (struct zwp_relative_pointer_manager_v1*)wl_registry_bind(registry, id, &zwp_relative_pointer_manager_v1_interface, 1);
        ASSERT(relativePointerManager);
    } else if(strcmp(interface, "wl_data_device_manager") == 0) {
        // For drag and drop and clipboard support
        u32 compositorSupportedVersion = version;
        u32 requestedVersion = MIN(version, 3); // We support up to version 3
        dataDeviceManager = (struct wl_data_device_manager*)wl_registry_bind(registry, id, wl_data_device_manager_interface, requestedVersion);
        ASSERT(dataDeviceManager);
        if(dataDeviceManager) {
            dataDeviceManagerVersion = requestedVersion;
        }
    } else if(strcmp(interface, "zxdg_exporter_v2") == 0) {
        zxdgExporter = (struct zxdg_exporter_v2*)wl_registry_bind(registry, id, &zxdg_exporter_v2_interface, version);
        ASSERT(zxdgExporter);
    } else if(strcmp(interface, "wl_shm") == 0) {
        // Shared memory. Needed for custom cursor themes and framebuffers - TODO: might have been replaced by drm (Drm is not particular useful for software rendering)
        waylandShm = (struct wl_shm*)wl_registry_bind(registry, id, wl_shm_interface, 1);
        ASSERT(waylandShm);
    } else {
        GROUNDED_LOG_INFO(interface);
    }
}

static void registry_global_remove(void* data, struct wl_registry* registry, uint32_t id) {
    // Event when an interface is removed. For example a wl_output.
    // Unfortunately we do not know which interface if removed so we have to guess...
    for(u32 i = 0; i < waylandScreenCount; ++i) {
        if(waylandScreens[i].registryId == id) {
            //TODO: Handle screen removal
            waylandScreens[i].active = false;
            //TODO: Send screen removal event
            return;
        }
    }
    // Removal of unknown id
    ASSERT(false);
}

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

    //printf("Toplevel Configure\n");

    // States array tells us in which state the new window is
    u32* state;
    wl_array_for_each(state, states) {
        switch(*state) {
            case XDG_TOPLEVEL_STATE_MAXIMIZED:{
            } break;
            case XDG_TOPLEVEL_STATE_FULLSCREEN:{
                //TODO: Actually it might be better to only set the window->fullscreen flag when we get the configure here?
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
            .window = (GroundedWindow*)window,
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
        .window = (GroundedWindow*)window,
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

    //printf("Xdg Configure\n");

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

static void zxdgExportedHandleHandle(void* data, struct zxdg_exported_v2 *zxdg_exported_v2, const char *handle) {
    GroundedWaylandWindow* window = (GroundedWaylandWindow*)data;
    window->foreignHandle = (char*)malloc(strlen(handle)+1);
    memcpy(window->foreignHandle, handle, strlen(handle)+1);
    printf("Foreign handle:%s\n", handle);
}

static const struct zxdg_exported_v2_listener exportedListener = {
    zxdgExportedHandleHandle,
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

    waylandInitKeycodeTranslationTable();

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
            GROUNDED_PUSH_ERRORF("Could not load wayland function: %s\n", firstMissingFunctionName);
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
                GROUNDED_PUSH_ERRORF("Could not load wayland cursor function: %s\n", firstMissingFunctionName);
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
        } else {
            keyRepeatTimer = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
        }
    }

    if(!error) {
        registry = wl_display_get_registry(waylandDisplay);
        if(!registry) {
            error = "Could not get wayland registry";
        }
        wl_registry_add_listener(registry, &registryListener, 0);

        // Roundtrip to retrieve all registry objects
        wl_display_roundtrip(waylandDisplay);

        if(waylandShm) {
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
        }

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
    //TODO: Release all resources
    if(keyRepeatTimer >= 0) {
        close(keyRepeatTimer);
        keyRepeatTimer = -1;
    }
    //if(cursor) ... free
    if(cursorTheme) {
        wl_cursor_theme_destroy(cursorTheme);
    }
    if(cursorSurface) {
        wl_surface_destroy(cursorSurface);
    }
    if(compositor) {
        wl_compositor_destroy(compositor);
    }
    if(waylandShm) {
        wl_shm_destroy(waylandShm);
    }
    if(decorationManager) {
        zxdg_decoration_manager_v1_destroy(decorationManager);
    }
    if(xdgWmBase) {
        xdg_wm_base_destroy(xdgWmBase);
    }
    for(u32 i = 0; i < waylandScreenCount; ++i) {
        zxdg_output_v1_destroy(waylandScreens[i].xdgOutput);
        wl_output_destroy(waylandScreens[i].output);
    }
    if(xdgOutputManager) {
        zxdg_output_manager_v1_destroy(xdgOutputManager);
    }

    if(dragOffer) {
        printf("Leftover drag offer: %p, %p\n", dragOffer, dragOffer->offer);
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
    if(pointer) {
        wl_pointer_destroy(pointer);
    }
    if(keyboard) {
        wl_keyboard_destroy(keyboard);
    }
    if(pointerSeat) {
        wl_seat_destroy(pointerSeat);
    }
    if(relativePointerManager) {
        zwp_relative_pointer_manager_v1_destroy(relativePointerManager);
        relativePointerManager = 0;
    }
    if(pointerConstraints) {
        zwp_pointer_constraints_v1_destroy(pointerConstraints);
    }
    if(idleInhibitManager) {
        zwp_idle_inhibit_manager_v1_destroy(idleInhibitManager);
    }
    if(registry) {
        wl_registry_destroy(registry);
    }
    if(waylandDisplay) {
        wl_display_flush(waylandDisplay);
        wl_display_disconnect(waylandDisplay);
    }
}

static void waylandSetWindowTitle(GroundedWaylandWindow* window, String8 title) {
    u64 titleSize = MIN(255, title.size);
    MEMORY_COPY(window->titleBuffer, title.base, titleSize);
    window->titleBuffer[titleSize] = '\0';
    window->title.base = (u8*)window->titleBuffer;
    window->title.size = titleSize;

    // Title is guaranteed to be 0-terminated
    if(window->xdgToplevel) {
        xdg_toplevel_set_title(window->xdgToplevel, (const char*)window->title.base);
    }
}

static void waylandWindowSetFullsreen(GroundedWaylandWindow* window, bool fullscreen) {
    if(fullscreen) {
        xdg_toplevel_set_fullscreen(window->xdgToplevel, 0);
    } else {
        xdg_toplevel_unset_fullscreen(window->xdgToplevel);
    }
    window->fullscreen = fullscreen;
}

static void waylandWindowSetBorderless(GroundedWaylandWindow* window, bool borderless) {
    window->borderless = borderless;
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

static void waylandSetInhibitIdle(GroundedWaylandWindow* window, bool inhibitIdle) {
    window->inhibitIdle = inhibitIdle;
    if(inhibitIdle && !window->idleInhibitor) {
        window->idleInhibitor = zwp_idle_inhibit_manager_v1_create_inhibitor(idleInhibitManager, window->surface);
    } else if(!inhibitIdle && window->idleInhibitor) {
        zwp_idle_inhibitor_v1_destroy(window->idleInhibitor);
        window->idleInhibitor = 0;
    }
}

static void waylandWindowSetHidden(GroundedWaylandWindow* window, bool hidden) {
    // Wayland does not directly support this concept
    // Instead an xdg_surface and toplevel should only be created once the window is shown
    // This however requires to store all intermediate data like title, applicationid, min and max size and inhibit idle
    if(hidden && window->xdgSurface) {
        if (window->xdgToplevel) {
            xdg_toplevel_destroy(window->xdgToplevel);
        }

        if (window->xdgSurface) {
            xdg_surface_destroy(window->xdgSurface);
        }

        window->xdgToplevel = 0;
        window->xdgSurface = 0;

        wl_surface_attach(window->surface, 0, 0, 0);
        wl_surface_commit(window->surface);
    } else if(!hidden && !window->xdgSurface) {
        window->xdgSurface = xdg_wm_base_get_xdg_surface(xdgWmBase, window->surface);
        ASSERT(window->xdgSurface);
        xdg_surface_add_listener(window->xdgSurface, &xdgSurfaceListener, window);
        //xdg_surface_set_window_geometry(window->xdgSurface, x, y, window->width, window->height)
        window->xdgToplevel = xdg_surface_get_toplevel(window->xdgSurface);
        xdg_toplevel_add_listener(window->xdgToplevel, &xdgToplevelListener, window);
        if(!str8IsEmpty(window->applicationId)) {
            // ApplicationId is guaranteed to be 0-terminated
            xdg_toplevel_set_app_id(window->xdgToplevel, (const char*)window->applicationId.base);
        }
        ASSERT(window->xdgToplevel);
        
        // Window Title
        if(window->title.size > 0) {
            // Title is guaranteed to be 0-terminated
            xdg_toplevel_set_title(window->xdgToplevel, (const char*)window->title.base);
        }
        if(window->minWidth || window->minHeight) {
            xdg_toplevel_set_min_size(window->xdgToplevel, window->minWidth, window->minHeight);
        }
        if(window->maxWidth || window->maxHeight) {
            // 0 means no max Size in that dimension
            xdg_toplevel_set_max_size(window->xdgToplevel, window->maxWidth, window->maxHeight);
        }

        // Set decorations
        if(window->customTitlebarCallback) {
            waylandWindowSetBorderless(window, true);
        } else {
            waylandWindowSetBorderless(window, window->borderless);
        }
        //struct zxdg_toplevel_decoration_v1* decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(decorationManager, window->xdgToplevel);
        //zxdg_toplevel_decoration_v1_add_listener(decoration, &decorationListener, window);
        //zxdg_toplevel_decoration_v1_set_mode(decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);

        if(window->inhibitIdle) {
            waylandSetInhibitIdle(window, true);
        }

        if(zxdgExporter) {
            struct zxdg_exported_v2* exported = zxdg_exporter_v2_export_toplevel(zxdgExporter, window->surface);
            zxdg_exported_v2_add_listener(exported, &exportedListener, window);
        }

        wl_surface_commit(window->surface);
        wl_display_roundtrip(waylandDisplay);
    }
    
}

static void waylandWindowSetMaximized(GroundedWaylandWindow* window, bool maximized) {
    if(maximized) {
        xdg_toplevel_set_maximized(window->xdgToplevel);
    } else {
        xdg_toplevel_unset_maximized(window->xdgToplevel);
    }
}

static bool waylandWindowIsFullscreen(GroundedWaylandWindow* window) {
    return window->fullscreen;
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
    window->minWidth = parameters->minWidth;
    window->minHeight = parameters->minHeight;
    window->maxWidth = parameters->maxWidth;
    window->maxHeight = parameters->maxHeight;
    window->borderless = parameters->borderless;
    window->inhibitIdle = parameters->inhibitIdle;
    window->surface = wl_compositor_create_surface(compositor);
    wl_surface_set_user_data(window->surface, window);

    if(!str8IsEmpty(parameters->applicationId)) {
        window->applicationId = str8CopyAndNullTerminate(arena, parameters->applicationId);
    }
    if(!str8IsEmpty(parameters->title)) {
        waylandSetWindowTitle(window, parameters->title);
    }
    if(parameters->customTitlebarCallback) {
        window->customTitlebarCallback = parameters->customTitlebarCallback;
    }

    // Creates all necessary xdg objects if we should be visible
    waylandWindowSetHidden(window, parameters->hidden);

    if(parameters->userData) {
        waylandWindowSetUserData(window, parameters->userData);
    }
    window->dndUserData = parameters->dndUserData;

    // Make sure mouse position is outside screen
    window->mouseState.x = -1;
    window->mouseState.y = -1;

    window->dndCallback = parameters->dndCallback;

    wl_surface_commit(window->surface);

    wl_display_roundtrip(waylandDisplay);

    arenaEndTemp(temp);

    return (GroundedWindow*)window;
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

static void sendWaylandKeyRepeat() {
    printf("Sending key repeat\n");
    if(keyRepeatKey) {
        u8 keycode = translateWaylandKeycode(keyRepeatKey);
        u32 modifiers = 0;
        u32 codepoint = 0;
        if(xkbContext && xkbState) {
            // Wayland keycodes are offset by 8
            xkb_keysym_t keysym = xkb_state_key_get_one_sym(xkbState, keyRepeatKey + 8);
            codepoint = xkb_keysym_to_utf32(keysym);
            modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_SHIFT : 0;
            modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_CONTROL : 0;
            modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_ALT : 0;
            modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_WINDOWS : 0;
        } else {
            if(waylandKeyState.keys[GROUNDED_KEY_LSHIFT] || waylandKeyState.keys[GROUNDED_KEY_RSHIFT]) {
                modifiers |= GROUNDED_KEY_MODIFIER_SHIFT;
            }
        }
        eventQueue[eventQueueIndex++] = (GroundedEvent){
            .type = GROUNDED_EVENT_TYPE_KEY_DOWN,
            .keyDown.keycode = keycode,
            .keyDown.modifiers = modifiers,
            .keyDown.codepoint = codepoint,
        };
    }
}

String8 groundedWaylandGetNameOfKeycode(MemoryArena* arena, u32 keycode) {
    String8 result = EMPTY_STRING8;
    keycode = translateInverseWaylandKeycode(keycode);
    xkb_keysym_t keysym = xkb_state_key_get_one_sym(xkbState, keycode + 8); // Add 8 for evdev-to-XKB offset
    keysym = xkb_keysym_to_upper(keysym);
    u32 bufferSize = 256;
    char* buffer = ARENA_PUSH_ARRAY_NO_CLEAR(arena, bufferSize, char);
    ASSUME(buffer) {
        s32 length = MIN(bufferSize, xkb_keysym_get_name(keysym, buffer, bufferSize));
        if(length < 0) {
            length = 0;
        }
        result.base = (u8*)buffer;
        result.size = length;
        arenaPopTo(arena, result.base + result.size);
    }
    return result;
}   

// Returns true when poll was successful. False on timeout
static bool waylandPoll(u32 maxWaitingTimeInMs) {
	int ret;
	struct pollfd pfd[2];
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
    pfd[1].fd = keyRepeatTimer;
    pfd[1].events = POLLIN;
	do {
		ret = ppoll(pfd, ARRAY_COUNT(pfd), timeout < 0 ? 0 : &ts, 0);
	} while (ret == -1 && errno == EINTR); // A signal occured before

	if (ret == 0) {
        // Timed out
		return false;
    } else if(ret < 0) {
        // Error
        return false;
    }

    if(pfd[1].revents & POLLIN) {
        u64 repeats = 0;
        if (read(keyRepeatTimer, &repeats, sizeof(repeats)) == 8) {
            if(repeats >= 5) {
                printf("Somehow we got very many key repeat events. Probably some kind of delay so we ignore them\n");
                repeats = 0;
            }
            for(u64 i = 0; i < repeats; ++i) {
                // Key repeat
                sendWaylandKeyRepeat();
            }
        }
    }

	return true;
}

static GroundedEvent* waylandPollEvents(u32* eventCount) {
    eventQueueIndex = 0;

    // Check key repeat timer
    struct pollfd pfd[1];
    pfd[0].fd = keyRepeatTimer;
    pfd[0].events = POLLIN;
    int ret = poll(pfd, ARRAY_COUNT(pfd), 0);
    if(ret > 0) {
        if(pfd[0].revents & POLLIN) {
            u64 repeats;
            if (read(keyRepeatTimer, &repeats, sizeof(repeats)) == 8) {
                if(repeats >= 5) {
                    printf("Somehow we got very many key repeat events. Probably some kind of hickup so we ignore them\n");
                    repeats = 0;
                }
                for(u64 i = 0; i < repeats; ++i) {
                    // Key repeat
                    sendWaylandKeyRepeat();
                }
            }
        }
    }

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

    // Loop as we are otherwise waked up by egl rendering feedback calls
    while(!eventQueueIndex) {
        if(waylandPoll(maxWaitingTimeInMs)) {
            // This should be a blocking call
            wl_display_dispatch(waylandDisplay);
        } else {
            // Timeout
            break;
        }
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
    mouseState->relativeX = window->mouseState.relativeX;
    mouseState->relativeY = window->mouseState.relativeY;
    mouseState->mouseInWindow = window->mouseState.mouseInWindow;

    window->mouseState.scrollDelta = 0.0f;
    window->mouseState.horizontalScrollDelta = 0.0f;
    window->mouseState.relativeX = 0.0f;
    window->mouseState.relativeY = 0.0f;
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

static int set_cloexec_or_close(int fd) {
    long flags;
    
    if (fd == -1)
        return -1;
    
    flags = fcntl(fd, F_GETFD);
    if (flags == -1)
        goto err;
    
    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
        goto err;
    
    return fd;
    
    err:
    close(fd);
    return -1;
}

static int create_tmpfile_cloexec(char *tmpname) {
    int fd;
    
#ifdef HAVE_MKOSTEMP
    fd = mkostemp(tmpname, O_CLOEXEC);
    if (fd >= 0)
        unlink(tmpname);
#else
    fd = mkstemp(tmpname);
    if (fd >= 0) {
        fd = set_cloexec_or_close(fd);
        unlink(tmpname);
    }
#endif
    
    return fd;
}


static int os_create_anonymous_file(off_t size) {
    const char *path;
    char *name;
    int fd;
    
    path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        errno = ENOENT;
        return -1;
    }
    
    //TODO: Replace malloc
    name = (char*)malloc(strlen(path) + sizeof("/weston-shared-XXXXXX"));
    if (!name)
        return -1;
    strcpy(name, path);
    strcat(name, "/weston-shared-XXXXXX");
    
    fd = create_tmpfile_cloexec(name);
    
    free(name);
    
    if (fd < 0)
        return -1;
    
    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }
    
    return fd;
}



static void resizeWaylandFramebuffer(GroundedWaylandWindow* window) {
    //TODO: Support framebuffer on devices without shm support or at least print an error
    //TODO: Destroy wl_buffer of old framebuffer, old shm pool
    ASSERT(waylandShm);

    u32 newWidth = window->width;
    u32 newHeight = window->height;

    // Assumes 32bit per pixel for now
    u32 size = newWidth * newHeight * 4;
    printf("Resizing framebuffer to %ux%u", newWidth, newHeight);
    if(window->framebufferPointers[0]) {
        ASSERT(window->waylandBuffers[0] && window->waylandBuffers[1] && window->framebufferPointers[1]);
        // Detach shm
        shmdt(window->framebufferPointers[0]);
        shmdt(window->framebufferPointers[1]);
        
        wl_buffer_destroy(window->waylandBuffers[0]);
        wl_buffer_destroy(window->waylandBuffers[1]);
    }

    int fd = os_create_anonymous_file(size * 2);
    //window->waylandShm = shmget(IPC_PRIVATE, 640*480*4, IPC_CREAT | 0600);
    window->shm = fd;
    //window->framebufferMemory = shmat(window->waylandShm, 0, 0);
    window->framebufferPointers[0] = mmap(0, size * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    window->framebufferPointers[1] = ((u8*)window->framebufferPointers[0])+size;
    window->framebufferWidth = newWidth;
    window->framebufferHeight = newHeight;

    // Attach to wayland?
    struct wl_shm_pool* pool = wl_shm_create_pool(waylandShm, window->shm, size * 2);
    window->waylandBuffers[0] = wl_shm_pool_create_buffer(pool, 0, newWidth, newHeight, newWidth*4, WL_SHM_FORMAT_XRGB8888);
    window->waylandBuffers[1] = wl_shm_pool_create_buffer(pool, size, newWidth, newHeight, newWidth*4, WL_SHM_FORMAT_XRGB8888);
    //wl_shm_pool_destroy(pool);
}


GROUNDED_FUNCTION GroundedWindowFramebuffer waylandGetFramebuffer(GroundedWaylandWindow* window) {
    GroundedWindowFramebuffer result = {0};

    if(!window->shm || window->framebufferWidth != window->width || window->framebufferHeight != window->height) {
        resizeWaylandFramebuffer(window);
    }

    ASSERT(window->shm);
    ASSERT(window->framebufferPointers[0]);
    result.width = window->framebufferWidth;
    result.height = window->framebufferHeight;
    result.buffer = (u32*)window->framebufferPointers[0];

    return result;
}

GROUNDED_FUNCTION void waylandSubmitFramebuffer(GroundedWaylandWindow* window, GroundedWindowFramebuffer* framebuffer) {
    ASSERT(window->shm);
    ASSERT(window->waylandBuffers[0]);
    ASSERT(window->framebufferPointers[0] == framebuffer->buffer);

    wl_surface_attach(window->surface, window->waylandBuffers[0], 0, 0);
    // Always use damage buffer instead of damage
    wl_surface_damage_buffer(window->surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(window->surface);

    struct wl_buffer* nextBuffer = window->waylandBuffers[1];
    window->waylandBuffers[1] = window->waylandBuffers[0];
    window->waylandBuffers[0] = nextBuffer;

    void* nextFramebuffer = window->framebufferPointers[1];
    window->framebufferPointers[1] = window->framebufferPointers[0];
    window->framebufferPointers[0] = nextFramebuffer;
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
            return 0;
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
                return 0;
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
            return 0;
        }
        int eglVersionMajor = 0;
        int eglVersionMinor = 0;
        if(!eglInitialize(waylandEglDisplay, &eglVersionMajor, &eglVersionMinor)) {
            GROUNDED_LOG_ERROR("Error initializing EGL display");
            eglTerminate(waylandEglDisplay);
            dlclose(waylandEglLibrary);
            waylandEglLibrary = 0;
            waylandEglDisplay = 0;
            return 0;
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
        return 0;
    }

    //TODO: Config is required when creating the context and when creating the window
    EGLConfig config;
    int numConfigs = 0;
    int attribList[] = {EGL_RED_SIZE, 1, EGL_GREEN_SIZE, 1, EGL_BLUE_SIZE, 1, EGL_NONE};
    if(!eglChooseConfig(waylandEglDisplay, attribList, &config, 1, &numConfigs) || numConfigs <= 0) {
        GROUNDED_LOG_ERROR("Error choosing OpenGL config");
        return 0;
    }

    EGLContext shareContext = contextToShareResources ? contextToShareResources->eglContext : EGL_NO_CONTEXT;
    result->eglContext = eglCreateContext(waylandEglDisplay, config, shareContext, 0);
    if(result->eglContext == EGL_NO_CONTEXT) {
        GROUNDED_LOG_ERROR("Error creating EGL Context");
        return 0;
    }
    return result;
}

static bool waylandCreateEglSurface(GroundedWaylandWindow* window) {
    if(!window->xdgSurface) {
        // No surface yet. Window is probably not visible. We have to make it visible to go further
        waylandWindowSetHidden(window, false);
    }
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

static void waylandOpenGLMakeCurrent(GroundedWaylandWindow* window, GroundedOpenGLContext* context) {
    if(!window->eglWindow || !window->eglSurface) {
        waylandCreateEglSurface(window);
    }
    if(!eglMakeCurrent(waylandEglDisplay, window->eglSurface, window->eglSurface, context->eglContext)) {
        //LOG_INFO("Error: ", eglGetError());
        GROUNDED_LOG_ERROR("Error making OpenGL context current");
        //return false;
    }
}

static void waylandWindowGlSwapBuffers(GroundedWaylandWindow* window) {
    eglSwapBuffers(waylandEglDisplay, window->eglSurface);
}

static void waylandWindowSetGlSwapInterval(int interval) {
    EGLBoolean result = eglSwapInterval(waylandEglDisplay, interval);
    if(!result) {
        GROUNDED_PUSH_ERROR("Failed to set swap interval");
    }
}

static void waylandDestroyOpenGLContext(GroundedOpenGLContext* context) {
    eglDestroyContext(waylandEglDisplay, context);
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
        GROUNDED_PUSH_ERRORF("Error creating vulkan surface: %s\n", error);
    }
    
    return surface;
}
#endif // GROUNDED_VULKAN_SUPPORT




































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
    GROUNDED_WAYLAND_LOG_HANDLER("dataOffer.offer");

    // Sent immediately after creating a wl_data_offer object once for every mime type.
    
    struct WaylandDataOffer* waylandOffer = (struct WaylandDataOffer*)userData;
    ASSERT(waylandOffer->offer == offer);
    // Basically mark the data offer with the type it has. Is called once for every available mime type of the offer
    // Typical stuff: text/plain;charset=utf-8, text/uri-list, etc.

    // The first mime types are not actual mime types. They are all caps-lock and maybe we want to filter them out

    // I do not know if a copy is really necessary but it defenitely feels safer
    str8ListPushCopyAndNullTerminate(&waylandOffer->arena, &waylandOffer->availableMimeTypeList, str8FromCstr(mimeType));
    //printf("Mimme: %s offer %p\n", waylandOffer->availableMimeTypeList.last->string.base, waylandOffer);
    //wl_data_offer_accept(waylandOffer->offer, waylandOffer->enterSerial, mimeType);
    //str8ListPush(&waylandOffer->arena, &waylandOffer->availableMimeTypeList, str8FromCstr(mimeType));
}

static void dataOfferHandleSourceActions(void* userData, struct wl_data_offer *wl_data_offer, uint32_t sourceActions) {
    GROUNDED_WAYLAND_LOG_HANDLER("dataOffer.sourceActions");

    ASSERT(dataDeviceManagerVersion >= 3);
    // This gives the actions that are supported by the source side. Called directly after the mime types and when source changes available actions
    // We only receive this for dnd data offers

    struct WaylandDataOffer* waylandOffer = (struct WaylandDataOffer*)userData;
    waylandOffer->allowedActions = 0;

    //printf("Data offer source actions\n");
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
    GROUNDED_WAYLAND_LOG_HANDLER("dataOffer.action");

    // The action the compositor selected for us based on source and destination preferences. We simply ignore it for now...
    // Most recent action received is always the valid one
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

    printf("Selected action: %i\n", waylandOffer->selectedAction);
}

static const struct wl_data_offer_listener dataOfferListener = {
    dataOfferHandleOffer,
    dataOfferHandleSourceActions,
    dataOfferHandleAction,
};



static void dataDeviceListenerOffer(void* data, struct wl_data_device* dataDevice, struct wl_data_offer* offer) {
    GROUNDED_WAYLAND_LOG_HANDLER("dataDevice.offer");

    // Compositor announces a new data offer. Can be dnd or clipboard
    struct WaylandDataOffer* waylandOffer = ARENA_BOOTSTRAP_PUSH_STRUCT(createGrowingArena(osGetMemorySubsystem(), KB(4)), struct WaylandDataOffer, arena);
    waylandOffer->offer = offer;
    waylandOffer->lastAcceptedMimeIndex = 0xFFFFFFFF;

    //printf("New offer %p\n", waylandOffer);

    // Add listener which tells us, what data type the offer contains
    wl_data_offer_add_listener(offer, &dataOfferListener, waylandOffer);
}

static void updateWaylandDragPosition(GroundedWaylandWindow* window, struct WaylandDataOffer* waylandOffer, s32 posX, s32 posY) {
    ASSERT(waylandOffer->dnd);
    ASSERT(waylandOffer->enterSerial);
    waylandOffer->x = posX;
    waylandOffer->y = posY;
    if(activeWindow) {
        activeWindow->mouseState.x = posX;
        activeWindow->mouseState.y = posY;
    }
    u32 newMimeIndex = window->dndCallback(&waylandOffer->payload, (GroundedWindow*)window, posX, posY, waylandOffer->mimeTypeCount, waylandOffer->mimeTypes, &waylandOffer->dropCallback, window->dndUserData);
    // I think spec told us that we do not have to answer if the last accept is still valid. However in practice it seems to be best to always answer with an accept
    if(newMimeIndex != waylandOffer->lastAcceptedMimeIndex || true) {
        if(newMimeIndex < waylandOffer->mimeTypeCount) {
            wl_data_offer_accept(waylandOffer->offer, waylandOffer->enterSerial, (const char*)waylandOffer->mimeTypes[newMimeIndex].base);
            if(dataDeviceManagerVersion >= 3) {
                wl_data_offer_set_actions(waylandOffer->offer, waylandOffer->allowedActions, WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
            }
        } else {
            wl_data_offer_accept(waylandOffer->offer, waylandOffer->enterSerial, 0);
        }
        waylandOffer->lastAcceptedMimeIndex = newMimeIndex;
    }
}

static void dataDeviceListenerEnter(void* data, struct wl_data_device* dataDevice, u32 serial, struct wl_surface* surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer* offer) {
    GROUNDED_WAYLAND_LOG_HANDLER("dataDevice.enter");

    // At this point we know that the data offer is a drag offer
    struct WaylandDataOffer* waylandOffer = (struct WaylandDataOffer*)wl_data_offer_get_user_data(offer);
    ASSERT(waylandOffer);
    ASSERT(waylandOffer->offer == offer);
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
        activeWindow = window;

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
}

static void dataDeviceListenerLeave(void* data, struct wl_data_device* dataDevice) {
    GROUNDED_WAYLAND_LOG_HANDLER("dataDevice.leave");
    ASSERT(dragOffer);
    ASSERT(dragOffer->dnd);

    if(activeWindow && activeWindow == dragOffer->window) {
        // Make sure mouse position is outside screen
        activeWindow->mouseState.x = -1;
        activeWindow->mouseState.y = -1;

        // Reset buttons to non-pressed state
        MEMORY_CLEAR_ARRAY(activeWindow->mouseState.buttons);
        //TODO: Should probably also send button up events for all buttons that were pressed upon leave. Or maybe we send a cursor leave event and let the applicatio handle it
    }

    // We have to destroy the offer
    wl_data_offer_destroy(dragOffer->offer);
    arenaRelease(&dragOffer->arena);
    dragOffer = 0;
}

static void dataDeviceListenerMotion(void* data, struct wl_data_device* dataDevice, u32 time, wl_fixed_t x, wl_fixed_t y) {
    GROUNDED_WAYLAND_LOG_HANDLER("dataDevice.move");

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
    GROUNDED_WAYLAND_LOG_HANDLER("dataDevice.drop");

    struct WaylandDataOffer* waylandOffer = dragOffer;
    ASSERT(waylandOffer);
    
    // We should honor last action received from dataOffer.action. If it is copy or move we can do receive requests. End transfer with wl_data_offer_finish()
    
    if(waylandOffer->lastAcceptedMimeIndex < waylandOffer->mimeTypeCount) {
        int fds[2];
	    pipe(fds);

        String8 mimeType = waylandOffer->mimeTypes[waylandOffer->lastAcceptedMimeIndex];
        // We know that mimetype is 0-terminated as we copy it with null termination when creating
        ASSERT(mimeType.base[mimeType.size] == '\0');
        wl_data_offer_accept(waylandOffer->offer, waylandOffer->enterSerial, (const char*)mimeType.base);
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
    } else {
        // We do not accept
        wl_data_offer_accept(waylandOffer->offer, waylandOffer->enterSerial, 0);
    }

    if(dataDeviceManagerVersion >= 3) {
        wl_data_offer_finish(waylandOffer->offer);
    }
    wl_data_offer_destroy(waylandOffer->offer);
    arenaRelease(&waylandOffer->arena);
    dragOffer = 0;
}

static void dataDeviceListenerSelection(void* data, struct wl_data_device* dataDevice, struct wl_data_offer* dataOffer) {
    GROUNDED_WAYLAND_LOG_HANDLER("dataDevice.selection");

    // Can happen if the clipboard is empty
    if(dataOffer != 0) {
        // This is for copy and paste
        struct WaylandDataOffer* waylandOffer = (struct WaylandDataOffer*)wl_data_offer_get_user_data(dataOffer);
        ASSERT(waylandOffer);
        waylandOffer->dnd = false;
        
        //TODO: Read in the data
        
        //printf("Data offer %p is selection and gets destroyed\n", waylandOffer);
        wl_data_offer_destroy(waylandOffer->offer);
        arenaRelease(&waylandOffer->arena);
    }
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
    GroundedWindowDndDataCallback* dataCallback;
    GroundedWindowDndDragFinishCallback* dragFinishCallback;
    void* userData;
    struct wl_data_source* dataSource;
    enum wl_data_device_manager_dnd_action last_dnd_action;
};

static void dataSourceHandleTarget(void* data, struct wl_data_source* source, const char* mimeType) {
    GROUNDED_WAYLAND_LOG_HANDLER("dataSource.target");

	if (mimeType && *mimeType) {
		printf("Destination would accept MIME type if dropped: %s\n", mimeType);
        setCursorOverwrite(GROUNDED_MOUSE_CURSOR_GRABBING);
	} else {
        if(mimeType) {
            printf("Empty mime type: %s\n", mimeType);
        }
		printf("Destination would reject if dropped\n");
        setCursorOverwrite(GROUNDED_MOUSE_CURSOR_DND_NO_DROP);
	}
}

static void dataSourceHandleSend(void *data, struct wl_data_source *wl_data_source, const char* _mimeType, int32_t fd) {
    GROUNDED_WAYLAND_LOG_HANDLER("dataSource.send");

    struct WaylandDataSource* waylandDataSource = (struct WaylandDataSource*) data;
    ASSERT(waylandDataSource);
    //ASSERT(waylandDataSource->sendCallback);
    //printf("Target requests data\n");
    
    // Get index of mimeType
    String8 mimeType = str8FromCstr(_mimeType);
    u64 mimeTypeIndex = UINT64_MAX;
    if(waylandDataSource) {
        for(u64 i = 0; i < waylandDataSource->mimeTypeCount; ++i) {
            if(str8IsEqual(waylandDataSource->mimeTypes[i], mimeType)) {
                mimeTypeIndex = i;
                mimeType = waylandDataSource->mimeTypes[i];
                break;
            }
        }
    }

    if(mimeTypeIndex < waylandDataSource->mimeTypeCount && waylandDataSource->dataCallback) {
        //TODO: Cache data of this mimetype
        String8 data = waylandDataSource->dataCallback(waylandDataSource->arena, mimeType, mimeTypeIndex, waylandDataSource->userData);
        write(fd, data.base, data.size);
    }
	close(fd);
}

// Drop has been cancelled. Now we can release resources. Only gets called when replaced by a new data source
static void dataSourceHandleCancelled(void *data, struct wl_data_source * dataSource) {
    GROUNDED_WAYLAND_LOG_HANDLER("dataSource.cancelled");

    // If version is <= 2 this is only sent when the data source has been replaced by another source
    struct WaylandDataSource* waylandDataSource = (struct WaylandDataSource*) data;
    if(waylandDataSource->dragFinishCallback) {
        waylandDataSource->dragFinishCallback(waylandDataSource->arena, waylandDataSource->userData, GROUNDED_DRAG_FINISH_TYPE_CANCEL);
    }
    
    removeCursorOverwrite();
    if(dragDataSource->dataSource == dataSource) {
        dragDataSource = 0;
    }
    wl_data_source_destroy(dataSource);
    arenaRelease(waylandDataSource->arena);
}

// Since version 3: Basically no useful information for us so we do nothing
static void dataSourceHandleDndDropPerformed(void *data, struct wl_data_source *wl_data_source) {
    GROUNDED_WAYLAND_LOG_HANDLER("dataSource.dropPerformed");
}

// Since version 3: We are now allowed to free all resources as drop was successful
static void dataSourceHandleDndFinished(void *data, struct wl_data_source* dataSource) {
    GROUNDED_WAYLAND_LOG_HANDLER("dataSource.finished");

    struct WaylandDataSource* waylandDataSource = (struct WaylandDataSource*) data;
    if(waylandDataSource->dragFinishCallback) {
        // We default to move action
        GroundedDragFinishType finishType = GROUNDED_DRAG_FINISH_TYPE_MOVE;
        ASSERT(waylandDataSource->last_dnd_action != WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE);
        if(waylandDataSource->last_dnd_action == WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY) {
            finishType = GROUNDED_DRAG_FINISH_TYPE_COPY;
        }
        waylandDataSource->dragFinishCallback(waylandDataSource->arena, waylandDataSource->userData, finishType);
    }
    removeCursorOverwrite();
    if(dragDataSource->dataSource == dataSource) {
        dragDataSource = 0;
    }
    wl_data_source_destroy(dataSource);
    arenaRelease(waylandDataSource->arena);
}

static void dataSourceHandleAction(void *data, struct wl_data_source *source, u32 dnd_action) {
    GROUNDED_WAYLAND_LOG_HANDLER("dataSource.action");

    struct WaylandDataSource* waylandDataSource = (struct WaylandDataSource*) data;
    if(data) {
        waylandDataSource->last_dnd_action = dnd_action;
    }
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

GROUNDED_FUNCTION void groundedWaylandDragPayloadSetImage(GroundedWindowDragPayloadDescription* desc, u8* data, u32 width, u32 height) {
    // It is not allowed to set the image twice
    ASSERT(!desc->waylandIcon);
    
    if(!desc->waylandIcon) {
        desc->waylandIcon = wl_compositor_create_surface(compositor);
        if(desc->waylandIcon) {
            u64 imageSize = width * height * sizeof(u32);
            int fd = createSharedMemoryFile(imageSize);
            u8* poolData = mmap(0, imageSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            struct wl_shm_pool* pool = wl_shm_create_pool(waylandShm, fd, imageSize);

            struct wl_buffer* wlBuffer = wl_shm_pool_create_buffer(pool, 0, width, height, width * sizeof(u32), WL_SHM_FORMAT_XRGB8888);
            MEMORY_COPY(poolData, data, imageSize);

            wl_surface_attach(desc->waylandIcon, wlBuffer, 0, 0);
            //TODO: Do we always want to set this or only when the user requests this via a flag or similar?
            wl_surface_set_buffer_transform(desc->waylandIcon, WL_OUTPUT_TRANSFORM_FLIPPED_180);
            //wl_surface_offset(desc->icon, 100, 100);
            wl_surface_damage(desc->waylandIcon, 0, 0, UINT32_MAX, UINT32_MAX);
            wl_surface_commit(desc->waylandIcon); // Commit staged changes to surface
        }
    }
}

GROUNDED_FUNCTION void groundedWaylandBeginDragAndDrop(GroundedWindowDragPayloadDescription* desc, void* userData) {
    // Serial is the last pointer serial. Should probably be pointer button serial
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    
    if(dragDataSource) {
        GROUNDED_LOG_WARNING("Begin drag while already having a drag active");
        // At least KWin seems to have problems with multiple active drags so we disallow it here
        // However this seems to also cancel the second drag at least if we use the same dragSerial.
        // We could completely ignore the request when the same dragSerial is going to be used and just keep
        // the first drag
        dragDataSource->dragFinishCallback(dragDataSource->arena, dragDataSource->userData, GROUNDED_DRAG_FINISH_TYPE_CANCEL);
        removeCursorOverwrite();
        wl_data_source_destroy(dragDataSource->dataSource);
        arenaRelease(dragDataSource->arena);
        dragDataSource = 0;
    }

    dragDataSource = ARENA_PUSH_STRUCT(&desc->arena, struct WaylandDataSource);
    dragDataSource->arena = &desc->arena;
    dragDataSource->dataCallback = desc->dataCallback;
    dragDataSource->dragFinishCallback = desc->dragFinishCallback;
    dragDataSource->userData = userData;
    dragDataSource->mimeTypeCount = desc->mimeTypeCount;
    dragDataSource->mimeTypes = desc->mimeTypes;

    struct wl_data_source* dataSource = wl_data_device_manager_create_data_source(dataDeviceManager);
    wl_data_source_add_listener(dataSource, &dataSourceListener, dragDataSource);
    for(u64 i = 0; i < dragDataSource->mimeTypeCount; ++i) {
        wl_data_source_offer(dataSource, str8GetCstr(scratch, dragDataSource->mimeTypes[i]));
    }
    if(dataDeviceManagerVersion >= 3) {
        wl_data_source_set_actions(dataSource, WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE | WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
    }

    setCursorOverwrite(GROUNDED_MOUSE_CURSOR_GRABBING);

    printf("Drag serial: %u\n", lastPointerSerial);
    dragDataSource->dataSource = dataSource;
    wl_data_device_start_drag(dataDevice, dataSource, activeWindow->surface, desc->waylandIcon, lastPointerSerial);

    arenaEndTemp(temp);
}

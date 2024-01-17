#include <grounded/window/grounded_window.h>

GroundedEvent eventQueue[256];
u32 eventQueueIndex;

static const char** getCursorNameCandidates(enum GroundedMouseCursor cursorType, u64* candidateCount);

struct GroundedWindowDragPayloadDescription {
    MemoryArena arena;
	union {
    	struct wl_surface* waylandIcon;
        u32 xcbIcon;
	};
    u32 mimeTypeCount;
    String8* mimeTypes;
    GroundedWindowDndDataCallback* dataCallback;
    GroundedWindowDndDragFinishCallback* dragFinishCallback;
};

#ifdef GROUNDED_OPENGL_SUPPORT
//#include <EGL/egl.h>
#include "types/grounded_egl_types.h"
struct GroundedOpenGLContext {
    EGLContext eglContext;
};

// egl function types
#define X(N, R, P) typedef R grounded_egl_##N P;
#include "types/grounded_egl_functions.h"
#undef X

// egl function pointers
#define X(N, R, P) static grounded_egl_##N * N = 0;
#include "types/grounded_egl_functions.h"
#undef X
#endif

#include "grounded_xcb.c"
#include "grounded_wayland.c"

// This is a linux header
#include <time.h>

typedef enum WindowBackend {
    GROUNDED_LINUX_WINDOW_BACKEND_NONE,
    GROUNDED_LINUX_WINDOW_BACKEND_XCB,
    GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND,
} WindowBackend;

WindowBackend linuxWindowBackend = GROUNDED_LINUX_WINDOW_BACKEND_NONE;

GROUNDED_FUNCTION void groundedInitWindowSystem() {
    bool skipWayland = true;
    if(!skipWayland && initWayland()) {
        linuxWindowBackend = GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND;
    } else {
        initXcb();
        linuxWindowBackend = GROUNDED_LINUX_WINDOW_BACKEND_XCB;
    }
}
GROUNDED_FUNCTION void groundedShutdownWindowSystem() {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            shutdownWayland();
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            shutdownXcb();
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION GroundedWindow* groundedCreateWindow(MemoryArena* arena, struct GroundedWindowCreateParameters* parameters) {
    if(!parameters) {
        static struct GroundedWindowCreateParameters defaultParameters = {0};
        parameters = &defaultParameters;
    }

    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandCreateWindow(arena, parameters);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbCreateWindow(arena, parameters);
        } break;
        default:break;
    }
    return 0;
}

GROUNDED_FUNCTION void groundedDestroyWindow(GroundedWindow* window) {
    ASSERT(window);
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandDestroyWindow((GroundedWaylandWindow*)window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbDestroyWindow((GroundedXcbWindow*)window);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION u32 groundedWindowGetWidth(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandGetWindowWidth((GroundedWaylandWindow*)window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbGetWindowWidth((GroundedXcbWindow*)window);
        } break;
        default:break;
    }
    return 0;
}

GROUNDED_FUNCTION u32 groundedWindowGetHeight(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandGetWindowHeight((GroundedWaylandWindow*)window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbGetWindowHeight((GroundedXcbWindow*)window);
        } break;
        default:break;
    }
    return 0;
}

GROUNDED_FUNCTION void groundedWindowSetTitle(GroundedWindow* window, String8 title) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandSetWindowTitle((GroundedWaylandWindow*)window, title);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbSetWindowTitle((GroundedXcbWindow*)window, title, true);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowSetFullscreen(GroundedWindow* window, bool fullscreen) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandWindowSetFullsreen((GroundedWaylandWindow*)window, fullscreen);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbWindowSetFullscreen((GroundedXcbWindow*)window, fullscreen);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowSetBorderless(GroundedWindow* window, bool borderless) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandWindowSetBorderless((GroundedWaylandWindow*)window, borderless);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbWindowSetBorderless((GroundedXcbWindow*)window, borderless);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowSetHidden(GroundedWindow* window, bool hidden) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandWindowSetHidden((GroundedWaylandWindow*)window, hidden);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbWindowSetHidden((GroundedXcbWindow*)window, hidden);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowSetUserData(GroundedWindow* window, void* userData) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandWindowSetUserData((GroundedWaylandWindow*)window, userData);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbWindowSetUserData((GroundedXcbWindow*)window, userData);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void* groundedWindowGetUserData(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandWindowGetUserData((GroundedWaylandWindow*)window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbWindowGetUserData((GroundedXcbWindow*)window);
        } break;
        default:break;
    }
    return 0;
}

GROUNDED_FUNCTION GroundedEvent* groundedWindowGetEvents(u32* eventCount, u32 maxWaitingTimeInMs) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    *eventCount = 0;

    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandGetEvents(eventCount, maxWaitingTimeInMs);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbGetEvents(eventCount, maxWaitingTimeInMs);
        } break;
        default:break;
    }
    return 0;
}

GROUNDED_FUNCTION GroundedEvent* groundedWindowPollEvents(u32* eventCount) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    *eventCount = 0;
    
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandPollEvents(eventCount);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbPollEvents(eventCount);
        } break;
        default:break;
    }
    return 0;
}

// Get time in nanoseconds
GROUNDED_FUNCTION u64 groundedGetCounter() {
    u64 result;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
    result = now.tv_sec;
    result *= 1000000000;
    result += now.tv_nsec;
    return result;
}

GROUNDED_FUNCTION void groundedWindowFetchKeyboardState(GroundedKeyboardState* keyboardState) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandFetchKeyboardState(keyboardState);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbFetchKeyboardState(keyboardState);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowFetchMouseState(GroundedWindow* window, MouseState* mouseState) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);

    mouseState->lastX = mouseState->x;
    mouseState->lastY = mouseState->y;
    mouseState->scrollDelta = 0.0f;
    mouseState->horizontalScrollDelta = 0.0f;

    ASSERT(sizeof(mouseState->buttons) == sizeof(mouseState->lastButtons));
    memcpy(mouseState->lastButtons, mouseState->buttons, sizeof(mouseState->buttons));

    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandFetchMouseState((GroundedWaylandWindow*)window, mouseState);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbFetchMouseState((GroundedXcbWindow*)window, mouseState);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowSetIcon(u8* data, u32 width, u32 height) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);

    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandSetIcon(data, width, height);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbSetIcon(data, width, height);
        } break;
        default:break;
    }
}

static const char** getCursorNameCandidates(enum GroundedMouseCursor cursorType, u64* candidateCount) {
    // https://github.com/chromium/chromium/blob/db174a51cdde1785b378e532700af65dfd5b2e28/ui/base/cursor/cursor_factory.cc#L163
    //TODO: The tee icons might be interesting in some occasions but probably not supported on win32 natively
    *candidateCount = 0;

    static const char* defaultCursors[] = {"default", "arrow", "left_ptr"};
    static const char* iBeamCursors[] = {"text", "xterm"};
    static const char* helpCursors[] = {"help", "question_arrow"};
    static const char* pointerCursors[] = {"pointer", "hand", "hand2"};
    static const char* progressCursors[] = {"progress", "left_ptr_watch", "watch"};
    static const char* waitCursors[] = {"wait", "watch"};
    static const char* dndCopyCursors[] = {"dnd-copy", "copy", "hand2"}; //TODO: Seems those are not necessarily dnd related?
    static const char* dndMoveCursors[] = {"dnd-move", "move", "hand2"};
    static const char* dndLinkCursors[] = {"dnd-link", "alias", "hand2"};
    static const char* dndNoDropCursors[] = {"dnd-none", "no-drop", "not-allowed", "crossed_circle", "hand2"}; //TODO: What is with circle?
    static const char* notAllowedCursors[] = {"not-allowed", "crossed_circle"};
    static const char* allScrollCursors[] = {"all-scroll", "fleur"}; // Also cursor for movement. However there might be special cursors for that?
    static const char* rowResizeCursors[] = {"row-resize", "sb_v_double_arrow"};
    static const char* columnResizeCursors[] = {"col-resize", "sb_h_double_arrow"};
    static const char* eastResizeCursors[] = {"e-resize", "right_side"};
    static const char* northEastResizeCursors[] = {"ne-resize", "top_right_corner"};
    static const char* northWestResizeCursors[] = {"nw-resize", "top_left_corner"};
    static const char* northResizeCursors[] = {"n-resize", "top_side"};
    static const char* southEastResizeCursors[] = {"se-resize", "bottom_right_corner"};
    static const char* southWestResizeCursors[] = {"sw-resize", "bottom_left_corner"};
    static const char* southResizeCursors[] = {"s-resize", "bottom_side"};
    static const char* westResizeCursors[] = {"w-resize", "left_side"};
    static const char* northSouthResizeCursors[] = {"sb_v_double_arrow", "ns-resize"};
    static const char* eastWestResizeCursors[] = {"sb_h_double_arrow", "ew-resize"};

    static const char* crosshairCursors[] = {"crosshair", "cross"};
    static const char* verticalTextCursors[] = {"vertical-text"};
    static const char* cellCursors[] = {"cell", "plus"};
    static const char* contextMenuCursors[] = {"context-menu"};
    // Not useful in a practical sense but interestig nonetheless
    static const char* specialCursors[] = {"dot", "pirate", "heart"};

    static const char* grabbingCursors[] = {"closedhand", "grabbing", "hand2"};

    // Grab: openhand, grab, hand1
    // northeastsouthwestresize: size_bdiag, nesw-resize, fd_double_arrow
    // northwestsourtheastresize: size_fdiag, nwse-resize, bd_double_arrow
    // zoomin: zoom-in
    // zoomout: zoom-out

    #define USE_CURSOR_CANDIDATE(candidates) cursorCandidates = candidates; cursorCandidateCount = ARRAY_COUNT(candidates)
    const char** cursorCandidates;
    u64 cursorCandidateCount = 0;
    switch(cursorType) {
        case GROUNDED_MOUSE_CURSOR_DEFAULT:{
            USE_CURSOR_CANDIDATE(defaultCursors);
        } break;
        case GROUNDED_MOUSE_CURSOR_IBEAM:{
            USE_CURSOR_CANDIDATE(iBeamCursors);
        } break;
        case GROUNDED_MOUSE_CURSOR_LEFTRIGHT:{
            USE_CURSOR_CANDIDATE(eastWestResizeCursors);
        } break;
        case GROUNDED_MOUSE_CURSOR_UPDOWN:{
            USE_CURSOR_CANDIDATE(northSouthResizeCursors);
        } break;
        case GROUNDED_MOUSE_CURSOR_UPRIGHT:{
            USE_CURSOR_CANDIDATE(northEastResizeCursors);
        } break;
        case GROUNDED_MOUSE_CURSOR_UPLEFT:{
            USE_CURSOR_CANDIDATE(northWestResizeCursors);
        } break;
        case GROUNDED_MOUSE_CURSOR_DOWNRIGHT:{
            USE_CURSOR_CANDIDATE(southEastResizeCursors);
        } break;
        case GROUNDED_MOUSE_CURSOR_DOWNLEFT:{
            USE_CURSOR_CANDIDATE(southWestResizeCursors);
        } break;
        case GROUNDED_MOUSE_CURSOR_POINTER:{
            USE_CURSOR_CANDIDATE(pointerCursors);
        } break;
        case GROUNDED_MOUSE_CURSOR_DND_NO_DROP:{
            USE_CURSOR_CANDIDATE(dndNoDropCursors);
        } break;
        case GROUNDED_MOUSE_CURSOR_DND_MOVE:{
            USE_CURSOR_CANDIDATE(dndMoveCursors);
        } break;
        case GROUNDED_MOUSE_CURSOR_DND_COPY:{
            USE_CURSOR_CANDIDATE(dndCopyCursors);
        } break;
        case GROUNDED_MOUSE_CURSOR_GRABBING:{
            USE_CURSOR_CANDIDATE(grabbingCursors);
        } break;
        default:{
            // Cursor not found. Try to use a default
            USE_CURSOR_CANDIDATE(defaultCursors);
        } break;
    }
    #undef USE_CURSOR_CANDIDATE

    *candidateCount = cursorCandidateCount;
    return cursorCandidates;
}

GROUNDED_FUNCTION void groundedSetCursorType(enum GroundedMouseCursor cursorType) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandSetCursorType(cursorType);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbSetCursorType(cursorType);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedSetCustomCursor(u8* data, u32 width, u32 height) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandSetCustomCursor(data, width, height);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbSetCustomCursor(data, width, height);
        } break;
        default:break;
    }
}

// Release of the arena is done in wayland/xcb backend once the drag is over
/*static void _groundedStartDragAndDrop(MemoryArena* arena, GroundedWindow* window, u64 mimeTypeCount, String8* mimeTypes, GroundedWindowDndSendCallback* callback, GroundedWindowDragPayloadImage* image, void* userData) {
    ASSERT(arena);
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandStartDragAndDrop(arena, (GroundedWaylandWindow*)window, mimeTypeCount, mimeTypes, callback, image, userData);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            //xcbSetCustomCursor(data, width, height);
            ASSERT(false);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedStartDragAndDrop(GroundedWindow* window, u64 mimeTypeCount, String8* mimeTypes, GroundedWindowDndSendCallback* callback, GroundedWindowDragPayloadImage* image, void* userData) {
    MemoryArena* arena = ARENA_BOOTSTRAP(createGrowingArena(osGetMemorySubsystem(), KB(4)));
    _groundedStartDragAndDrop(arena, window, mimeTypeCount, mimeTypes, callback, image, userData);
}*/

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

GROUNDED_FUNCTION MemoryArena* groundedWindowDragPayloadGetArena(GroundedWindowDragPayloadDescription* desc) {
    return &desc->arena;
}

GROUNDED_FUNCTION void groundedWindowDragPayloadSetDataCallback(GroundedWindowDragPayloadDescription* desc, GroundedWindowDndDataCallback* callback) {
    desc->dataCallback = callback;
}

GROUNDED_FUNCTION void groundedWindowDragPayloadSetDragFinishCallback(GroundedWindowDragPayloadDescription* desc, GroundedWindowDndDragFinishCallback* callback) {
    desc->dragFinishCallback = callback;
}

GROUNDED_FUNCTION GroundedWindowDragPayloadDescription* groundedWindowPrepareDragPayload(GroundedWindow* window) {
    GroundedWindowDragPayloadDescription* result = ARENA_BOOTSTRAP_PUSH_STRUCT(createGrowingArena(osGetMemorySubsystem(), KB(4)), GroundedWindowDragPayloadDescription, arena);
    return result;
}

GROUNDED_FUNCTION GroundedWindowDndCallback* groundedWindowGetDndCallback(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    GroundedWindowDndCallback* result = 0;
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            result = ((GroundedWaylandWindow*)window)->dndCallback;
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            result = ((GroundedXcbWindow*)window)->dndCallback;
        } break;
        default:break;
    }
    return result;
}

GROUNDED_WINDOW_DND_DATA_CALLBACK(simpleDragAndDropSend) {
    ASSERT(mimeIndex == 0);
    ASSERT(userData);
    String8* payload = (String8*) userData;
    return *payload;
}

GROUNDED_FUNCTION void groundedWindowDragPayloadSetImage(GroundedWindowDragPayloadDescription* desc, u8* data, u32 width, u32 height) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            groundedWaylandDragPayloadSetImage(desc, data, width, height);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            groundedXcbDragPayloadSetImage(desc, data, width, height);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowBeginDragAndDrop(GroundedWindowDragPayloadDescription* desc, void* userData) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            groundedWaylandBeginDragAndDrop(desc, userData);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            groundedXcbBeginDragAndDrop(desc, userData);
        } break;
        default:break;
    }
}

/*GROUNDED_FUNCTION void groundedStartDragAndDropWithSingleDataType(GroundedWindow* window, String8 mimeType, u8* data, u64 size, GroundedWindowDragPayloadImage* image) {
    // Create the arena and store itself as well as the payload data itself in it
    MemoryArena* arena = ARENA_BOOTSTRAP(createGrowingArena(osGetMemorySubsystem(), size + KB(2)));
    String8 original = str8FromBlock(data, size);
    String8* payload = ARENA_PUSH_STRUCT_NO_CLEAR(arena, String8);
    *payload = str8Copy(arena, original);
    _groundedStartDragAndDrop(arena, window, 1, &mimeType, simpleDragAndDropSend, image, payload);
}'*/

// ************
// OpenGL stuff
#ifdef GROUNDED_OPENGL_SUPPORT
static void* glLibrary;
GROUNDED_FUNCTION GroundedOpenGLContext* groundedCreateOpenGLContext(MemoryArena* arena, GroundedOpenGLContext* contextToShareResources) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);

    if(!eglCreateContext) {
        void* eglLibrary = dlopen("libEGL.so", RTLD_LAZY | RTLD_LOCAL);
        if(!eglLibrary) {
            GROUNDED_LOG_ERROR("No egl library found");
            return 0;
        } else {
            const char* firstMissingFunctionName = 0;
            #define X(N, R, P) N = (grounded_egl_##N *) dlsym(eglLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "types/grounded_egl_functions.h"
            #undef X
            if(firstMissingFunctionName) {
                printf("Could not load egl function: %s\n", firstMissingFunctionName);
                const char* error = "Could not load all egl functions. Your egl version is incompatible";
                GROUNDED_LOG_ERROR(error);
                dlclose(eglLibrary);
                eglLibrary = 0;
                return 0;
            }
        }
    }

    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandCreateOpenGLContext(arena, contextToShareResources);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbCreateOpenGLContext(arena, contextToShareResources);
        }break;
        default:break;
    }
    return false;
}

GROUNDED_FUNCTION void* groundedWindowLoadGlFunction(const char* symbol) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    void* result = 0;

    if(!glLibrary) {
        glLibrary = dlopen("libGL.so", RTLD_LAZY | RTLD_LOCAL);
        if(!glLibrary) {
            GROUNDED_LOG_ERROR("Could not find libGL.so");
        }
    }

    if(glLibrary) {
        result = dlsym(glLibrary, symbol);
        if(!result) {
            GROUNDED_LOG_ERROR("Could not load symbol from GL");
        }
    }

    return result;
}

GROUNDED_FUNCTION void groundedMakeOpenGLContextCurrent(GroundedWindow* window, GroundedOpenGLContext* context) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandOpenGLMakeCurrent((GroundedWaylandWindow*)window, context);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbOpenGLMakeCurrent((GroundedXcbWindow*)window, context);
        }break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowGlSwapBuffers(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandWindowGlSwapBuffers((GroundedWaylandWindow*)window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbWindowGlSwapBuffers((GroundedXcbWindow*)window);
        }break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowSetGlSwapInterval(int interval) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandWindowSetGlSwapInterval(interval);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbWindowSetGlSwapInterval(interval);
        }break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowDestroyOpenglGontext(GroundedOpenGLContext* context) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandDestroyOpenGLContext(context);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbDestroyOpenGLContext(context);
        }break;
        default:break;
    }
}
#endif // GROUNDED_OPENGL_SUPPORT


// ************
// Vulkan stuff
#ifdef GROUNDED_VULKAN_SUPPORT
GROUNDED_FUNCTION const char** groundedWindowGetVulkanInstanceExtensions(u32* count) {
    static const char* waylandInstanceExtensions[] = {
        "VK_KHR_surface",
        "VK_KHR_wayland_surface",
    };
    static const char* xcbInstanceExtensions[] = {
        "VK_KHR_surface",
        "VK_KHR_xcb_surface",
    };
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            *count = ARRAY_COUNT(waylandInstanceExtensions);
            return waylandInstanceExtensions;
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            *count = ARRAY_COUNT(xcbInstanceExtensions);
            return xcbInstanceExtensions;
        }break;
        default:break;
    }
    *count = 0;
    return 0;
}

GROUNDED_FUNCTION VkSurfaceKHR groundedWindowGetVulkanSurface(GroundedWindow* window, VkInstance instance) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandGetVulkanSurface(window, instance);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbGetVulkanSurface(window, instance);
        }break;
        default:break;
    }
    return 0;
}
#endif // GROUNDED_VULKAN_SUPPORT

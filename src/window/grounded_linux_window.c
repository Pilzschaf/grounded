#include <grounded/window/grounded_window.h>

GroundedEvent eventQueue[256];
u32 eventQueueIndex;

static const char** getCursorNameCandidates(enum GroundedMouseCursor cursorType, u64* candidateCount);

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
    bool skipWayland = false;
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

GROUNDED_FUNCTION GroundedWindow* groundedCreateWindow(struct GroundedWindowCreateParameters* parameters) {
    if(!parameters) {
        static struct GroundedWindowCreateParameters defaultParameters = {0};
        parameters = &defaultParameters;
    }

    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandCreateWindow(parameters);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbCreateWindow(parameters);
        } break;
        default:break;
    }
    return 0;
}

GROUNDED_FUNCTION void groundedDestroyWindow(GroundedWindow* window) {
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

GROUNDED_FUNCTION u32 groundedGetWindowWidth(GroundedWindow* window) {
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

GROUNDED_FUNCTION u32 groundedGetWindowHeight(GroundedWindow* window) {
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

GROUNDED_FUNCTION GroundedEvent* groundedGetEvents(u32* eventCount, u32 maxWaitingTimeInMs) {
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

GROUNDED_FUNCTION GroundedEvent* groundedPollEvents(u32* eventCount) {
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

GROUNDED_FUNCTION void groundedFetchKeyboardState(GroundedKeyboardState* keyboardState) {
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

GROUNDED_FUNCTION void groundedFetchMouseState(GroundedWindow* window, MouseState* mouseState) {
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
    static const char* dndCopyCursors[] = {"copy"}; //TODO: Seems those are not necessarily dnd related?
    static const char* dndAliasCursors[] = {"alias"};
    static const char* dndNoDropCursors[] = {"no-drop", "not-allowed", "crossed_circle"}; //TODO: What is with circle?
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


    // Grab: openhand, grab, hand1
    // Grabbing: closedhand, grabbing, hand2
    // northeastsouthwestresize: size_bdiag, nesw-resize, fd_double_arrow
    // northwestsourtheastresize: size_fdiag, nwse-resize, bd_double_arrow
    // zoomin: zoom-in
    // zoomout: zoom-out
    // DNDNone: dnd-none, hand2
    // DNDMove: dnd-move, hand2
    // DNDCopy: dnd-copy, hand2
    // DNDLink: dnd-link, hand2

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
        case GROUNDED_MOUSE_CURSOR_POINTER:{
            USE_CURSOR_CANDIDATE(pointerCursors);
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

// ************
// OpenGL stuff
#ifdef GROUNDED_OPENGL_SUPPORT
GROUNDED_FUNCTION bool groundedCreateOpenGLContext(GroundedWindow* window, u32 flags, GroundedWindow* windowContextToShareResources) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandCreateOpenGLContext((GroundedWaylandWindow*)window, flags, (GroundedWaylandWindow*)windowContextToShareResources);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbCreateOpenGLContext((GroundedXcbWindow*)window, flags, (GroundedXcbWindow*)windowContextToShareResources);
        }break;
        default:break;
    }
    return false;
}

GROUNDED_FUNCTION void groundedMakeOpenGLContextCurrent(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandOpenGLMakeCurrent((GroundedWaylandWindow*)window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbOpenGLMakeCurrent((GroundedXcbWindow*)window);
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

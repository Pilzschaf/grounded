#include <grounded/window/grounded_window.h>

GroundedEvent eventQueue[256];
u32 eventQueueIndex;

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
    if(initWayland() && !skipWayland) {
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
            waylandDestroyWindow(window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbDestroyWindow(window);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION u32 groundedGetWindowWidth(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandGetWindowWidth(window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbGetWindowWidth(window);
        } break;
        default:break;
    }
    return 0;
}

GROUNDED_FUNCTION u32 groundedGetWindowHeight(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandGetWindowHeight(window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbGetWindowHeight(window);
        } break;
        default:break;
    }
    return 0;
}

GROUNDED_FUNCTION void groundedWindowSetTitle(GroundedWindow* window, String8 title) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandSetWindowTitle(window, title);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbSetWindowTitle(window, title, true);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowSetFullscreen(GroundedWindow* window, bool fullscreen) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandWindowSetFullsreen(window, fullscreen);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbWindowSetFullscreen(window, fullscreen);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowSetBorderless(GroundedWindow* window, bool borderless) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandWindowSetBorderless(window, borderless);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbWindowSetBorderless(window, borderless);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowSetHidden(GroundedWindow* window, bool hidden) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandWindowSetHidden(window, hidden);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbWindowSetHidden(window, hidden);
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
            waylandFetchMouseState(window, mouseState);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbFetchMouseState(window, mouseState);
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
            return waylandCreateOpenGLContext(window, flags, windowContextToShareResources);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbCreateOpenGLContext(window, flags, windowContextToShareResources);
        }break;
        default:break;
    }
    return false;
}

GROUNDED_FUNCTION void groundedMakeOpenGLContextCurrent(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandOpenGLMakeCurrent(window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbOpenGLMakeCurrent(window);
        }break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowGlSwapBuffers(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandWindowGlSwapBuffers(window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbWindowGlSwapBuffers(window);
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

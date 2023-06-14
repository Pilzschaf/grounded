#include "grounded_window.h"

#if 0
#include "grounded_win32_types.h"
#else
//#include <windef.h>
//#include <winuser.h>
//#include <ntdef.h>
#include <windows.h>
#endif

#include <stdio.h>
#include "../threading/grounded_threading.h"

typedef struct GroundedWin32Window {
    HWND hWnd;
    u32 minWidth;
    u32 minHeight;
    u32 maxWidth;
    u32 maxHeight;
} GroundedWin32Window;

MouseState win32MouseState;
GroundedKeyboardState win32KeyboardState;

GroundedEvent eventQueue[256];
u32 eventQueueIndex;

static u8 translateWin32Keycode(WPARAM wParam) {
    u8 result = 0;
    if (wParam >= 0x30 && wParam <= 0x39) return (u8)wParam; // Numbers
    if (wParam >= 0x41 && wParam <= 0x5A) return (u8)wParam; // Characters
    if (wParam >= 0x70 && wParam <= 0x7F) return (u8)wParam; // F keys
    switch (wParam) {
    case(0x08):
        result = GROUNDED_KEY_BACKSPACE;
        break;
    case(0x09):
        result = GROUNDED_KEY_TAB;
        break;
    case (0x10):
        //TODO: Differentiation between LSHIFT and RSHIFT is only done by low level keyboard handler.
        // See https://stackoverflow.com/questions/1811206/on-win32-how-to-detect-whether-a-left-shift-or-right-alt-is-pressed-using-perl
        result = GROUNDED_KEY_LSHIFT;
        break;
    case(0x0D):
        result = GROUNDED_KEY_RETURN;
        break;
    case(0x1B):
        result = GROUNDED_KEY_ESCAPE;
        break;
    case(0x20):
        result = GROUNDED_KEY_SPACE;
        break;
    case(0x25):
        result = GROUNDED_KEY_LEFT;
        break;
    case(0x226):
        result = GROUNDED_KEY_UP;
        break;
    case(0x27):
        result = GROUNDED_KEY_RIGHT;
        break;
    case(0x28):
        result = GROUNDED_KEY_DOWN;
        break;
    default:
        printf("Unknown keycode: %i\n", (int)wParam);
        break;
    }
    return result;
}

GroundedWin32Window windows;

// This can be called directly from the thread that created the window with this message loop. For example by calls to createWindow, ShowWindow etc.
// Not all messages go through the queue eg. GetMessage/PollMessage. But also non-queued messages are only delivered if the creating thread is blocking
static LRESULT CALLBACK win32MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_CLOSE: {
            // Get window from hWnd parameter
            PostQuitMessage(0);

            // Can differentiate by message before this.
            // If WM_SYSCOMMAND, wParam = SC_CLOSE and lParam != 0 => Close button
            // If WM_SYSCOMMAND, wParam = SC_CLOSE and LlParam == 0 => Alt+F4 or kill from taskbar/task manager
            // Alt + F4 could be detected from WM_SYSKEYDOWN wParam = VK_F4
            // InSendMessage could be used to detect if request came from other application (like task manager)
        } break;
        case WM_LBUTTONDOWN: {
            win32MouseState.buttons[GROUNDED_MOUSE_BUTTON_LEFT] = true;
            win32MouseState.buttonDownTransitions[GROUNDED_MOUSE_BUTTON_LEFT]++;
        } break;
        case WM_LBUTTONUP: {
            win32MouseState.buttons[GROUNDED_MOUSE_BUTTON_LEFT] = false;
            win32MouseState.buttonUpTransitions[GROUNDED_MOUSE_BUTTON_LEFT]++;
        } break;
        case WM_RBUTTONDOWN: {
            win32MouseState.buttons[GROUNDED_MOUSE_BUTTON_RIGHT] = true;
            win32MouseState.buttonDownTransitions[GROUNDED_MOUSE_BUTTON_RIGHT]++;
        } break;
        case WM_RBUTTONUP: {
            win32MouseState.buttons[GROUNDED_MOUSE_BUTTON_RIGHT] = false;
            win32MouseState.buttonUpTransitions[GROUNDED_MOUSE_BUTTON_RIGHT]++;
        } break;
        case WM_MBUTTONDOWN: {
            win32MouseState.buttons[GROUNDED_MOUSE_BUTTON_MIDDLE] = true;
            win32MouseState.buttonDownTransitions[GROUNDED_MOUSE_BUTTON_MIDDLE]++;
        } break;
        case WM_MBUTTONUP: {
            win32MouseState.buttons[GROUNDED_MOUSE_BUTTON_MIDDLE] = false;
            win32MouseState.buttonUpTransitions[GROUNDED_MOUSE_BUTTON_MIDDLE]++;
        } break;
        case WM_MOUSEWHEEL: {
            // Zoom in is positive
            s16 amount = GET_WHEEL_DELTA_WPARAM(wParam);
            win32MouseState.scrollDelta = amount / 120.0f;
        } break;
        case WM_MOUSEHWHEEL: {
            // Right is positive
            s16 amount = GET_WHEEL_DELTA_WPARAM(wParam);
            win32MouseState.horizontalScrollDelta = amount / 120.0f;
        } break;
        case WM_KEYDOWN: {
            u8 keycode = translateWin32Keycode(wParam);
            win32KeyboardState.keys[keycode] = true;
            win32KeyboardState.keyDownTransitions[keycode]++;
        } break;
        case WM_KEYUP: {
            u8 keycode = translateWin32Keycode(wParam);
            win32KeyboardState.keys[keycode] = false;
            win32KeyboardState.keyUpTransitions[keycode]++;
        } break;
        case WM_GETMINMAXINFO: {
            // Sent when size is about to change. We can specify min and max size here

            GroundedWin32Window* window = &windows;

            LPMINMAXINFO minMaxInfo = (LPMINMAXINFO)lParam;
            if (window->minWidth) {
                minMaxInfo->ptMinTrackSize.x = window->minWidth;
            } else {
                minMaxInfo->ptMinTrackSize.x = GetSystemMetrics(SM_CXMINTRACK);
            }
            if (window->minHeight) {
                minMaxInfo->ptMinTrackSize.y = window->minHeight;
            } else {
                minMaxInfo->ptMinTrackSize.y = GetSystemMetrics(SM_CYMINTRACK);
            }
            if (window->maxWidth) {
                minMaxInfo->ptMaxTrackSize.x = window->maxWidth;
            } else {
                minMaxInfo->ptMaxTrackSize.x = GetSystemMetrics(SM_CXMAXTRACK);
            }
            if (window->maxHeight) {
                minMaxInfo->ptMaxTrackSize.y = window->maxHeight;
            } else {
                minMaxInfo->ptMaxTrackSize.y = GetSystemMetrics(SM_CYMAXTRACK);
            }
        } break;
        default: {
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		} break;
    }
    return 0;
}

GROUNDED_FUNCTION void groundedInitWindowSystem() {
    const char* error = 0;
    HINSTANCE instance = GetModuleHandle(0);
    //HICON icon = LoadIconW(0, MAKEINTRESOURCEW(IDI_ERROR));
    // Ex variant allows to set a small icon. Small icon seems to be used by taskbar and in the top left corner of caption
    WNDCLASSW wnd = {0};
    // Redraw on size/movement on x and y axis https://docs.microsoft.com/en-us/windows/win32/winmsg/window-class-styles
    wnd.style = CS_HREDRAW | CS_VREDRAW;
    wnd.lpfnWndProc = win32MessageHandler;
    //TODO: Possible to allocate extra bytes for each class / window with cbWndExtra and cbClsExtra
    wnd.hInstance = instance;
    // Default icon
    wnd.hIcon = 0;
    // If this would be 0, the application must set the cursor on every enter event.
    wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
    // We have to draw into window ourselves.
    wnd.hbrBackground = 0;
    wnd.lpszClassName = L"GroundedDefaultWindowClass";

    ATOM atom = RegisterClassW(&wnd);
    if(!atom) {
        error = "Could not register window class";
    }
}

GROUNDED_FUNCTION void groundedShutdownWindowSystem() {
    //TODO: Release resources
}

GROUNDED_FUNCTION GroundedWindow* groundedCreateWindow(struct GroundedWindowCreateParameters* parameters) {
    if (!parameters) {
        struct GroundedWindowCreateParameters defaultParameters = {0};
        parameters = &defaultParameters;
    }
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    const char* error = 0;
    HINSTANCE instance = GetModuleHandle(0);
    GroundedWin32Window* result = &windows;
    *result = (GroundedWin32Window){0};
    HWND parentWindow = 0;
    // str16FromStr8 guarantees 0-termination
    String16 utf16Title = str16FromStr8(scratch, parameters->title);
    DWORD styles = WS_OVERLAPPEDWINDOW;
    result->hWnd = CreateWindowExW(
        0, // https://docs.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles
        L"GroundedDefaultWindowClass",
        utf16Title.base,
        styles,
        CW_USEDEFAULT, // position
        CW_USEDEFAULT,
        parameters->width ? parameters->width : CW_USEDEFAULT, // size
        parameters->height ? parameters->height : CW_USEDEFAULT,
        parentWindow,
        0,  //TODO: Must be a unique child window identifier if this is a child window
        instance,
        0 // User pointer passed to WM_CREATE
    );
    if(!result->hWnd) {
        error = "Could not create win32 window";
    }

    if(!error) {
        result->minWidth = parameters->minWidth;
        result->minHeight = parameters->minHeight;
        result->maxWidth = parameters->maxWidth;
        result->maxHeight = parameters->maxHeight;
        ShowWindow(result->hWnd, SW_SHOWDEFAULT);
    }

    if(error) {
        result = 0;
    }
    
    arenaEndTemp(temp);
    return result;
}

GROUNDED_FUNCTION void groundedDestroyWindow(GroundedWindow* window) {
    GroundedWin32Window* win32Window = (GroundedWin32Window*)window;
    DestroyWindow(win32Window->hWnd);
}

GROUNDED_FUNCTION void groundedSetWindowTitle(GroundedWindow* window, String8 title) {
    GroundedWin32Window* win32Window = (GroundedWin32Window*)window;
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    // str16FromStr8 guarantees 0-termination
    String16 utf16Title = str16FromStr8(scratch, title);
    SetWindowTextW(win32Window->hWnd, utf16Title.base);

    arenaEndTemp(temp);
}

GROUNDED_FUNCTION GroundedEvent* groundedGetEvents(u32* eventCount, u32 maxWaitingTimeInMs) {
    *eventCount = 0;
    eventQueueIndex = 0;

    MSG msg;
    // Option to do timeout: 
    //  - timerId = SetTimer(0, 0, maxWaitingTimeInMs, 0); GetMessage() After that KillTImer(0, timerId)
    //  - MsgWaitForMultipleObjects(0, 0, false, timeout, QS_ALLINPUT); If it returns WAIT_OBJECT_0, Call peekMessage
    // Using the first option for now as it is easier to skip it if no waiting time is specified
    
    // Timer is always recreated as it might otherwise trigger while the application is not waiting for events. There does not seem to be an option to pause the timer
    UINT_PTR timer = SetTimer(0, 0, maxWaitingTimeInMs, 0);
    BOOL keepRunning = GetMessageW(&msg, 0, 0, 0);
    KillTimer(0, timer);

    TranslateMessage(&msg);
    DispatchMessageW(&msg);

    if(!keepRunning) {
        eventQueue[eventQueueIndex++] = (GroundedEvent){
            .type = GROUNDED_EVENT_TYPE_CLOSE_REQUEST,
        };
    }

    *eventCount = eventQueueIndex;
    return eventQueue;
}

GROUNDED_FUNCTION GroundedEvent* groundedPollEvents(u32* eventCount) {
    *eventCount = 0;
    eventQueueIndex = 0;

    MSG msg;
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (msg.message == WM_QUIT) {
            eventQueue[eventQueueIndex++] = (GroundedEvent){
                .type = GROUNDED_EVENT_TYPE_CLOSE_REQUEST,
            };
            break;
        }
    }

    *eventCount = eventQueueIndex;
    return eventQueue;
}

GROUNDED_FUNCTION u32 groundedGetWindowWidth(GroundedWindow* opaqueWindow) {
    GroundedWin32Window* window = (GroundedWin32Window*)opaqueWindow;
    RECT size = {0};
    GetWindowRect(window->hWnd, &size);
    return ABS(size.right - size.left);
}

GROUNDED_FUNCTION u32 groundedGetWindowHeight(GroundedWindow* opaqueWindow) {
    GroundedWin32Window* window = (GroundedWin32Window*)opaqueWindow;
    RECT size = {0};
    GetWindowRect(window->hWnd, &size);
    return ABS(size.top - size.bottom);
}

GROUNDED_FUNCTION void groundedFetchMouseState(GroundedWin32Window* window, MouseState* mouseState) {
    mouseState->lastX = mouseState->x;
    mouseState->lastY = mouseState->y;
    ASSERT(sizeof(mouseState->buttons) == sizeof(mouseState->lastButtons));
    memcpy(mouseState->lastButtons, mouseState->buttons, sizeof(mouseState->buttons));

    POINT p;
    if (!GetCursorPos(&p)) {
        ASSERT(false);
        return;
    }
    if (!ScreenToClient(window->hWnd, &p)) {
        ASSERT(false);
        return;
    }
    mouseState->x = p.x;
    mouseState->y = p.y;
    //printf("Mouse location: %i,%i\n", p.x, p.y);

    // Set mouse button state
    memcpy(mouseState->buttons, win32MouseState.buttons, sizeof(win32MouseState.buttons));
    memcpy(mouseState->buttonDownTransitions, win32MouseState.buttonDownTransitions, sizeof(win32MouseState.buttonDownTransitions));
    memset(win32MouseState.buttonDownTransitions, 0, sizeof(win32MouseState.buttonDownTransitions));
    memcpy(mouseState->buttonUpTransitions, win32MouseState.buttonUpTransitions, sizeof(win32MouseState.buttonUpTransitions));
    memset(win32MouseState.buttonUpTransitions, 0, sizeof(win32MouseState.buttonUpTransitions));
    // Set mouse scroll state
    mouseState->scrollDelta = win32MouseState.scrollDelta;
    mouseState->horizontalScrollDelta = win32MouseState.horizontalScrollDelta;
    win32MouseState.scrollDelta = 0;
    win32MouseState.horizontalScrollDelta = 0;

    mouseState->deltaX = mouseState->x - mouseState->lastX;
    mouseState->deltaY = mouseState->y - mouseState->lastY;
    //printf("Mouse delta: %i,%i\n", mouseState->deltaX, mouseState->deltaY);
}

GROUNDED_FUNCTION void groundedFetchKeyboardState(GroundedKeyboardState* keyState) {
    memcpy(keyState->lastKeys, keyState->keys, sizeof(keyState->lastKeys));

    memcpy(keyState->keys, win32KeyboardState.keys, sizeof(keyState->keys));
    keyState->modifiers = win32KeyboardState.modifiers;

    memcpy(keyState->keyDownTransitions, win32KeyboardState.keyDownTransitions, sizeof(win32KeyboardState.keyDownTransitions));
    memset(win32KeyboardState.keyDownTransitions, 0, sizeof(win32KeyboardState.keyDownTransitions));
    memcpy(keyState->keyUpTransitions, win32KeyboardState.keyUpTransitions, sizeof(win32KeyboardState.keyUpTransitions));
    memset(win32KeyboardState.keyUpTransitions, 0, sizeof(win32KeyboardState.keyUpTransitions));
}


// Get time in nanoseconds
GROUNDED_FUNCTION u64 groundedGetCounter() {
    static u64 ticksPerMicroSecond = 0;
    if (!ticksPerMicroSecond) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        ticksPerMicroSecond = frequency.QuadPart / 1000000;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart * 1000) / ticksPerMicroSecond;
}


// ************
// OpenGL stuff
GROUNDED_FUNCTION void groundedWindowGlSwapBuffers(GroundedWindow* window) {
    HDC hDC = GetDC(window->hWnd);
    SwapBuffers(hDC);
    ReleaseDC(window->hWnd, hDC);
}



// ************
// Vulkan stuff
#ifdef GROUNDED_VULKAN_SUPPORT
typedef VkFlags VkWin32SurfaceCreateFlagsKHR;
typedef struct VkWin32SurfaceCreateInfoKHR {
    VkStructureType                 sType;
    const void*                     pNext;
    VkWin32SurfaceCreateFlagsKHR    flags;
    HINSTANCE                       hinstance;
    HWND                            hwnd;
} VkWin32SurfaceCreateInfoKHR;

typedef VkResult (VKAPI_PTR *PFN_vkCreateWin32SurfaceKHR)(VkInstance instance, const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
typedef VkBool32 (VKAPI_PTR *PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex);

const char** groundedWindowGetVulkanInstanceExtensions(u32* count) {
    static const char* instanceExtensions[] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface",
    };
    *count = ARRAY_COUNT(instanceExtensions);
    return instanceExtensions;
}

VkSurfaceKHR groundedWindowGetVulkanSurface(GroundedWindow* w, VkInstance instance) {
    GroundedWin32Window* window = (GroundedWin32Window*)w;
    const char* error = 0;
    VkSurfaceKHR surface = 0;
    static PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = 0;
    if(!vkCreateWin32SurfaceKHR) {
        vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");    
    }
    if(!vkCreateWin32SurfaceKHR) {
        error = "Could not load vkCreateWin32SurfaceKHR. Is the win32 surface instance extension enabled and available?";
    }

    if(!error) {
        VkWin32SurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
        createInfo.hinstance = GetModuleHandle(0);
        createInfo.hwnd = window->hWnd;
        VkResult result = vkCreateWin32SurfaceKHR(instance, &createInfo, 0, &surface);
    }

    if(error) {
        printf("Error creating vulkan surface: %s\n", error);
    }
    
    return surface;
}
#endif // GROUNDED_VULKAN_SUPPORT
#include <grounded/window/grounded_window.h>

//TODO: Temporary
//#define GROUNDED_OPENGL_SUPPORT

#if 0
#include "grounded_win32_types.h"
#else
//#include <windef.h>
//#include <winuser.h>
//#include <ntdef.h>
#include <windows.h>
#endif

#include <stdio.h>
#include <grounded/threading/grounded_threading.h>
#include <grounded/memory/grounded_memory.h>

#ifdef GROUNDED_OPENGL_SUPPORT
struct GroundedOpenGLContext {
    HGLRC hGLRC;
};
#endif

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
            eventQueue[eventQueueIndex++] = (GroundedEvent){
                .type = GROUNDED_EVENT_TYPE_BUTTON_DOWN,
                .buttonDown = GROUNDED_MOUSE_BUTTON_LEFT,
            };
        } break;
        case WM_LBUTTONUP: {
            win32MouseState.buttons[GROUNDED_MOUSE_BUTTON_LEFT] = false;
            win32MouseState.buttonUpTransitions[GROUNDED_MOUSE_BUTTON_LEFT]++;
            eventQueue[eventQueueIndex++] = (GroundedEvent){
                .type = GROUNDED_EVENT_TYPE_BUTTON_UP,
                .buttonDown = GROUNDED_MOUSE_BUTTON_LEFT,
            };
        } break;
        case WM_RBUTTONDOWN: {
            win32MouseState.buttons[GROUNDED_MOUSE_BUTTON_RIGHT] = true;
            win32MouseState.buttonDownTransitions[GROUNDED_MOUSE_BUTTON_RIGHT]++;
            eventQueue[eventQueueIndex++] = (GroundedEvent){
                .type = GROUNDED_EVENT_TYPE_BUTTON_DOWN,
                .buttonDown = GROUNDED_MOUSE_BUTTON_RIGHT,
            };
        } break;
        case WM_RBUTTONUP: {
            win32MouseState.buttons[GROUNDED_MOUSE_BUTTON_RIGHT] = false;
            win32MouseState.buttonUpTransitions[GROUNDED_MOUSE_BUTTON_RIGHT]++;
            eventQueue[eventQueueIndex++] = (GroundedEvent){
                .type = GROUNDED_EVENT_TYPE_BUTTON_UP,
                .buttonDown = GROUNDED_MOUSE_BUTTON_RIGHT,
            };
        } break;
        case WM_MBUTTONDOWN: {
            win32MouseState.buttons[GROUNDED_MOUSE_BUTTON_MIDDLE] = true;
            win32MouseState.buttonDownTransitions[GROUNDED_MOUSE_BUTTON_MIDDLE]++;
            eventQueue[eventQueueIndex++] = (GroundedEvent){
                .type = GROUNDED_EVENT_TYPE_BUTTON_DOWN,
                .buttonDown = GROUNDED_MOUSE_BUTTON_MIDDLE,
            };
        } break;
        case WM_MBUTTONUP: {
            win32MouseState.buttons[GROUNDED_MOUSE_BUTTON_MIDDLE] = false;
            win32MouseState.buttonUpTransitions[GROUNDED_MOUSE_BUTTON_MIDDLE]++;
            eventQueue[eventQueueIndex++] = (GroundedEvent){
                .type = GROUNDED_EVENT_TYPE_BUTTON_UP,
                .buttonDown = GROUNDED_MOUSE_BUTTON_MIDDLE,
            };
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
            eventQueue[eventQueueIndex++] = (GroundedEvent){
                .type = GROUNDED_EVENT_TYPE_KEY_DOWN,
                .keyDown.keycode = keycode,
                .keyDown.modifiers = 0, //TODO: Modifiers
            };
        } break;
        case WM_KEYUP: {
            u8 keycode = translateWin32Keycode(wParam);
            win32KeyboardState.keys[keycode] = false;
            win32KeyboardState.keyUpTransitions[keycode]++;
            eventQueue[eventQueueIndex++] = (GroundedEvent){
                .type = GROUNDED_EVENT_TYPE_KEY_UP,
                .keyUp.keycode = keycode,
            };
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
    // We set this to 0 so we can overwrite the cursor with SetCursor
    wnd.hCursor = 0;
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

GROUNDED_FUNCTION GroundedWindow* groundedCreateWindow(MemoryArena* arena, struct GroundedWindowCreateParameters* parameters) {
    if (!parameters) {
        static struct GroundedWindowCreateParameters defaultParameters = {0};
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
    return (GroundedWindow*)result;
}

GROUNDED_FUNCTION void groundedDestroyWindow(GroundedWindow* window) {
    ASSERT(window);
    GroundedWin32Window* win32Window = (GroundedWin32Window*)window;
    DestroyWindow(win32Window->hWnd);
}

GROUNDED_FUNCTION void groundedWindowSetTitle(GroundedWindow* window, String8 title) {
    GroundedWin32Window* win32Window = (GroundedWin32Window*)window;
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    // str16FromStr8 guarantees 0-termination
    String16 utf16Title = str16FromStr8(scratch, title);
    SetWindowTextW(win32Window->hWnd, utf16Title.base);

    arenaEndTemp(temp);
}

GROUNDED_FUNCTION GroundedEvent* groundedWindowGetEvents(u32* eventCount, u32 maxWaitingTimeInMs) {
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

GROUNDED_FUNCTION GroundedEvent* groundedWindowPollEvents(u32* eventCount) {
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

GROUNDED_FUNCTION u32 groundedWindowGetWidth(GroundedWindow* opaqueWindow) {
    GroundedWin32Window* window = (GroundedWin32Window*)opaqueWindow;
    RECT size = {0};
    GetWindowRect(window->hWnd, &size);
    return ABS(size.right - size.left);
}

GROUNDED_FUNCTION u32 groundedWindowGetHeight(GroundedWindow* opaqueWindow) {
    GroundedWin32Window* window = (GroundedWin32Window*)opaqueWindow;
    RECT size = {0};
    GetWindowRect(window->hWnd, &size);
    return ABS(size.top - size.bottom);
}

GROUNDED_FUNCTION void groundedWindowFetchMouseState(GroundedWindow* opaqueWindow, MouseState* mouseState) {
    GroundedWin32Window* window = (GroundedWin32Window*)opaqueWindow;
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

GROUNDED_FUNCTION void groundedWindowFetchKeyboardState(GroundedKeyboardState* keyState) {
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

GROUNDED_FUNCTION void groundedSetCursorType(enum GroundedMouseCursor cursorType) {
    HCURSOR hCursor = 0;
    switch (cursorType) {
        case GROUNDED_MOUSE_CURSOR_DEFAULT: {
            // Skip switch and use same as fallback
        } break;
        case GROUNDED_MOUSE_CURSOR_IBEAM: {
            hCursor = LoadCursorA(0, (LPCSTR)IDC_IBEAM);
        } break;
        case GROUNDED_MOUSE_CURSOR_LEFTRIGHT: {
            hCursor = LoadCursorA(0, (LPCSTR)IDC_SIZEWE);
        } break;
        case GROUNDED_MOUSE_CURSOR_UPDOWN: {
            hCursor = LoadCursorA(0, (LPCSTR)IDC_SIZENS);
        } break;
        case GROUNDED_MOUSE_CURSOR_UPRIGHT: {
            hCursor = LoadCursorA(0, (LPCSTR)IDC_SIZENESW);
        } break;
        case GROUNDED_MOUSE_CURSOR_UPLEFT: {
            hCursor = LoadCursorA(0, (LPCSTR)IDC_SIZENWSE);
        } break;
        case GROUNDED_MOUSE_CURSOR_DOWNRIGHT: {
            hCursor = LoadCursorA(0, (LPCSTR)IDC_SIZENWSE);
        } break;
        case GROUNDED_MOUSE_CURSOR_DOWNLEFT: {
            hCursor = LoadCursorA(0, (LPCSTR)IDC_SIZENESW);
        } break;
        case GROUNDED_MOUSE_CURSOR_POINTER: {
            hCursor = LoadCursorA(0, (LPCSTR)IDC_HAND);
        } break;
        case GROUNDED_MOUSE_CURSOR_DND_NO_DROP: {
            hCursor = LoadCursorA(0, (LPCSTR)IDC_NO);
        } break;
        case GROUNDED_MOUSE_CURSOR_DND_MOVE: {
            //TODO: No good icon
            hCursor = LoadCursorA(0, (LPCSTR)IDC_SIZEALL);
        } break;
        case GROUNDED_MOUSE_CURSOR_DND_COPY: {
            //TODO: No good icon
            hCursor = LoadCursorA(0, (LPCSTR)IDC_SIZEALL);
        } break;
        case GROUNDED_MOUSE_CURSOR_GRABBING: {
            //TODO: No good icon
            hCursor = LoadCursorA(0, (LPCSTR)IDC_HAND);
        } break;
        default: {
            // Unknown cursor type
            ASSERT(false);
        } break;
    }
    if (!hCursor) {
        hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    }

    if (hCursor) {
        SetCursor(hCursor);
    }
}

GROUNDED_FUNCTION void groundedSetCustomCursor(u8* data, u32 width, u32 height) {
    BITMAPV5HEADER bitmapInfo = {
        .bV5Size = sizeof(bitmapInfo),
        .bV5Width = width,
        .bV5Height = -(s32)height,
        .bV5Planes = 1,
        .bV5BitCount = 32,
        .bV5Compression = BI_BITFIELDS,
        .bV5RedMask = 0x00FF0000,
        .bV5GreenMask = 0x0000FF00,
        .bV5BlueMask = 0x000000FF,
        .bV5AlphaMask = 0xFF000000,
    };
    HDC dc = GetDC(NULL);
    u8* colorBitmapData = 0;
    HBITMAP color = CreateDIBSection(dc, (const BITMAPINFO*) &bitmapInfo, DIB_RGB_COLORS, (void**)&colorBitmapData, 0, 0);
    ReleaseDC(0, dc);

    HBITMAP mask = CreateBitmap(width, height, 1, 1, 0);

    for (u32 i = 0; i < width * height; ++i) {
        //TODO: Check if we need data swizzling. If not this can be simplified
        colorBitmapData[0] = data[0];
        colorBitmapData[1] = data[1];
        colorBitmapData[2] = data[2];
        colorBitmapData[3] = data[3];
        data += 4;
        colorBitmapData += 4;
    }
    
    if (color && mask) {
        ICONINFO iconInfo = {
            .fIcon = false, // False for cursor
            .xHotspot = 0,
            .yHotspot = 0,
            .hbmMask = 0,
            .hbmColor = 0,
        };
        HICON hIcon = CreateIconIndirect(&iconInfo);
        if (hIcon) {
            SetCursor(hIcon);
        }
    }
}

// *********
// DND stuff
struct GroundedWindowDragPayloadDescription {
    MemoryArena arena;
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
    //TODO: Implement
}

//TODO: Could create linked list of mime types. This would allow an API where the user can add mime types to a list. 
// Even different handling functions per mime type would be possible but is this actually desirable?
GROUNDED_FUNCTION void groundedWindowDragPayloadSetMimeTypes(GroundedWindowDragPayloadDescription* desc, u32 mimeTypeCount, String8* mimeTypes) {
    // Only allowed to set mime types once
    ASSERT(!desc->mimeTypeCount);
    ASSERT(!desc->mimeTypes);

    desc->mimeTypes = ARENA_PUSH_ARRAY_NO_CLEAR(&desc->arena, mimeTypeCount, String8);
    for (u64 i = 0; i < mimeTypeCount; ++i) {
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

    //TODO: Implement

    arenaEndTemp(temp);
}



// ************
// OpenGL stuff
#ifdef GROUNDED_OPENGL_SUPPORT

#include <gl/gl.h>
#include "wgl/wglext.h"

static PFNWGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribivARB;
static PFNWGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribfvARB;
static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
static bool wglLoaded;

//TODO: Spec says we require CS_OWNDC on windows that use opengl
static const wchar_t* openGLDummyClassName = L"Grounded OpenGL Initialization Dummy Window";

static bool loadWGL() {
    HWND dummyWindowHwnd = 0;
    HDC dummyDC = 0;
    HGLRC dummyGLRC = 0;
    HINSTANCE hInstance = GetModuleHandle(0);
    bool loaded = false;

    WNDCLASS wc = { 0 };
    wc.style = CS_OWNDC;
    wc.hInstance = hInstance;
    wc.lpszClassName = openGLDummyClassName;
    wc.lpfnWndProc = DefWindowProc;

    if (!RegisterClass(&wc)) {
        GROUNDED_LOG_ERROR("Error registering OpenGL Initialization Dummy Window class");
        return false;
    }

    // WS_CLIPCHILDREN | WS_CLIPSIBLINGS because opengl is only allowed to draw into exactly this window. Should be irrelevant for the dummy window however
    //TODO: WS_CLIPCHILDREN | WS_CLIPSIBLINGS for actual opengl windows?
    // WS_EX_APPWINDOW
    // TODO: Is CreateWindowEx faster?
    dummyWindowHwnd = CreateWindowW(wc.lpszClassName, L"", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);
    if (dummyWindowHwnd == 0) {
        GROUNDED_LOG_ERROR("Error creating OpenGL Initialization Dummy Window");
        goto exit;
    }

    dummyDC = GetDC(dummyWindowHwnd);
    if (dummyDC == 0) {
        GROUNDED_LOG_ERROR("Error creating dummy DC");
        goto exit;
    }

    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int formatIndex = ChoosePixelFormat(dummyDC, &pfd);
    if (formatIndex == 0) {
        GROUNDED_LOG_WARNING("Failed to select pixel format for dummy window. Trying first option.");
        formatIndex = 1;
    }

    if (!SetPixelFormat(dummyDC, formatIndex, &pfd)) {
        GROUNDED_LOG_ERROR("Error setting pixel format of dummy window");
        goto exit;
    }

    dummyGLRC = wglCreateContext(dummyDC);
    if (dummyGLRC == 0) {
        GROUNDED_LOG_ERROR("Error creating dummy WGL context");
        goto exit;
    }

    if (!wglMakeCurrent(dummyDC, dummyGLRC)) {
        GROUNDED_LOG_ERROR("Error making dummy context current");
        goto exit;
    }

    // Techincally the extensions should be looked for with wglGetExtensionsString.
    // As the chance that a symbol might be aliased with something different is very low the functions are retrieved directly with wglGetProcAddress
    // Load function pointers
    //TODO: What extensions do we want to hard require?
    wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");
    wglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribfvARB");
    wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if (!(wglGetPixelFormatAttribivARB && wglGetPixelFormatAttribfvARB && wglChoosePixelFormatARB && wglCreateContextAttribsARB && wglSwapIntervalEXT)) {
        GROUNDED_LOG_ERROR("Not all extension pointers could be loaded");
        wglMakeCurrent(0, 0);
        goto exit;
    }

    if (!wglMakeCurrent(0, 0)) {
        GROUNDED_LOG_ERROR("Error releasing dummy context");
        // We continue from this as it is a shutdown error and might not interfere with correct initialization
    }

    loaded = true;

exit:
    {
        if (dummyGLRC) wglDeleteContext(dummyGLRC);
        if (dummyDC) ReleaseDC(dummyWindowHwnd, dummyDC);
        if (dummyWindowHwnd) DestroyWindow(dummyWindowHwnd);
        // We let the class registered so we can create dummy windows for context pixel format selection
        //UnregisterClass(wc.lpszClassName, hInstance);
        return loaded;
    }
}

GROUNDED_FUNCTION void* groundedWindowLoadGlFunction(const char* symbol) {
    if (!wglLoaded) {
        wglLoaded = loadWGL();
    }

    void* result = wglGetProcAddress(symbol);
    return result;
}

//TODO: On Windows it seems to be the case that we have to know the pixel format upon at window creation. How to solve this nicely?
// This is also the case for linux I think. But on windows we acually require the window to set the pixel format. This is the actual problem
// HDC is required for context creation...
// Can share a single hglrc between multiple windows
GROUNDED_FUNCTION GroundedOpenGLContext* groundedCreateOpenGLContext(MemoryArena* arena, GroundedOpenGLContext* contextToShareResources) {
    GroundedOpenGLContext* result = ARENA_PUSH_STRUCT(arena, GroundedOpenGLContext);
    if (!wglLoaded) {
        wglLoaded = loadWGL();
    }

    // Workaround for context creation is to create a dummy window for each context
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(0);
    HWND dummyWindowHwnd = CreateWindowW(openGLDummyClassName, L"", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);
    if (dummyWindowHwnd == 0) {
        GROUNDED_LOG_ERROR("Error creating OpenGL Initialization Dummy Window");
    }

    HDC dummyDC = GetDC(dummyWindowHwnd);
    if (dummyDC == 0) {
        GROUNDED_LOG_ERROR("Error creating dummy DC");
    }

    // Try R8G8B8A8
    int desiredAttributes[] = {
        WGL_DRAW_TO_WINDOW_ARB, 1,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_RED_BITS_ARB, 8, // 10
        WGL_GREEN_BITS_ARB, 8, // 10
        WGL_BLUE_BITS_ARB, 8, // 10
        WGL_ALPHA_BITS_ARB, 8, // 2
        WGL_DOUBLE_BUFFER_ARB, 1,
        0,0,
    };

    int pixelFormatIndex = 0;
    UINT numFormats = 0;
    wglChoosePixelFormatARB(dummyDC, desiredAttributes, 0, 1, &pixelFormatIndex, &numFormats);
    // TODO: Describe pixel format?
    // Needs to be called before wglCreateContext to set the pixel format
    SetPixelFormat(dummyDC, pixelFormatIndex, 0);

    const int contextAttributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        //WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    HGLRC shareContext = contextToShareResources ? contextToShareResources->hGLRC : 0;
    result->hGLRC = wglCreateContextAttribsARB(dummyDC, shareContext, contextAttributes);

    // Already done at context creation
    /*if (windowContextToShareResources) {
        if (!wglShareLists(windowContextToShareResources->hGLRC, window->hGLRC)) {
            LOG_ERROR("Error requesting shared resources between two wgl contexts");
            // Hard error as the caller likely expect resource share availability if we would succeed
            return false;
        }
    }*/

    ReleaseDC(dummyWindowHwnd, dummyDC);
    //TODO: Can we maybe also destroy the window?

    return result;
}

GROUNDED_FUNCTION void groundedMakeOpenGLContextCurrent(GroundedWindow* opaqueWindow, GroundedOpenGLContext* context) {
    GroundedWin32Window* window = (GroundedWin32Window*)opaqueWindow;
    assert(context->hGLRC);

    HDC hDC = GetDC(window->hWnd);

    { // TODO: Must only be done once
        // Try R8G8B8A8
        int desiredAttributes[] = {
            WGL_DRAW_TO_WINDOW_ARB, 1,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_RED_BITS_ARB, 8, // 10
            WGL_GREEN_BITS_ARB, 8, // 10
            WGL_BLUE_BITS_ARB, 8, // 10
            WGL_ALPHA_BITS_ARB, 8, // 2
            WGL_DOUBLE_BUFFER_ARB, 1,
            0,0,
        };

        int pixelFormatIndex = 0;
        UINT numFormats = 0;
        wglChoosePixelFormatARB(hDC, desiredAttributes, 0, 1, &pixelFormatIndex, &numFormats);
        SetPixelFormat(hDC, pixelFormatIndex, 0);
    }

    wglMakeCurrent(hDC, context->hGLRC);
    ReleaseDC(window->hWnd, hDC);
}

GROUNDED_FUNCTION void groundedWindowGlSwapBuffers(GroundedWindow* w) {
    GroundedWin32Window* window = (GroundedWin32Window*)w;
    HDC hDC = GetDC(window->hWnd);
    SwapBuffers(hDC);
    ReleaseDC(window->hWnd, hDC);
}

GROUNDED_FUNCTION void groundedWindowSetGlSwapInterval(int interval) {
    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(interval);
    }
}

#endif // GROUNDED_OPENGL_SUPPORT


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
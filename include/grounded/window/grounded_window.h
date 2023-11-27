#ifndef GROUNDED_WINDOW_H
#define GROUNDED_WINDOW_H

#include "../grounded.h"
#include "../memory/grounded_arena.h"
#include "../string/grounded_string.h"

typedef struct GroundedWindow GroundedWindow;

void groundedInitWindowSystem();
void groundedShutdownWindowSystem();

typedef enum GroundedEventType {
    GROUNDED_EVENT_TYPE_NONE,
    GROUNDED_EVENT_TYPE_CLOSE_REQUEST,
	GROUNDED_EVENT_TYPE_RESIZE,
    GROUNDED_EVENT_TYPE_KEY_DOWN,
	GROUNDED_EVENT_TYPE_KEY_UP,
	GROUNDED_EVENT_TYPE_BUTTON_DOWN,
	GROUNDED_EVENT_TYPE_BUTTON_UP,
	GROUNDED_EVENT_TYPE_DISPLAY,
    GROUNDED_EVENT_TYPE_COUNT,
} GroundedEventType;
typedef struct GroundedEvent {
    GroundedEventType type;
    union {
        struct {
            // The window which recieved the close request
            GroundedWindow* window;
        } closeRequest;
		struct {
			u32 width;
			u32 height;
			GroundedWindow* window;
		} resize;
		struct {
			u32 keycode;
			u32 modifiers;
		} keyDown;
		struct {
			u32 keycode;
		} keyUp;
		struct {
			u32 button;
			GroundedWindow* window;
			s32 mousePositionX;
			s32 mousePositionY;
		} buttonDown;
		struct {
			u32 button;
			GroundedWindow* window;
			s32 mousePositionX;
			s32 mousePositionY;
		} buttonUp;
		struct {
			struct GroundedWindowDisplay* display;
			bool connected; // true on connect, false on disconnect
		} display;
    };
} GroundedEvent;

typedef struct GroundedKeyboardState {
	u32 modifiers;
	bool keys[256];
	bool lastKeys[256];
	u16 keyDownTransitions[256];
	u16 keyUpTransitions[256];
} GroundedKeyboardState;

typedef struct MouseState {
    s32 x, y;
    s32 lastX, lastY;
    s32 deltaX, deltaY;
    
	// Positive means scrolling "in" or "up" and negative means scrolling "out" or "down"
	// A value of 1.0f usually means one scroll unit. Value can be smaller and larger though
	float scrollDelta;
    
	// Positive means right and negative means left
	// A value of 1.0f usually means one scroll unit. Value can be smaller and larger though
	float horizontalScrollDelta;
    
	// We support up to 32 mouse buttons
	bool buttons[32]; //TODO: Maybe switch to a bitfield? 32 bit would be enough
	bool lastButtons[32];
    bool mouseInWindow;

	u16 buttonDownTransitions[32];
	u16 buttonUpTransitions[32];
} MouseState;

// Monitor, Display, Screen
typedef struct GroundedWindowDisplay {
	s32 virtualX, virtualY;
	u32 width, height;
	String8 name;
	String8 manufacturer;
	bool primary;
} GroundedWindowDisplay;

typedef struct GroundedWindowDragPayloadDescription GroundedWindowDragPayloadDescription;

typedef enum GroundedMouseCursor {
	GROUNDED_MOUSE_CURSOR_ARROW = 0,
	GROUNDED_MOUSE_CURSOR_DEFAULT = GROUNDED_MOUSE_CURSOR_ARROW,
    GROUNDED_MOUSE_CURSOR_IBEAM, // The default text edit cursor
    GROUNDED_MOUSE_CURSOR_LEFTRIGHT,
    GROUNDED_MOUSE_CURSOR_UPDOWN,
	GROUNDED_MOUSE_CURSOR_UPRIGHT,
	GROUNDED_MOUSE_CURSOR_UPLEFT,
	GROUNDED_MOUSE_CURSOR_DOWNRIGHT,
	GROUNDED_MOUSE_CURSOR_DOWNLEFT,
	GROUNDED_MOUSE_CURSOR_POINTER, // Usually used when mouse hovers a clickable element
	GROUNDED_MOUSE_CURSOR_DND_NO_DROP,
	GROUNDED_MOUSE_CURSOR_DND_MOVE,
	GROUNDED_MOUSE_CURSOR_DND_COPY,
	//GROUNDED_MOUSE_CURSOR_DND_LINK,
	GROUNDED_MOUSE_CURSOR_GRABBING,
	GROUNDED_MOUSE_CURSOR_CUSTOM,
    GROUNDED_MOUSE_CURSOR_COUNT
} GroundedMouseCursor;

typedef enum GroundedWindowCustomTitlebarHit {
	GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_NONE,
	GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_BAR, // Click on this will initiate move
	GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_MINIMIZE,
	GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_MAXIMIZE,
	GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_CLOSE,
	GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_BORDER, // Click on this will initiate move
	GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_HELP, // Click here will open some kind of help dialog by the application
	GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_COUNT,
} GroundedWindowCustomTitlebarHit;
#define GROUNDED_WINDOW_CUSTOM_TITLEBAR_CALLBACK(name) GroundedWindowCustomTitlebarHit name(GroundedWindow* window, u32 x, u32 y)
typedef GROUNDED_WINDOW_CUSTOM_TITLEBAR_CALLBACK(GroundedWindowCustomTitlebarCallback);

//#define GROUNDED_WINDOW_MIME_TYPE_CALLBACK(name) bool name(String8 mimeType)
//typedef GROUNDED_WINDOW_MIME_TYPE_CALLBACK(GroundedWindowMimeTypeCallback);

struct GroundedDragPayload;

#define GROUNDED_WINDOW_DND_DROP_CALLBACK(name) void name(struct GroundedDragPayload* payload, String8 data, GroundedWindow* window, s32 x, s32 y, String8 mimeType)
typedef GROUNDED_WINDOW_DND_DROP_CALLBACK(GroundedWindowDndDropCallback);
#define GROUNDED_WINDOW_DND_CALLBACK(name) u32 name(struct GroundedDragPayload* payload, GroundedWindow* window, s32 x, s32 y, u32 mimeTypeCount, String8* mimeTypes, GroundedWindowDndDropCallback** onDropCallback)
typedef GROUNDED_WINDOW_DND_CALLBACK(GroundedWindowDndCallback);
#define GROUNDED_WINDOW_DND_SEND_CALLBACK(name) String8 name(MemoryArena* arena, String8 mimeType, u64 mimeIndex, void* userData)
typedef GROUNDED_WINDOW_DND_SEND_CALLBACK(GroundedWindowDndSendCallback);
#define GROUNDED_WINDOW_DND_CANCEL_CALLBACK(name) void name(MemoryArena* arena, void* userData)
typedef GROUNDED_WINDOW_DND_CANCEL_CALLBACK(GroundedWindowDndCancelCallback);

struct GroundedWindowCreateParameters {
	String8 title;
	String8 applicationId; // Should be the name of your application in lowercase. In contrast to the title this is not expected to change
	u32 width; // 0 for a platform-specific default size
	u32 height; // 0 for a platform-specific default size
	u32 minWidth;
	u32 minHeight;
	u32 maxWidth;
	u32 maxHeight;
	bool borderless;
	//bool fullscreen;
	//bool hidden;
	//bool transparent;
	void* userData;
	bool inhibitIdle;
	GroundedWindowCustomTitlebarCallback* customTitlebarCallback; //TODO: We should let clients with custom title bars know if they should show a titlebar or not
	GroundedWindowDndCallback* dndCallback; // Application should return the index of the mime type it wants to accept. UINT32_MAX for none?
	GroundedWindowDisplay* displayToStartOn; // 0 means no preference on which display to start on
};

GROUNDED_FUNCTION GroundedWindowDisplay* groundedGetPrimaryDisplay();
GROUNDED_FUNCTION GroundedWindowDisplay* groundedGetDisplays(MemoryArena* arena, u32* displayCount); // Result array is allocated on submitted arena

GROUNDED_FUNCTION GroundedWindow* groundedCreateWindow(MemoryArena* arena, struct GroundedWindowCreateParameters* parameters);
GROUNDED_FUNCTION void groundedDestroyWindow(GroundedWindow* window);

GROUNDED_FUNCTION u32 groundedWindowGetWidth(GroundedWindow* window);
GROUNDED_FUNCTION u32 groundedWindowGetHeight(GroundedWindow* window);

GROUNDED_FUNCTION void groundedWindowSetSize(GroundedWindow* window, u32 width, u32 height);
GROUNDED_FUNCTION void groundedWindowSetTitle(GroundedWindow* window, String8 title);
GROUNDED_FUNCTION void groundedWindowSetFullscreen(GroundedWindow* window, bool fullscreen);
GROUNDED_FUNCTION void groundedWindowSetBorderless(GroundedWindow* window, bool borderless);
GROUNDED_FUNCTION void groundedWindowSetHidden(GroundedWindow* window, bool hidden);

GROUNDED_FUNCTION void groundedWindowSetUserData(GroundedWindow* window, void* userData);
GROUNDED_FUNCTION void* groundedWindowGetUserData(GroundedWindow* window);

GROUNDED_FUNCTION void groundedWindowSetIcon(u8* data, u32 width, u32 height);

GROUNDED_FUNCTION void groundedSetCursorGrab(GroundedWindow* window, bool grab);
GROUNDED_FUNCTION void groundedSetCursorVisibility(bool visible);
GROUNDED_FUNCTION void groundedSetCursorType(enum GroundedMouseCursor cursorType);
// Maybe make custom cursors loadable and applyable. Then the cursors do not have to be recreated when application wants to switch cursors
GROUNDED_FUNCTION void groundedSetCustomCursor(u8* data, u32 width, u32 height);

GROUNDED_FUNCTION void groundedWindowConstrainPointer(GroundedWindow* window, bool constrain);

GROUNDED_FUNCTION GroundedWindowDragPayloadDescription* groundedWindowPrepareDragPayload(GroundedWindow* window);
GROUNDED_FUNCTION MemoryArena* groundedWindowDragPayloadGetArena(GroundedWindowDragPayloadDescription* desc);
GROUNDED_FUNCTION void groundedWindowDragPayloadSetImage(GroundedWindowDragPayloadDescription* desc, u8* data, u32 width, u32 height);
GROUNDED_FUNCTION void groundedWindowDragPayloadSetMimeTypes(GroundedWindowDragPayloadDescription* desc, u32 mimeTypeCount, String8* mimeTypes);
GROUNDED_FUNCTION void groundedWindowDragPayloadSetSendCallback(GroundedWindowDragPayloadDescription* desc, GroundedWindowDndSendCallback* callback);
GROUNDED_FUNCTION void groundedWindowDragPayloadSetCancelCallback(GroundedWindowDragPayloadDescription* desc, GroundedWindowDndCancelCallback* callback);
GROUNDED_FUNCTION void groundedWindowBeginDragAndDrop(GroundedWindowDragPayloadDescription* desc, void* userData);

//GROUNDED_FUNCTION void groundedStartDragAndDrop(GroundedWindow* window, u64 mimeTypeCount, String8* mimeTypes, GroundedWindowDndSendCallback* callback, GroundedWindowDragPayloadImage* image, void* userData);
//GROUNDED_FUNCTION void groundedStartDragAndDropWithSingleDataType(GroundedWindow* window, String8 mimeType, u8* data, u64 size, GroundedWindowDragPayloadImage* image); // Data can be freed after this call
//GROUNDED_FUNCTION String8 groundedGetDragAndDropDataAsMimeType(struct GroundedDragPayload* payload, String8 mimeType);
//GROUNDED_FUNCTION MemoryArena* groundedGetPayloadMemoryArena(struct GroundedDragPayload* payload);
//GROUNDED_FUNCTION GroundedWindowDragPayloadImage* groundedWindowCreateDragImage(MemoryArena* arena, u8* data, u32 width, u32 height);
//TODO: Additional helpers for quick retrieval of text, single file and multiple files
//TODO: Maybe create structs for DragPayload and DropPayload to differentiate between them

// Retuned array must not be used anymore once get or poll events is called again
GROUNDED_FUNCTION GroundedEvent* groundedWindowGetEvents(u32* eventCount, u32 maxWaitingTimeInMs);
GROUNDED_FUNCTION GroundedEvent* groundedWindowPollEvents(u32* eventCount);

// Get time in nanoseconds
GROUNDED_FUNCTION u64 groundedGetCounter();

GROUNDED_FUNCTION void groundedWindowFetchKeyboardState(GroundedKeyboardState* keyboardState);

// Mouse state relative to given window
GROUNDED_FUNCTION void groundedWindowFetchMouseState(GroundedWindow* window, MouseState* mouseState);

// Alias
#define GROUNDED_KEY_ENTER 				GROUNDED_KEY_RETURN
#define GROUNDED_KEY_PERIOD 			GROUNDED_KEY_DOT
#define GROUNDED_KEY_HYPHEN				GROUNDED_KEY_MINUS
#define GROUNDED_KEY_MODIFIER_WINDOWS  	GROUNDED_KEY_MODIFIER_META
#define GROUNDED_KEY_MODIFIER_COMMAND  	GROUNDED_KEY_MODIFIER_META
#define GROUNDED_KEY_MODIFIER_SUPER    	GROUNDED_KEY_MODIFIER_META

// Key modifiers are a bitfield
#define GROUNDED_KEY_MODIFIER_NONE     0x00
#define GROUNDED_KEY_MODIFIER_CONTROL  0x01
#define GROUNDED_KEY_MODIFIER_SHIFT    0x02
#define GROUNDED_KEY_MODIFIER_ALT      0x04
#define GROUNDED_KEY_MODIFIER_META     0x08
#define GROUNDED_KEY_MODIFIER_ALTGR    0x10
#define GROUNDED_KEY_MODIFIER_FN       0x20

#define GROUNDED_KEY_UNKNOWN 0

// As we have a keystate array the keycodes should never exceed 255
#define GROUNDED_KEY_BACKSPACE  8 // 0x08      ASCII
#define GROUNDED_KEY_TAB        9 // 0x09      ASCII
#define GROUNDED_KEY_RETURN    13 // 0x0D      ASCII Alias with Enter
#define GROUNDED_KEY_LSHIFT    16
#define GROUNDED_KEY_RSHIFT    17
#define GROUNDED_KEY_ESCAPE    27 // 0x1B      ASCII
#define GROUNDED_KEY_SPACE     32 // 0x20      ASCII


//TODO: Temporary for now. Choose final values
#define GROUNDED_KEY_LEFT		1
#define GROUNDED_KEY_RIGHT		2
#define GROUNDED_KEY_UP			3
#define GROUNDED_KEY_DOWN		4
#define GROUNDED_KEY_PAGE_UP	5
#define GROUNDED_KEY_PAGE_DOWN	6
#define GROUNDED_KEY_HOME		18
#define GROUNDED_KEY_END		19
#define GROUNDED_KEY_INSERT		20
#define GROUNDED_KEY_DELETE		21

// ASCII special characters
#define GROUNDED_KEY_PLUS		43 // 0x2B
#define GROUNDED_KEY_COMMA		44 // 0x2C
#define GROUNDED_KEY_MINUS		45 // 0x2D // Alias with Hyphen
#define GROUNDED_KEY_DOT		46 // 0x2E // Alias with Period
#define GROUNDED_KEY_SLASH     	47 // 0x2F 

// ASCII numbers
#define GROUNDED_KEY_0 48 // 0x30
#define GROUNDED_KEY_1 49 // 0x31
#define GROUNDED_KEY_2 50 // 0x32
#define GROUNDED_KEY_3 51 // 0x33
#define GROUNDED_KEY_4 52 // 0x34
#define GROUNDED_KEY_5 53 // 0x35
#define GROUNDED_KEY_6 54 // 0x36
#define GROUNDED_KEY_7 55 // 0x37
#define GROUNDED_KEY_8 56 // 0x38
#define GROUNDED_KEY_9 57 // 0x39

// ASCII characters
#define GROUNDED_KEY_A 65 // 0x41
#define GROUNDED_KEY_B 66 // 0x42
#define GROUNDED_KEY_C 67 // 0x43
#define GROUNDED_KEY_D 68 // 0x44
#define GROUNDED_KEY_E 69 // 0x45
#define GROUNDED_KEY_F 70 // 0x46
#define GROUNDED_KEY_G 71 // 0x47
#define GROUNDED_KEY_H 72 // 0x48
#define GROUNDED_KEY_I 73 // 0x49
#define GROUNDED_KEY_J 74 // 0x4A
#define GROUNDED_KEY_K 75 // 0x4B
#define GROUNDED_KEY_L 76 // 0x4C
#define GROUNDED_KEY_M 77 // 0x4D
#define GROUNDED_KEY_N 78 // 0x4E
#define GROUNDED_KEY_O 79 // 0x4F
#define GROUNDED_KEY_P 80 // 0x50
#define GROUNDED_KEY_Q 81 // 0x51
#define GROUNDED_KEY_R 82 // 0x52
#define GROUNDED_KEY_S 83 // 0x53
#define GROUNDED_KEY_T 84 // 0x54
#define GROUNDED_KEY_U 85 // 0x55
#define GROUNDED_KEY_V 86 // 0x56
#define GROUNDED_KEY_W 87 // 0x57
#define GROUNDED_KEY_X 88 // 0x58
#define GROUNDED_KEY_Y 89 // 0x59
#define GROUNDED_KEY_Z 90 // 0x5A

#define GROUNDED_KEY_F1 112 // 0x70
#define GROUNDED_KEY_F2 113 // 0x71
#define GROUNDED_KEY_F3 114 // 0x72
#define GROUNDED_KEY_F4 115 // 0x73
#define GROUNDED_KEY_F5 116 // 0x74
#define GROUNDED_KEY_F6 117 // 0x75
#define GROUNDED_KEY_F7 118 // 0x76
#define GROUNDED_KEY_F8 119 // 0x77
#define GROUNDED_KEY_F9 120 // 0x78
#define GROUNDED_KEY_F10 121 // 0x79
#define GROUNDED_KEY_F11 122 // 0x7A
#define GROUNDED_KEY_F12 123 // 0x7B
#define GROUNDED_KEY_F13 124 // 0x7C
#define GROUNDED_KEY_F14 125 // 0x7D
#define GROUNDED_KEY_F15 126 // 0x7E
#define GROUNDED_KEY_F16 127 // 0x7F
// Windows and Wayland actually support up to F24...

#define GROUNDED_MOUSE_BUTTON_LEFT     1
#define GROUNDED_MOUSE_BUTTON_MIDDLE   2
#define GROUNDED_MOUSE_BUTTON_RIGHT    3


GROUNDED_FUNCTION_INLINE u32 groundedGetUnicodeCodepointForKeycode(u8 keycode) {
	if((keycode >= GROUNDED_KEY_A && keycode <= GROUNDED_KEY_Z) || (keycode >= GROUNDED_KEY_PLUS && keycode <= GROUNDED_KEY_9)) {
		return keycode;
	}
	return 0;
}


// ********************
// OpenGL related stuff
#ifdef GROUNDED_OPENGL_SUPPORT
typedef struct GroundedOpenGLContext GroundedOpenGLContext;
GROUNDED_FUNCTION GroundedOpenGLContext* groundedCreateOpenGLContext(MemoryArena* arena, GroundedOpenGLContext* contextToShareResources);
GROUNDED_FUNCTION void groundedMakeOpenGLContextCurrent(GroundedWindow* window, GroundedOpenGLContext* context);
GROUNDED_FUNCTION void groundedWindowGlSwapBuffers(GroundedWindow* window);
GROUNDED_FUNCTION void groundedWindowSetGlSwapInterval(int interval);
GROUNDED_FUNCTION void* groundedWindowLoadGlFunction(const char* symbol);
//GROUNDED_FUNCTION GroundedWindowDragPayloadImage* groundedCreateDragImageOpenGL(int texture);
#endif // GROUNDED_VULKAN_SUPPORT

// ********************
// Vulkan related stuff
#ifdef GROUNDED_VULKAN_SUPPORT
#include <volk/volk.h>
GROUNDED_FUNCTION const char** groundedWindowGetVulkanInstanceExtensions(u32* count);
GROUNDED_FUNCTION VkSurfaceKHR groundedWindowGetVulkanSurface(GroundedWindow* window, VkInstance instance);
#endif // GROUNDED_VULKAN_SUPPORT


#endif //GROUNDED_WINDOW_H

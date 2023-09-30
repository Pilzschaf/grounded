#include <grounded/window/grounded_window.h>
#include <grounded/logger/grounded_logger.h>
#include <grounded/threading/grounded_threading.h>

#include <dlfcn.h>
#include <stdio.h> // Required for printf
#include <poll.h>
#include <stdlib.h> // For free (required for releasing memory from xcb)
#include <sys/shm.h>

#ifdef GROUNDED_OPENGL_SUPPORT
//#include <EGL/egl.h>
#endif

// Just some defines for x cursors
#include <X11/cursorfont.h>
//#include <xcb/xcb_renderutil.h>
//#include <xcb/xcb_icccm.h>
//#include <X11/Xcursor/Xcursor.h>
//#include <xcb/render.h>

/*
 * Unchecked xcb calls push errors into the event loop. Checked xcb calls allow to check the error in a blocking fashion
 */

//#include <xcb/shm.h>
//#include <xcb/xcb_image.h>

#include "types/grounded_xcb_types.h"

//#include <xcb/xcb_cursor.h>
//#include <xcb/xcb.h>

typedef struct GroundedXcbWindow {
    xcb_window_t window;
    u32 width;
    u32 height;
    void* userData;
    GroundedWindowCustomTitlebarCallback* customTitlebarCallback;

#ifdef GROUNDED_OPENGL_SUPPORT
    EGLSurface eglSurface;
#endif
} GroundedXcbWindow;

// xcb function types
#define X(N, R, P) typedef R grounded_xcb_##N P;
#include "types/grounded_xcb_functions.h"
#undef X

// xcb function pointers
#define X(N, R, P) static grounded_xcb_##N * N = 0;
#include "types/grounded_xcb_functions.h"
#undef X

// xcb cursor function types
#define X(N, R, P) typedef R grounded_xcb_##N P;
#include "types/grounded_xcb_cursor_functions.h"
#undef X

// xcb cursor function pointers
#define X(N, R, P) static grounded_xcb_##N * N = 0;
#include "types/grounded_xcb_cursor_functions.h"
#undef X

// xcb shm function types
#define X(N, R, P) typedef R grounded_xcb_##N P;
#include "types/grounded_xcb_shm_functions.h"
#undef X

// xcb shm function pointers
#define X(N, R, P) static grounded_xcb_##N * N = 0;
#include "types/grounded_xcb_shm_functions.h"
#undef X

// xcb image function types
#define X(N, R, P) typedef R grounded_xcb_##N P;
#include "types/grounded_xcb_image_functions.h"
#undef X

// xcb image function pointers
#define X(N, R, P) static grounded_xcb_##N * N = 0;
#include "types/grounded_xcb_image_functions.h"
#undef X

// xcb render function types
#define X(N, R, P) typedef R grounded_xcb_##N P;
#include "types/grounded_xcb_render_functions.h"
#undef X

// xcb render function pointers
#define X(N, R, P) static grounded_xcb_##N * N = 0;
#include "types/grounded_xcb_render_functions.h"
#undef X


xcb_connection_t* xcbConnection;
xcb_screen_t* xcbScreen;
xcb_intern_atom_reply_t* xcbDeleteAtom;
xcb_intern_atom_reply_t* xcbProtocolsAtom;
xcb_depth_t* xcbDepth;

xcb_font_t xcbCursorFont; // Could be used as a fallback when xcb_cursor is not available
xcb_cursor_t xcbDefaultCursor; // Not really used right now and probably also does not contain correct default cursor
xcb_cursor_context_t* xcbCursorContext;
xcb_cursor_t xcbCurrentCursor;
GroundedMouseCursor currentCursorType = GROUNDED_MOUSE_CURSOR_DEFAULT;
xcb_render_pictforminfo_t* rgbaFormat;

GroundedKeyboardState xcbKeyboardState;
MouseState xcbMouseState;
bool xcbShmAvailable;

GROUNDED_FUNCTION void xcbSetCursorType(enum GroundedMouseCursor cursorType);
static void xcbSetCursor(GroundedXcbWindow* window, xcb_cursor_t cursor);

static void reportXcbError(const char* message) {
    printf("Error: %s\n", message);
}

static void initXcb() {
    const char* error = 0;

    // Load library
    void* xcbLibrary = dlopen("libxcb.so", RTLD_LAZY | RTLD_LOCAL);
    if(!xcbLibrary) {
        error = "Could no find xcb library";
    }
    
    if(!error) { // Load function pointers
        const char* firstMissingFunctionName = 0;
        #define X(N, R, P) N = (grounded_xcb_##N*)dlsym(xcbLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
        #include "types/grounded_xcb_functions.h"
        #undef X
        if(firstMissingFunctionName) {
            printf("Could not load xcb function: %s\n", firstMissingFunctionName);
            error = "Could not load all xcb functions. Your xcb version is incompatible";
        }
    }

    if(!error) { // Open xcb connection
        xcbConnection = xcb_connect(0, 0);
        if(!xcbConnection) {
            error = "Could not connect to X Window server";
        }
    }

    if(!error) {
        const xcb_setup_t* xcbSetup = xcb_get_setup(xcbConnection);
        if(xcbSetup) {
            xcb_screen_iterator_t xcbScreenIterator = xcb_setup_roots_iterator(xcbSetup);
            // First screen should be primary screen
            xcbScreen = xcbScreenIterator.data;
            if(!xcbScreen) {
                error = "Could not find any xcb screens";
            }
        } else {
            error = "Could not retrieve xcb setup";
        }
    }

    if(!error) {
        void* xcbCursorLibrary = dlopen("libxcb-cursor.so", RTLD_LAZY | RTLD_LOCAL);
        if(xcbCursorLibrary) {
            const char* firstMissingFunctionName = 0;
            #define X(N, R, P) N = (grounded_xcb_##N*)dlsym(xcbCursorLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "types/grounded_xcb_cursor_functions.h"
            #undef X
            if(firstMissingFunctionName) {
                printf("Could not load xcb cursor function: %s\n", firstMissingFunctionName);
                GROUNDED_LOG_WARNING("Could not load all xcb cursor functions. Cursor support might be limited");
            } else {
                xcb_cursor_context_new(xcbConnection, xcbScreen, &xcbCursorContext);
            }
        }
    }

    if(!error) {
        void* xcbShmLibrary = dlopen("libxcb-shm.so", RTLD_LAZY | RTLD_LOCAL);
        if(xcbShmLibrary) {
            const char* firstMissingFunctionName = 0;
            #define X(N, R, P) N = (grounded_xcb_##N*)dlsym(xcbShmLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "types/grounded_xcb_shm_functions.h"
            #undef X
            if(firstMissingFunctionName) {
                printf("Could not load xcb shm function: %s\n", firstMissingFunctionName);
                GROUNDED_LOG_WARNING("Could not load all xcb shm functions");
            } else {
                xcb_shm_query_version_reply_t* reply;
                reply = xcb_shm_query_version_reply(xcbConnection, xcb_shm_query_version(xcbConnection), 0);
                if(!reply) {
                    GROUNDED_LOG_WARNING("No xcb shm available");
                } else {
                    xcbShmAvailable = true;
                }
            }
        }
    }

    if(!error) {
        void* xcbImageLibrary = dlopen("libxcb-image.so", RTLD_LAZY | RTLD_LOCAL);
        if(xcbImageLibrary) {
            const char* firstMissingFunctionName = 0;
            #define X(N, R, P) N = (grounded_xcb_##N*)dlsym(xcbImageLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "types/grounded_xcb_image_functions.h"
            #undef X
            if(firstMissingFunctionName) {
                printf("Could not load xcb image function: %s\n", firstMissingFunctionName);
                GROUNDED_LOG_WARNING("Could not load all xcb image functions");
            }
        }
    }

    if(!error) {
        void* xcbRenderLibrary = dlopen("libxcb-render.so", RTLD_LAZY | RTLD_LOCAL);
        if(xcbRenderLibrary) {
            const char* firstMissingFunctionName = 0;
            #define X(N, R, P) N = (grounded_xcb_##N*)dlsym(xcbRenderLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "types/grounded_xcb_render_functions.h"
            #undef X
            if(firstMissingFunctionName) {
                printf("Could not load xcb render function: %s\n", firstMissingFunctionName);
                GROUNDED_LOG_WARNING("Could not load all xcb render functions");
            } else {
                xcb_render_query_pict_formats_cookie_t formats_cookie = xcb_render_query_pict_formats(xcbConnection);
                xcb_render_query_pict_formats_reply_t* formatsReply = xcb_render_query_pict_formats_reply(xcbConnection,
							    formats_cookie, 0);
                ASSERT(formatsReply);
                xcb_render_pictforminfo_t* formats = xcb_render_query_pict_formats_formats(formatsReply);
                for (u32 i = 0; i < formatsReply->num_formats; i++) {

                    if (formats[i].type == XCB_RENDER_PICT_TYPE_DIRECT &&
                        formats[i].depth == 32 &&
                        formats[i].direct.alpha_mask == 0xff &&
                        formats[i].direct.alpha_shift == 24) {
                        rgbaFormat = &formats[i];
                    }
                }
            }
        }
    }

    if(!error) {
        xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(xcbScreen);
        while (depth_iter.rem) {
            if (depth_iter.data->depth == 32 && depth_iter.data->visuals_len) {
                xcbDepth = depth_iter.data;
                break;
            }
            xcb_depth_next(&depth_iter);
        }
        if (!xcbDepth) {
            error = "ERROR: screen does not support 32 bit color depth";
            xcb_disconnect(xcbConnection);
        }
    }

    if(!error) { // Register delete notify atom
        xcb_intern_atom_cookie_t  protocols_cookie = xcb_intern_atom( xcbConnection, 1, 12, "WM_PROTOCOLS" );
        xcb_intern_atom_cookie_t  delete_cookie    = xcb_intern_atom( xcbConnection, 0, 16, "WM_DELETE_WINDOW" );
        xcbProtocolsAtom  = xcb_intern_atom_reply( xcbConnection, protocols_cookie, 0 );
        xcbDeleteAtom = xcb_intern_atom_reply( xcbConnection, delete_cookie, 0 );
        xcbCursorFont = xcb_generate_id (xcbConnection);
        String8 fontName = STR8_LITERAL("cursor");
        xcb_open_font(xcbConnection, xcbCursorFont, fontName.size, (const char*)fontName.base);
        xcbDefaultCursor = xcb_generate_id(xcbConnection);
        xcb_create_glyph_cursor(xcbConnection, xcbDefaultCursor, xcbCursorFont, XC_X_cursor, XC_X_cursor + 1, 0, 0, 0, 0, 0, 0, 0);
    }

    if(error) {
        if(xcbDeleteAtom) {
            free(xcbDeleteAtom);
            xcbDeleteAtom = 0;
        }
        if(xcbProtocolsAtom) {
            free(xcbProtocolsAtom);
            xcbProtocolsAtom = 0;
        }
        if(xcbConnection) {
            xcb_disconnect(xcbConnection);
            xcbConnection = 0;
        }
        reportXcbError(error);
    }

    return;
}

static void shutdownXcb() {
    if(xcbCurrentCursor) {
        xcb_free_cursor(xcbConnection, xcbCurrentCursor);
        xcbCurrentCursor = 0;
    }
    if(xcbCursorContext) {
        xcb_cursor_context_free(xcbCursorContext);
        xcbCursorContext = 0;
    }
    if(xcbCursorFont) {
        xcb_close_font(xcbConnection, xcbCursorFont);
        xcbCursorFont = 0;
    }
    if(xcbDefaultCursor) {
        xcb_free_cursor(xcbConnection, xcbDefaultCursor);
    }
    if(xcbDeleteAtom) {
        free(xcbDeleteAtom);
        xcbDeleteAtom = 0;
    }
    if(xcbProtocolsAtom) {
        free(xcbProtocolsAtom);
        xcbProtocolsAtom = 0;
    }
    if(xcbConnection) {
        xcb_disconnect(xcbConnection);
        xcbConnection = 0;
    }
    //TODO: Should techincally dlclose(xcb)
}

static void xcbSetWindowTitle(GroundedXcbWindow* window, String8 title, bool flush) {
    u64 len = title.size;
    u8 format = 8; // Single element size in bits. 8 bits in this case. Used for endian swap
    // Actual title
    xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, window->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, format, len, title.base);
    // Title if iconified
    xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, window->window, XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, format, len, title.base);

    if(flush) {
        xcb_flush(xcbConnection);
    }
}

static inline xcb_intern_atom_reply_t* intern_helper(xcb_connection_t *conn, bool only_if_exists, const char *str) {
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, only_if_exists, strlen(str), str);
    return xcb_intern_atom_reply(conn, cookie, NULL);
}

//TODO: Doesn't work if window has already been mapped
static void xcbWindowSetFullscreen(GroundedXcbWindow* window, bool fullscreen) {
    xcb_intern_atom_reply_t *atom_wm_state = intern_helper(xcbConnection, false, "_NET_WM_STATE");
    xcb_intern_atom_reply_t *atom_wm_fullscreen = intern_helper(xcbConnection, false, "_NET_WM_STATE_FULLSCREEN");
    xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, window->window, atom_wm_state->atom, XCB_ATOM_ATOM, 32, fullscreen, &(atom_wm_fullscreen->atom));               
    free(atom_wm_fullscreen);
    free(atom_wm_state);
}

static void xcbWindowSetBorderless(GroundedXcbWindow* window, bool borderless) {
    xcb_intern_atom_cookie_t setWindowDecorationsCookie = xcb_intern_atom(xcbConnection, 0, strlen("_MOTIF_WM_HINTS"), "_MOTIF_WM_HINTS");    
    
    // motif hints
    struct MotifHints{
        uint32_t   flags;
        uint32_t   functions;
        uint32_t   decorations;
        int32_t    input_mode;
        uint32_t   status;
    };
    
    struct MotifHints hints;
    hints.flags = 2;
    hints.functions = 0;
    hints.decorations = (u32)!borderless;
    hints.input_mode = 0;
    hints.status = 0;
    
    xcb_intern_atom_reply_t *setWindowDecorationsReply = xcb_intern_atom_reply (xcbConnection, setWindowDecorationsCookie, 0);
    
    xcb_change_property (xcbConnection,
                         XCB_PROP_MODE_REPLACE,
                         window->window,
                         setWindowDecorationsReply->atom,
                         setWindowDecorationsReply->atom,
                         32,  // format of property
                         5,   // length of data (5x32 bit) , followed by pointer to data
                         &hints ); // is this is a motif hints struct
    
    free(setWindowDecorationsReply);
}

static void xcbWindowSetHidden(GroundedXcbWindow* window, bool hidden) {    
	// ShowWindowAsync version available which does not wait for completion
    xcb_void_cookie_t cookie;
	if (hidden) {
		cookie = xcb_unmap_window_checked (xcbConnection, window->window);
	} else {
		cookie = xcb_map_window_checked (xcbConnection, window->window);
	}
    // Wait for reply by server
    // We should actually wait for an expose event to make sure the window is actually mapped.
    xcb_generic_error_t* error = xcb_request_check(xcbConnection, cookie);
    //xcb_flush(xcbConnection);
}

static void xcbWindowSetUserData(GroundedXcbWindow* window, void* userData) {
    window->userData = userData;
}

static void* xcbWindowGetUserData(GroundedXcbWindow* window) {
    return window->userData;
}

static void xcbSetCursorGrab(GroundedXcbWindow* window, bool grab) {
    if(grab) {
        xcb_generic_error_t* error = 0;
        const u8 ownerEvents = 0;
        xcb_grab_pointer_cookie_t cookie = xcb_grab_pointer(xcbConnection, ownerEvents, window->window, 0, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, window->window, XCB_NONE, XCB_CURRENT_TIME);
        xcb_grab_pointer_reply_t* reply = xcb_grab_pointer_reply(xcbConnection, cookie, &error);
        int retries = 0;
        while(retries < 100 && reply->status == XCB_GRAB_STATUS_NOT_VIEWABLE) {
            // Window is not yet visible but should be after a short wait
            GROUNDED_LOG_WARNING("Window not yet visible on cursor grab request. Retrying shortly");
            groundedYield();
            retries++;
            cookie = xcb_grab_pointer(xcbConnection, ownerEvents, window->window, 0, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, window->window, XCB_NONE, XCB_CURRENT_TIME);
            reply = xcb_grab_pointer_reply(xcbConnection, cookie, &error);
        }
    } else {
        xcb_ungrab_pointer(xcbConnection, XCB_CURRENT_TIME);
    }
}

static void xcbSetCursorVisibility(GroundedXcbWindow* window, bool visible) {
    if(!visible) {
        u32 cursorId = xcb_generate_id(xcbConnection);
        xcb_create_glyph_cursor(xcbConnection, cursorId, xcbCursorFont, xcbCursorFont, ' ', 0, 0, 0, 0, 0, 0, 0);
        xcbSetCursor(window, cursorId);
    }
}

static u32 xcbGetWindowWidth(GroundedXcbWindow* window) {
    return window->width;
}

static u32 xcbGetWindowHeight(GroundedXcbWindow* window) {
    return window->height;
}

static GroundedWindow* xcbCreateWindow(MemoryArena* arena, struct GroundedWindowCreateParameters* parameters) {
    ASSERT(xcbConnection);
    if(!parameters) {
        static struct GroundedWindowCreateParameters defaultParameters = {0};
        parameters = &defaultParameters;
    }

    const char* error = 0;
    ArenaMarker errorMarker = arenaCreateMarker(arena);
    GroundedXcbWindow* result = ARENA_PUSH_STRUCT(arena, GroundedXcbWindow);
    xcb_window_t parent = xcbScreen->root;

    if(result) { // Generate window id
        result->window = xcb_generate_id(xcbConnection);
        if(result->window == 0xFFFFFFFF) {
            error = "Could not reserve xid for window";
            arenaResetToMarker(errorMarker);
            result = 0;
        }
    }

    if(result) { // Create and setup window
        xcb_void_cookie_t windowCheckCookie = {};
        u8 depth = XCB_COPY_FROM_PARENT;
        
        u16 width = parameters->width;
        u16 height = parameters->height;
        if(!width) width = 1920;
        if(!height) height = 1080;
        result->width = width;
        result->height = height;

        u16 border = 0;
        xcb_visualid_t visual = xcbScreen->root_visual;
        // Notify x server which events we want to receive for this window
        uint32_t valueMask = XCB_CW_EVENT_MASK;
        // One entry for each bit in the value mask. Correct sort is important!
        uint32_t valueList[] = {
            XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY,
        };
        if(!error) {
            windowCheckCookie = xcb_create_window_checked(xcbConnection, depth, result->window, parent,
                                                        0, 0, width, height, border, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                                        visual, valueMask, valueList);
        }

        // Add delete notify
        xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, result->window, xcbProtocolsAtom->atom, 4, 32, 1, &xcbDeleteAtom->atom );

        if(!error && parameters->title.size > 0) { // Set window title
            xcbSetWindowTitle(result, parameters->title, false);
        }
        xcb_size_hints_t sizeHints = {0};
        if(parameters->minWidth || parameters->minHeight) {
            sizeHints.flags |= XCB_ICCCM_SIZE_HINT_P_MIN_SIZE; // Flags specifies which properties we want to set
            sizeHints.min_width = parameters->minWidth;
            sizeHints.min_height = parameters->minHeight;
        }
        if(parameters->maxWidth || parameters->maxHeight) {
            sizeHints.flags |= XCB_ICCCM_SIZE_HINT_P_MAX_SIZE; // Flags specifies which properties we want to set
            sizeHints.max_width = parameters->maxWidth;
            sizeHints.max_height = parameters->maxHeight;
        }
        if(sizeHints.flags) {
            xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, result->window, XCB_ATOM_WM_NORMAL_HINTS, XCB_ATOM_WM_SIZE_HINTS, 32, sizeof(sizeHints) >> 2, &sizeHints);
        }

        // At least when running through XWayland this seems to be required to get the correct mouse cursor
        xcbSetCursorType(currentCursorType);

        result->customTitlebarCallback = parameters->customTitlebarCallback;

        // Make window visible
        xcb_map_window(xcbConnection, result->window);

        // Flush all pending requests to the X server
        xcb_flush(xcbConnection);

        xcb_generic_error_t* xcbError = xcb_request_check(xcbConnection, windowCheckCookie);
    }    

    return (GroundedWindow*)result;
}

static void xcbDestroyWindow(GroundedXcbWindow* window) {
    xcb_destroy_window(xcbConnection, window->window);

    // Flush all pending requests to the X server
    xcb_flush(xcbConnection);
}

static u8 translateXcbKeycode(u8 xcbKeycode) {
    u8 result = 0;
    switch(xcbKeycode) {
        case 9:
        result = GROUNDED_KEY_ESCAPE;
        break;
        case 22:
        result = GROUNDED_KEY_BACKSPACE;
        break;
        case 36:
        result = GROUNDED_KEY_RETURN;
        break;
        case 119:
        result = GROUNDED_KEY_DELETE;
        break;
        case 62:
        // RShift
        result = GROUNDED_KEY_RSHIFT;
        break;
        case 50:
        // LShift
        result = GROUNDED_KEY_LSHIFT;
        break;
        case 65:
        result = GROUNDED_KEY_SPACE;
        break;
        case 59:
        result = GROUNDED_KEY_COMMA;
        break;
        case 60:
        result = GROUNDED_KEY_DOT;
        break;
        case 19:
        result = GROUNDED_KEY_0;
        break;
        case 10:
        result = GROUNDED_KEY_1;
        break;
        case 11:
        result = GROUNDED_KEY_2;
        break;
        case 12:
        result = GROUNDED_KEY_3;
        break;
        case 13:
        result = GROUNDED_KEY_4;
        break;
        case 14:
        result = GROUNDED_KEY_5;
        break;
        case 15:
        result = GROUNDED_KEY_6;
        break;
        case 16:
        result = GROUNDED_KEY_7;
        break;
        case 17:
        result = GROUNDED_KEY_8;
        break;
        case 18:
        result = GROUNDED_KEY_9;
        break;
        case 38:
        result = GROUNDED_KEY_A;
        break;
        case 56:
        result = GROUNDED_KEY_B;
        break;
        case 54:
        result = GROUNDED_KEY_C;
        break;
        case 40:
        result = GROUNDED_KEY_D;
        break;
        case 26:
        result = GROUNDED_KEY_E;
        break;
        case 41:
        result = GROUNDED_KEY_F;
        break;
        case 42:
        result = GROUNDED_KEY_G;
        break;
        case 43:
        result = GROUNDED_KEY_H;
        break;
        case 31:
        result = GROUNDED_KEY_I;
        break;
        case 44:
        result = GROUNDED_KEY_J;
        break;
        case 45:
        result = GROUNDED_KEY_K;
        break;
        case 46:
        result = GROUNDED_KEY_L;
        break;
        case 58:
        result = GROUNDED_KEY_M;
        break;
        case 57:
        result = GROUNDED_KEY_N;
        break;
        case 32:
        result = GROUNDED_KEY_O;
        break;
        case 33:
        result = GROUNDED_KEY_P;
        break;
        case 24:
        result = GROUNDED_KEY_Q;
        break;
        case 27:
        result = GROUNDED_KEY_R;
        break;
        case 39:
        result = GROUNDED_KEY_S;
        break;
        case 28:
        result = GROUNDED_KEY_T;
        break;
        case 30:
        result = GROUNDED_KEY_U;
        break;
        case 55:
        result = GROUNDED_KEY_V;
        break;
        case 25:
        result = GROUNDED_KEY_W;
        break;
        case 53:
        result = GROUNDED_KEY_X;
        break;
        case 29:
        result = GROUNDED_KEY_Y;
        break;
        case 52:
        result = GROUNDED_KEY_Z;
        break;
        case 67:
        result = GROUNDED_KEY_F1;
        break;
        case 68:
        result = GROUNDED_KEY_F2;
        break;
        case 69:
        result = GROUNDED_KEY_F3;
        break;
        case 70:
        result = GROUNDED_KEY_F4;
        break;
        case 71:
        result = GROUNDED_KEY_F5;
        break;
        case 72:
        result = GROUNDED_KEY_F6;
        break;
        case 73:
        result = GROUNDED_KEY_F7;
        break;
        case 74:
        result = GROUNDED_KEY_F8;
        break;
        case 75:
        result = GROUNDED_KEY_F9;
        break;
        case 76:
        result = GROUNDED_KEY_F10;
        break;
        case 95:
        result = GROUNDED_KEY_F11;
        break;
        case 96:
        result = GROUNDED_KEY_F12;
        break;
        default:
        printf("Unknown keycode: %i\n", (int)xcbKeycode);
        break;
    }
    return result;
}

static GroundedEvent xcbTranslateToGroundedEvent(xcb_generic_event_t* event) {
    GroundedEvent result = {};
    result.type = GROUNDED_EVENT_TYPE_NONE;

    // The & 0x7f is required to receive the XCB_CLIENT_MESSAGE
    switch(event->response_type & 0x7f) {
        case XCB_CLIENT_MESSAGE:{
            xcb_client_message_event_t* clientMessageEvent = (xcb_client_message_event_t*)event;
            ASSERT(false);
            /*GroundedXcbWindow* window = getWindowSlot(clientMessageEvent->window);
            if(window) {
                if(clientMessageEvent->data.data32[0] == xcbDeleteAtom->atom) {
                    // Delete request
                    result.type = GROUNDED_EVENT_TYPE_CLOSE_REQUEST;
                }
            } else {
                reportXcbError("Xcb client message for unknown window");
            }*/
        } break;
        case XCB_BUTTON_PRESS:{
            xcb_button_press_event_t* mouseButtonEvent = (xcb_button_press_event_t*) event;
            // Window id is in mouseButtonEvent->event
            // 4, 5 is scrolling 6,7 is horizontal scrolling
            if(mouseButtonEvent->detail == 1) {
                xcbMouseState.buttons[GROUNDED_MOUSE_BUTTON_LEFT] = true;
            } else if(mouseButtonEvent->detail == 2) {
                xcbMouseState.buttons[GROUNDED_MOUSE_BUTTON_MIDDLE] = true;
            } else if(mouseButtonEvent->detail == 3) {
                xcbMouseState.buttons[GROUNDED_MOUSE_BUTTON_RIGHT] = true;
            } else if(mouseButtonEvent->detail == 4) {
                xcbMouseState.scrollDelta += 1.0f;
            } else if(mouseButtonEvent->detail == 5) {
                xcbMouseState.scrollDelta -= 1.0f;
            } else if(mouseButtonEvent->detail == 6) {
                xcbMouseState.horizontalScrollDelta -= 1.0f;
            } else if(mouseButtonEvent->detail == 7) {
                xcbMouseState.horizontalScrollDelta += 1.0f;
            }
            result.type = GROUNDED_EVENT_TYPE_BUTTON_DOWN;
            // Button mapping seems to be the same for xcb and our definition
            result.buttonDown.button = mouseButtonEvent->detail;
        } break;
        case XCB_BUTTON_RELEASE:{
            xcb_button_release_event_t* mouseButtonReleaseEvent = (xcb_button_release_event_t*) event;
            if(mouseButtonReleaseEvent->detail == 1) {
                xcbMouseState.buttons[GROUNDED_MOUSE_BUTTON_LEFT] = false;
            } else if(mouseButtonReleaseEvent->detail == 2) {
                xcbMouseState.buttons[GROUNDED_MOUSE_BUTTON_MIDDLE] = false;
            } else if(mouseButtonReleaseEvent->detail == 3) {
                xcbMouseState.buttons[GROUNDED_MOUSE_BUTTON_RIGHT] = false;
            }
            result.type = GROUNDED_EVENT_TYPE_BUTTON_UP;
            // Button mapping seems to be the same for xcb and our definition
            result.buttonUp.button = mouseButtonReleaseEvent->detail;
        } break;
        //TODO: Key modifiers
        case XCB_KEY_PRESS:{
            xcb_key_press_event_t* keyPressEvent = (xcb_key_press_event_t*)event;
            u8 keycode = translateXcbKeycode(keyPressEvent->detail);
            //xcb_input_device_id_t sourceId = keyPressEvent->sourceid;
            xcbKeyboardState.keys[keycode] = true;
            xcbKeyboardState.keyDownTransitions[keycode]++;
            result.type = GROUNDED_EVENT_TYPE_KEY_DOWN;
            result.keyDown.keycode = keycode;
            result.keyDown.modifiers = 0;
        } break;
        case XCB_KEY_RELEASE:{
            xcb_key_release_event_t* keyReleaseEvent = (xcb_key_release_event_t*)event;
            u8 keycode = translateXcbKeycode(keyReleaseEvent->detail);
            xcbKeyboardState.keys[keycode] = false;
            xcbKeyboardState.keyUpTransitions[keycode]++;
            result.type = GROUNDED_EVENT_TYPE_KEY_UP;
            result.keyUp.keycode = keycode;
        } break;
        case XCB_CONFIGURE_NOTIFY:{
            xcb_configure_notify_event_t* configureEvent = (xcb_configure_notify_event_t*)event;
            ASSERT(false);
            /*GroundedXcbWindow* window = getWindowSlot(configureEvent->window);
            if(configureEvent->width > 0) window->width = configureEvent->width;
            if(configureEvent->height > 0) window->height = configureEvent->height;
            result.type = GROUNDED_EVENT_TYPE_RESIZE;
            result.resize.width = window->width;
            result.resize.height = window->height;
            result.resize.window = (GroundedWindow*)window;*/
        } break;
        case XCB_REPARENT_NOTIFY:{
            // This window now has a new parent. We ignore this event for now...
        } break;
        case XCB_CREATE_NOTIFY:{
            // This window has been newly created. We ignore this event for now...
        } break;
        case XCB_MAP_NOTIFY:{
            // This window has been mapped eg. the window has been set visible. We ignore this for now...
        } break;
        case XCB_MOTION_NOTIFY:{
            // Mouse movement event
        } break;
        case XCB_VISIBILITY_NOTIFY:{
            // Chane of window visibility. Half or fully covered. Happens for example on minimzation
        } break;
        case XCB_UNMAP_NOTIFY:{
            // Window has been unmapped. For example because of minimization
        } break;
        case XCB_MAPPING_NOTIFY:{
            // New keyboard mapping. Basically new keyboard layout.
            // WERIRD: Happens if I use my custom keyboard mappings to change volume
            /*xcb_mapping_notify_event_t* mappingEvent = (xcb_mapping_notify_event_t*)event;
            // mappingEvent->request can be
            // XCB_MAPPING_KEYBOARD for keyboard mappings
            // XCB_MAPPING_POINTER mouse mappings
            // XCB_MAPPING_MODIFIER mapping change of modifier keys
            // Should then call  to get new mapping
            xcb_get_keyboard_mapping_cookie_t mappingCookie;
            xcb_get_keyboard_mapping_reply_t* mappingReply;
            mappingCookie = xcb_get_keyboard_mapping(xcbConnection, mappingEvent->first_keycode, mappingEvent->count);
            mappingReply = xcb_get_keyboard_mapping_reply(xcbConnection, mappingCookie, 0);
            if (mappingReply) {
                xcb_keysym_t* keySyms = xcb_get_keyboard_mapping_keysyms(mappingReply);
                // We have mappingReply->keysyms_per_keycode many keysyms per keycode
                // So the keySyms array should be keysyms_per_keycode * keycode_count items large


                // Free the resources
                free(mappingReply);
            }*/
        } break;
        case 0:{
            // response_type 0 means error
            GROUNDED_LOG_ERROR("Received error from xcb");
            xcb_generic_error_t* error = (xcb_generic_error_t*)event;
            printf("XCB errorcode: %d\n", error->error_code);
            ASSERT(false);
        } break;
        default:{
            ASSERT(false);
        } break;
    }
    
    return result;
}

static void xcbFetchMouseState(GroundedXcbWindow* window, MouseState* mouseState) {
    xcb_query_pointer_cookie_t pointerCookie = xcb_query_pointer(xcbConnection, window->window);
    xcb_generic_error_t * error = 0;
    xcb_flush(xcbConnection);
    xcb_query_pointer_reply_t* pointerReply = xcb_query_pointer_reply(xcbConnection, pointerCookie, &error);
    if(pointerReply && pointerReply->same_screen) {
        // Pointer is on same screen as window
        mouseState->x = pointerReply->win_x;
        mouseState->y = pointerReply->win_y;
        mouseState->mouseInWindow = true;
    } else {
        // Cursor not in window
        mouseState->x = -1;
        mouseState->y = -1;
        mouseState->mouseInWindow = false;
    }

    // Set mouse button state
    memcpy(mouseState->buttons, xcbMouseState.buttons, sizeof(mouseState->buttons));

    mouseState->scrollDelta = xcbMouseState.scrollDelta;
    mouseState->horizontalScrollDelta = xcbMouseState.horizontalScrollDelta;
    xcbMouseState.scrollDelta = 0.0f;
    xcbMouseState.horizontalScrollDelta = 0.0f;
    mouseState->deltaX = mouseState->x - mouseState->lastX;
    mouseState->deltaY = mouseState->y - mouseState->lastY;
}

static GroundedEvent* xcbGetEvents(u32* eventCount, u32 maxWaitingTimeInMs) {
    eventQueueIndex = 0;
    xcb_generic_event_t* event;
    
    event = xcb_poll_for_queued_event(xcbConnection);
    if(!event) {
        int xcbFd = xcb_get_file_descriptor(xcbConnection);
        struct pollfd pfd;
        pfd.fd = xcbFd;
        pfd.events = POLLIN;
        int timeout = maxWaitingTimeInMs ? ((int)maxWaitingTimeInMs) : -1;
        poll(&pfd, 1, timeout);
        event = xcb_poll_for_event(xcbConnection);
    }

    while(event) {
        GroundedEvent result = xcbTranslateToGroundedEvent(event);
        if(result.type != GROUNDED_EVENT_TYPE_NONE) {
            eventQueue[eventQueueIndex++] = result;
        }
        event = xcb_poll_for_event(xcbConnection);
    }

    *eventCount = eventQueueIndex;
    return eventQueue;
}

static GroundedEvent* xcbPollEvents(u32* eventCount) {
    eventQueueIndex = 0;
    xcb_generic_event_t* event;
    do {
        // Returns 0 if no events available
        // Also return 0 if connection is broken
        event = xcb_poll_for_event(xcbConnection);
        if(event) {
            GroundedEvent result = xcbTranslateToGroundedEvent(event);
            if(result.type != GROUNDED_EVENT_TYPE_NONE) {
                eventQueue[eventQueueIndex++] = result;
            }
        }
    } while(event != 0);

    *eventCount = eventQueueIndex;
    return eventQueue;
}

static void xcbFetchKeyboardState(GroundedKeyboardState* keyboard) {
    memcpy(keyboard->keys, &xcbKeyboardState.keys, sizeof(keyboard->keys));
    keyboard->modifiers = xcbKeyboardState.modifiers;
    memcpy(keyboard->keyDownTransitions, xcbKeyboardState.keyDownTransitions, sizeof(xcbKeyboardState.keyDownTransitions));
	memset(xcbKeyboardState.keyDownTransitions, 0, sizeof(xcbKeyboardState.keyDownTransitions));
	memcpy(keyboard->keyUpTransitions, xcbKeyboardState.keyUpTransitions, sizeof(xcbKeyboardState.keyUpTransitions));
	memset(xcbKeyboardState.keyUpTransitions, 0, sizeof(xcbKeyboardState.keyUpTransitions));
}

static void xcbSetCursor(GroundedXcbWindow* window, xcb_cursor_t cursor) {
    u32 mask = XCB_CW_CURSOR;
    u32 value_list[1] = {cursor};
    xcb_change_window_attributes (xcbConnection, window->window, mask, (const u32*)&value_list);
    if(xcbCurrentCursor) {
        xcb_free_cursor(xcbConnection, xcbCurrentCursor);
    }
    xcbCurrentCursor = cursor;
}

GROUNDED_FUNCTION void xcbSetCursorType(enum GroundedMouseCursor cursorType) {
    ASSERT(cursorType != GROUNDED_MOUSE_CURSOR_CUSTOM);
    ASSERT(cursorType < GROUNDED_MOUSE_CURSOR_COUNT);

    const char* error = 0;

    if(xcbCursorContext) {
        u64 cursorCandidateCount;
        const char** cursorCandidates = getCursorNameCandidates(cursorType, &cursorCandidateCount);
        xcb_cursor_t cursor = 0;
        for(u32 i = 0; i < cursorCandidateCount; ++i) {
            cursor = xcb_cursor_load_cursor(xcbCursorContext, cursorCandidates[i]);
            if(cursor) {
                break;
            }
        }
        if(cursor) {
            // Apply cursor to window
            ASSERT(false);
            //xcbSetCursor(&xcbWindowSlots[0], cursor);
            xcb_flush(xcbConnection);
            currentCursorType = cursorType;
        } else {
            error = "Could not find suitable cursor type";
        }
    } else {
        // We could techincally still use X default glyph cursors.
        // But those are typically not in the style the user has selected
        error = "No cursor context available. Cursor support limited";
    }

    if(error) {
        // Fallback to xcbDefaultCursor
        if(xcbDefaultCursor) {
            ASSERT(false);
            //xcbSetCursor(&xcbWindowSlots[0], xcbDefaultCursor);
            currentCursorType = GROUNDED_MOUSE_CURSOR_DEFAULT;
        }
        GROUNDED_LOG_WARNING(error);
    }
}

GROUNDED_FUNCTION void xcbSetIcon(u8* data, u32 width, u32 height) {

}

GROUNDED_FUNCTION void xcbSetCustomCursor(u8* data, u32 width, u32 height) {
    const char* error = 0;
    if(rgbaFormat && xcb_render_create_picture) {
        u64 imageSize = width * height * 4;
        int depth = xcbDepth->depth;

        xcb_pixmap_t pixmap = xcb_generate_id(xcbConnection);
        xcb_create_pixmap(xcbConnection, depth, pixmap, xcbScreen->root, width, height);

        xcb_gcontext_t gc = xcb_generate_id(xcbConnection);
        xcb_create_gc(xcbConnection, gc, pixmap, 0, 0);
        xcb_put_image(xcbConnection, XCB_IMAGE_FORMAT_Z_PIXMAP, pixmap, gc, width, height, 0, 0, 0, depth, imageSize, data);
        xcb_free_gc(xcbConnection, gc);

        xcb_render_picture_t picture = xcb_generate_id(xcbConnection);
        xcb_render_create_picture(xcbConnection, picture, pixmap, rgbaFormat->id, 0, 0);

        xcb_cursor_t cursor = xcb_generate_id(xcbConnection);
        xcb_render_create_cursor(xcbConnection, cursor, picture, 0, 0);
        xcb_free_pixmap(xcbConnection, pixmap);
        xcb_render_free_picture(xcbConnection, picture);

        ASSERT(false);
        //xcbSetCursor(&xcbWindowSlots[0], cursor);
        xcb_flush(xcbConnection);
        
        currentCursorType = GROUNDED_MOUSE_CURSOR_CUSTOM;
    } else {
        error = "Necessary xcb render or rgba format not available. Custom cursors not supported";
    }
    if(error) {
        // There was an error setting the custom cursor. We fall back to a default cursor
        xcbSetCursorType(GROUNDED_MOUSE_CURSOR_DEFAULT);
        GROUNDED_LOG_WARNING(error);
    }
}

//*************
// OpenGL stuff
#ifdef GROUNDED_OPENGL_SUPPORT
EGLDisplay xcbEglDisplay;

GROUNDED_FUNCTION GroundedOpenGLContext* xcbCreateOpenGLContext(MemoryArena* arena, GroundedOpenGLContext* contextToShareResources) {
    GroundedOpenGLContext* result = ARENA_PUSH_STRUCT(arena, GroundedOpenGLContext);

    //display = eglGetDisplay((NativeDisplayType) xcbConnection);
    //TODO: eglGetPlatformDisplay
    xcbEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(xcbEglDisplay == EGL_NO_DISPLAY) {
        GROUNDED_LOG_ERROR("Error obtaining EGL display");
        return 0;
    }
    int eglVersionMajor = 0;
    int eglVersionMinor = 0;
    if(!eglInitialize(xcbEglDisplay, &eglVersionMajor, &eglVersionMinor)) {
        GROUNDED_LOG_ERROR("Error initializing EGL display");
        eglTerminate(xcbEglDisplay);
        return 0;
    }
    //GROUNDED_LOG_INFO("Using EGL version ", eglVersionMajor, ".", eglVersionMinor);
    
    // OPENGL_ES available instead of EGL_OPENGL_API
    if(!eglBindAPI(EGL_OPENGL_API)) {
        GROUNDED_LOG_ERROR("Error binding OpenGL API");
        return 0;
    }
    
    // if config is 0 the total number of configs is returned
    //TODO: Config is required when creating the context and when creating the window
    EGLConfig config;
    int numConfigs = 0;
    int attribList[] = {EGL_RED_SIZE, 1, EGL_GREEN_SIZE, 1, EGL_BLUE_SIZE, 1, EGL_NONE};
    // MSAA
    //attribList[] = {EGL_SAMPLE_BUFFERS, 1, EGL_SAMPLES, 4};
    if(!eglChooseConfig(xcbEglDisplay, attribList, &config, 1, &numConfigs) || numConfigs <= 0) {
        GROUNDED_LOG_ERROR("Error choosing OpenGL config");
        return 0;
    }
    
    result->eglContext = eglCreateContext(xcbEglDisplay, config, EGL_NO_CONTEXT, 0);
    if(result->eglContext == EGL_NO_CONTEXT) {
        GROUNDED_LOG_ERROR("Error creating EGL Context");
        return 0;
    }
    
    return result;
}

static bool xcbCreateEglSurface(GroundedXcbWindow* window) {
    // if config is 0 the total number of configs is returned
    EGLConfig config;
    int numConfigs = 0;
    int attribList[] = {EGL_RED_SIZE, 1, EGL_GREEN_SIZE, 1, EGL_BLUE_SIZE, 1, EGL_NONE};
    // MSAA
    //attribList[] = {EGL_SAMPLE_BUFFERS, 1, EGL_SAMPLES, 4};
    if(!eglChooseConfig(xcbEglDisplay, attribList, &config, 1, &numConfigs) || numConfigs <= 0) {
        GROUNDED_LOG_ERROR("Error choosing OpenGL config");
        return 0;
    }

    window->eglSurface = eglCreateWindowSurface(xcbEglDisplay, config, PTR_FROM_INT(window->window), 0);
    if(window->eglSurface == EGL_NO_SURFACE) {
        GROUNDED_LOG_ERROR("Error creating surface");
        return false;
    }
    return true;
}

GROUNDED_FUNCTION void xcbOpenGLMakeCurrent(GroundedXcbWindow* window, GroundedOpenGLContext* context) {
    if(!eglMakeCurrent(xcbEglDisplay, window->eglSurface, window->eglSurface, context->eglContext)) {
        //GROUNDED_LOG_ERROR("Error: ", eglGetError());
        GROUNDED_LOG_ERROR("Error making OpenGL context current");
        //return false;
    }
}

GROUNDED_FUNCTION void xcbWindowGlSwapBuffers(GroundedXcbWindow* window) {
    eglSwapBuffers(xcbEglDisplay, window->eglSurface);
}

GROUNDED_FUNCTION void xcbWindowSetGlSwapInterval(int interval) {
    eglSwapInterval(xcbEglDisplay, interval);
}
#endif // GROUNDED_OPENGL_SUPPORT

//*************
// Vulkan stuff
#ifdef GROUNDED_VULKAN_SUPPORT
typedef VkFlags VkXcbSurfaceCreateFlagsKHR;
typedef struct VkXcbSurfaceCreateInfoKHR {
    VkStructureType               sType;
    const void*                   pNext;
    VkXcbSurfaceCreateFlagsKHR    flags;
    xcb_connection_t*             connection;
    xcb_window_t                  window;
} VkXcbSurfaceCreateInfoKHR;

typedef VkResult (VKAPI_PTR *PFN_vkCreateXcbSurfaceKHR)(VkInstance instance, const VkXcbSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
typedef VkBool32 (VKAPI_PTR *PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, xcb_connection_t* connection, xcb_visualid_t visual_id);

static VkSurfaceKHR xcbGetVulkanSurface(GroundedXcbWindow* window, VkInstance instance) {
    const char* error = 0;
    VkSurfaceKHR surface = 0;
    static PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR = 0;
    if(!vkCreateXcbSurfaceKHR) {
        vkCreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR");    
    }
    if(!vkCreateXcbSurfaceKHR) {
        error = "Could not load vkCreateXcbSurfaceKHR. Is the xcb instance extension enabled and available?";
    }

    if(!error) {
        VkXcbSurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
        createInfo.connection = xcbConnection;
        createInfo.window = window->window;
        VkResult result = vkCreateXcbSurfaceKHR(instance, &createInfo, 0, &surface);
    }

    if(error) {
        printf("Error creating vulkan surface: %s\n", error);
    }
    
    return surface;
}
#endif // GROUNDED_VULKAN_SUPPORT

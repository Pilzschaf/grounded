#include <grounded/window/grounded_window.h>
#include <grounded/logger/grounded_logger.h>
#include <grounded/threading/grounded_threading.h>
#include <grounded/memory/grounded_memory.h>

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

#ifndef GROUNDED_XCB_MAX_MIMETYPES
#define GROUNDED_XCB_MAX_MIMETYPES 256
#endif

//#include <xcb/xcb_cursor.h>
//#include <xcb/xcb.h>

typedef struct GroundedXcbWindow {
    xcb_window_t window;
    u32 width;
    u32 height;
    bool pointerInside; // True when cursor is currently over this window
    void* userData;
    GroundedWindowCustomTitlebarCallback* customTitlebarCallback;
    MouseState xcbMouseState;
    GroundedWindowDndCallback* dndCallback;

#ifdef GROUNDED_OPENGL_SUPPORT
    EGLSurface eglSurface;
#endif
} GroundedXcbWindow;

struct {
    xcb_atom_t* mimeTypes;
    u64 mimeTypeCount;
    xcb_window_t target;
    xcb_window_t source;
    bool statusPending;
    bool dragActive;
    GroundedWindowDragPayloadDescription* desc;
    void* userData;
    GroundedDragFinishType finishType;
    xcb_window_t dragImageWindow;
} xdndSourceData;
    
struct {
    MemoryArena offerArena;
    ArenaMarker offerArenaResetMarker;
    GroundedWindowDndDropCallback* currentDropCallback;
    u64 mimeTypeCount;
    String8* mimeTypes;
    u32 currentMimeIndex;
} xdndTargetData;

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

void* xcbLibrary;
xcb_connection_t* xcbConnection;
xcb_screen_t* xcbScreen;
xcb_depth_t* xcbTransparentDepth; // 32bit depth used for transparent windows
xcb_visualtype_t* xcbTransparentVisual;
xcb_colormap_t xcbTransparentColormap;

xcb_font_t xcbCursorFont; // Could be used as a fallback when xcb_cursor is not available
xcb_cursor_t xcbDefaultCursor; // Not really used right now and probably also does not contain correct default cursor
xcb_cursor_context_t* xcbCursorContext;
xcb_cursor_t xcbCurrentCursor;
xcb_cursor_t xcbCursorOverwrite;
GroundedMouseCursor currentCursorType = GROUNDED_MOUSE_CURSOR_DEFAULT;
xcb_render_pictforminfo_t* rgbaFormat;

struct {
    xcb_atom_t xcbDeleteAtom;
    xcb_atom_t xcbProtocolsAtom;
    xcb_atom_t xcbCustomDataAtom;
    xcb_atom_t xdndAwareAtom;
    xcb_atom_t xdndEnterAtom;
    xcb_atom_t xdndPositionAtom;
    xcb_atom_t xdndStatusAtom;
    xcb_atom_t xdndLeaveAtom;
    xcb_atom_t xdndDropAtom;
    xcb_atom_t xdndFinishedAtom;
    xcb_atom_t xdndTypeListAtom;
    xcb_atom_t xdndActionCopyAtom;
    xcb_atom_t xdndProxyAtom;
    xcb_atom_t xdndSelectionAtom;
} xcbAtoms;

GroundedXcbWindow* activeXcbWindow;
GroundedKeyboardState xcbKeyboardState;
bool xcbShmAvailable;

static void shutdownXcb();
static void xcbSetCursorType(GroundedMouseCursor cursorType);
static void xcbSetCursor(xcb_cursor_t cursor);
static void xcbSetOverwriteCursor(xcb_cursor_t cursor);
static void xcbApplyCurrentCursor(GroundedXcbWindow* window);
static xcb_window_t xcbGetHoveredWindow(xcb_window_t startingWindow, int rootX, int rootY, int originX, int originY);
static bool xcbIsWindowDndAware(xcb_window_t window);
static void xcbSendDndStatus(xcb_window_t source, xcb_window_t target);
static xcb_window_t getXdndAwareTarget(int rootX, int rootY);
static void xcbSendDndPosition(xcb_window_t source, xcb_window_t target, int x, int y);
static void xcbSendDndLeave(xcb_window_t source, xcb_window_t target);
static void xcbSendDndDrop(xcb_window_t source, xcb_window_t target);
static void xcbSendDndEnter(xcb_window_t source, xcb_window_t target);

static void reportXcbError(const char* message) {
    printf("Error: %s\n", message);
}

static GroundedXcbWindow* groundedWindowFromXcb(xcb_window_t window) {
    xcb_get_property_reply_t* reply = xcb_get_property_reply(xcbConnection, xcb_get_property(xcbConnection, 0, window, xcbAtoms.xcbCustomDataAtom, XCB_ATOM_CARDINAL, 0, sizeof(void*)/4), 0);
    if(!reply) {
        GROUNDED_LOG_WARNING("Request for window without custom data");
        return 0;
    }
    void* customData = xcb_get_property_value(reply);
    GroundedXcbWindow* result = *(GroundedXcbWindow**)customData;
    return result;
}

static void initXcb() {
    const char* error = 0;

    // Load library
    xcbLibrary = dlopen("libxcb.so", RTLD_LAZY | RTLD_LOCAL);
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
        xcb_depth_iterator_t depthIterator = xcb_screen_allowed_depths_iterator(xcbScreen);
        while (depthIterator.rem) {
            if (depthIterator.data->depth == 32 && depthIterator.data->visuals_len) {
                xcbTransparentDepth = depthIterator.data;
                break;
            }
            xcb_depth_next(&depthIterator);
        }
        if (!xcbTransparentDepth) {
            error = "ERROR: screen does not support 32 bit color depth";
        }
    }

    if(!error) {
        xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(xcbTransparentDepth);
        while (visual_iter.rem) {
            if (visual_iter.data->_class == XCB_VISUAL_CLASS_TRUE_COLOR) {
                xcbTransparentVisual = visual_iter.data;
                break;
            }
            xcb_visualtype_next(&visual_iter);
        }
        if (!xcbTransparentVisual) {
            error = "ERROR: screen does not support True Color";
        }
    }
    
    if(!error) {
        xcbTransparentColormap = xcb_generate_id(xcbConnection);
        xcb_void_cookie_t cookie = xcb_create_colormap_checked(xcbConnection, XCB_COLORMAP_ALLOC_NONE, xcbTransparentColormap, xcbScreen->root, xcbTransparentVisual->visual_id);
        xcb_generic_error_t* requestError = xcb_request_check(xcbConnection, cookie);
        if(requestError) {
            error = "ERROR: could not retrieve colormap";
        }
    }

    if(!error) {
        xcbCursorFont = xcb_generate_id (xcbConnection);
        String8 fontName = STR8_LITERAL("cursor");
        xcb_open_font(xcbConnection, xcbCursorFont, fontName.size, (const char*)fontName.base);
        xcbDefaultCursor = xcb_generate_id(xcbConnection);
        xcb_create_glyph_cursor(xcbConnection, xcbDefaultCursor, xcbCursorFont, XC_X_cursor, XC_X_cursor + 1, 0, 0, 0, 0, 0, 0, 0);
    }

    if(!error) { // Register all atoms
#define INTERN_ATOM(atomName, atomText) \
        xcb_intern_atom_cookie_t atomName##Cookie = xcb_intern_atom(xcbConnection, onlyIfExists, sizeof(atomText)-1, atomText); \
        xcb_intern_atom_reply_t* atomName##Reply = xcb_intern_atom_reply(xcbConnection, atomName##Cookie, 0); \
        xcbAtoms.atomName = atomName##Reply->atom; \
        free(atomName##Reply)
        
        u8 onlyIfExists = 0;
        INTERN_ATOM(xcbProtocolsAtom, "WM_PROTOCOLS");
        INTERN_ATOM(xcbDeleteAtom, "WM_DELETE_WINDOW");
        INTERN_ATOM(xcbCustomDataAtom, "GROUNDED_CUSTOM_DATA");
        INTERN_ATOM(xdndAwareAtom, "XdndAware");
        INTERN_ATOM(xdndEnterAtom, "XdndEnter");
        INTERN_ATOM(xdndPositionAtom, "XdndPosition");
        INTERN_ATOM(xdndStatusAtom, "XdndStatus");
        INTERN_ATOM(xdndLeaveAtom, "XdndLeave");
        INTERN_ATOM(xdndDropAtom, "XdndDrop");
        INTERN_ATOM(xdndFinishedAtom, "XdndFinished");
        INTERN_ATOM(xdndTypeListAtom, "XdndTypeList");
        INTERN_ATOM(xdndActionCopyAtom, "XdndActionCopy");
        INTERN_ATOM(xdndProxyAtom, "XdndProxy");
        INTERN_ATOM(xdndSelectionAtom, "XdndSelection");
#undef INTERN_ATOM

        xdndTargetData.offerArena = createGrowingArena(osGetMemorySubsystem(), KB(4));
        xdndTargetData.offerArenaResetMarker = arenaCreateMarker(&xdndTargetData.offerArena);
    }

    if(error) {
        shutdownXcb();
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
    MEMORY_CLEAR_STRUCT(&xcbAtoms);
    if(xcbConnection) {
        xcb_disconnect(xcbConnection);
        xcbConnection = 0;
    }
    if(xcbLibrary) {
        dlclose(xcbLibrary);
        xcbLibrary = 0;
    }
}

static void xcbMoveDragImageWindow(int x, int y) {
    if(xdndSourceData.dragImageWindow) {
        xcb_configure_window(xcbConnection, xdndSourceData.dragImageWindow, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,(uint32_t[]) {x, y});
        xcb_flush(xcbConnection);
    }
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
        xcbSetCursor(cursorId);
    }
}

static u32 xcbGetWindowWidth(GroundedXcbWindow* window) {
    return window->width;
}

static u32 xcbGetWindowHeight(GroundedXcbWindow* window) {
    return window->height;
}

static void setDndAware(GroundedXcbWindow* window) { //TODO: Why 12???
    //TODO: Look like we have to directly set the version here and not need a second version
    xcb_atom_t xdndAware = xcbAtoms.xdndAwareAtom;
    xcb_atom_t atm = 5;
    xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, window->window, xdndAware, XCB_ATOM_ATOM, 32, 1, &atm);


    // Set our supported xdnd protocol version to version 5
    //TODO: Why 11??? XdndProtocolVersion or XdndVersion???
    /*xcb_intern_atom_cookie_t xdndVersionAtomCookie = xcb_intern_atom(xcbConnection, 0, 11, "XdndProtocolVersion");
    xcb_intern_atom_reply_t* xdndVersionAtomReply = xcb_intern_atom_reply(xcbConnection, xdndVersionAtomCookie, 0);
    xcb_atom_t xdndVersionAtom = xdndVersionAtomReply->atom;
    free(xdndVersionAtomReply);
    xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, window->window, xdndVersionAtom, XCB_ATOM_ATOM, 32, 1, (const uint32_t[]){5});

    //TODO: This can probably be emitted in most case as it is done at other points
    xcb_flush(xcbConnection);*/
    //xcb_flush(xcbConnection);
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

        result->dndCallback = parameters->dndCallback;

        u16 border = 0;
        xcb_visualid_t visual = xcbScreen->root_visual;
        // Notify x server which events we want to receive for this window
        uint32_t valueMask = XCB_CW_EVENT_MASK;
        // One entry for each bit in the value mask. Correct sort is important!
        uint32_t valueList[] = {
            XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW,
        };
        if(!error) {
            windowCheckCookie = xcb_create_window_checked(xcbConnection, depth, result->window, parent,
                                                        0, 0, width, height, border, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                                        visual, valueMask, valueList);
        }

        // Add delete notify
        xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, result->window, xcbAtoms.xcbProtocolsAtom, 4, 32, 1, &xcbAtoms.xcbDeleteAtom );

        // Set custom data pointer
        xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, result->window, xcbAtoms.xcbCustomDataAtom, XCB_ATOM_CARDINAL, 32, sizeof(result)/4, &result);

        // Set dnd aware
        if(result->dndCallback) {
            setDndAware(result);
        }

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
    if(activeXcbWindow == window) {
        activeXcbWindow = 0;
    }
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
            GroundedXcbWindow* window = groundedWindowFromXcb(clientMessageEvent->window);
            if(window) {
                if(clientMessageEvent->data.data32[0] == xcbAtoms.xcbDeleteAtom) {
                    // Delete request
                    result.type = GROUNDED_EVENT_TYPE_CLOSE_REQUEST;
                } else if(clientMessageEvent->type == xcbAtoms.xdndEnterAtom) {
                    printf("DND Enter\n");
                    if(window->dndCallback) {
                        // We override pointer inside as we want to receive mouse position during drag
                        window->pointerInside = true;
                        
                        arenaResetToMarker(xdndTargetData.offerArenaResetMarker);
                        u32 version = clientMessageEvent->data.data32[1] >> 24;
                        xcb_window_t source = clientMessageEvent->data.data32[0];
                        xdndTargetData.mimeTypeCount = 0;
                        xdndTargetData.mimeTypes = 0;
                        if (clientMessageEvent->data.data32[1] & 1) {
                            // More than 3 mime types so get types from xdndtypelist
                            xcb_get_property_cookie_t propertyCookie = xcb_get_property(xcbConnection, 0, source, xcbAtoms.xdndTypeListAtom, XCB_ATOM_ATOM, 0, GROUNDED_XCB_MAX_MIMETYPES);
                            xcb_get_property_reply_t* propertyReply = xcb_get_property_reply(xcbConnection, propertyCookie, 0);
                            if(propertyReply && propertyReply->type != XCB_NONE && propertyReply->format == 32) {
                                int length = xcb_get_property_value_length(propertyReply) / 4;
                                xcb_atom_t* atoms = (xcb_atom_t*)xcb_get_property_value(propertyReply);
                                xdndTargetData.mimeTypeCount = length;
                                xdndTargetData.mimeTypes = ARENA_PUSH_ARRAY(&xdndTargetData.offerArena, xdndTargetData.mimeTypeCount, String8);
                                for(int i = 0; i < length; ++i) {
                                    xcb_get_atom_name_cookie_t nameCookie = xcb_get_atom_name(xcbConnection, atoms[i]);
                                    xcb_get_atom_name_reply_t* nameReply = xcb_get_atom_name_reply(xcbConnection, nameCookie, 0);
                                    String8 mimeType = str8FromBlock((u8*)xcb_get_atom_name_name(nameReply), nameReply->name_len);
                                    xdndTargetData.mimeTypes[i] = str8CopyAndNullTerminate(&xdndTargetData.offerArena, mimeType);
                                }
                                free(propertyReply);
                            }
                        } else {
                            // Take up to 3 mime types from event data
                            xdndTargetData.mimeTypes = ARENA_PUSH_ARRAY(&xdndTargetData.offerArena, 3, String8);
                            int i = 0;
                            for(;i < 3; ++i) {
                                xcb_atom_t atom = clientMessageEvent->data.data32[2+i];
                                if(!atom) {
                                    break;
                                }
                                xcb_get_atom_name_cookie_t nameCookie = xcb_get_atom_name(xcbConnection, atom);
                                xcb_get_atom_name_reply_t* nameReply = xcb_get_atom_name_reply(xcbConnection, nameCookie, 0);
                                String8 mimeType = str8FromBlock((u8*)xcb_get_atom_name_name(nameReply), nameReply->name_len);
                                xdndTargetData.mimeTypes[i] = str8CopyAndNullTerminate(&xdndTargetData.offerArena, mimeType);
                            }
                            xdndTargetData.mimeTypeCount = i;
                        }
                        //TODO: Get position
                        s32 x = 0;
                        s32 y = 0;
                        xdndTargetData.currentMimeIndex = window->dndCallback(0, (GroundedWindow*)window, x, y, xdndTargetData.mimeTypeCount, xdndTargetData.mimeTypes, &xdndTargetData.currentDropCallback);
                    }
                } else if(clientMessageEvent->type == xcbAtoms.xdndPositionAtom) {
                    printf("DND Move\n");
                    xcbSendDndStatus(window->window, clientMessageEvent->data.data32[0]);
                } else if(clientMessageEvent->type == xcbAtoms.xdndLeaveAtom) {
                    printf("DND Leave\n");
                    window->pointerInside = false;
                } else if(clientMessageEvent->type == xcbAtoms.xdndDropAtom) {
                    printf("DND Drop\n");
                    if(window->dndCallback && xdndTargetData.currentDropCallback) {
                        s32 x = 0;
                        s32 y = 0;
                        String8 mimeType = xdndTargetData.mimeTypes[xdndTargetData.currentMimeIndex];
                        xdndTargetData.currentDropCallback(0, EMPTY_STRING8, (GroundedWindow*)window, x, y, mimeType);
                    }
                } else if(clientMessageEvent->type == xcbAtoms.xdndStatusAtom) {
                    printf("DND Status\n");
                    xdndSourceData.statusPending = false;
                    bool dropPossible = clientMessageEvent->data.data32[1];
                    printf("Drop possible\n");
                } else if(clientMessageEvent->type == xcbAtoms.xdndFinishedAtom) {
                    printf("DND Finished\n");
                    if(xdndSourceData.desc->dragFinishCallback) {
                        xdndSourceData.desc->dragFinishCallback(&xdndSourceData.desc->arena, xdndSourceData.userData, xdndSourceData.finishType);
                    }
                }
            } else {
                reportXcbError("Xcb client message for unknown window");
            }
        } break;
        case XCB_BUTTON_PRESS:{
            xcb_button_press_event_t* mouseButtonEvent = (xcb_button_press_event_t*) event;
            GroundedXcbWindow* window = groundedWindowFromXcb(mouseButtonEvent->event);
            MouseState* mouseState = &window->xcbMouseState;
            // 4, 5 is scrolling 6,7 is horizontal scrolling
            if(mouseButtonEvent->detail == 1) {
                mouseState->buttons[GROUNDED_MOUSE_BUTTON_LEFT] = true;
            } else if(mouseButtonEvent->detail == 2) {
                mouseState->buttons[GROUNDED_MOUSE_BUTTON_MIDDLE] = true;
            } else if(mouseButtonEvent->detail == 3) {
                mouseState->buttons[GROUNDED_MOUSE_BUTTON_RIGHT] = true;
            } else if(mouseButtonEvent->detail == 4) {
                mouseState->scrollDelta += 1.0f;
            } else if(mouseButtonEvent->detail == 5) {
                mouseState->scrollDelta -= 1.0f;
            } else if(mouseButtonEvent->detail == 6) {
                mouseState->horizontalScrollDelta -= 1.0f;
            } else if(mouseButtonEvent->detail == 7) {
                mouseState->horizontalScrollDelta += 1.0f;
            }
            result.type = GROUNDED_EVENT_TYPE_BUTTON_DOWN;
            // Button mapping seems to be the same for xcb and our definition
            result.buttonDown.button = mouseButtonEvent->detail;
        } break;
        case XCB_ENTER_NOTIFY:{
            xcb_enter_notify_event_t* enterEvent = (xcb_enter_notify_event_t*)event;
            GroundedXcbWindow* window = groundedWindowFromXcb(enterEvent->event);
            if(window) {
                window->pointerInside = true;
            }
        } break;
        case XCB_LEAVE_NOTIFY:{
            xcb_leave_notify_event_t* leaveEvent = (xcb_leave_notify_event_t*)event;
            GroundedXcbWindow* window = groundedWindowFromXcb(leaveEvent->event);
            if(window) {
                window->pointerInside = false;
            }
        } break;
        case XCB_BUTTON_RELEASE:{
            xcb_button_release_event_t* mouseButtonReleaseEvent = (xcb_button_release_event_t*) event;
            GroundedXcbWindow* window = groundedWindowFromXcb(mouseButtonReleaseEvent->event);
            MouseState* mouseState = &window->xcbMouseState;
            if(mouseButtonReleaseEvent->detail == 1) {
                mouseState->buttons[GROUNDED_MOUSE_BUTTON_LEFT] = false;
                if(xdndSourceData.dragActive) {
                    xdndSourceData.finishType = GROUNDED_DRAG_FINISH_TYPE_CANCEL;
                    if(xdndSourceData.target) {
                        xcbSendDndDrop(window->window, xdndSourceData.target);
                        xdndSourceData.finishType = GROUNDED_DRAG_FINISH_TYPE_COPY;
                        xcbSetOverwriteCursor(0);
                    }
                    
                    /*if(xdndDragData.desc->dragFinishCallback) {
                        xdndDragData.desc->dragFinishCallback(&xdndDragData.desc->arena, xdndDragData.userData, finishType);
                    }*/
                    if(xdndSourceData.dragImageWindow) {
                        xcb_destroy_window(xcbConnection, xdndSourceData.dragImageWindow);
                    }
                    xcb_flush(xcbConnection);

                    printf("Stopping drag\n");
                    xdndSourceData.dragActive = false;
                }
            } else if(mouseButtonReleaseEvent->detail == 2) {
                mouseState->buttons[GROUNDED_MOUSE_BUTTON_MIDDLE] = false;
            } else if(mouseButtonReleaseEvent->detail == 3) {
                mouseState->buttons[GROUNDED_MOUSE_BUTTON_RIGHT] = false;
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
        case XCB_NO_EXPOSURE:{
            // We ignore for now
        } break;
        case XCB_CONFIGURE_NOTIFY:{
            xcb_configure_notify_event_t* configureEvent = (xcb_configure_notify_event_t*)event;
            GroundedXcbWindow* window = groundedWindowFromXcb(configureEvent->window);
            if(configureEvent->width > 0) window->width = configureEvent->width;
            if(configureEvent->height > 0) window->height = configureEvent->height;
            result.type = GROUNDED_EVENT_TYPE_RESIZE;
            result.resize.width = window->width;
            result.resize.height = window->height;
            result.resize.window = (GroundedWindow*)window;
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
            xcb_motion_notify_event_t* motionEvent = (xcb_motion_notify_event_t*) event;
            GroundedXcbWindow* window = groundedWindowFromXcb(motionEvent->event);
            if(xdndSourceData.dragActive) {
                xcb_window_t hoveredWindow = getXdndAwareTarget(motionEvent->root_x, motionEvent->root_y);
                xcbMoveDragImageWindow(motionEvent->root_x, motionEvent->root_y);
                if(hoveredWindow != 0 && xcbIsWindowDndAware(hoveredWindow)) {
                    //printf("Hovered window: %u\n", hoveredWindow);
                    if(!xdndSourceData.target) {
                        // Send enter event to hovered window
                        xcbSendDndEnter(window->window, hoveredWindow);
                        xdndSourceData.statusPending = false;
                    } else if(xdndSourceData.target != hoveredWindow) {
                        // Window switch so send leave to old and enter to new window
                        xcbSendDndLeave(window->window, xdndSourceData.target);
                        xcbSendDndEnter(window->window, hoveredWindow);
                        xdndSourceData.statusPending = false;
                    }
                    if(!xdndSourceData.statusPending) {
                        // Send current position
                        xcbSendDndPosition(window->window, hoveredWindow, motionEvent->root_x, motionEvent->root_y);
                        xdndSourceData.statusPending = true;
                    }
                    xdndSourceData.target = hoveredWindow;
                } else {
                    if(xdndSourceData.target) {
                        // Send leave event
                        xcbSendDndLeave(window->window, hoveredWindow);
                        xdndSourceData.statusPending = false;
                    }
                    xdndSourceData.target = 0;
                }
            }
        } break;
        case XCB_VISIBILITY_NOTIFY:{
            // Change of window visibility. Half or fully covered. Happens for example on minimzation
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
        case XCB_DESTROY_NOTIFY:{
            // Window is getting destroyed.
        } break;
        case XCB_FOCUS_IN:{
            xcb_focus_in_event_t* focusEvent = (xcb_focus_in_event_t*)event;
            GroundedXcbWindow* window = groundedWindowFromXcb(focusEvent->event);
            if(window) {
                activeXcbWindow = window;
                xcbApplyCurrentCursor(window);
            }
        } break;
        case XCB_FOCUS_OUT:{
            activeXcbWindow = 0;
        } break;
        case XCB_SELECTION_REQUEST:{
            xcb_selection_request_event_t* selectionRequestEvent = (xcb_selection_request_event_t*)event;
            GroundedXcbWindow* window = groundedWindowFromXcb(selectionRequestEvent->owner);
            if(selectionRequestEvent->selection == xcbAtoms.xdndSelectionAtom) {
                bool mimeKnown = false;
                for(u32 i = 0; i < xdndSourceData.desc->mimeTypeCount; ++i) {
                    if(xdndSourceData.mimeTypes[i] == selectionRequestEvent->target) {
                        String8 mimeType = xdndSourceData.desc->mimeTypes[i];
                        String8 data = EMPTY_STRING8;
                        if(xdndSourceData.desc->dataCallback) {
                            data = xdndSourceData.desc->dataCallback(&xdndSourceData.desc->arena, mimeType, i, xdndSourceData.userData);
                        }
                        printf("Requested mime type %s\n", (const char*)mimeType.base);
                        
                        // Send data
                        xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, selectionRequestEvent->requestor, selectionRequestEvent->property, selectionRequestEvent->target, 8, data.size, data.base);
                        
                        // Notify that data has been written
                        xcb_selection_notify_event_t selectionEvent = {
                            .property = selectionRequestEvent->property,
                            .response_type = XCB_SELECTION_NOTIFY,
                            .requestor = selectionRequestEvent->requestor,
                            .target = selectionRequestEvent->target,
                            .time = selectionRequestEvent->time,
                            .selection = selectionRequestEvent->selection,
                        };
                        xcb_send_event(xcbConnection, 0, selectionRequestEvent->requestor, XCB_EVENT_MASK_NO_EVENT, (const char*)&selectionEvent);
                        xcb_flush(xcbConnection);
                        mimeKnown = true;
                        break;
                    }
                }
                if(!mimeKnown) {
                    GROUNDED_LOG_WARNING("Selection request for unknown mime type");
                    ASSERT(false);
                }
            }
        } break;
        case 0:{
            // response_type 0 means error
            GROUNDED_LOG_ERROR("Received error from xcb");
            xcb_generic_error_t* error = (xcb_generic_error_t*)event;
            printf("XCB errorcode: %d sequence:%i\n", error->error_code, error->sequence);
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
    if(pointerReply && window->pointerInside) {
        // Pointer is on this window
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
    memcpy(mouseState->buttons, window->xcbMouseState.buttons, sizeof(mouseState->buttons));

    mouseState->scrollDelta = window->xcbMouseState.scrollDelta;
    mouseState->horizontalScrollDelta = window->xcbMouseState.horizontalScrollDelta;
    window->xcbMouseState.scrollDelta = 0.0f;
    window->xcbMouseState.horizontalScrollDelta = 0.0f;
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

static void xcbApplyCurrentCursor(GroundedXcbWindow* window) {
    u32 mask = XCB_CW_CURSOR;
    xcb_cursor_t cursor = xcbCurrentCursor;
    if(xcbCursorOverwrite) {
        cursor = xcbCursorOverwrite;
    }
    u32 value_list[1] = {cursor};
    xcb_change_window_attributes (xcbConnection, window->window, mask, (const u32*)&value_list);
}

static void xcbSetCursor(xcb_cursor_t cursor) {
    if(xcbCurrentCursor) {
        xcb_free_cursor(xcbConnection, xcbCurrentCursor);
    }
    xcbCurrentCursor = cursor;

    if(activeXcbWindow) {
        xcbApplyCurrentCursor(activeXcbWindow);
    }
}

static void xcbSetOverwriteCursor(xcb_cursor_t cursor) {
    if(xcbCursorOverwrite) {
        xcb_free_cursor(xcbConnection, xcbCursorOverwrite);
    }
    xcbCursorOverwrite = cursor;

    if(activeXcbWindow) {
        xcbApplyCurrentCursor(activeXcbWindow);
    }
}

static xcb_cursor_t xcbGetCursorOfType(GroundedMouseCursor cursorType) {
    ASSERT(cursorType != GROUNDED_MOUSE_CURSOR_CUSTOM);
    ASSERT(cursorType < GROUNDED_MOUSE_CURSOR_COUNT);

    const char* error = 0;
    xcb_cursor_t result = 0;

    if(xcbCursorContext) {
        u64 cursorCandidateCount;
        const char** cursorCandidates = getCursorNameCandidates(cursorType, &cursorCandidateCount);
        for(u32 i = 0; i < cursorCandidateCount; ++i) {
            result = xcb_cursor_load_cursor(xcbCursorContext, cursorCandidates[i]);
            if(result) {
                break;
            }
        }
    } else {
        // We could techincally still use X default glyph cursors.
        // But those are typically not in the style the user has selected
        error = "No cursor context available. Cursor support limited";
    }

    if(error || !result) {
        // Fallback to xcbDefaultCursor
        if(xcbDefaultCursor) {
            ASSERT(false);
            //xcbSetCursor(&xcbWindowSlots[0], xcbDefaultCursor);
            currentCursorType = GROUNDED_MOUSE_CURSOR_DEFAULT;
        }
        GROUNDED_LOG_WARNING(error);
    }

    return result;
}

static void xcbSetCursorType(GroundedMouseCursor cursorType) {
    ASSERT(cursorType != GROUNDED_MOUSE_CURSOR_CUSTOM);
    ASSERT(cursorType < GROUNDED_MOUSE_CURSOR_COUNT);

    xcbSetCursor(xcbGetCursorOfType(cursorType));
    currentCursorType = cursorType;
}

static void xcbSetIcon(u8* data, u32 width, u32 height) {

}

static void flipImage(u32 width, u32 height, u8* data) {
    int rowSize = width * 4;  // Assuming 32-bit (4 bytes) per pixel
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    uint8_t *tempRow = ARENA_PUSH_ARRAY(scratch, rowSize, u8);

    for (int row = 0; row < height / 2; row++) {
        uint8_t *rowStart = data + row * rowSize;
        uint8_t *oppositeRow = data + (height - row - 1) * rowSize;

        // Swap rows
        memcpy(tempRow, rowStart, rowSize);
        memcpy(rowStart, oppositeRow, rowSize);
        memcpy(oppositeRow, tempRow, rowSize);
    }
    
    arenaEndTemp(temp);
    
}

static xcb_pixmap_t xcbCreateDragPixmap(u8* data, u32 width, u32 height) {
    xcb_pixmap_t result = 0;
    if(rgbaFormat && xcb_render_create_picture) {
        u64 imageSize = width * height * 4;
        int depth = xcbTransparentDepth->depth;

        result = xcb_generate_id(xcbConnection);
        xcb_create_pixmap(xcbConnection, depth, result, xcbScreen->root, width, height);

        xcb_gcontext_t gc = xcb_generate_id(xcbConnection);
        xcb_create_gc(xcbConnection, gc, result, 0, 0);
        xcb_void_cookie_t error = xcb_put_image(xcbConnection, XCB_IMAGE_FORMAT_Z_PIXMAP, result, gc, width, height, 0, 0, 0, depth, imageSize, data);
        xcb_free_gc(xcbConnection, gc);
    }
    return result;
}

static void xcbSetCustomCursor(u8* data, u32 width, u32 height) {
    const char* error = 0;
    if(rgbaFormat && xcb_render_create_picture) {
        u64 imageSize = width * height * 4;
        int depth = xcbTransparentDepth->depth;

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

        xcbSetCursor(cursor);
        xcb_flush(xcbConnection);
        
        currentCursorType = GROUNDED_MOUSE_CURSOR_CUSTOM;
    } else {
        error = "Necessary xcb render or rgba format not available. Custom cursors not supported";
    }
    if(error) {
        // There was an error setting the custom cursor. We fall back to a default cursor
        currentCursorType = GROUNDED_MOUSE_CURSOR_DEFAULT;
        //xcbSetCursorType(GROUNDED_MOUSE_CURSOR_DEFAULT);
        GROUNDED_LOG_WARNING(error);
    }
}

// xcb_query_pointer does all this for us.
static xcb_window_t xcbGetHoveredWindow(xcb_window_t startingWindow, int rootX, int rootY, int originX, int originY) {
    xcb_window_t result = 0;
    xcb_query_tree_cookie_t queryTreeCookie = xcb_query_tree(xcbConnection, startingWindow);
    xcb_query_tree_reply_t* queryTreeReply = xcb_query_tree_reply(xcbConnection, queryTreeCookie, 0);
    if(queryTreeReply->response_type) {
        xcb_window_t* children = xcb_query_tree_children(queryTreeReply);
        int childrenCount = xcb_query_tree_children_length(queryTreeReply);
        printf("children count: %i\n", childrenCount);
        for(int i = childrenCount -1; i >= 0; --i) {
            // Get window attributes
            //xcb_get_window_attributes_cookie_t attributesCookie = xcb_get_window_attributes(xcbConnection, children[i]);
            //xcb_get_window_attributes_reply_t* attributesReply = xcb_get_window_attributes_reply(xcbConnection, attributesCookie, 0);

            xcb_get_geometry_cookie_t geometryCookie = xcb_get_geometry(xcbConnection, children[i]);
            xcb_get_geometry_reply_t* geometryReply = xcb_get_geometry_reply(xcbConnection, geometryCookie, 0);

            // Check if cursor is hovering
            if (rootX >= originX + geometryReply->x &&
				rootX < originX + geometryReply->x + geometryReply->width &&
				rootY >= originY + geometryReply->y &&
				rootY < originY + geometryReply->y + geometryReply->height) {
				result = xcbGetHoveredWindow(children[i],
					rootX, rootY, originX + geometryReply->x, originY + geometryReply->y);
				break;
			}
            free(geometryReply);
        }
    }
    free(queryTreeReply);

    if(!result) {
        result = startingWindow;
    }
    return result;
}

static xcb_window_t getXdndAwareTarget(int rootX, int rootY) {
    xcb_window_t root = xcbScreen->root;
    xcb_translate_coordinates_cookie_t translateCookie = xcb_translate_coordinates(xcbConnection, root, root, rootX, rootY);
    xcb_translate_coordinates_reply_t* translateReply = xcb_translate_coordinates_reply(xcbConnection, translateCookie, 0);
    
    xcb_window_t target = translateReply->child;
    int x = translateReply->dst_x;
    int y = translateReply->dst_y;

    if(target && target != root) {
        xcb_window_t src = root;
        while(target != 0) {
            xcb_translate_coordinates_cookie_t translateCookie = xcb_translate_coordinates(xcbConnection, src, target, x, y);
            xcb_translate_coordinates_reply_t* translateReply = xcb_translate_coordinates_reply(xcbConnection, translateCookie, 0);
            if(!translateReply) {
                target = 0;
                break;
            }
            x = translateReply->dst_x;
            y = translateReply->dst_y;
            src = target;
            xcb_window_t child = translateReply->child;
            
            xcb_get_property_cookie_t propertyCookie = xcb_get_property(xcbConnection, 0, target, xcbAtoms.xdndAwareAtom, XCB_GET_PROPERTY_TYPE_ANY, 0, 0);
            xcb_get_property_reply_t* propertyReply = xcb_get_property_reply(xcbConnection, propertyCookie, 0);
            bool dndAware = propertyReply && propertyReply->type != XCB_NONE;
            if(dndAware) {
                break;
            }
            target = child;
        }
    }
    return target;
}

static bool xcbIsWindowDndAware(xcb_window_t window) {
    xcb_get_property_cookie_t propertyCookie = xcb_get_property(xcbConnection, 0, window, xcbAtoms.xdndAwareAtom, XCB_GET_PROPERTY_TYPE_ANY, 0, 0);
    xcb_get_property_reply_t* propertyReply = xcb_get_property_reply(xcbConnection, propertyCookie, 0);
    if(propertyReply && propertyReply->type != XCB_NONE) {
        return true;
        //int isDndAware = propertyReply->type == XCB_ATOM_ATOM && propertyReply->format == 32 && propertyReply->length > 0;

        //void* data = xcb_get_property_value(propertyReply);
        //return propertyReply->type != XCB_NONE;
        //free(propertyReply);
        
    }
    {
        xcb_get_property_cookie_t propertyCookie = xcb_get_property(xcbConnection, 0, window, xcbAtoms.xdndProxyAtom, XCB_GET_PROPERTY_TYPE_ANY, 0, 1);
        xcb_get_property_reply_t* propertyReply = xcb_get_property_reply(xcbConnection, propertyCookie, 0);
        if(propertyReply && propertyReply->type != XCB_NONE) {
            return true;
        }
    }
    return false;
}

static void xcbSendDndEnter(xcb_window_t source, xcb_window_t target) {
    printf("Sending enter from %u to %u\n", source, target);
    u32 flags = (5 << 24);
    if(xdndSourceData.mimeTypeCount > 3) {
        flags |= 0x01;
    }
    xcb_atom_t mimeType0 = xdndSourceData.mimeTypeCount > 0 ? xdndSourceData.mimeTypes[0] : 0;
    xcb_atom_t mimeType1 = xdndSourceData.mimeTypeCount > 1 ? xdndSourceData.mimeTypes[1] : 0;
    xcb_atom_t mimeType2 = xdndSourceData.mimeTypeCount > 2 ? xdndSourceData.mimeTypes[2] : 0;
    xcb_client_message_event_t enterEvent = {
        .response_type = XCB_CLIENT_MESSAGE,
        .format = 32,
        .sequence = 0,
        .window = target,
        .type = xcbAtoms.xdndEnterAtom,
        .data.data32 = {source, flags, mimeType0, mimeType1, mimeType2},
    };
    xcb_send_event(xcbConnection, 0, target, XCB_EVENT_MASK_NO_EVENT, (const char *)&enterEvent);
    xcb_flush(xcbConnection);
}

static void xcbSendDndPosition(xcb_window_t source, xcb_window_t target, int x, int y) {
    printf("Sending position from %u to %u\n", source, target);
    xcb_client_message_event_t positionEvent = {
        .response_type = XCB_CLIENT_MESSAGE,
        .window = target,
        .type = xcbAtoms.xdndPositionAtom,
        .format = 32,
        .data.data32 = {source, x << 16 | y, XCB_CURRENT_TIME, xcbAtoms.xdndActionCopyAtom},
    };
    xcb_send_event(xcbConnection, 0, target, XCB_EVENT_MASK_NO_EVENT, (const char *)&positionEvent);
    xcb_flush(xcbConnection);
}

// Destination to source as response to dnd position event
static void xcbSendDndStatus(xcb_window_t source, xcb_window_t target) {
    printf("Sending status from %u to %u\n", source, target);
    int accept = 1;
    xcb_client_message_event_t statusEvent = {
        .response_type = XCB_CLIENT_MESSAGE,
        .window = target,
        .format = 32,
        .data.data32 = {source, accept, 0, 0, xcbAtoms.xdndActionCopyAtom},
    };
    xcb_send_event(xcbConnection, 0, target, XCB_EVENT_MASK_NO_EVENT, (const char *)&statusEvent);
    xcb_flush(xcbConnection);
}

static void xcbSendDndLeave(xcb_window_t source, xcb_window_t target) {
    printf("Sending leave from %u to %u\n", source, target);
    xcb_client_message_event_t leaveEvent = {
        .response_type = XCB_CLIENT_MESSAGE,
        .window = target,
        .type = xcbAtoms.xdndLeaveAtom,
        .format = 32,
        .data.data32 = {source}
    };
    xcb_send_event(xcbConnection, 0, target, XCB_EVENT_MASK_NO_EVENT, (const char *)&leaveEvent);
    xcb_flush(xcbConnection);
}

static void xcbSendDndDrop(xcb_window_t source, xcb_window_t target) {
    printf("Sending drop from %u to %u\n", source, target);
    xcb_client_message_event_t dropEvent = {
        .response_type = XCB_CLIENT_MESSAGE,
        .window = target,
        .type = xcbAtoms.xdndDropAtom,
        .format = 32,
        //.data.data32 = {source, 1, actionCopy},
        .data.data32 = {source, 0, XCB_CURRENT_TIME},
    };
    xcb_send_event(xcbConnection, 0, target, XCB_EVENT_MASK_NO_EVENT, (const char *)&dropEvent);
    xcb_flush(xcbConnection);
}

static void groundedXcbDragPayloadSetImage(GroundedWindowDragPayloadDescription* desc, u8* data, u32 width, u32 height) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    
    // Flip y axis
    u8* tempData = ARENA_PUSH_ARRAY(scratch, width * height * 4, u8);
    MEMORY_COPY(tempData, data, width * height * 4);
    flipImage(width, height, tempData);

    desc->xcbIcon = xcbCreateDragPixmap(tempData, width, height);

    arenaEndTemp(temp);
}

static void groundedXcbBeginDragAndDrop(GroundedWindowDragPayloadDescription* desc, void* userData) {
    ASSERT(activeXcbWindow);
    ASSERT(!xdndSourceData.dragActive);
    if(xdndSourceData.dragActive) {
        return;
    }

    MEMORY_CLEAR_STRUCT(&xdndSourceData);
    xdndSourceData.desc = desc;
    xdndSourceData.userData = userData;
    xcb_set_selection_owner(xcbConnection, activeXcbWindow->window, xcbAtoms.xdndSelectionAtom, XCB_CURRENT_TIME);
    xdndSourceData.mimeTypes = ARENA_PUSH_ARRAY(&desc->arena, desc->mimeTypeCount, xcb_atom_t);
    xdndSourceData.mimeTypeCount = desc->mimeTypeCount;
    for(u64 i = 0; i < desc->mimeTypeCount; ++i) {
        xcb_intern_atom_cookie_t atomCookie = xcb_intern_atom(xcbConnection, 0, desc->mimeTypes[i].size, (const char*)desc->mimeTypes[i].base);
        xcb_intern_atom_reply_t* atomReply = xcb_intern_atom_reply(xcbConnection, atomCookie, 0);
        xdndSourceData.mimeTypes[i] = atomReply->atom;
        free(atomReply);
    }
    if(false && desc->xcbIcon) {
        xcb_get_geometry_cookie_t geometryCookie = xcb_get_geometry(xcbConnection, desc->xcbIcon);
        xcb_get_geometry_reply_t* geometryReply = xcb_get_geometry_reply(xcbConnection, geometryCookie, 0);
        int width = geometryReply->width;
        int height = geometryReply->height;
        
        // Obviously we have to set the border pixel in order to get a transparent window. WTF???
        uint32_t valueMask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL| XCB_CW_OVERRIDE_REDIRECT | XCB_CW_COLORMAP;
        // One entry for each bit in the value mask. Correct sort is important!
        uint32_t valueList[] = {
            xcbScreen->black_pixel, 0, 1, xcbTransparentColormap,
        };
        u8 depth = xcbTransparentDepth->depth;
        xcb_window_t parent = xcbScreen->root;
        xcb_visualid_t visual = xcbTransparentVisual->visual_id;
        xdndSourceData.dragImageWindow = xcb_generate_id(xcbConnection);
        xcb_void_cookie_t windowCookie = xcb_create_window_checked(xcbConnection, depth, xdndSourceData.dragImageWindow, parent,
         0, 0, width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, 
         visual, valueMask, valueList);
        xcb_generic_error_t* windowError = xcb_request_check(xcbConnection, windowCookie);
        {
            xcb_request_error_t* matchError = (xcb_request_error_t*)windowError;
            xcb_get_geometry_cookie_t geometryCookie = xcb_get_geometry(xcbConnection, xdndSourceData.dragImageWindow);
            xcb_get_geometry_reply_t* geometryReply = xcb_get_geometry_reply(xcbConnection, geometryCookie, 0);
            int x = 0;
        }
        /*xcb_change_property(
            xcbConnection,
            XCB_PROP_MODE_REPLACE,
            xdndDragData.dragImageWindow,
            XCB_ATOM_NET_WM_WINDOW_OPACITY,
            XCB_ATOM_CARDINAL,
            32,
            1,
            (uint32_t[]) {0xffffffff}
        );*/
        xcb_map_window(xcbConnection, xdndSourceData.dragImageWindow);
        xcb_flush(xcbConnection);

        xcb_gcontext_t gc = xcb_generate_id(xcbConnection);
        xcb_create_gc(xcbConnection, gc, xdndSourceData.dragImageWindow, 0, 0);

        // Draw the existing pixmap onto the ghost window
        //xcb_copy_area(xcbConnection, xdndDragData.desc->xcbIcon, xdndDragData.dragImageWindow, gc, 0, 0, 0, 0, 16, 16);
        xcb_void_cookie_t copyCheck = xcb_copy_area_checked(xcbConnection, desc->xcbIcon, xdndSourceData.dragImageWindow, gc, 0, 0, 0, 0, width, height);
        xcb_generic_error_t* error = xcb_request_check(xcbConnection, copyCheck);
        xcb_value_error_t* valueError = (xcb_value_error_t*)error;
        printf("Copy sequence: %i\n", copyCheck.sequence);

        xcb_free_gc(xcbConnection, gc);
        xcb_flush(xcbConnection);
    }

    if(desc->mimeTypeCount > 3) {
        xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, activeXcbWindow->window, xcbAtoms.xdndTypeListAtom, XCB_ATOM_ATOM, 32, desc->mimeTypeCount, (const char*)xdndSourceData.mimeTypes);
    }

    xcbSetOverwriteCursor(xcbGetCursorOfType(GROUNDED_MOUSE_CURSOR_GRABBING));

    printf("Starting drag in xcb\n");
    xdndSourceData.dragActive = true;
}

//*************
// OpenGL stuff
#ifdef GROUNDED_OPENGL_SUPPORT
EGLDisplay xcbEglDisplay;

static GroundedOpenGLContext* xcbCreateOpenGLContext(MemoryArena* arena, GroundedOpenGLContext* contextToShareResources) {
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
    
    EGLContext shareContext = contextToShareResources ? contextToShareResources->eglContext : EGL_NO_CONTEXT;
    result->eglContext = eglCreateContext(xcbEglDisplay, config, shareContext, 0);
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

static void xcbOpenGLMakeCurrent(GroundedXcbWindow* window, GroundedOpenGLContext* context) {
    if(!window->eglSurface) {
        xcbCreateEglSurface(window);
    }
    if(!eglMakeCurrent(xcbEglDisplay, window->eglSurface, window->eglSurface, context->eglContext)) {
        //GROUNDED_LOG_ERROR("Error: ", eglGetError());
        GROUNDED_LOG_ERROR("Error making OpenGL context current");
        //return false;
    }
}

static void xcbWindowGlSwapBuffers(GroundedXcbWindow* window) {
    eglSwapBuffers(xcbEglDisplay, window->eglSurface);
}

static void xcbWindowSetGlSwapInterval(int interval) {
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

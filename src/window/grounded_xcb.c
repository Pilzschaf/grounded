#include <grounded/window/grounded_window.h>
#include <grounded/logger/grounded_logger.h>
#include <grounded/threading/grounded_threading.h>
#include <grounded/memory/grounded_memory.h>

#include <dlfcn.h>
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
//#include <xcb/xproto.h>
#include <X11/keysymdef.h>

#include "types/grounded_xcb_types.h"

#ifndef GROUNDED_XCB_MAX_MIMETYPES
#define GROUNDED_XCB_MAX_MIMETYPES 256
#endif

//#include <xcb/xcb_cursor.h>
//#include <xcb/xcb.h>

//TODO: Currently in xcb dataSource finish is never called when an invalid client is involved which never sends dndFinished. Can be handled using a timeout

typedef struct GroundedXcbWindow {
    xcb_window_t window;
    u32 width;
    u32 height;
    bool pointerInside; // True when cursor is currently over this window
    void* userData;
    void* dndUserData;
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
    struct GroundedDragPayload payload;
} xdndTargetData;

// xcb function types
#define X(N, R, P) typedef R grounded_xcb_##N P;
#include "types/grounded_xcb_functions.h"
#undef X

// xcb function pointers
#define X(N, R, P) static grounded_xcb_##N * N = 0;
#include "types/grounded_xcb_functions.h"
#undef X

// xcb xkb function types
#define X(N, R, P) typedef R grounded_xcb_##N P;
#include "types/grounded_xcb_xkb_functions.h"
#undef X

// xcb xkb function pointers
#define X(N, R, P) static grounded_xcb_##N * N = 0;
#include "types/grounded_xcb_xkb_functions.h"
#undef X

// xcb xkb function types
#define X(N, R, P) typedef R grounded_xcb_##N P;
#include "types/grounded_xkbcommon_x11_functions.h"
#undef X

// xcb xkb function pointers
#define X(N, R, P) static grounded_xcb_##N * N = 0;
#include "types/grounded_xkbcommon_x11_functions.h"
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
void* xcbXkbLibrary;
void* xcbCursorLibrary;
void* xcbShmLibrary;
void* xcbImageLibrary;
void* xcbRenderLibrary;
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

//https://x.org/releases/X11R7.7/doc/xproto/x11protocol.html#Keyboards
//https://www.x.org/releases/current/doc/kbproto/xkbproto.html#Computing_A_State_Field_from_an_XKB_State
u8 xkbEventIndex; // The event index of xkb events
xcb_keycode_t minKeycode;
xcb_keycode_t maxKeycode;

//u32 keycodeLookupTablePhysical[256];
//u32 keycodeLookupTableLanguage[256];
/*typedef struct SymCode {
    unsigned long sym;
    u32 keycode;
} SymCode;*/

MemoryArena clipboardArena;
ArenaMarker clipboardArenaMarker;
xcb_window_t clipboardWindow;
String8 clipboardString;

struct {
    xcb_atom_t xcbDeleteAtom;
    xcb_atom_t xcbProtocolsAtom;
    xcb_atom_t xcbCustomDataAtom;
    xcb_atom_t xcbNetWmState;
    xcb_atom_t xcbNetWmStateFullscreen;
    xcb_atom_t xcbMotifWmHints;
    xcb_atom_t xcbNetWmMoveResize;
    xcb_atom_t xcbClipboard;
    xcb_atom_t xcbUtf8String;
    xcb_atom_t xcbTargets;
    xcb_atom_t xcbMultiple;
    xcb_atom_t xcbSaveTargets;
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

GroundedXcbWindow* activeXcbWindow; // Keyboard focus
GroundedXcbWindow* hoveredWindow;
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
static void xcbSendFinished(xcb_window_t source, xcb_window_t target);
static void xcbSendDndEnter(xcb_window_t source, xcb_window_t target);
static void xcbHandleFinished();
static GroundedEvent xcbTranslateToGroundedEvent(xcb_generic_event_t* event);
static xcb_cursor_t xcbGetCursorOfType(GroundedMouseCursor cursorType);

static void reportXcbError(const char* message) {
    GROUNDED_PUSH_ERRORF("XCB error: %s\n", message);
}

static GroundedXcbWindow* groundedWindowFromXcb(xcb_window_t window) {
    xcb_get_property_reply_t* reply = xcb_get_property_reply(xcbConnection, xcb_get_property(xcbConnection, 0, window, xcbAtoms.xcbCustomDataAtom, XCB_ATOM_CARDINAL, 0, sizeof(void*)/4), 0);
    if(!reply) {
        GROUNDED_LOG_WARNING("Request for window without custom data");
        return 0;
    }
    void* customData = xcb_get_property_value(reply);
    GroundedXcbWindow* result = *(GroundedXcbWindow**)customData;
    free(reply);
    return result;
}

#if 0
static void refreshKeyboardLayout() {
    MEMORY_CLEAR_ARRAY(keycodeLookupTablePhysical);
    MEMORY_CLEAR_ARRAY(keycodeLookupTableLanguage);
    return;

    // Keycode is an index describing a physical key. It is independent from the keyboard layout used
    // keysyms describe the meaning of a key.
    // We need a mapping of a keycode to a keysym

    { // Init physical
        // Find common keys by their respective label
        SymCode symTable[100];
        SymCode* symPointer = symTable;
        *symPointer++ = (SymCode){' ', GROUNDED_KEY_SPACE};

        for(int i = 0; i <= XCB_XKB_CONST_MAX_LEGAL_KEY_CODE; ++i) {
            //xcb_key_symbols_t *key_symbols = xcb_key_symbols_alloc(xcbConnection);
            //xcb_key_symbols_get_keycode()
        }

        xcb_xkb_get_map_cookie_t mapCookie = xcb_xkb_get_map(xcbConnection, XCB_XKB_ID_USE_CORE_KBD, XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);       
        xcb_generic_error_t* error = 0;
        xcb_xkb_get_map_reply_t* mapReply = xcb_xkb_get_map_reply(xcbConnection, mapCookie, &error);
        xcb_xkb_get_map_map_t map;
        //if(mapReply->present & get_map_required_components)
        // https://github.com/xkbcommon/libxkbcommon/blob/89ceb3515b50707be908c091ecd777186f8eb705/src/x11/keymap.c#L699
        xcb_xkb_get_map_map_unpack(xcb_xkb_get_map_map(mapReply), mapReply->nTypes, mapReply->nKeySyms, mapReply->nKeyActions, mapReply->totalActions, mapReply->totalKeyBehaviors, mapReply->virtualMods, mapReply->totalKeyExplicit, mapReply->totalModMapKeys, mapReply->totalVModMapKeys, mapReply->present, &map);
        int sym_maps_length = xcb_xkb_get_map_map_syms_rtrn_length(mapReply, &map);
        xcb_xkb_key_sym_map_iterator_t sym_maps_iter = xcb_xkb_get_map_map_syms_rtrn_iterator(mapReply, &map);

        for (int i = 0; i < sym_maps_length; i++) {
            xcb_xkb_key_sym_map_t *wire_sym_map = sym_maps_iter.data;
            u32 keyIndex = mapReply->firstKeySym + i;

            xcb_xkb_key_sym_map_next(&sym_maps_iter);
        }

        //xcb_get_keyboard_mapping_keysyms()

        free(mapReply);
    }

    // Init language
}
#endif

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
            GROUNDED_LOG_WARNINGF("Could not load xcb function: %s\n", firstMissingFunctionName);
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
            minKeycode = xcbSetup->min_keycode;
            maxKeycode = xcbSetup->max_keycode;
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

    // Load XKB (X Keyboard Extension)
    if(!error) {
        xcbXkbLibrary = dlopen("libxcb-xkb.so", RTLD_LAZY | RTLD_LOCAL);
        if(xcbXkbLibrary) {
            const char* firstMissingFunctionName = 0;
            #define X(N, R, P) N = (grounded_xcb_##N*)dlsym(xcbXkbLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "types/grounded_xcb_xkb_functions.h"
            #undef X
            if(firstMissingFunctionName) {
                GROUNDED_LOG_WARNINGF("Could not load xcb xkb function: %s\n", firstMissingFunctionName);
                error = "Could not load all xcb xkb functions";
            } else {
                xcb_extension_t* xcb_xkb_id = (xcb_extension_t*)dlsym(xcbXkbLibrary, "xcb_xkb_id");
                if(xcb_xkb_id) {
                    const xcb_query_extension_reply_t* reply = xcb_get_extension_data(xcbConnection, xcb_xkb_id);
                    if(reply && reply->present) {
                        xkbEventIndex = reply->first_event;
                        xcb_xkb_use_extension_cookie_t cookie = xcb_xkb_use_extension(xcbConnection, 1, 0);
                        xcb_xkb_use_extension_reply_t* reply = xcb_xkb_use_extension_reply(xcbConnection, cookie, 0);
                        if(reply && reply->supported) {
                            // This shall mark what events we are interested in
                            //xcb_xkb_select_events(xcbConnection, deviceSpec, affectWhich, clear, selectAll, affectMap, map, details);
                            xcb_xkb_select_events(xcbConnection, XCB_XKB_ID_USE_CORE_KBD, XCB_XKB_EVENT_TYPE_STATE_NOTIFY, 0, XCB_XKB_EVENT_TYPE_STATE_NOTIFY, 0xFFF, 0xFFF, 0);

                            //xcb_xkb_get_map_cookie_t mapCookie = xcb_xkb_get_map(xcbConnection, XCB_XKB_ID_USE_CORE_KBD, XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS, XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                            /*xcb_xkb_get_map_cookie_t mapCookie = xcb_xkb_get_map(xcbConnection, XCB_XKB_ID_USE_CORE_KBD, XCB_XKB_MAP_PART_KEY_TYPES | XCB_XKB_MAP_PART_KEY_SYMS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                            
                            xcb_generic_error_t* error = 0;
                            xcb_xkb_get_map_reply_t* mapReply = xcb_xkb_get_map_reply(xcbConnection, mapCookie, &error);
                            
                            xcb_xkb_get_names_cookie_t namesCookie = xcb_xkb_get_names(xcbConnection, XCB_XKB_ID_USE_CORE_KBD, XCB_XKB_NAME_DETAIL_KEY_NAMES);
                            xcb_xkb_get_names_reply_t* namesReply = xcb_xkb_get_names_reply(xcbConnection, namesCookie, 0);*/

                            // Set detectable auto-repeat. So we do not get virtual key-up events while user holds down
                            xcb_xkb_per_client_flags(xcbConnection, XCB_XKB_ID_USE_CORE_KBD,
                                XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
                                XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
                                XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
                                0, 0);
                        } else {
                            error = "Could not enable xcb xkb extension";
                        }
                    } else {
                        error = "Could not load xcb xkb extension";
                    }
                } else {
                    error = "Could not find xcb_xkb_id symbol in libxcb-xkb.so";
                }
            }
        } else {
            error = "Could not find libxcb-xkb.so";
        }
    }

    if(!error) {
        const char* firstMissingFunctionName = 0;
        void* xkbCommonX11Library = dlopen("libxkbcommon-x11.so", RTLD_LAZY | RTLD_LOCAL);
        if(xkbCommonX11Library) {
            #define X(N, R, P) N = (grounded_xcb_##N*)dlsym(xkbCommonX11Library, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "types/grounded_xkbcommon_x11_functions.h"
            #undef X
        }
        xkbContext = xkb_context_new(0);
        xcb_xkb_get_device_info_cookie_t deviceInfoCookie = xcb_xkb_get_device_info(xcbConnection, XCB_XKB_ID_USE_CORE_KBD, 0, 0, 0, 0, 0, 0);
        xcb_xkb_get_device_info_reply_t* deviceInfoReply = xcb_xkb_get_device_info_reply(xcbConnection, deviceInfoCookie, 0);
        u8 deviceId = deviceInfoReply->deviceID;
        struct xkb_keymap* keymap = xkb_x11_keymap_new_from_device(xkbContext, xcbConnection, deviceInfoReply->deviceID, 0);

        /* Keymap related stuff
            xkb_keycode_t xkb_keymap_min_keycode(struct xkb_keymap *keymap);
            xkb_keycode_t xkb_keymap_max_keycode(struct xkb_keymap *keymap);
            void xkb_keymap_key_for_each(struct xkb_keymap *keymap, xkb_keymap_key_iter_t iter, void *data);
            const char * xkb_keymap_key_get_name(struct xkb_keymap *keymap, xkb_keycode_t key);
            xkb_mod_index_t xkb_keymap_num_mods(struct xkb_keymap *keymap);
            const char * xkb_keymap_mod_get_name(struct xkb_keymap *keymap, xkb_mod_index_t idx);
            xkb_layout_index_t xkb_keymap_num_layouts(struct xkb_keymap *keymap);
            const char * xkb_keymap_layout_get_name(struct xkb_keymap *keymap, xkb_layout_index_t idx);
            xkb_layout_index_t xkb_keymap_num_layouts_for_key(struct xkb_keymap *keymap, xkb_keycode_t key);
            xkb_level_index_t xkb_keymap_num_levels_for_key(struct xkb_keymap *keymap, xkb_keycode_t key,xkb_layout_index_t layout);
            int xkb_keymap_key_get_syms_by_level(struct xkb_keymap *keymap, xkb_keycode_t key, xkb_layout_index_t layout, xkb_level_index_t level, const xkb_keysym_t **syms_out);
        */

        // xkbcommon actually wants us to use the xkb_state object
        xkbState = xkb_x11_state_new_from_device(keymap, xcbConnection, deviceId);
        // Set detectable auto repeat here
        // Listen for newKeyboardnotify, mapnotify and statenotify using xcb_xkb_select_events_aux()
        // On new keyboard or map notify rereate xkb_keymap and xkb_state
        // On StateNotify update xkb_state using xkb_state_update_mask()
        //xkb_state_update_mask(xkbState, );
    }

    if(!error) {
        xcbCursorLibrary = dlopen("libxcb-cursor.so", RTLD_LAZY | RTLD_LOCAL);
        if(xcbCursorLibrary) {
            const char* firstMissingFunctionName = 0;
            #define X(N, R, P) N = (grounded_xcb_##N*)dlsym(xcbCursorLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "types/grounded_xcb_cursor_functions.h"
            #undef X
            if(firstMissingFunctionName) {
                GROUNDED_LOG_WARNINGF("Could not load xcb cursor function: %s\n", firstMissingFunctionName);
                GROUNDED_LOG_WARNING("Could not load all xcb cursor functions. Cursor support might be limited");
            } else {
                xcb_cursor_context_new(xcbConnection, xcbScreen, &xcbCursorContext);
            }
        }
    }

    if(!error) {
        xcbShmLibrary = dlopen("libxcb-shm.so", RTLD_LAZY | RTLD_LOCAL);
        if(xcbShmLibrary) {
            const char* firstMissingFunctionName = 0;
            #define X(N, R, P) N = (grounded_xcb_##N*)dlsym(xcbShmLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "types/grounded_xcb_shm_functions.h"
            #undef X
            if(firstMissingFunctionName) {
                GROUNDED_LOG_WARNINGF("Could not load xcb shm function: %s\n", firstMissingFunctionName);
                GROUNDED_LOG_WARNING("Could not load all xcb shm functions");
            } else {
                xcb_shm_query_version_reply_t* reply;
                reply = xcb_shm_query_version_reply(xcbConnection, xcb_shm_query_version(xcbConnection), 0);
                if(!reply) {
                    GROUNDED_LOG_WARNING("No xcb shm available");
                } else {
                    xcbShmAvailable = true;
                    free(reply);
                }
            }
        }
    }

    if(!error) {
        xcbImageLibrary = dlopen("libxcb-image.so", RTLD_LAZY | RTLD_LOCAL);
        if(xcbImageLibrary) {
            const char* firstMissingFunctionName = 0;
            #define X(N, R, P) N = (grounded_xcb_##N*)dlsym(xcbImageLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "types/grounded_xcb_image_functions.h"
            #undef X
            if(firstMissingFunctionName) {
                GROUNDED_LOG_WARNINGF("Could not load xcb image function: %s\n", firstMissingFunctionName);
                GROUNDED_LOG_WARNING("Could not load all xcb image functions");
            }
        }
    }

    if(!error) {
        xcbRenderLibrary = dlopen("libxcb-render.so", RTLD_LAZY | RTLD_LOCAL);
        if(xcbRenderLibrary) {
            const char* firstMissingFunctionName = 0;
            #define X(N, R, P) N = (grounded_xcb_##N*)dlsym(xcbRenderLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
            #include "types/grounded_xcb_render_functions.h"
            #undef X
            if(firstMissingFunctionName) {
                GROUNDED_LOG_WARNINGF("Could not load xcb render function: %s\n", firstMissingFunctionName);
            } else {
                xcb_render_query_pict_formats_cookie_t formats_cookie = xcb_render_query_pict_formats(xcbConnection);
                xcb_render_query_pict_formats_reply_t* formatsReply = xcb_render_query_pict_formats_reply(xcbConnection,
							    formats_cookie, 0);
                ASSERT(formatsReply);
                if(formatsReply) {
                    xcb_render_pictforminfo_t* formats = xcb_render_query_pict_formats_formats(formatsReply);
                    for (u32 i = 0; i < formatsReply->num_formats; i++) {

                        if (formats[i].type == XCB_RENDER_PICT_TYPE_DIRECT &&
                            formats[i].depth == 32 &&
                            formats[i].direct.alpha_mask == 0xff &&
                            formats[i].direct.alpha_shift == 24) {
                            rgbaFormat = &formats[i];
                        }
                    }
                    free(formatsReply);
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
        INTERN_ATOM(xcbNetWmState, "_NET_WM_STATE");
        INTERN_ATOM(xcbNetWmStateFullscreen, "_NET_WM_STATE_FULLSCREEN");
        INTERN_ATOM(xcbMotifWmHints, "_MOTIF_WM_HINTS");
        INTERN_ATOM(xcbNetWmMoveResize, "_NET_WM_MOVERESIZE");
        INTERN_ATOM(xcbClipboard, "CLIPBOARD");
        INTERN_ATOM(xcbUtf8String, "UTF8_STRING");
        INTERN_ATOM(xcbTargets, "TARGETS");
        INTERN_ATOM(xcbMultiple, "MULTIPLE");
        INTERN_ATOM(xcbSaveTargets, "SAVE_TARGETS");
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

    //TODO: Init clipboardWindow
    clipboardArena = createGrowingArena(osGetMemorySubsystem(), KB(4));
    clipboardArenaMarker = arenaCreateMarker(&clipboardArena);
    clipboardWindow = xcb_generate_id(xcbConnection);
    u32 valueMask = XCB_CW_EVENT_MASK;
    // One entry for each bit in the value mask. Correct sort is important!
    u32 valueList[] = {
        XCB_EVENT_MASK_PROPERTY_CHANGE,
    };
    xcb_create_window_checked(xcbConnection, XCB_COPY_FROM_PARENT, clipboardWindow, xcbScreen->root, 0, 0, 1, 1, 0, 0, xcbScreen->root_visual, valueMask, valueList);

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
    //TODO: Do libraries in a similar pattern to atoms for easier unloading?
    if(xcbRenderLibrary) {
        dlclose(xcbRenderLibrary);
        xcbRenderLibrary = 0;
    }
    if(xcbImageLibrary) {
        dlclose(xcbImageLibrary);
        xcbImageLibrary = 0;
    }
    if(xcbShmLibrary) {
        dlclose(xcbShmLibrary);
        xcbShmLibrary = 0;
    }
    if(xcbRenderLibrary) {
        dlclose(xcbRenderLibrary);
        xcbRenderLibrary = 0;
    }
    if(xcbCursorLibrary) {
        dlclose(xcbCursorLibrary);
        xcbCursorLibrary = 0;
    }
    if(xcbXkbLibrary) {
        dlclose(xcbXkbLibrary);
        xcbXkbLibrary = 0;
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

#define NET_WM_STATE_REMOVE 0
#define NET_WM_STATE_ADD    1
#define NET_WM_STATE_TOGGLE 2

// https://specifications.freedesktop.org/wm-spec/wm-spec-latest.html
static void xcbWindowSetFullscreen(GroundedXcbWindow* window, bool fullscreen) {
    // Doesn't work if window has already been mapped
    //xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, window->window, xcbAtoms.xcbNetWmState, XCB_ATOM_ATOM, 32, 1, &xcbAtoms.xcbNetWmStateFullscreen);
    
    u32 action = fullscreen ? NET_WM_STATE_ADD : NET_WM_STATE_REMOVE;

    /* From ICCCM "Changing Window State" */
    xcb_client_message_event_t event = {
        .response_type = XCB_CLIENT_MESSAGE,
        .format = 32,
        .window = window->window,
        .type = xcbAtoms.xcbNetWmState,
        .data.data32 = {action, xcbAtoms.xcbNetWmStateFullscreen, 0, 1},
    };
    xcb_send_event(xcbConnection, 0, xcbScreen->root, XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event);
}

static bool xcbWindowIsFullscreen(GroundedXcbWindow* window) {
    bool result = false;
    xcb_get_property_cookie_t cookie = xcb_get_property(xcbConnection, 0, window->window, xcbAtoms.xcbNetWmState, XCB_ATOM_ATOM, 0, 1024);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(xcbConnection, cookie, 0);
    if(reply) {
        xcb_atom_t* atoms = xcb_get_property_value(reply);
        u32 atomCount = xcb_get_property_value_length(reply) / sizeof(xcb_atom_t);
        for(u32 i = 0; i < atomCount; ++i) {
            if(atoms[i] == xcbAtoms.xcbNetWmStateFullscreen) {
                result = true;
                break;
            }
        }
        free(reply);
    }
    return result;
}

static void xcbWindowSetBorderless(GroundedXcbWindow* window, bool borderless) {
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
    
    xcb_change_property (xcbConnection,
                         XCB_PROP_MODE_REPLACE,
                         window->window,
                         xcbAtoms.xcbMotifWmHints,
                         xcbAtoms.xcbMotifWmHints,
                         32,  // format of property
                         5,   // length of data (5x32 bit) , followed by pointer to data
                         &hints ); // is this is a motif hints struct

    xcb_flush(xcbConnection);
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
            free(reply);
            groundedYield();
            retries++;
            cookie = xcb_grab_pointer(xcbConnection, ownerEvents, window->window, 0, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, window->window, XCB_NONE, XCB_CURRENT_TIME);
            reply = xcb_grab_pointer_reply(xcbConnection, cookie, &error);
        }
        free(reply);
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

static void setDndAware(GroundedXcbWindow* window) {
    xcb_atom_t version = 5;
    xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, window->window, xcbAtoms.xdndAwareAtom, XCB_ATOM_ATOM, 32, 1, &version);
}

static void xcbHandleDndEnter(GroundedXcbWindow* window, xcb_atom_t* mimeTypes, u64 mimeTypeCount, s32 x, s32 y) {
    window->pointerInside = true;
    arenaResetToMarker(xdndTargetData.offerArenaResetMarker);
    xdndTargetData.mimeTypeCount = mimeTypeCount;
    xdndTargetData.mimeTypes = ARENA_PUSH_ARRAY(&xdndTargetData.offerArena, mimeTypeCount, String8);
    for(u64 i = 0; i < mimeTypeCount; ++i) {
        xcb_get_atom_name_cookie_t nameCookie = xcb_get_atom_name(xcbConnection, mimeTypes[i]);
        xcb_get_atom_name_reply_t* nameReply = xcb_get_atom_name_reply(xcbConnection, nameCookie, 0);
        String8 mimeType = str8FromBlock((u8*)xcb_get_atom_name_name(nameReply), nameReply->name_len);
        xdndTargetData.mimeTypes[i] = str8CopyAndNullTerminate(&xdndTargetData.offerArena, mimeType);
        free(nameReply);
    }
    
    xdndTargetData.currentMimeIndex = window->dndCallback(&xdndTargetData.payload, (GroundedWindow*)window, x, y, xdndTargetData.mimeTypeCount, xdndTargetData.mimeTypes, &xdndTargetData.currentDropCallback, window->dndUserData);
}

static void xcbHandleDndMove(GroundedXcbWindow* window, xcb_window_t source, s32 x, s32 y) {
    window->pointerInside = true;
    xdndTargetData.currentMimeIndex = window->dndCallback(&xdndTargetData.payload, (GroundedWindow*)window, x, y, xdndTargetData.mimeTypeCount, xdndTargetData.mimeTypes, &xdndTargetData.currentDropCallback, window->dndUserData);
    xcbSendDndStatus(window->window, source);
}

static void xcbHandleLeave(GroundedXcbWindow* window) {
    window->pointerInside = false;
}

static void xcbHandleDrop(GroundedXcbWindow* window, xcb_window_t source, xcb_timestamp_t time) {
    if(window->dndCallback && xdndTargetData.currentDropCallback) {
        xcb_query_pointer_cookie_t pointerCookie = xcb_query_pointer(xcbConnection, window->window);
        xcb_query_pointer_reply_t* pointerReply = xcb_query_pointer_reply(xcbConnection, pointerCookie, 0);
        s32 x = pointerReply->win_x;
        s32 y = pointerReply->win_y;
        free(pointerReply);

        String8 mimeType = xdndTargetData.mimeTypes[xdndTargetData.currentMimeIndex];
        String8 data = EMPTY_STRING8;
        
        xcb_window_t owner = 0;
        xcb_get_selection_owner_cookie_t selectionOwnerCookie = xcb_get_selection_owner(xcbConnection, xcbAtoms.xdndSelectionAtom);
        xcb_get_selection_owner_reply_t* selectionOwnerReply = xcb_get_selection_owner_reply(xcbConnection, selectionOwnerCookie, 0);
        ASSERT(selectionOwnerReply->owner == source);

        if(xdndSourceData.target == window->window) {
            // We target ourselves so we can get the data directly. Hooray
            if(xdndSourceData.desc->dataCallback) {
                u32 mimeIndex = xdndTargetData.currentMimeIndex;
                data = xdndSourceData.desc->dataCallback(&xdndSourceData.desc->arena, mimeType, mimeIndex, xdndSourceData.userData);
            }
            xdndTargetData.currentDropCallback(&xdndTargetData.payload, data, (GroundedWindow*)window, x, y, mimeType);
            xdndSourceData.finishType = GROUNDED_DRAG_FINISH_TYPE_COPY;
            xcbHandleFinished();
        } else {

            // We intern the type atom here
            xcb_intern_atom_cookie_t internCookie = xcb_intern_atom(xcbConnection, 0, mimeType.size, (const char*)mimeType.base);
            xcb_intern_atom_reply_t* internReply = xcb_intern_atom_reply(xcbConnection, internCookie, 0);
            xcb_convert_selection(xcbConnection, window->window, xcbAtoms.xdndSelectionAtom, internReply->atom, xcbAtoms.xdndSelectionAtom, time);
            xcb_flush(xcbConnection);

            //TODO: Wait for response event. Could do this by using a pthred_cond.
            //TODO: Implement some kind of timeout so we can not hang indefinetely if we get not response
            while(true) {
                // xcb_poll_for_queued_event
                xcb_generic_event_t* event = xcb_wait_for_event(xcbConnection);
                if(event) {
                    if((event->response_type & 0x7f) == XCB_SELECTION_NOTIFY) {
                        //TODO: Retrieve dnd data
                        xcb_selection_notify_event_t *notifyEvent = (xcb_selection_notify_event_t*)event;
                        if(notifyEvent->property != XCB_NONE) {
                            // Retrieve the selected data from the property
                            xcb_get_property_cookie_t propertyCookie = xcb_get_property(xcbConnection, 0, notifyEvent->requestor, notifyEvent->property, XCB_GET_PROPERTY_TYPE_ANY, 0, UINT32_MAX);
                            xcb_get_property_reply_t* propertyReply = xcb_get_property_reply(xcbConnection, propertyCookie, 0);
                            if (propertyReply) {
                                // Process the data as needed
                                data = str8Copy(&xdndTargetData.offerArena, str8FromBlock(xcb_get_property_value(propertyReply), xcb_get_property_value_length(propertyReply)));
                                //GROUNDED_LOG_INFOF("Received data: %.*s\n", xcb_get_property_value_length(propertyReply),    (char *)xcb_get_property_value(propertyReply));

                                free(propertyReply);
                            }
                        }
                        free(event);
                        break;
                    } else {
                        GroundedEvent result = xcbTranslateToGroundedEvent(event);
                        if(result.type != GROUNDED_EVENT_TYPE_NONE) {
                            eventQueue[eventQueueIndex++] = result;
                        }
                    }
                    free(event);
                }
            }

            free(internReply);

            xdndTargetData.currentDropCallback(0, data, (GroundedWindow*)window, x, y, mimeType);
            xcbSendFinished(window->window, source);
        }
        free(selectionOwnerReply);
    }
}

static void xcbHandleStatus(bool dropPossible) {
    xdndSourceData.statusPending = false;
}

static void xcbHandleFinished() {
    xdndSourceData.target = 0;
    if(xdndSourceData.desc->dragFinishCallback) {
        xdndSourceData.desc->dragFinishCallback(&xdndSourceData.desc->arena, xdndSourceData.userData, xdndSourceData.finishType);
    }
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
        u32 valueMask = XCB_CW_EVENT_MASK;
        // One entry for each bit in the value mask. Correct sort is important!
        u32 valueList[] = {
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

        if(parameters->customTitlebarCallback) {
            result->customTitlebarCallback = parameters->customTitlebarCallback;
            xcbWindowSetBorderless(result, true);
        }

        if(parameters->userData) {
            xcbWindowSetUserData(result, parameters->userData);
        }
        result->dndUserData = parameters->dndUserData;

        // Make window visible
        if(!parameters->hidden) {
            xcb_map_window(xcbConnection, result->window);
        }

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
    if(hoveredWindow == window) {
        hoveredWindow = 0;
    }
    xcb_destroy_window(xcbConnection, window->window);

    // Flush all pending requests to the X server
    xcb_flush(xcbConnection);
}

//#include <linux/input-event-codes.h>
// scancodes are defined in  <linux/input-event-codes.h> + 8. However those are the physical keys and might be mapped differently!
static u8 translateXcbKeycode(u8 xcbKeycode) {
    u8 result = 0;
    switch(xcbKeycode) {
        case 9:
        result = GROUNDED_KEY_ESCAPE;
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
        case 22:
        result = GROUNDED_KEY_BACKSPACE;
        break;
        case 23:
        result = GROUNDED_KEY_TAB;
        break;
        case 36:
        result = GROUNDED_KEY_RETURN;
        break;
        case 37:
        result = GROUNDED_KEY_LCTRL;
        break;

        // Characters
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
        case 105:
        result = GROUNDED_KEY_RCTRL;
        break;
        case 110:
        result = GROUNDED_KEY_HOME;
        break;
        case 111:
        result = GROUNDED_KEY_UP;
        break;
        case 112:
        result = GROUNDED_KEY_PAGE_UP;
        break;
        case 113:
        result = GROUNDED_KEY_LEFT;
        break;
        case 114:
        result = GROUNDED_KEY_RIGHT;
        break;
        case 115:
        result = GROUNDED_KEY_END;
        break;
        case 116:
        result = GROUNDED_KEY_DOWN;
        break;
        case 117:
        result = GROUNDED_KEY_PAGE_DOWN;
        break;
        case 118:
        result = GROUNDED_KEY_INSERT;
        break;
        case 119:
        result = GROUNDED_KEY_DELETE;
        break;
        //case 127:
        //result = GROUNDED_KEY_PAUSE;
        //break;
        default:
        GROUNDED_LOG_WARNINGF("Unknown keycode: %i\n", (int)xcbKeycode);
        break;
    }
    return result;
}

enum XcbEdges {
	XCB_RESIZE_EDGE_NONE = 0,
	XCB_RESIZE_EDGE_TOP = 1,
	XCB_RESIZE_EDGE_BOTTOM = 2,
	XCB_RESIZE_EDGE_LEFT = 4,
	XCB_RESIZE_EDGE_TOP_LEFT = 5,
	XCB_RESIZE_EDGE_BOTTOM_LEFT = 6,
	XCB_RESIZE_EDGE_RIGHT = 8,
	XCB_RESIZE_EDGE_TOP_RIGHT = 9,
	XCB_RESIZE_EDGE_BOTTOM_RIGHT = 10,
};

static GroundedEvent xcbTranslateToGroundedEvent(xcb_generic_event_t* event) {
    GroundedEvent result = {};
    result.type = GROUNDED_EVENT_TYPE_NONE;

    // The & 0x7f is required to receive the XCB_CLIENT_MESSAGE
    u8 eventType = event->response_type & 0x7f;
    switch(eventType) {
        case XCB_PROPERTY_NOTIFY:{
            //TODO: What to do with this event?
        };
        /*case XKB_BASE:{

        } break;*/
        case XCB_CLIENT_MESSAGE:{
            xcb_client_message_event_t* clientMessageEvent = (xcb_client_message_event_t*)event;
            GroundedXcbWindow* window = groundedWindowFromXcb(clientMessageEvent->window);
            if(window) {
                if(clientMessageEvent->data.data32[0] == xcbAtoms.xcbDeleteAtom) {
                    // Delete request
                    result.type = GROUNDED_EVENT_TYPE_CLOSE_REQUEST;
                    result.window = (GroundedWindow*)window;
                } else if(clientMessageEvent->type == xcbAtoms.xdndEnterAtom) {
                    GROUNDED_LOG_INFO("DND Enter\n");
                    if(window->dndCallback) {
                        // We override pointer inside as we want to receive mouse position during drag
                        window->pointerInside = true;
                        u32 version = clientMessageEvent->data.data32[1] >> 24;
                        xcb_window_t source = clientMessageEvent->data.data32[0];
                        u32 flags = clientMessageEvent->data.data32[1] & 0xFFFFFF;

                        xcb_query_pointer_cookie_t pointerCookie = xcb_query_pointer(xcbConnection, window->window);
                        xcb_query_pointer_reply_t* pointerReply = xcb_query_pointer_reply(xcbConnection, pointerCookie, 0);
                        s32 x = pointerReply->win_x;
                        s32 y = pointerReply->win_y;
                        free(pointerReply);

                        if (flags & 1) {
                            // More than 3 mime types so get types from xdndtypelist
                            xcb_get_property_cookie_t propertyCookie = xcb_get_property(xcbConnection, 0, source, xcbAtoms.xdndTypeListAtom, XCB_ATOM_ATOM, 0, GROUNDED_XCB_MAX_MIMETYPES);
                            xcb_get_property_reply_t* propertyReply = xcb_get_property_reply(xcbConnection, propertyCookie, 0);
                            if(propertyReply && propertyReply->type != XCB_NONE && propertyReply->format == 32) {
                                int length = xcb_get_property_value_length(propertyReply) / 4;
                                xcb_atom_t* atoms = (xcb_atom_t*)xcb_get_property_value(propertyReply);
                                xcbHandleDndEnter(window, atoms, length, x, y);
                                free(propertyReply);
                            }
                        } else {
                            // Take up to 3 mime types from event data
                            xcb_atom_t* atoms = &clientMessageEvent->data.data32[2];
                            int i = 0;
                            for(;i < 3; ++i) {
                                xcb_atom_t atom = clientMessageEvent->data.data32[2+i];
                                if(!atom) {
                                    break;
                                }
                            }
                            xcbHandleDndEnter(window, atoms, i, x, y);
                        }
                    }
                } else if(clientMessageEvent->type == xcbAtoms.xdndPositionAtom) {
                    GROUNDED_LOG_INFO("DND Move\n");
                    xcb_window_t source = clientMessageEvent->data.data32[0];
                    s32 x = clientMessageEvent->data.data32[2] >> 16;
                    s32 y = clientMessageEvent->data.data32[2] & 0xFFFF;
                    xcbHandleDndMove(window, source, x, y);
                } else if(clientMessageEvent->type == xcbAtoms.xdndLeaveAtom) {
                    GROUNDED_LOG_INFO("DND Leave\n");
                    xcbHandleLeave(window);
                } else if(clientMessageEvent->type == xcbAtoms.xdndDropAtom) {
                    GROUNDED_LOG_INFO("DND Drop\n");
                    xcb_window_t source = clientMessageEvent->data.data32[0];
                    xcb_timestamp_t time = clientMessageEvent->data.data32[2];
                    xcbHandleDrop(window, source, time);
                } else if(clientMessageEvent->type == xcbAtoms.xdndStatusAtom) {
                    GROUNDED_LOG_INFO("DND Status\n");
                    bool dropPossible = clientMessageEvent->data.data32[1];
                    xcbHandleStatus(dropPossible);
                } else if(clientMessageEvent->type == xcbAtoms.xdndFinishedAtom) {
                    GROUNDED_LOG_INFO("DND Finished\n");
                    xcbHandleFinished();
                }
            } else {
                reportXcbError("Xcb client message for unknown window");
            }
        } break;
        case XCB_BUTTON_PRESS:{
            xcb_button_press_event_t* mouseButtonEvent = (xcb_button_press_event_t*) event;
            GroundedXcbWindow* window = groundedWindowFromXcb(mouseButtonEvent->event);
            MouseState* mouseState = &window->xcbMouseState;
            if(window && window->customTitlebarCallback && mouseButtonEvent->detail == 1) {
                GroundedWindowCustomTitlebarHit hit = window->customTitlebarCallback((GroundedWindow*)window, mouseButtonEvent->event_x, mouseButtonEvent->event_y);
                if(hit == GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_BAR) {
                    xcb_client_message_event_t event = {
                        .response_type = XCB_CLIENT_MESSAGE,
                        .format = 32,
                        .window = window->window,
                        .type = xcbAtoms.xcbNetWmMoveResize,
                        .data.data32 = {mouseButtonEvent->root_x, mouseButtonEvent->root_y, _NET_WM_MOVERESIZE_MOVE, mouseButtonEvent->detail, 0}
                    };
                    xcb_ungrab_pointer(xcbConnection, XCB_CURRENT_TIME);
                    xcb_send_event(xcbConnection, 0, xcbScreen->root, XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char*)&event);
                    xcb_flush(xcbConnection);
                    // Do not process this event further
                    break;
                } else if(hit == GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_BORDER) {
                    u32 edges = 0;
                    float x = mouseButtonEvent->event_x;
                    float y = mouseButtonEvent->event_y;
                    float width = groundedWindowGetWidth((GroundedWindow*)window);
                    float height = groundedWindowGetHeight((GroundedWindow*)window);
                    float offset = 20.0f;
                    if(x < offset) {
                        edges |= XCB_RESIZE_EDGE_LEFT;
                    } else if(x > width - offset) {
                        edges |= XCB_RESIZE_EDGE_RIGHT;
                    }
                    if(y < offset) {
                        edges |= XCB_RESIZE_EDGE_TOP;
                    } else if(y > height - offset) {
                        edges |= XCB_RESIZE_EDGE_BOTTOM;
                    }
                    xcb_atom_t sizeAction = 0;
                    switch(edges) {
                        case XCB_RESIZE_EDGE_TOP: sizeAction = _NET_WM_MOVERESIZE_SIZE_TOP; break;
                        case XCB_RESIZE_EDGE_BOTTOM: sizeAction = _NET_WM_MOVERESIZE_SIZE_BOTTOM; break;
                        case XCB_RESIZE_EDGE_LEFT: sizeAction = _NET_WM_MOVERESIZE_SIZE_LEFT; break;
                        case XCB_RESIZE_EDGE_RIGHT: sizeAction = _NET_WM_MOVERESIZE_SIZE_RIGHT; break;
                        case XCB_RESIZE_EDGE_TOP_RIGHT: sizeAction = _NET_WM_MOVERESIZE_SIZE_TOPRIGHT; break;
                        case XCB_RESIZE_EDGE_TOP_LEFT: sizeAction = _NET_WM_MOVERESIZE_SIZE_TOPLEFT; break;
                        case XCB_RESIZE_EDGE_BOTTOM_RIGHT: sizeAction = _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT; break;
                        case XCB_RESIZE_EDGE_BOTTOM_LEFT: sizeAction = _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT; break;
                    }

                    xcb_client_message_event_t event = {
                        .response_type = XCB_CLIENT_MESSAGE,
                        .format = 32,
                        .window = window->window,
                        .type = xcbAtoms.xcbNetWmMoveResize,
                        .data.data32 = {mouseButtonEvent->root_x, mouseButtonEvent->root_y, sizeAction, mouseButtonEvent->detail, 0}
                    };
                    xcb_ungrab_pointer(xcbConnection, XCB_CURRENT_TIME);
                    xcb_send_event(xcbConnection, 0, xcbScreen->root, XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char*)&event);
                    xcb_flush(xcbConnection);
                    // Do not process this event further
                    break;
                }
            }

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
            result.window = (GroundedWindow*)window;
            // Button mapping seems to be the same for xcb and our definition
            result.buttonDown.button = mouseButtonEvent->detail;
            
            ASSERT(mouseButtonEvent->time);
            result.buttonDown.timestamp = mouseButtonEvent->time;
            
            //TODO: Make sure we have the latest mouse position here. Either request mouse position or use latest motion event
            xcb_query_pointer_cookie_t pointerCookie = xcb_query_pointer(xcbConnection, window->window);
            xcb_query_pointer_reply_t* pointerReply = xcb_query_pointer_reply(xcbConnection, pointerCookie, 0);
            result.buttonDown.mousePositionX = pointerReply->win_x;
            result.buttonDown.mousePositionY = pointerReply->win_y;
        } break;
        case XCB_ENTER_NOTIFY:{
            xcb_enter_notify_event_t* enterEvent = (xcb_enter_notify_event_t*)event;
            GroundedXcbWindow* window = groundedWindowFromXcb(enterEvent->event);
            if(window) {
                window->pointerInside = true;
                xcbApplyCurrentCursor(window);
            }
            hoveredWindow = window;
        } break;
        case XCB_LEAVE_NOTIFY:{
            xcb_leave_notify_event_t* leaveEvent = (xcb_leave_notify_event_t*)event;
            GroundedXcbWindow* window = groundedWindowFromXcb(leaveEvent->event);
            if(window && !xdndSourceData.dragActive) {
                // During drag this is controlled by the drag
                window->pointerInside = false;
            }
            if(hoveredWindow == window) {
                hoveredWindow = 0;
                if(!xdndSourceData.dragActive) {
                    xcbSetOverwriteCursor(0);
                }
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
                        if(xdndSourceData.target == window->window) {
                            // We target ourselves so call directly
                            xcbHandleDrop(window, xdndSourceData.target, 0);
                        } else {
                            xcbSendDndDrop(window->window, xdndSourceData.target);
                        }
                        //TODO: Does not make any sense to set the finish type here as the target should decide it
                        xdndSourceData.finishType = GROUNDED_DRAG_FINISH_TYPE_COPY;
                        xcbSetOverwriteCursor(0);
                    } else {
                        // Directly send cancelation
                        xdndSourceData.finishType = GROUNDED_DRAG_FINISH_TYPE_CANCEL;
                        xcbHandleFinished();
                    }
                    
                    /*if(xdndDragData.desc->dragFinishCallback) {
                        xdndDragData.desc->dragFinishCallback(&xdndDragData.desc->arena, xdndDragData.userData, finishType);
                    }*/
                    if(xdndSourceData.dragImageWindow) {
                        xcb_destroy_window(xcbConnection, xdndSourceData.dragImageWindow);
                    }
                    xcb_flush(xcbConnection);

                    GROUNDED_LOG_INFO("Stopping drag\n");
                    xdndSourceData.dragActive = false;
                }
            } else if(mouseButtonReleaseEvent->detail == 2) {
                mouseState->buttons[GROUNDED_MOUSE_BUTTON_MIDDLE] = false;
            } else if(mouseButtonReleaseEvent->detail == 3) {
                mouseState->buttons[GROUNDED_MOUSE_BUTTON_RIGHT] = false;
            }
            result.type = GROUNDED_EVENT_TYPE_BUTTON_UP;
            result.window = (GroundedWindow*)window;
            // Button mapping seems to be the same for xcb and our definition
            result.buttonUp.button = mouseButtonReleaseEvent->detail;
            ASSERT(mouseButtonReleaseEvent->time);
            result.buttonUp.timestamp = mouseButtonReleaseEvent->time;
            //TODO: Make sure we have the latest mouse position here
            result.buttonUp.mousePositionX = mouseState->x;
            result.buttonUp.mousePositionY = mouseState->y;
        } break;
        case XCB_KEY_PRESS:{
            xcb_key_press_event_t* keyPressEvent = (xcb_key_press_event_t*)event;
            u8 keycode = translateXcbKeycode(keyPressEvent->detail);
            //xcb_input_device_id_t sourceId = keyPressEvent->sourceid;
            xcbKeyboardState.keys[keycode] = true;
            xcbKeyboardState.keyDownTransitions[keycode]++;
            result.type = GROUNDED_EVENT_TYPE_KEY_DOWN;
            result.keyDown.keycode = keycode;
            result.keyDown.modifiers = 0;
            result.keyDown.modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_SHIFT : 0;
            result.keyDown.modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_CONTROL : 0;
            result.keyDown.modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_ALT : 0;
            result.keyDown.modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_WINDOWS : 0;
            
            // Get the unicode codepoint for this key
            xkb_keysym_t keysym = xkb_state_key_get_one_sym(xkbState, keyPressEvent->detail);
            u32 codepoint = xkb_keysym_to_utf32(keysym);
            result.keyDown.codepoint = codepoint;
        } break;
        case XCB_KEY_RELEASE:{
            xcb_key_release_event_t* keyReleaseEvent = (xcb_key_release_event_t*)event;
            u8 keycode = translateXcbKeycode(keyReleaseEvent->detail);
            xcbKeyboardState.keys[keycode] = false;
            xcbKeyboardState.keyUpTransitions[keycode]++;
            result.type = GROUNDED_EVENT_TYPE_KEY_UP;
            result.keyUp.keycode = keycode;
            result.keyUp.modifiers = 0;
            result.keyUp.modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_SHIFT : 0;
            result.keyUp.modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_CONTROL : 0;
            result.keyUp.modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_ALT : 0;
            result.keyUp.modifiers |= (xkb_state_mod_name_is_active(xkbState, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) == 1) ? GROUNDED_KEY_MODIFIER_WINDOWS : 0;
            // Get the unicode codepoint for this key
            xkb_keysym_t keysym = xkb_state_key_get_one_sym(xkbState, keyReleaseEvent->detail);
            u32 codepoint = xkb_keysym_to_utf32(keysym);
            result.keyUp.codepoint = codepoint;
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
            result.window = (GroundedWindow*)window;
            result.resize.width = window->width;
            result.resize.height = window->height;
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
                    GROUNDED_LOG_INFO("Nothing hovered\n");
                    if(xdndSourceData.target) {
                        // Send leave event
                        xcbSendDndLeave(window->window, hoveredWindow);
                        xdndSourceData.statusPending = false;
                    }
                    xdndSourceData.target = 0;
                }
            } else if(window->customTitlebarCallback) {
                GroundedWindowCustomTitlebarHit hit = window->customTitlebarCallback((GroundedWindow*)window, motionEvent->event_x, motionEvent->event_y);
                if(hit == GROUNDED_WINDOW_CUSTOM_TITLEBAR_HIT_BORDER) {
                    float x = motionEvent->event_x;
                    float y = motionEvent->event_y;
                    float width = groundedWindowGetWidth((GroundedWindow*)window);
                    float height = groundedWindowGetHeight((GroundedWindow*)window);
                    float offset = 10.0f;
                    u32 edges = 0;
                    if(x < offset) {
                        edges |= XCB_RESIZE_EDGE_LEFT;
                    } else if(x > width - offset) {
                        edges |= XCB_RESIZE_EDGE_RIGHT;
                    }
                    if(y < offset) {
                        edges |= XCB_RESIZE_EDGE_TOP;
                    } else if(y > height - offset) {
                        edges |= XCB_RESIZE_EDGE_BOTTOM;
                    }
                    GroundedMouseCursor cursorType = GROUNDED_MOUSE_CURSOR_DEFAULT;
                    switch(edges) {
                        case XCB_RESIZE_EDGE_TOP: cursorType = GROUNDED_MOUSE_CURSOR_UPDOWN; break;
                        case XCB_RESIZE_EDGE_BOTTOM: cursorType = GROUNDED_MOUSE_CURSOR_UPDOWN; break;
                        case XCB_RESIZE_EDGE_LEFT: cursorType = GROUNDED_MOUSE_CURSOR_LEFTRIGHT; break;
                        case XCB_RESIZE_EDGE_RIGHT: cursorType = GROUNDED_MOUSE_CURSOR_LEFTRIGHT; break;
                        case XCB_RESIZE_EDGE_TOP_RIGHT: cursorType = GROUNDED_MOUSE_CURSOR_UPRIGHT; break;
                        case XCB_RESIZE_EDGE_TOP_LEFT: cursorType = GROUNDED_MOUSE_CURSOR_UPLEFT; break;
                        case XCB_RESIZE_EDGE_BOTTOM_RIGHT: cursorType = GROUNDED_MOUSE_CURSOR_DOWNRIGHT; break;
                        case XCB_RESIZE_EDGE_BOTTOM_LEFT: cursorType = GROUNDED_MOUSE_CURSOR_DOWNLEFT; break;
                    }
                    xcbSetOverwriteCursor(xcbGetCursorOfType(cursorType));
                } else {
                    xcbSetOverwriteCursor(0);
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
            xcb_mapping_notify_event_t* mappingEvent = (xcb_mapping_notify_event_t*)event;
            // mappingEvent->request can be
            // XCB_MAPPING_KEYBOARD for keyboard mappings
            // XCB_MAPPING_POINTER mouse mappings
            // XCB_MAPPING_MODIFIER mapping change of modifier keys
            // Should then call  to get new mapping
            xcb_get_keyboard_mapping_cookie_t mappingCookie;
            xcb_get_keyboard_mapping_reply_t* mappingReply;
            mappingCookie = xcb_get_keyboard_mapping(xcbConnection, mappingEvent->first_keycode, mappingEvent->count);
            //mappingCookie = xcb_get_keyboard_mapping(xcbConnection, ->first_keycode, mappingEvent->count);
            mappingReply = xcb_get_keyboard_mapping_reply(xcbConnection, mappingCookie, 0);
            if (mappingReply) {
                xcb_keysym_t* keySyms = xcb_get_keyboard_mapping_keysyms(mappingReply);
                // We have mappingReply->keysyms_per_keycode many keysyms per keycode
                // So the keySyms array should be keysyms_per_keycode * keycode_count items large


                // Free the resources
                free(mappingReply);
            }
        } break;
        case XCB_DESTROY_NOTIFY:{
            // Window is getting destroyed.
        } break;
        case XCB_FOCUS_IN:{
            xcb_focus_in_event_t* focusEvent = (xcb_focus_in_event_t*)event;
            GroundedXcbWindow* window = groundedWindowFromXcb(focusEvent->event);
            if(window) {
                activeXcbWindow = window;
                //xcbApplyCurrentCursor(window);
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
                        GROUNDED_LOG_INFOF("Requested mime type %s\n", (const char*)mimeType.base);
                        
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
            } else if(selectionRequestEvent->selection == xcbAtoms.xcbClipboard) {
                // Clipboard event

                // Send data
                //xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, selectionRequestEvent->requestor, selectionRequestEvent->property, selectionRequestEvent->target, 8, clipboardString.size, clipboardString.base);
                xcb_atom_t property = selectionRequestEvent->property;
                if(selectionRequestEvent->property == XCB_ATOM_NONE) {
                    // Old version we do not support!
                    ASSERT(false);
                    break;       
                }
                if(selectionRequestEvent->target == xcbAtoms.xcbTargets) {
                    // List of supported targets was requested
                    xcb_atom_t targets[] = {xcbAtoms.xcbUtf8String};
                    xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, selectionRequestEvent->requestor, property, XCB_ATOM_ATOM, 32, ARRAY_COUNT(targets), targets);
                } else if(selectionRequestEvent->target == xcbAtoms.xcbMultiple) {
                    // Multiple conversions requested
                    ASSERT(false);
                } else if(selectionRequestEvent->target == xcbAtoms.xcbSaveTargets) {

                } else {
                    // Conversion to a data target requested
                    if(selectionRequestEvent->target == xcbAtoms.xcbUtf8String) {
                        xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, selectionRequestEvent->requestor, property, selectionRequestEvent->target, 8, clipboardString.size, clipboardString.base);
                    } else {
                        ASSERT(false);
                    }
                }

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
            }
        } break;
        case XCB_SELECTION_NOTIFY:{
            GROUNDED_LOG_INFO("Selection notify\n");
            // Notifies us that a seleciton is now available to us. Probably something we have requested before. For example a drag payload
            
        } break;
        case XCB_SELECTION_CLEAR:{
            //TODO: What to do here?
        } break;
        case 0:{
            // response_type 0 means error
            xcb_generic_error_t* error = (xcb_generic_error_t*)event;
            GROUNDED_PUSH_ERRORF("XCB errorcode: %d sequence:%i\n", error->error_code, error->sequence);
            ASSERT(false);
        } break;
        case XCB_GE_GENERIC:{
            // Some custom event we are probably not interested in
            GROUNDED_LOG_WARNING("Unknown generic event\n");
            //ASSERT(false);
        } break;
        default:{
            // Check for non-constant event types
            if(eventType == xkbEventIndex) {
                if(event->pad0 == XCB_XKB_STATE_NOTIFY) {
                    xcb_xkb_state_notify_event_t* stateNotifyEvent = (xcb_xkb_state_notify_event_t*)event;
                    xkb_state_update_mask(xkbState, stateNotifyEvent->baseMods, stateNotifyEvent->latchedMods, stateNotifyEvent->lockedMods, stateNotifyEvent->baseGroup, stateNotifyEvent->latchedGroup, stateNotifyEvent->lockedGroup);
                    //int x = 0;
                    // Keyboard layout chenged
                    //refreshKeyboardLayout();
                } else if(event->pad0 == XCB_XKB_NEW_KEYBOARD_NOTIFY) {

                } else if(event->pad0 == XCB_XKB_MAP_NOTIFY) {

                } else {
                    ASSERT(false);
                }
            } else {
                ASSERT(false);
            }
        } break;
    }
    
    return result;
}

static void xcbFetchMouseState(GroundedXcbWindow* window, MouseState* mouseState) {
    xcb_query_pointer_cookie_t pointerCookie = xcb_query_pointer(xcbConnection, window->window);
    xcb_generic_error_t * error = 0;
    xcb_flush(xcbConnection);
    xcb_query_pointer_reply_t* pointerReply = xcb_query_pointer_reply(xcbConnection, pointerCookie, &error);
    if(pointerReply && activeXcbWindow == window) {
        // Pointer is on this window
        mouseState->x = pointerReply->win_x;
        mouseState->y = pointerReply->win_y;
        mouseState->mouseInWindow = window->pointerInside;
    } else {
        // Cursor not in window
        ASSERT(!mouseState->scrollDelta);
        mouseState->x = -1;
        mouseState->y = -1;
        mouseState->mouseInWindow = false;
        //GROUNDED_LOG_INFOF("Pointer not in window%u\n", window->window);
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
            free(event);
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
    xcb_flush(xcbConnection);
}

static void xcbSetCursor(xcb_cursor_t cursor) {
    if(xcbCurrentCursor) {
        xcb_free_cursor(xcbConnection, xcbCurrentCursor);
    }
    xcbCurrentCursor = cursor;

    if(hoveredWindow) {
        xcbApplyCurrentCursor(hoveredWindow);
    }
}

static void xcbSetOverwriteCursor(xcb_cursor_t cursor) {
    if(xcbCursorOverwrite) {
        xcb_free_cursor(xcbConnection, xcbCursorOverwrite);
    }
    xcbCursorOverwrite = cursor;

    if(hoveredWindow) {
        xcbApplyCurrentCursor(hoveredWindow);
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
    if(cursorType != currentCursorType) {
        xcb_cursor_t cursor = xcbGetCursorOfType(cursorType);
        xcbSetCursor(cursor);
        ASSERT(xcbCurrentCursor == cursor);
        currentCursorType = cursorType;
    }
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
        GROUNDED_LOG_INFOF("children count: %i\n", childrenCount);
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

static bool xcbIsWindowDndAware(xcb_window_t window) {
    xcb_get_property_cookie_t propertyCookie = xcb_get_property(xcbConnection, 0, window, xcbAtoms.xdndAwareAtom, XCB_GET_PROPERTY_TYPE_ANY, 0, 0);
    xcb_get_property_reply_t* propertyReply = xcb_get_property_reply(xcbConnection, propertyCookie, 0);
    bool dndAware = propertyReply && propertyReply->type != XCB_NONE;
    free(propertyReply);
    return dndAware;
}

static xcb_window_t getXdndAwareTargetQueryTree(int posX, int posY, xcb_window_t window, int maxDepth) {
    xcb_window_t result = 0;

    if(window != xdndSourceData.dragImageWindow && maxDepth > 0) {
        xcb_get_window_attributes_cookie_t attributeCookie = xcb_get_window_attributes(xcbConnection, window);
        xcb_get_window_attributes_reply_t* attributesReply = xcb_get_window_attributes_reply(xcbConnection, attributeCookie, 0);
        if(attributesReply && attributesReply->map_state == XCB_MAP_STATE_VIEWABLE) {
            xcb_get_geometry_cookie_t geometryCookie = xcb_get_geometry(xcbConnection, window);
            xcb_get_geometry_reply_t* geometryReply = xcb_get_geometry_reply(xcbConnection, geometryCookie, 0);
            if(geometryReply && geometryReply->x <= posX && geometryReply->x + geometryReply->width > posX &&
                geometryReply->y <= posY && geometryReply->y + geometryReply->height > posY) {
                // Point lies inside this window. Check if it is dnd aware
                bool dndAware = xcbIsWindowDndAware(window);
                if(dndAware) {
                    result = window; 
                } else {
                    // Query tree to find dnd aware clients at pos
                    xcb_query_tree_cookie_t queryTreeCookie = xcb_query_tree(xcbConnection, window);
                    xcb_query_tree_reply_t* queryTreeReply = xcb_query_tree_reply(xcbConnection, queryTreeCookie, 0);
                    if(queryTreeReply) {
                        if(!queryTreeReply) {
                            return 0;
                        }
                        int childrenCount = xcb_query_tree_children_length(queryTreeReply);
                        xcb_window_t* children = xcb_query_tree_children(queryTreeReply);
                        for(int i = childrenCount; result == 0 && i > 0; i--) {
                            result = getXdndAwareTargetQueryTree(posX - geometryReply->x, posY - geometryReply->y, children[i-1], maxDepth-1);
                        }
                        free(queryTreeReply);
                    }
                }
            }
        }
    }

    ASSERT(result == 0 || xcbIsWindowDndAware(result));
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
            
            bool dndAware = xcbIsWindowDndAware(target);
            if(dndAware) {
                break;
            }
            target = child;
        }

        if(target == 0 || target == xdndSourceData.dragImageWindow) {
            // We have hit nothing or our drag payload image
            target = getXdndAwareTargetQueryTree(rootX, rootY, xcbScreen->root, 8);
            if(target) {
                GROUNDED_LOG_INFO("Found target by query tree search\n");
            }
        }
    }
    ASSERT(target == 0 || xcbIsWindowDndAware(target));
    return target;
}

static void xcbSendDndEnter(xcb_window_t source, xcb_window_t target) {
    GROUNDED_LOG_INFOF("Sending enter from %u to %u\n", source, target);
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
    GROUNDED_LOG_INFOF("Sending position from %u to %u\n", source, target);
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
    int accept = xdndTargetData.currentDropCallback != 0;
    xcb_atom_t action = accept ? xcbAtoms.xdndActionCopyAtom : XCB_NONE;
    if(accept) {
        //accept |= 2;
    }
    GROUNDED_LOG_INFOF("Sending status from %u to %u with accept=%i\n", source, target, accept);
    xcb_client_message_event_t statusEvent = {
        .response_type = XCB_CLIENT_MESSAGE,
        .window = target,
        .type = xcbAtoms.xdndStatusAtom,
        .format = 32,
        .data.data32 = {source, accept, 0, 0, action},
    };
    xcb_send_event(xcbConnection, 0, target, XCB_EVENT_MASK_NO_EVENT, (const char *)&statusEvent);
    xcb_flush(xcbConnection);
}

static void xcbSendDndLeave(xcb_window_t source, xcb_window_t target) {
    GROUNDED_LOG_INFOF("Sending leave from %u to %u\n", source, target);
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
    GROUNDED_LOG_INFOF("Sending drop from %u to %u\n", source, target);
    xcb_client_message_event_t dropEvent = {
        .response_type = XCB_CLIENT_MESSAGE,
        .window = target,
        .type = xcbAtoms.xdndDropAtom,
        .format = 32,
        .data.data32 = {source, 0, XCB_CURRENT_TIME},
    };
    xcb_send_event(xcbConnection, 0, target, XCB_EVENT_MASK_NO_EVENT, (const char *)&dropEvent);
    xcb_flush(xcbConnection);
}

static void xcbSendFinished(xcb_window_t source, xcb_window_t target) {
    GROUNDED_LOG_INFOF("Sending dnd finished from %u to %u\n", source, target);
    int accepted = 1; // 1 for accepted
    xcb_atom_t action = 0;
    if(accepted) {
        action = xcbAtoms.xdndActionCopyAtom;
    }
    xcb_client_message_event_t finishedEvent = {
        .response_type = XCB_CLIENT_MESSAGE,
        .window = target,
        .type = xcbAtoms.xdndFinishedAtom,
        .format = 32,
        .data.data32 = {source, accepted, action},
    };
    xcb_void_cookie_t cookie = xcb_send_event(xcbConnection, 0, target, XCB_EVENT_MASK_NO_EVENT, (const char *)&finishedEvent);
    GROUNDED_LOG_INFOF("Send finished sequence: %u\n", cookie.sequence);
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
    if(desc->xcbIcon) {
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
        GROUNDED_LOG_INFOF("Copy sequence: %i\n", copyCheck.sequence);

        xcb_free_gc(xcbConnection, gc);
        xcb_flush(xcbConnection);
    }

    if(desc->mimeTypeCount > 3) {
        xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, activeXcbWindow->window, xcbAtoms.xdndTypeListAtom, XCB_ATOM_ATOM, 32, desc->mimeTypeCount, (const char*)xdndSourceData.mimeTypes);
    }

    xcbSetOverwriteCursor(xcbGetCursorOfType(GROUNDED_MOUSE_CURSOR_GRABBING));

    GROUNDED_LOG_INFO("Starting drag in xcb\n");
    xdndSourceData.dragActive = true;
}

void groundedXcbSetClipboardText(String8 text) {
    arenaResetToMarker(clipboardArenaMarker);
    clipboardString = str8Copy(&clipboardArena, text);

    //xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, clipboardWindow, xcbAtoms.xcbClipboard, xcbAtoms.xcbUtf8String, 8, text.size, text.base);

    xcb_set_selection_owner(xcbConnection, clipboardWindow, xcbAtoms.xcbClipboard, XCB_CURRENT_TIME);
    
    return;
    //xcb_atom_t target = XCB_ATOM_UTF8_STRING;

    xcb_get_selection_owner_cookie_t ownerCookie = xcb_get_selection_owner(xcbConnection, xcbAtoms.xcbClipboard);
    xcb_get_selection_owner_reply_t* ownerReply = xcb_get_selection_owner_reply(xcbConnection, ownerCookie, 0);
    if(ownerReply) {
        xcb_window_t owner = ownerReply->owner;
        free(ownerReply);

        if(owner == XCB_NONE) {

        }

        arenaResetToMarker(clipboardArenaMarker);
        clipboardString = str8Copy(&clipboardArena, text);

        xcb_set_selection_owner(xcbConnection, clipboardWindow, xcbAtoms.xcbClipboard, XCB_CURRENT_TIME);
        //xcb_change_property(xcbConnection, XCB_PROP_MODE_REPLACE, clipboardWindow, xcbAtoms.xcbClipboard, XCB_ATOM_STRING, 8, text.size, text.base);

        //xcb_flush(xcbConnection);
    }
}

String8 groundedXcbGetClipboardText(MemoryArena* arena) {
    String8 result = EMPTY_STRING8;

    xcb_get_selection_owner_cookie_t ownerCookie = xcb_get_selection_owner(xcbConnection, xcbAtoms.xcbClipboard);
    xcb_get_selection_owner_reply_t* ownerReply = xcb_get_selection_owner_reply(xcbConnection, ownerCookie, 0);
    if(ownerReply && ownerReply->owner != XCB_NONE) {
        xcb_convert_selection(xcbConnection, clipboardWindow, xcbAtoms.xcbClipboard, XCB_ATOM_STRING, XCB_ATOM_STRING, XCB_CURRENT_TIME);
        xcb_flush(xcbConnection);

        // Handle events until we retrieve our data
        //TODO: Some timeout would be a good idea
        while(true) {
            xcb_generic_event_t* event = xcb_wait_for_event(xcbConnection);
            if(event) {
                if((event->response_type & 0x7f) == XCB_SELECTION_NOTIFY) {
                    xcb_selection_notify_event_t *notifyEvent = (xcb_selection_notify_event_t*)event;
                    if(notifyEvent->property != XCB_NONE) {
                        // Retrieve the selected data from the property
                        xcb_get_property_cookie_t propertyCookie = xcb_get_property(xcbConnection, 0, notifyEvent->requestor, notifyEvent->property, XCB_GET_PROPERTY_TYPE_ANY, 0, UINT32_MAX);
                        xcb_get_property_reply_t* propertyReply = xcb_get_property_reply(xcbConnection, propertyCookie, 0);
                        if (propertyReply) {
                            // Process the data as needed
                            result = str8Copy(&xdndTargetData.offerArena, str8FromBlock(xcb_get_property_value(propertyReply), xcb_get_property_value_length(propertyReply)));
                            GROUNDED_LOG_INFOF("Received data: %.*s\n", xcb_get_property_value_length(propertyReply),    (char *)xcb_get_property_value(propertyReply));

                            free(propertyReply);
                        }
                    }
                    free(event);
                    break;
                } else {
                    GroundedEvent result = xcbTranslateToGroundedEvent(event);
                    if(result.type != GROUNDED_EVENT_TYPE_NONE) {
                        eventQueue[eventQueueIndex++] = result;
                    }
                }
            }
        }
    }
    free(ownerReply);

    return result;
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

static void xcbDestroyOpenGLContext(GroundedOpenGLContext* context) {
    eglDestroyContext(xcbEglDisplay, context);
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
        GROUNDED_LOG_ERRORF("Error creating vulkan surface: %s\n", error);
    }
    
    return surface;
}
#endif // GROUNDED_VULKAN_SUPPORT

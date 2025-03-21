#include <grounded/window/grounded_window.h>
//#include <dbus/dbus.h>

GroundedEvent eventQueue[256];
u32 eventQueueIndex;

static const char** getCursorNameCandidates(enum GroundedMouseCursor cursorType, u64* candidateCount);

struct GroundedDragPayload {
    void* userData;
};

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

#define __XCB_H__
//#include <xkbcommon/xkbcommon.h>
//#include <xkbcommon/xkbcommon-x11.h>
#undef __XCB_H__
#include "types/grounded_xkb_types.h"

// xkb function types
#define X(N, R, P) typedef R grounded_##N P;
#include "types/grounded_xkbcommon_functions.h"
#undef X

// xkb function pointers
#define X(N, R, P) static grounded_##N * N = 0;
#include "types/grounded_xkbcommon_functions.h"
#undef X

#include "types/grounded_dbus_types.h"

// dbus function types
#define X(N, R, P) typedef R grounded_##N P;
#include "types/grounded_dbus_functions.h"
#undef X

// dbus function types
#define X(N, R, P) static grounded_##N * N = 0;
#include "types/grounded_dbus_functions.h"
#undef X

struct xkb_context* xkbContext;
struct xkb_state* xkbState;
struct xkb_keymap* xkbKeymap;

DBusConnection* dbusConnection;
DBusError dbusError;
void* dbusLibrary;

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
    const char* firstMissingFunctionName = 0;
    void* xkbCommonLibrary = dlopen("libxkbcommon.so", RTLD_LAZY | RTLD_LOCAL);
    if(xkbCommonLibrary) {
        #define X(N, R, P) N = (grounded_##N*)dlsym(xkbCommonLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
        #include "types/grounded_xkbcommon_functions.h"
        #undef X
        if(firstMissingFunctionName) {
            GROUNDED_PUSH_ERRORF("Could not load xkb function: %s", firstMissingFunctionName);
            dlclose(xkbCommonLibrary);
            xkbContext = 0;
        } else {
            xkbContext = xkb_context_new(0);
        }
    }

    bool skipWayland = false;
    if(!skipWayland && initWayland()) {
        linuxWindowBackend = GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND;
    } else {
        initXcb();
        linuxWindowBackend = GROUNDED_LINUX_WINDOW_BACKEND_XCB;
    }
}

static void shutdownDbus() {
    if(dbusConnection) {
        dbus_connection_unref(dbusConnection);
    }
    if(dbusLibrary) {
        dlclose(dbusLibrary);
        dbusLibrary = 0;
    }
}

static void initDbus() {
    dbusLibrary = dlopen("libdbus-1.so", RTLD_LAZY);
    if(!dbusLibrary) {
        GROUNDED_LOG_INFO("Could not find dbus library (libdbus-1.so)\n");
    } else {
        const char* firstMissingFunctionName = 0;
        #define X(N, R, P) N = (grounded_##N*)dlsym(dbusLibrary, #N); if(!N && !firstMissingFunctionName) {firstMissingFunctionName = #N ;}
        #include "types/grounded_dbus_functions.h"
        #undef X
        if(firstMissingFunctionName) {
            GROUNDED_PUSH_ERRORF("Could not load dbus function: %s", firstMissingFunctionName);
            dlclose(dbusLibrary);
            dbusLibrary = 0;
        }
    }

    dbus_error_init(&dbusError);
    dbusConnection = dbus_bus_get(DBUS_BUS_SESSION, &dbusError);
    if(dbus_error_is_set(&dbusError)) {
        GROUNDED_PUSH_ERROR(dbusError.message);
        dbus_error_free(&dbusError);
        shutdownDbus();
    }
    if(!dbusConnection) {
        GROUNDED_PUSH_ERROR("Could not open dbus connection\n");
        shutdownDbus();
    }
}

GROUNDED_FUNCTION void groundedWindowGetSystemColorTheme() {
    // Env COLORFGBG for terminal foreground and background colors
    
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    if(!dbusConnection) {
        initDbus();
    }
    if(dbusConnection) {
        const char* portalService = "org.freedesktop.portal.Desktop";
        const char* portalObject = "/org/freedesktop/portal/desktop";
        const char* portalInterface = "org.freedesktop.portal.Settings";
        //const char* portalMethod = "OpenFile";

        DBusMessage* message = dbus_message_new_method_call(portalService, portalObject, portalInterface, "ReadAll");
        DBusMessage* reply = 0;
        if(!message) {
            GROUNDED_PUSH_ERROR("Could not create dbus message");
        } else {
            DBusMessageIter args, arrayIter;
            dbus_message_iter_init_append(message, &args);
            dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s", &arrayIter);
            const char* key = "org.freedesktop.appearance";
            dbus_message_iter_append_basic(&arrayIter, DBUS_TYPE_STRING, &key);
            dbus_message_iter_close_container(&args, &arrayIter);

            reply = dbus_connection_send_with_reply_and_block(dbusConnection, message, DBUS_TIMEOUT_INFINITE, &dbusError);
            dbus_message_unref(message);
        }
        if(reply) {
            DBusMessageIter args, dictIter, entryIter, valueIter, innerEntryIter, innerValueIter;
            if(dbus_message_iter_init(reply, &args) && dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_ARRAY) {
                dbus_message_iter_recurse(&args, &dictIter);
                while(dbus_message_iter_get_arg_type(&dictIter) == DBUS_TYPE_DICT_ENTRY) {
                    dbus_message_iter_recurse(&dictIter, &entryIter);
                    char* key = 0;
                    dbus_message_iter_get_basic(&entryIter, &key);
                    dbus_message_iter_next(&entryIter);
                    dbus_message_iter_recurse(&entryIter, &valueIter);
                    while(dbus_message_iter_get_arg_type(&valueIter) == DBUS_TYPE_DICT_ENTRY) {
                        dbus_message_iter_recurse(&valueIter, &innerEntryIter);
                        char* key = 0;
                        dbus_message_iter_get_basic(&innerEntryIter, &key);
                        dbus_message_iter_next(&innerEntryIter);
                        dbus_message_iter_recurse(&innerEntryIter, &innerValueIter);
                        int type = dbus_message_iter_get_arg_type(&innerValueIter);
                        char* value = 0;
                        if(type == DBUS_TYPE_STRING) {
                            dbus_message_iter_get_basic(&innerValueIter, &value);
                        } else if(type == DBUS_TYPE_UINT32) {
                            u32 value = 0;
                            dbus_message_iter_get_basic(&innerValueIter, &value);
                            int x = 0;
                            // color-scheme: 0: no preference 1: dark 2: light
                        } else if(type == DBUS_TYPE_STRUCT) {
                            // RGB in SRGB format
                        }
                        dbus_message_iter_next(&valueIter);
                    }
                    dbus_message_iter_next(&dictIter);
                }
            }
        }
    }
    arenaEndTemp(temp);
}

String8* dbusWaitForFileDialog(MemoryArena* arena, char* path, u32* outResultCount) {
	DBusMessage *msg;
	DBusMessageIter args, opts, dict, var, uris;
    bool responseReceived = false;
    String8* result = 0;
    u32 resultCount = 0;

    while(!responseReceived) {
        dbus_connection_read_write(dbusConnection, 1000); // 1s timeout
        msg = dbus_connection_pop_message(dbusConnection);

        if (msg) {
            if (dbus_message_is_signal(msg, "org.freedesktop.portal.Request", "Response") && !strcmp(dbus_message_get_path(msg), path)) {
                responseReceived = true;
                if (dbus_message_iter_init(msg, &args) && dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_UINT32) {
                    u32 response = 0; // 0 for success, 1 for user cancel, 2 for error.
                    dbus_message_iter_get_basic(&args, &response); 
                    
                    if (response == 0 && dbus_message_iter_next(&args) && dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_ARRAY) {
                        dbus_message_iter_recurse(&args, &opts);
                        for (; dbus_message_iter_get_arg_type(&opts) == DBUS_TYPE_DICT_ENTRY; dbus_message_iter_next(&opts)) {
                            dbus_message_iter_recurse(&opts, &dict);
                            if (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_STRING) {
                                const char *optname;
                                dbus_message_iter_get_basic(&dict, &optname);
                                if (dbus_message_iter_next(&dict) && dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_VARIANT) {
                                    dbus_message_iter_recurse(&dict, &var);
                                    if (!strcmp(optname, "uris") && dbus_message_iter_get_arg_type(&var) == DBUS_TYPE_ARRAY) {
                                        s32 uriCount = dbus_message_iter_get_element_count(&var);
                                        ASSERT(uriCount);
                                        result = ARENA_PUSH_ARRAY(arena, uriCount, String8);

                                        dbus_message_iter_recurse(&var, &uris);
                                        for (; dbus_message_iter_get_arg_type(&uris) == DBUS_TYPE_STRING; dbus_message_iter_next(&uris)) {
                                            const char *filename;
                                            dbus_message_iter_get_basic(&uris, &filename);
                                            result[resultCount++] = str8CopyAndNullTerminate(arena, str8FromCstr(filename));
                                            //printf("File:%s\n", filename);
                                            //resolveFileURI();
                                        }
                                        ASSERT(resultCount == uriCount);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            dbus_message_unref(msg);
        }
    }
    if(outResultCount) {
        *outResultCount = resultCount;
    }
    return result;
}

// Using https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.FileChooser.html
// https://github.com/godotengine/godot/blob/89001f91d21ebd08b59756841426f540b154b97d/platform/linuxbsd/freedesktop_portal_desktop.cpp#L53
GROUNDED_FUNCTION String8* groundedWindowOpenFileDialog(GroundedWindow* window, MemoryArena* arena, u32* outFileCount, struct GroundedFileDialogParameters* parameters) {
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    if(!dbusConnection) {
        initDbus();
    }
    String8* result = 0;
    u32 resultCount = 0;
    if(dbusConnection) {
        const char* portalService = "org.freedesktop.portal.Desktop";
        const char* portalObject = "/org/freedesktop/portal/desktop";
        const char* portalInterface = "org.freedesktop.portal.FileChooser";
        const char* portalMethod = "OpenFile";
        String8 dbusFilter = EMPTY_STRING8;
        String8 path = EMPTY_STRING8;

        DBusMessage *message = dbus_message_new_method_call(portalService, portalObject, portalInterface, portalMethod);
        if(!message) {
            GROUNDED_PUSH_ERROR("Could not create dbus message");
        } else {
            DBusMessageIter args;
            dbus_message_iter_init_append(message, &args);

            const char* parentWindow = "";
            const char* title = "Open File";
            if(window) {
                if(linuxWindowBackend == GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND) {
                    GroundedWaylandWindow* waylandWindow = (GroundedWaylandWindow*)window;
                    String8 parentString = str8FromFormat(scratch, "wayland:%s", waylandWindow->foreignHandle);
                    parentWindow = (const char*)parentString.base;
                    if(!str8IsEmpty(waylandWindow->title)) {
                        title = (const char*)str8FromFormat(scratch, "Open File - %s", (const char*)waylandWindow->title.base).base;
                    }
                } else if(linuxWindowBackend == GROUNDED_LINUX_WINDOW_BACKEND_XCB) {
                    GroundedXcbWindow* xcbWindow = (GroundedXcbWindow*)window;
                    String8 parentString = str8FromFormat(scratch, "x11:%x", xcbWindow->window);
                    parentWindow = (const char*)parentString.base;
                }
            }
            
            dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &parentWindow);
            dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &title);

            // Options (empty dictionary for no extra options)
            DBusMessageIter options;
            dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &options);

            const char* token = "27424772";
            {
                DBusMessageIter dictIter;
                DBusMessageIter variantIter;
                DBusMessageIter arrayIter;
                const char* key = "handle_token";
                dbus_message_iter_open_container(&options, DBUS_TYPE_DICT_ENTRY, 0, &dictIter);
                dbus_message_iter_append_basic(&dictIter, DBUS_TYPE_STRING, &key);

                dbus_message_iter_open_container(&dictIter, DBUS_TYPE_VARIANT, "s", &variantIter);
                dbus_message_iter_append_basic(&variantIter, DBUS_TYPE_STRING, &token);

                dbus_message_iter_close_container(&dictIter, &variantIter);
                dbus_message_iter_close_container(&options, &dictIter);
            }

            // Apply filters
            if(parameters && parameters->filterCount > 0) {
                DBusMessageIter dictIter;
                DBusMessageIter variantIter;
                DBusMessageIter arrayIter;
                const char* filtersKey = "filters";

                dbus_message_iter_open_container(&options, DBUS_TYPE_DICT_ENTRY, 0, &dictIter);
                dbus_message_iter_append_basic(&dictIter, DBUS_TYPE_STRING, &filtersKey);
                dbus_message_iter_open_container(&dictIter, DBUS_TYPE_VARIANT, "a(sa(us))", &variantIter);
                dbus_message_iter_open_container(&variantIter, DBUS_TYPE_ARRAY, "(sa(us))", &arrayIter);

                for(u32 i = 0; i < parameters->filterCount; ++i) {
                    DBusMessageIter structIter;
                    DBusMessageIter patternIter;
                    DBusMessageIter patternStructIter;
                    dbus_message_iter_open_container(&arrayIter, DBUS_TYPE_STRUCT, 0, &structIter);
                    const char* name = str8GetCstr(scratch, parameters->filters[i].name);
                    dbus_message_iter_append_basic(&structIter, DBUS_TYPE_STRING, &name);

                    // Array of filter patterns
                    dbus_message_iter_open_container(&structIter, DBUS_TYPE_ARRAY, "(us)", &patternIter);
                    u8 splitCharacter = ';';
                    String8List patternList = str8Split(scratch, parameters->filters[i].filterString, &splitCharacter, 1);
                    String8Node* patternNode = patternList.first;
                    while(patternNode) {
                        dbus_message_iter_open_container(&patternIter, DBUS_TYPE_STRUCT, 0, &patternStructIter);
                        
                        u32 filterIsMime = 0;
                        if(str8GetFirstOccurence(patternNode->string, '/') < patternNode->string.size) {
                            filterIsMime = 1;
                        }

                        //TODO: For different capitalizations use instead of *.ico  use *.[iI][cC][oO]
                        const char* patternString = "";
                        if(str8GetFirstOccurence(patternNode->string, '.') == 0) {
                            // First character is a . which is windows style. we need a * before it
                            patternString = (const char*)str8FromFormat(scratch, "*%.*s", patternNode->string.size, patternNode->string.base).base;
                        } else {
                            patternString = str8GetCstr(scratch, patternNode->string);
                        }
                        
                        dbus_message_iter_append_basic(&patternStructIter, DBUS_TYPE_UINT32, &filterIsMime);
                        dbus_message_iter_append_basic(&patternStructIter, DBUS_TYPE_STRING, &patternString);

                        dbus_message_iter_close_container(&patternIter, &patternStructIter);
                        patternNode = patternNode->next;
                    }                    
                    
                    dbus_message_iter_close_container(&structIter, &patternIter);
		            dbus_message_iter_close_container(&arrayIter, &structIter);
                }

                dbus_message_iter_close_container(&variantIter, &arrayIter);
                dbus_message_iter_close_container(&dictIter, &variantIter);
                dbus_message_iter_close_container(&options, &dictIter);
            }

            if(parameters && parameters->chooseDirectories) {
                DBusMessageIter dictIter;
                DBusMessageIter variantIter;
                const char* key = "directory";
                dbus_message_iter_open_container(&options, DBUS_TYPE_DICT_ENTRY, 0, &dictIter);
                dbus_message_iter_append_basic(&dictIter, DBUS_TYPE_STRING, &key);

                dbus_message_iter_open_container(&dictIter, DBUS_TYPE_VARIANT, "b", &variantIter);
                int val = 1;
                dbus_message_iter_append_basic(&variantIter, DBUS_TYPE_BOOLEAN, &val);

                dbus_message_iter_close_container(&dictIter, &variantIter);
                dbus_message_iter_close_container(&options, &dictIter);
            }

            if(parameters && parameters->multiSelect) {
                DBusMessageIter dictIter;
                DBusMessageIter variantIter;
                const char* key = "multiple";
                dbus_message_iter_open_container(&options, DBUS_TYPE_DICT_ENTRY, 0, &dictIter);
                dbus_message_iter_append_basic(&dictIter, DBUS_TYPE_STRING, &key);

                dbus_message_iter_open_container(&dictIter, DBUS_TYPE_VARIANT, "b", &variantIter);
                int val = 1;
                dbus_message_iter_append_basic(&variantIter, DBUS_TYPE_BOOLEAN, &val);

                dbus_message_iter_close_container(&dictIter, &variantIter);
                dbus_message_iter_close_container(&options, &dictIter);
            }

            if(parameters && !str8IsEmpty(parameters->currentDirectory)) {
                DBusMessageIter dictIter;
                DBusMessageIter variantIter;
                DBusMessageIter arrayIter;
                const char* key = "current_folder";
                dbus_message_iter_open_container(&options, DBUS_TYPE_DICT_ENTRY, 0, &dictIter);
                dbus_message_iter_append_basic(&dictIter, DBUS_TYPE_STRING, &key);

                dbus_message_iter_open_container(&dictIter, DBUS_TYPE_VARIANT, "ay", &variantIter);
                dbus_message_iter_open_container(&variantIter, DBUS_TYPE_ARRAY, "y", &arrayIter);
                
                for(u64 i = 0; i < parameters->currentDirectory.size; ++i) {
                    dbus_message_iter_append_basic(&arrayIter, DBUS_TYPE_BYTE, &parameters->currentDirectory.base[i]);
                }
                char nullTerminator = 0;
                dbus_message_iter_append_basic(&arrayIter, DBUS_TYPE_BYTE, &nullTerminator);

                dbus_message_iter_close_container(&variantIter, &arrayIter);
                dbus_message_iter_close_container(&dictIter, &variantIter);
                dbus_message_iter_close_container(&options, &dictIter);
            }
            // Options includes:
            // handletoken: 
            // acceptLabel: What should be written on the open button
            // choices:
            dbus_message_iter_close_container(&args, &options);

            // Register for response
            const char* dbusUniqueName = dbus_bus_get_unique_name(dbusConnection);
            String8 dbusConvertedUniqueName = str8ReplaceCharacter(scratch, str8FromCstr(dbusUniqueName), '.', '_');
            dbusConvertedUniqueName = str8RemoveCharacter(scratch, dbusConvertedUniqueName, ':');
            path = str8FromFormat(scratch, "/org/freedesktop/portal/desktop/request/%s/%s", (const char*)dbusConvertedUniqueName.base, token);
            dbusFilter = str8FromFormat(scratch, "type='signal',sender='org.freedesktop.portal.Desktop',path='%s',interface='org.freedesktop.portal.Request',member='Response',destination='%s'", (const char*)path.base, dbusUniqueName);
            dbus_bus_add_match(dbusConnection, (const char*)dbusFilter.base, &dbusError);
            if(dbus_error_is_set(&dbusError)) {
                dbus_error_free(&dbusError);
                GROUNDED_PUSH_ERROR("Failed to add DBus match");
                dbus_message_unref(message);
                message = 0;
            }
        }

        if(message) {
            DBusMessage *reply = dbus_connection_send_with_reply_and_block(dbusConnection, message, DBUS_TIMEOUT_INFINITE, &dbusError);
            dbus_message_unref(message);
            message = 0;
            if(!reply || dbus_error_is_set(&dbusError)) {
                GROUNDED_PUSH_ERRORF("Failed to send DBus message: %s", dbusError.message);
                dbus_error_free(&dbusError);
                reply = 0;
            } else {
                // Now we need to wait for response
                DBusMessageIter iter;
                if (dbus_message_iter_init(reply, &iter)) {
                    if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_OBJECT_PATH) {
                        const char *newPath = 0;
                        dbus_message_iter_get_basic(&iter, &newPath);
                        if(!str8IsEqual(str8FromCstr(newPath), path)) {
                            dbus_bus_remove_match(dbusConnection, (const char*)dbusFilter.base, &dbusError);
                            if (dbus_error_is_set(&dbusError)) {
                                GROUNDED_PUSH_ERRORF("Failed to remove DBus match: %s", dbusError.message);
                                dbus_error_free(&dbusError);
                            }
                            path = str8FromCstr(newPath);
                            dbus_bus_add_match(dbusConnection, (const char*)dbusFilter.base, &dbusError);
                            if (dbus_error_is_set(&dbusError)) {
                                GROUNDED_PUSH_ERRORF("Failed to add DBus match: %s", dbusError.message);
                                dbus_error_free(&dbusError);
                            }
                        }
                    }
                }

                dbus_message_unref(reply);
                result = dbusWaitForFileDialog(arena, (char*)path.base, &resultCount);
                
                dbus_bus_remove_match(dbusConnection, (const char*)dbusFilter.base, &dbusError);
                if (dbus_error_is_set(&dbusError)) {
                    GROUNDED_PUSH_ERRORF("Failed to remove DBus match: %s", dbusError.message);
                    dbus_error_free(&dbusError);
                }
            }
        }
    }
    arenaEndTemp(temp);
    if(outFileCount) {
        *outFileCount = resultCount;
    }
    return result;
}

// On Gnome this should be available
static String8 dbusGetCursorThemeName(MemoryArena* arena) {
    String8 result = EMPTY_STRING8;

    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    if(!dbusConnection) {
        initDbus();
    }

    if(dbusConnection) {
        // https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Settings.html
        // Available for query: color-scheme, accent-color, contrast

        // Try first with gnome
        const char* portalService = "org.freedesktop.portal.Desktop";
        const char* portalObject = "/org/freedesktop/portal/desktop";
        const char* portalInterface = "org.freedesktop.portal.Settings";
        const char* portalMethod = "Read";

        DBusMessage *message = dbus_message_new_method_call(portalService, portalObject, portalInterface, portalMethod);
        if(!message) {
            GROUNDED_PUSH_ERROR("Could not create dbus message");
        } else {
            // Add data to message
            DBusMessageIter args;
            dbus_message_iter_init_append(message, &args);

            // https://github.com/GNOME/gsettings-desktop-schemas/blob/master/schemas/org.gnome.desktop.interface.gschema.xml.in
            static const char* namespace = "org.gnome.desktop.interface";
            dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &namespace);

            static const char* key = "cursor-theme"; // Also available: "cursor-size"
            dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &key);
        }

        if(message) {
            DBusMessage *reply = dbus_connection_send_with_reply_and_block(dbusConnection, message, DBUS_TIMEOUT_INFINITE, &dbusError);
            dbus_message_unref(message);

            if(dbus_error_is_set(&dbusError) || !reply) {

            } else {
                DBusMessageIter args, variantIter;
                if(dbus_message_iter_init(reply, &args) && dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_VARIANT) {
                    dbus_message_iter_recurse(&args, &variantIter);

                    if (dbus_message_iter_get_arg_type(&variantIter) == DBUS_TYPE_STRING) {
                        const char* setting_value = 0;
                        dbus_message_iter_get_basic(&variantIter, &setting_value);
                        GROUNDED_LOG_INFOF("Setting Value: %s\n", setting_value);
                    } else {
                        GROUNDED_LOG_INFO("Unexpected value type in reply\n");
                    }
                }
            }
        }
    }
    
    arenaEndTemp(temp);
    return result;
}

GROUNDED_FUNCTION GroundedWindowBackend groundedWindowSystemGetSelectedBackend() {
    GroundedWindowBackend result = GROUNDED_WINDOW_BACKEND_NONE;
    if(linuxWindowBackend == GROUNDED_LINUX_WINDOW_BACKEND_XCB) {
        result = GROUNDED_WINDOW_BACKEND_XCB;
    } else if(linuxWindowBackend == GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND) {
        result = GROUNDED_WINDOW_BACKEND_WAYLAND;
    }
    return result;
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
    shutdownDbus();
}

GROUNDED_FUNCTION GroundedWindow* groundedCreateWindow(MemoryArena* arena, struct GroundedWindowCreateParameters* parameters) {
    ASSERT(arena);
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

GROUNDED_FUNCTION void groundedWindowMinimize(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandWindowMinimize((GroundedWaylandWindow*)window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbWindowMinimize((GroundedXcbWindow*)window);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION void groundedWindowSetMaximized(GroundedWindow* window, bool maximized) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandWindowSetMaximized((GroundedWaylandWindow*)window, maximized);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            xcbWindowSetMaximized((GroundedXcbWindow*)window, maximized);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION bool groundedWindowIsFullscreen(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandWindowIsFullscreen((GroundedWaylandWindow*)window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbWindowIsFullscreen((GroundedXcbWindow*)window);
        } break;
        default:break;
    }
    return false;
}

GROUNDED_FUNCTION bool groundedWindowIsMaximized(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandWindowIsMaximized((GroundedWaylandWindow*)window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbWindowIsMaximized((GroundedXcbWindow*)window);
        } break;
        default:break;
    }
    return false;
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

    ASSERT(sizeof(keyboardState->lastKeys) == sizeof(keyboardState->keys));
    memcpy(keyboardState->lastKeys, keyboardState->keys, sizeof(keyboardState->keys));

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
    mouseState->windowWidth = groundedWindowGetWidth(window);
    mouseState->windowHeight = groundedWindowGetHeight(window);

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

GROUNDED_FUNCTION GroundedWindowFramebuffer groundedWindowGetFramebuffer(GroundedWindow* window) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);

    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return waylandGetFramebuffer((GroundedWaylandWindow*)window);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            //return xcbGetFramebuffer((GroundedXcbWindow*)window);
        } break;
        default:break;
    }
    return (GroundedWindowFramebuffer){0};
}

GROUNDED_FUNCTION void groundedWindowSubmitFramebuffer(GroundedWindow* window, GroundedWindowFramebuffer* framebuffer) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);

    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            waylandSubmitFramebuffer((GroundedWaylandWindow*)window, framebuffer);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            //xcbSubmitFramebuffer((GroundedXcbWindow*)window, framebuffer);
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
    static const char* allDirResizeCursors[] = {"all-scroll", "fleur"}; //TODO: Is this correct?

    static const char* crosshairCursors[] = {"crosshair", "cross"};
    static const char* verticalTextCursors[] = {"vertical-text"};
    static const char* cellCursors[] = {"cell", "plus"};
    static const char* contextMenuCursors[] = {"context-menu"};
    // Not useful in a practical sense but interestig nonetheless
    static const char* specialCursors[] = {"dot", "pirate", "heart"};

    static const char* grabbableCursors[] = {"openhand", "grab", "hand1", "closedhand", "grabbing", "hand2"};
    static const char* grabbingCursors[] = {"closedhand", "grabbing", "hand2", "grab", "hand1"};

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
        case GROUNDED_MOUSE_CURSOR_ALLDIR:{
            USE_CURSOR_CANDIDATE(allDirResizeCursors);
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
        case GROUNDED_MOUSE_CURSOR_GRABBABLE:{
            USE_CURSOR_CANDIDATE(grabbableCursors);
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

GROUNDED_FUNCTION void groundedWindowSetClipboardText(String8 text) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            ASSERT(false);
            //groundedWaylandSetClipboardText(text);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            groundedXcbSetClipboardText(text);
        } break;
        default:break;
    }
}

GROUNDED_FUNCTION String8 groundedWindowGetClipboardText(MemoryArena* arena) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            ASSERT(false);
            //return groundedWaylandSetClipboardText(text);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return groundedXcbGetClipboardText(arena);
        } break;
        default:break;
    }
    return EMPTY_STRING8;
}

GROUNDED_FUNCTION void groundedWindowDragPayloadSetUserData(struct GroundedDragPayload* payload, void* userData) {
    GROUNDED_LOG_INFO("Setting drag payload user data\n");
    if(payload) {
        payload->userData = userData;
    }
}

GROUNDED_FUNCTION void* groundedWindowDragPayloadGetUserData(struct  GroundedDragPayload* payload) {
    void* result = 0;
    if(payload) {
        result = payload->userData;
    }
    return result;
}

GROUNDED_FUNCTION String8 groundedGetNameOfKeycode(MemoryArena* arena, u32 keycode) {
    ASSERT(linuxWindowBackend != GROUNDED_LINUX_WINDOW_BACKEND_NONE);
    switch(linuxWindowBackend) {
        case GROUNDED_LINUX_WINDOW_BACKEND_WAYLAND:{
            return groundedWaylandGetNameOfKeycode(arena, keycode);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            ASSERT(false);
            //return groundedXcbGetClipboardText(arena);
        } break;
        default:break;
    }
    return EMPTY_STRING8;
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
                GROUNDED_LOG_WARNINGF("Could not load egl function: %s\n", firstMissingFunctionName);
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
    return 0;
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
            return waylandGetVulkanSurface((GroundedWaylandWindow*)window, instance);
        } break;
        case GROUNDED_LINUX_WINDOW_BACKEND_XCB:{
            return xcbGetVulkanSurface((GroundedXcbWindow*)window, instance);
        }break;
        default:break;
    }
    return 0;
}
#endif // GROUNDED_VULKAN_SUPPORT

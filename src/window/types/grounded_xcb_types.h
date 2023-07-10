#ifndef GROUNDED_XCB_TYPES_H
#define GROUNDED_XCB_TYPES_H

// Event types
#define XCB_KEY_PRESS 2
#define XCB_KEY_RELEASE 3
#define XCB_BUTTON_PRESS 4
#define XCB_BUTTON_RELEASE 5
#define XCB_MOTION_NOTIFY 6
// #define XCB_ENTER_NOTIFY 7
// #define XCB_LEAVE_NOTIFY 8
// #define XCB_FOCUS_IN 9
// #define XCB_FOCUS_OUT 10
// #define XCB_KEYMAP_NOTIFY 11
// #define XCB_EXPOSE 12
// #define XCB_GRAPHICS_EXPOSURE 13
// #define XCB_NO_EXPOSURE 14
#define XCB_VISIBILITY_NOTIFY 15
#define XCB_CREATE_NOTIFY 16
// #define XCB_DESTROY_NOTIFY 17
#define XCB_UNMAP_NOTIFY 18
#define XCB_MAP_NOTIFY 19
// #define XCB_MAP_REQUEST 20
#define XCB_REPARENT_NOTIFY 21
#define XCB_CONFIGURE_NOTIFY 22
// #define XCB_CONFIGURE_REQUEST 23
// #define XCB_GRAVITY_NOTIFY 24
#define XCB_RESIZE_REQUEST 25
// #define XCB_CIRCULATE_NOTIFY 26
// #define XCB_CIRCULATE_REQUEST 27
// #define XCB_PROPERTY_NOTIFY 28
// #define XCB_SELECTION_CLEAR 29
// #define XCB_SELECTION_REQUEST 30
// #define XCB_SELECTION_NOTIFY 31
// #define XCB_COLORMAP_NOTIFY 32
#define XCB_CLIENT_MESSAGE 33
#define XCB_MAPPING_NOTIFY 34
// #define XCB_GE_GENERIC 35

#define XCB_COPY_FROM_PARENT 0L

#define XCB_CURRENT_TIME 0L

#define XCB_NONE 0L

typedef struct xcb_connection_t xcb_connection_t;  // Opaque structure containing all data that  XCB needs to communicate with an X server
typedef struct xcb_setup_t xcb_setup_t;
typedef struct xcb_cursor_context_t xcb_cursor_context_t;

typedef uint32_t xcb_timestamp_t;
typedef uint32_t xcb_window_t;
typedef uint32_t xcb_atom_t;
typedef uint8_t xcb_keycode_t;
typedef uint32_t xcb_drawable_t;
typedef uint32_t xcb_visualid_t;
typedef uint32_t xcb_colormap_t;
typedef uint8_t xcb_button_t;
typedef uint32_t xcb_gcontext_t;
typedef uint32_t xcb_pixmap_t;
typedef uint32_t xcb_cursor_t;
typedef uint32_t xcb_font_t;

typedef struct xcb_screen_t {
    xcb_window_t   root;
    xcb_colormap_t default_colormap;
    uint32_t       white_pixel;
    uint32_t       black_pixel;
    uint32_t       current_input_masks;
    uint16_t       width_in_pixels;
    uint16_t       height_in_pixels;
    uint16_t       width_in_millimeters;
    uint16_t       height_in_millimeters;
    uint16_t       min_installed_maps;
    uint16_t       max_installed_maps;
    xcb_visualid_t root_visual;
    uint8_t        backing_stores;
    uint8_t        save_unders;
    uint8_t        root_depth;
    uint8_t        allowed_depths_len;
} xcb_screen_t;

typedef struct {
    unsigned int sequence;  /**< Sequence number */
} xcb_void_cookie_t;

typedef struct xcb_intern_atom_cookie_t {
    unsigned int sequence;
} xcb_intern_atom_cookie_t;

typedef struct {
    uint8_t   response_type;  /**< Type of the response */
    uint8_t   error_code;     /**< Error code */
    uint16_t sequence;       /**< Sequence number */
    uint32_t resource_id;     /** < Resource ID for requests with side effects only */
    uint16_t minor_code;      /** < Minor opcode of the failed request */
    uint8_t major_code;       /** < Major opcode of the failed request */
    uint8_t pad0;
    uint32_t pad[5];         /**< Padding */
    uint32_t full_sequence;  /**< full sequence */
} xcb_generic_error_t;

typedef struct xcb_screen_iterator_t {
    xcb_screen_t *data;
    int           rem;
    int           index;
} xcb_screen_iterator_t;

typedef enum xcb_window_class_t {
    XCB_WINDOW_CLASS_COPY_FROM_PARENT = 0,
    XCB_WINDOW_CLASS_INPUT_OUTPUT = 1,
    XCB_WINDOW_CLASS_INPUT_ONLY = 2
} xcb_window_class_t;

typedef struct {
    uint8_t   response_type;  /**< Type of the response */
    uint8_t  pad0;           /**< Padding */
    uint16_t sequence;       /**< Sequence number */
    uint32_t pad[7];         /**< Padding */
    uint32_t full_sequence;  /**< full sequence */
} xcb_generic_event_t;

typedef struct xcb_key_press_event_t {
    uint8_t         response_type;
    xcb_keycode_t   detail;
    uint16_t        sequence;
    xcb_timestamp_t time;
    xcb_window_t    root;
    xcb_window_t    event;
    xcb_window_t    child;
    int16_t         root_x;
    int16_t         root_y;
    int16_t         event_x;
    int16_t         event_y;
    uint16_t        state;
    uint8_t         same_screen;
    uint8_t         pad0;
} xcb_key_press_event_t;

typedef xcb_key_press_event_t xcb_key_release_event_t;

typedef struct xcb_resize_request_event_t {
    uint8_t      response_type;
    uint8_t      pad0;
    uint16_t     sequence;
    xcb_window_t window;
    uint16_t     width;
    uint16_t     height;
} xcb_resize_request_event_t;

typedef struct xcb_configure_notify_event_t {
    uint8_t      response_type;
    uint8_t      pad0;
    uint16_t     sequence;
    xcb_window_t event;
    xcb_window_t window;
    xcb_window_t above_sibling;
    int16_t      x;
    int16_t      y;
    uint16_t     width;
    uint16_t     height;
    uint16_t     border_width;
    uint8_t      override_redirect;
    uint8_t      pad1;
} xcb_configure_notify_event_t;

typedef struct xcb_mapping_notify_event_t {
    uint8_t       response_type;
    uint8_t       pad0;
    uint16_t      sequence;
    uint8_t       request;
    xcb_keycode_t first_keycode;
    uint8_t       count;
    uint8_t       pad1;
} xcb_mapping_notify_event_t;

typedef enum xcb_mapping_t {
    XCB_MAPPING_MODIFIER = 0,
    XCB_MAPPING_KEYBOARD = 1,
    XCB_MAPPING_POINTER = 2
} xcb_mapping_t;

typedef struct xcb_get_keyboard_mapping_cookie_t {
    unsigned int sequence;
} xcb_get_keyboard_mapping_cookie_t;

typedef struct xcb_get_keyboard_mapping_reply_t {
    uint8_t  response_type;
    uint8_t  keysyms_per_keycode;
    uint16_t sequence;
    uint32_t length;
    uint8_t  pad0[24];
} xcb_get_keyboard_mapping_reply_t;

typedef struct xcb_query_pointer_cookie_t {
    unsigned int sequence;
} xcb_query_pointer_cookie_t;

typedef uint32_t xcb_render_picture_t;

typedef uint32_t xcb_render_pictformat_t;

typedef struct xcb_render_query_pict_formats_cookie_t {
    unsigned int sequence;
} xcb_render_query_pict_formats_cookie_t;

typedef uint32_t xcb_render_pictformat_t;

typedef enum xcb_render_pict_type_t {
    XCB_RENDER_PICT_TYPE_INDEXED = 0,
    XCB_RENDER_PICT_TYPE_DIRECT = 1
} xcb_render_pict_type_t;

typedef struct xcb_render_query_pict_formats_reply_t {
    uint8_t  response_type;
    uint8_t  pad0;
    uint16_t sequence;
    uint32_t length;
    uint32_t num_formats;
    uint32_t num_screens;
    uint32_t num_depths;
    uint32_t num_visuals;
    uint32_t num_subpixel;
    uint8_t  pad1[4];
} xcb_render_query_pict_formats_reply_t;

typedef struct xcb_render_directformat_t {
    uint16_t red_shift;
    uint16_t red_mask;
    uint16_t green_shift;
    uint16_t green_mask;
    uint16_t blue_shift;
    uint16_t blue_mask;
    uint16_t alpha_shift;
    uint16_t alpha_mask;
} xcb_render_directformat_t;

typedef struct xcb_render_pictforminfo_t {
    xcb_render_pictformat_t   id;
    uint8_t                   type;
    uint8_t                   depth;
    uint8_t                   pad0[2];
    xcb_render_directformat_t direct;
    xcb_colormap_t            colormap;
} xcb_render_pictforminfo_t;

typedef struct xcb_query_pointer_reply_t {
    uint8_t      response_type;
    uint8_t      same_screen;
    uint16_t     sequence;
    uint32_t     length;
    xcb_window_t root;
    xcb_window_t child;
    int16_t      root_x;
    int16_t      root_y;
    int16_t      win_x;
    int16_t      win_y;
    uint16_t     mask;
    uint8_t      pad0[2];
} xcb_query_pointer_reply_t;

typedef enum xcb_pict_standard_t {
	XCB_PICT_STANDARD_ARGB_32,
	XCB_PICT_STANDARD_RGB_24,
	XCB_PICT_STANDARD_A_8,
	XCB_PICT_STANDARD_A_4,
	XCB_PICT_STANDARD_A_1
} xcb_pict_standard_t;

typedef struct xcb_depth_t {
    uint8_t  depth;
    uint8_t  pad0;
    uint16_t visuals_len;
    uint8_t  pad1[4];
} xcb_depth_t;

typedef struct xcb_depth_iterator_t {
    xcb_depth_t *data;
    int          rem;
    int          index;
} xcb_depth_iterator_t;

typedef enum xcb_image_format_t {
    XCB_IMAGE_FORMAT_XY_BITMAP = 0,
    XCB_IMAGE_FORMAT_XY_PIXMAP = 1,
    XCB_IMAGE_FORMAT_Z_PIXMAP = 2
} xcb_image_format_t;

typedef struct xcb_shm_create_segment_cookie_t {
    unsigned int sequence;
} xcb_shm_create_segment_cookie_t;

typedef enum xcb_gc_t {
    XCB_GC_FUNCTION = 1,
    XCB_GC_PLANE_MASK = 2,
    XCB_GC_FOREGROUND = 4,
    XCB_GC_BACKGROUND = 8,
    XCB_GC_LINE_WIDTH = 16,
    XCB_GC_LINE_STYLE = 32,
    XCB_GC_CAP_STYLE = 64,
    XCB_GC_JOIN_STYLE = 128,
    XCB_GC_FILL_STYLE = 256,
    XCB_GC_FILL_RULE = 512,
    XCB_GC_TILE = 1024,
    XCB_GC_STIPPLE = 2048,
    XCB_GC_TILE_STIPPLE_ORIGIN_X = 4096,
    XCB_GC_TILE_STIPPLE_ORIGIN_Y = 8192,
    XCB_GC_FONT = 16384,
    XCB_GC_SUBWINDOW_MODE = 32768,
    XCB_GC_GRAPHICS_EXPOSURES = 65536,
    XCB_GC_CLIP_ORIGIN_X = 131072,
    XCB_GC_CLIP_ORIGIN_Y = 262144,
    XCB_GC_CLIP_MASK = 524288,
    XCB_GC_DASH_OFFSET = 1048576,
    XCB_GC_DASH_LIST = 2097152,
    XCB_GC_ARC_MODE = 4194304
} xcb_gc_t;

typedef uint32_t xcb_shm_seg_t;

typedef struct xcb_shm_query_version_cookie_t {
    unsigned int sequence;
} xcb_shm_query_version_cookie_t;

typedef enum xcb_prop_mode_t {
    XCB_PROP_MODE_REPLACE = 0,
    XCB_PROP_MODE_PREPEND = 1,
    XCB_PROP_MODE_APPEND = 2
} xcb_prop_mode_t;

typedef struct xcb_intern_atom_reply_t {
    uint8_t    response_type;
    uint8_t    pad0;
    uint16_t   sequence;
    uint32_t   length;
    xcb_atom_t atom;
} xcb_intern_atom_reply_t;

typedef struct xcb_shm_query_version_reply_t {
    uint8_t  response_type;
    uint8_t  shared_pixmaps;
    uint16_t sequence;
    uint32_t length;
    uint16_t major_version;
    uint16_t minor_version;
    uint16_t uid;
    uint16_t gid;
    uint8_t  pixmap_format;
    uint8_t  pad0[15];
} xcb_shm_query_version_reply_t;

typedef uint32_t xcb_keysym_t;

typedef enum xcb_atom_enum_t {
    XCB_ATOM_NONE = 0,
    XCB_ATOM_ANY = 0,
    XCB_ATOM_PRIMARY = 1,
    XCB_ATOM_SECONDARY = 2,
    XCB_ATOM_ARC = 3,
    XCB_ATOM_ATOM = 4,
    XCB_ATOM_BITMAP = 5,
    XCB_ATOM_CARDINAL = 6,
    XCB_ATOM_COLORMAP = 7,
    XCB_ATOM_CURSOR = 8,
    XCB_ATOM_CUT_BUFFER0 = 9,
    XCB_ATOM_CUT_BUFFER1 = 10,
    XCB_ATOM_CUT_BUFFER2 = 11,
    XCB_ATOM_CUT_BUFFER3 = 12,
    XCB_ATOM_CUT_BUFFER4 = 13,
    XCB_ATOM_CUT_BUFFER5 = 14,
    XCB_ATOM_CUT_BUFFER6 = 15,
    XCB_ATOM_CUT_BUFFER7 = 16,
    XCB_ATOM_DRAWABLE = 17,
    XCB_ATOM_FONT = 18,
    XCB_ATOM_INTEGER = 19,
    XCB_ATOM_PIXMAP = 20,
    XCB_ATOM_POINT = 21,
    XCB_ATOM_RECTANGLE = 22,
    XCB_ATOM_RESOURCE_MANAGER = 23,
    XCB_ATOM_RGB_COLOR_MAP = 24,
    XCB_ATOM_RGB_BEST_MAP = 25,
    XCB_ATOM_RGB_BLUE_MAP = 26,
    XCB_ATOM_RGB_DEFAULT_MAP = 27,
    XCB_ATOM_RGB_GRAY_MAP = 28,
    XCB_ATOM_RGB_GREEN_MAP = 29,
    XCB_ATOM_RGB_RED_MAP = 30,
    XCB_ATOM_STRING = 31,
    XCB_ATOM_VISUALID = 32,
    XCB_ATOM_WINDOW = 33,
    XCB_ATOM_WM_COMMAND = 34,
    XCB_ATOM_WM_HINTS = 35,
    XCB_ATOM_WM_CLIENT_MACHINE = 36,
    XCB_ATOM_WM_ICON_NAME = 37,
    XCB_ATOM_WM_ICON_SIZE = 38,
    XCB_ATOM_WM_NAME = 39,
    XCB_ATOM_WM_NORMAL_HINTS = 40,
    XCB_ATOM_WM_SIZE_HINTS = 41,
    XCB_ATOM_WM_ZOOM_HINTS = 42,
    XCB_ATOM_MIN_SPACE = 43,
    XCB_ATOM_NORM_SPACE = 44,
    XCB_ATOM_MAX_SPACE = 45,
    XCB_ATOM_END_SPACE = 46,
    XCB_ATOM_SUPERSCRIPT_X = 47,
    XCB_ATOM_SUPERSCRIPT_Y = 48,
    XCB_ATOM_SUBSCRIPT_X = 49,
    XCB_ATOM_SUBSCRIPT_Y = 50,
    XCB_ATOM_UNDERLINE_POSITION = 51,
    XCB_ATOM_UNDERLINE_THICKNESS = 52,
    XCB_ATOM_STRIKEOUT_ASCENT = 53,
    XCB_ATOM_STRIKEOUT_DESCENT = 54,
    XCB_ATOM_ITALIC_ANGLE = 55,
    XCB_ATOM_X_HEIGHT = 56,
    XCB_ATOM_QUAD_WIDTH = 57,
    XCB_ATOM_WEIGHT = 58,
    XCB_ATOM_POINT_SIZE = 59,
    XCB_ATOM_RESOLUTION = 60,
    XCB_ATOM_COPYRIGHT = 61,
    XCB_ATOM_NOTICE = 62,
    XCB_ATOM_FONT_NAME = 63,
    XCB_ATOM_FAMILY_NAME = 64,
    XCB_ATOM_FULL_NAME = 65,
    XCB_ATOM_CAP_HEIGHT = 66,
    XCB_ATOM_WM_CLASS = 67,
    XCB_ATOM_WM_TRANSIENT_FOR = 68
} xcb_atom_enum_t;


typedef enum xcb_cw_t {
    XCB_CW_BACK_PIXMAP = 1,
    /**< Overrides the default background-pixmap. The background pixmap and window must
    have the same root and same depth. Any size pixmap can be used, although some
    sizes may be faster than others.
    
    If `XCB_BACK_PIXMAP_NONE` is specified, the window has no defined background.
    The server may fill the contents with the previous screen contents or with
    contents of its own choosing.
    
    If `XCB_BACK_PIXMAP_PARENT_RELATIVE` is specified, the parent's background is
    used, but the window must have the same depth as the parent (or a Match error
    results).   The parent's background is tracked, and the current version is
    used each time the window background is required. */
    
    XCB_CW_BACK_PIXEL = 2,
    /**< Overrides `BackPixmap`. A pixmap of undefined size filled with the specified
    background pixel is used for the background. Range-checking is not performed,
    the background pixel is truncated to the appropriate number of bits. */
    
    XCB_CW_BORDER_PIXMAP = 4,
    /**< Overrides the default border-pixmap. The border pixmap and window must have the
    same root and the same depth. Any size pixmap can be used, although some sizes
    may be faster than others.
    
    The special value `XCB_COPY_FROM_PARENT` means the parent's border pixmap is
    copied (subsequent changes to the parent's border attribute do not affect the
    child), but the window must have the same depth as the parent. */
    
    XCB_CW_BORDER_PIXEL = 8,
    /**< Overrides `BorderPixmap`. A pixmap of undefined size filled with the specified
    border pixel is used for the border. Range checking is not performed on the
    border-pixel value, it is truncated to the appropriate number of bits. */
    
    XCB_CW_BIT_GRAVITY = 16,
    /**< Defines which region of the window should be retained if the window is resized. */
    
    XCB_CW_WIN_GRAVITY = 32,
    /**< Defines how the window should be repositioned if the parent is resized (see
    `ConfigureWindow`). */
    
    XCB_CW_BACKING_STORE = 64,
    /**< A backing-store of `WhenMapped` advises the server that maintaining contents of
    obscured regions when the window is mapped would be beneficial. A backing-store
    of `Always` advises the server that maintaining contents even when the window
    is unmapped would be beneficial. In this case, the server may generate an
    exposure event when the window is created. A value of `NotUseful` advises the
    server that maintaining contents is unnecessary, although a server may still
    choose to maintain contents while the window is mapped. Note that if the server
    maintains contents, then the server should maintain complete contents not just
    the region within the parent boundaries, even if the window is larger than its
    parent. While the server maintains contents, exposure events will not normally
    be generated, but the server may stop maintaining contents at any time. */
    
    XCB_CW_BACKING_PLANES = 128,
    /**< The backing-planes indicates (with bits set to 1) which bit planes of the
    window hold dynamic data that must be preserved in backing-stores and during
    save-unders. */
    
    XCB_CW_BACKING_PIXEL = 256,
    /**< The backing-pixel specifies what value to use in planes not covered by
    backing-planes. The server is free to save only the specified bit planes in the
    backing-store or save-under and regenerate the remaining planes with the
    specified pixel value. Any bits beyond the specified depth of the window in
    these values are simply ignored. */
    
    XCB_CW_OVERRIDE_REDIRECT = 512,
    /**< The override-redirect specifies whether map and configure requests on this
    window should override a SubstructureRedirect on the parent, typically to
    inform a window manager not to tamper with the window. */
    
    XCB_CW_SAVE_UNDER = 1024,
    /**< If 1, the server is advised that when this window is mapped, saving the
    contents of windows it obscures would be beneficial. */
    
    XCB_CW_EVENT_MASK = 2048,
    /**< The event-mask defines which events the client is interested in for this window
    (or for some event types, inferiors of the window). */
    
    XCB_CW_DONT_PROPAGATE = 4096,
    /**< The do-not-propagate-mask defines which events should not be propagated to
    ancestor windows when no client has the event type selected in this window. */
    
    XCB_CW_COLORMAP = 8192,
    /**< The colormap specifies the colormap that best reflects the true colors of the window. Servers
    capable of supporting multiple hardware colormaps may use this information, and window man-
    agers may use it for InstallColormap requests. The colormap must have the same visual type
    and root as the window (or a Match error results). If CopyFromParent is specified, the parent's
    colormap is copied (subsequent changes to the parent's colormap attribute do not affect the child).
    However, the window must have the same visual type as the parent (or a Match error results),
    and the parent must not have a colormap of None (or a Match error results). For an explanation
    of None, see FreeColormap request. The colormap is copied by sharing the colormap object
    between the child and the parent, not by making a complete copy of the colormap contents. */
    
    XCB_CW_CURSOR = 16384
        /**< If a cursor is specified, it will be used whenever the pointer is in the window. If None is speci-
fied, the parent's cursor will be used when the pointer is in the window, and any change in the
parent's cursor will cause an immediate change in the displayed cursor. */
    
} xcb_cw_t;

typedef enum xcb_event_mask_t {
    XCB_EVENT_MASK_NO_EVENT = 0,
    XCB_EVENT_MASK_KEY_PRESS = 1,
    XCB_EVENT_MASK_KEY_RELEASE = 2,
    XCB_EVENT_MASK_BUTTON_PRESS = 4,
    XCB_EVENT_MASK_BUTTON_RELEASE = 8,
    XCB_EVENT_MASK_ENTER_WINDOW = 16,
    XCB_EVENT_MASK_LEAVE_WINDOW = 32,
    XCB_EVENT_MASK_POINTER_MOTION = 64,
    XCB_EVENT_MASK_POINTER_MOTION_HINT = 128,
    XCB_EVENT_MASK_BUTTON_1_MOTION = 256,
    XCB_EVENT_MASK_BUTTON_2_MOTION = 512,
    XCB_EVENT_MASK_BUTTON_3_MOTION = 1024,
    XCB_EVENT_MASK_BUTTON_4_MOTION = 2048,
    XCB_EVENT_MASK_BUTTON_5_MOTION = 4096,
    XCB_EVENT_MASK_BUTTON_MOTION = 8192,
    XCB_EVENT_MASK_KEYMAP_STATE = 16384,
    XCB_EVENT_MASK_EXPOSURE = 32768,
    XCB_EVENT_MASK_VISIBILITY_CHANGE = 65536,
    XCB_EVENT_MASK_STRUCTURE_NOTIFY = 131072,
    XCB_EVENT_MASK_RESIZE_REDIRECT = 262144,
    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY = 524288,
    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT = 1048576,
    XCB_EVENT_MASK_FOCUS_CHANGE = 2097152,
    XCB_EVENT_MASK_PROPERTY_CHANGE = 4194304,
    XCB_EVENT_MASK_COLOR_MAP_CHANGE = 8388608,
    XCB_EVENT_MASK_OWNER_GRAB_BUTTON = 16777216
} xcb_event_mask_t;

typedef union xcb_client_message_data_t {
    uint8_t  data8[20];
    uint16_t data16[10];
    uint32_t data32[5];
} xcb_client_message_data_t;

typedef struct xcb_client_message_event_t {
    uint8_t                   response_type;
    uint8_t                   format;
    uint16_t                  sequence;
    xcb_window_t              window;
    xcb_atom_t                type;
    xcb_client_message_data_t data;
} xcb_client_message_event_t;

typedef struct xcb_button_press_event_t {
    uint8_t         response_type;
    xcb_button_t    detail;
    uint16_t        sequence;
    xcb_timestamp_t time;
    xcb_window_t    root;
    xcb_window_t    event;
    xcb_window_t    child;
    int16_t         root_x;
    int16_t         root_y;
    int16_t         event_x;
    int16_t         event_y;
    uint16_t        state;
    uint8_t         same_screen;
    uint8_t         pad0;
} xcb_button_press_event_t;

typedef xcb_button_press_event_t xcb_button_release_event_t;

typedef struct {
/** User specified flags */
uint32_t flags;
/** User-specified position */
int32_t x, y;
/** User-specified size */
int32_t width, height;
/** Program-specified minimum size */
int32_t min_width, min_height;
/** Program-specified maximum size */
int32_t max_width, max_height;
/** Program-specified resize increments */
int32_t width_inc, height_inc;
/** Program-specified minimum aspect ratios */
int32_t min_aspect_num, min_aspect_den;
/** Program-specified maximum aspect ratios */
int32_t max_aspect_num, max_aspect_den;
/** Program-specified base size */
int32_t base_width, base_height;
/** Program-specified window gravity */
uint32_t win_gravity;
} xcb_size_hints_t;

typedef enum {
    XCB_ICCCM_SIZE_HINT_US_POSITION = 1 << 0,
    XCB_ICCCM_SIZE_HINT_US_SIZE = 1 << 1,
    XCB_ICCCM_SIZE_HINT_P_POSITION = 1 << 2,
    XCB_ICCCM_SIZE_HINT_P_SIZE = 1 << 3,
    XCB_ICCCM_SIZE_HINT_P_MIN_SIZE = 1 << 4,
    XCB_ICCCM_SIZE_HINT_P_MAX_SIZE = 1 << 5,
    XCB_ICCCM_SIZE_HINT_P_RESIZE_INC = 1 << 6,
    XCB_ICCCM_SIZE_HINT_P_ASPECT = 1 << 7,
    XCB_ICCCM_SIZE_HINT_BASE_SIZE = 1 << 8,
    XCB_ICCCM_SIZE_HINT_P_WIN_GRAVITY = 1 << 9
} xcb_icccm_size_hints_flags_t;

typedef enum xcb_grab_status_t {
    XCB_GRAB_STATUS_SUCCESS = 0,
    XCB_GRAB_STATUS_ALREADY_GRABBED = 1,
    XCB_GRAB_STATUS_INVALID_TIME = 2,
    XCB_GRAB_STATUS_NOT_VIEWABLE = 3,
    XCB_GRAB_STATUS_FROZEN = 4
} xcb_grab_status_t;

typedef enum xcb_grab_mode_t {
    XCB_GRAB_MODE_SYNC = 0,
    XCB_GRAB_MODE_ASYNC = 1
} xcb_grab_mode_t;

typedef struct xcb_grab_pointer_cookie_t {
    unsigned int sequence;
} xcb_grab_pointer_cookie_t;

typedef struct xcb_grab_pointer_reply_t {
    uint8_t  response_type;
    uint8_t  status;
    uint16_t sequence;
    uint32_t length;
} xcb_grab_pointer_reply_t;

#endif // GROUNDED_XCB_TYPES_H
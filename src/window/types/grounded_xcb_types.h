#ifndef GROUNDED_XCB_TYPES_H
#define GROUNDED_XCB_TYPES_H

// Event types
#define XCB_KEY_PRESS 2
#define XCB_KEY_RELEASE 3
#define XCB_BUTTON_PRESS 4
#define XCB_BUTTON_RELEASE 5
#define XCB_MOTION_NOTIFY 6
#define XCB_ENTER_NOTIFY 7
#define XCB_LEAVE_NOTIFY 8
#define XCB_FOCUS_IN 9
#define XCB_FOCUS_OUT 10
#define XCB_KEYMAP_NOTIFY 11
// #define XCB_EXPOSE 12
// #define XCB_GRAPHICS_EXPOSURE 13
#define XCB_NO_EXPOSURE 14
#define XCB_VISIBILITY_NOTIFY 15
#define XCB_CREATE_NOTIFY 16
#define XCB_DESTROY_NOTIFY 17
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
#define XCB_PROPERTY_NOTIFY 28
#define XCB_SELECTION_CLEAR 29
#define XCB_SELECTION_REQUEST 30
#define XCB_SELECTION_NOTIFY 31
// #define XCB_COLORMAP_NOTIFY 32
#define XCB_CLIENT_MESSAGE 33
#define XCB_MAPPING_NOTIFY 34
#define XCB_GE_GENERIC 35

#define XCB_COPY_FROM_PARENT 0L

#define XCB_CURRENT_TIME 0L

#define XCB_NONE 0L

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8   /* movement only */
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD     9   /* size via keyboard */
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD    10   /* move via keyboard */
#define _NET_WM_MOVERESIZE_CANCEL           11   /* cancel operation */

typedef struct xcb_connection_t xcb_connection_t;  // Opaque structure containing all data that  XCB needs to communicate with an X server
typedef struct xcb_setup_t xcb_setup_t;
typedef struct xcb_cursor_context_t xcb_cursor_context_t;
typedef struct xcb_extension_t xcb_extension_t;

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

typedef struct xcb_request_error_t {
    uint8_t  response_type;
    uint8_t  error_code;
    uint16_t sequence;
    uint32_t bad_value;
    uint16_t minor_opcode;
    uint8_t  major_opcode;
    uint8_t  pad0;
} xcb_request_error_t;

typedef struct xcb_setup_t {
    uint8_t       status;
    uint8_t       pad0;
    uint16_t      protocol_major_version;
    uint16_t      protocol_minor_version;
    uint16_t      length;
    uint32_t      release_number;
    uint32_t      resource_id_base;
    uint32_t      resource_id_mask;
    uint32_t      motion_buffer_size;
    uint16_t      vendor_len;
    uint16_t      maximum_request_length;
    uint8_t       roots_len;
    uint8_t       pixmap_formats_len;
    uint8_t       image_byte_order;
    uint8_t       bitmap_format_bit_order;
    uint8_t       bitmap_format_scanline_unit;
    uint8_t       bitmap_format_scanline_pad;
    xcb_keycode_t min_keycode;
    xcb_keycode_t max_keycode;
    uint8_t       pad1[4];
} xcb_setup_t;

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

typedef enum xcb_map_state_t {
    XCB_MAP_STATE_UNMAPPED = 0,
    XCB_MAP_STATE_UNVIEWABLE = 1,
    XCB_MAP_STATE_VIEWABLE = 2
} xcb_map_state_t;

typedef struct {
    unsigned int sequence;  /**< Sequence number */
} xcb_void_cookie_t;

typedef struct xcb_intern_atom_cookie_t {
    unsigned int sequence;
} xcb_intern_atom_cookie_t;

typedef enum xcb_config_window_t {
    XCB_CONFIG_WINDOW_X = 1,
    XCB_CONFIG_WINDOW_Y = 2,
    XCB_CONFIG_WINDOW_WIDTH = 4,
    XCB_CONFIG_WINDOW_HEIGHT = 8,
    XCB_CONFIG_WINDOW_BORDER_WIDTH = 16,
    XCB_CONFIG_WINDOW_SIBLING = 32,
    XCB_CONFIG_WINDOW_STACK_MODE = 64
} xcb_config_window_t;

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

typedef struct xcb_value_error_t {
    uint8_t  response_type;
    uint8_t  error_code;
    uint16_t sequence;
    uint32_t bad_value;
    uint16_t minor_opcode;
    uint8_t  major_opcode;
    uint8_t  pad0;
} xcb_value_error_t;

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

typedef struct xcb_focus_in_event_t {
    uint8_t      response_type;
    uint8_t      detail;
    uint16_t     sequence;
    xcb_window_t event;
    uint8_t      mode;
    uint8_t      pad0[3];
} xcb_focus_in_event_t;

typedef xcb_focus_in_event_t xcb_focus_out_event_t;

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

typedef struct xcb_query_extension_reply_t {
    uint8_t  response_type;
    uint8_t  pad0;
    uint16_t sequence;
    uint32_t length;
    uint8_t  present;
    uint8_t  major_opcode;
    uint8_t  first_event;
    uint8_t  first_error;
} xcb_query_extension_reply_t;

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

typedef struct xcb_visualtype_t {
    xcb_visualid_t visual_id;
    uint8_t        _class;
    uint8_t        bits_per_rgb_value;
    uint16_t       colormap_entries;
    uint32_t       red_mask;
    uint32_t       green_mask;
    uint32_t       blue_mask;
    uint8_t        pad0[4];
} xcb_visualtype_t;

typedef struct xcb_visualtype_iterator_t {
    xcb_visualtype_t *data;
    int               rem;
    int               index;
} xcb_visualtype_iterator_t;

typedef enum xcb_visual_class_t {
    XCB_VISUAL_CLASS_STATIC_GRAY = 0,
    XCB_VISUAL_CLASS_GRAY_SCALE = 1,
    XCB_VISUAL_CLASS_STATIC_COLOR = 2,
    XCB_VISUAL_CLASS_PSEUDO_COLOR = 3,
    XCB_VISUAL_CLASS_TRUE_COLOR = 4,
    XCB_VISUAL_CLASS_DIRECT_COLOR = 5
} xcb_visual_class_t;

typedef enum xcb_colormap_alloc_t {
    XCB_COLORMAP_ALLOC_NONE = 0,
    XCB_COLORMAP_ALLOC_ALL = 1
} xcb_colormap_alloc_t;

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

#define XCB_COPY_AREA 62

typedef struct xcb_rectangle_t {
    int16_t  x;
    int16_t  y;
    uint16_t width;
    uint16_t height;
} xcb_rectangle_t;

typedef struct xcb_get_atom_name_cookie_t {
    unsigned int sequence;
} xcb_get_atom_name_cookie_t;

typedef struct xcb_get_atom_name_reply_t {
    uint8_t  response_type;
    uint8_t  pad0;
    uint16_t sequence;
    uint32_t length;
    uint16_t name_len;
    uint8_t  pad1[22];
} xcb_get_atom_name_reply_t;

typedef struct xcb_get_property_cookie_t {
    unsigned int sequence;
} xcb_get_property_cookie_t;

typedef struct xcb_get_property_reply_t {
    uint8_t    response_type;
    uint8_t    format;
    uint16_t   sequence;
    uint32_t   length;
    xcb_atom_t type;
    uint32_t   bytes_after;
    uint32_t   value_len;
    uint8_t    pad0[12];
} xcb_get_property_reply_t;

typedef enum xcb_get_property_type_t {
    XCB_GET_PROPERTY_TYPE_ANY = 0
} xcb_get_property_type_t;

typedef struct xcb_translate_coordinates_cookie_t {
    unsigned int sequence;
} xcb_translate_coordinates_cookie_t;

typedef struct xcb_translate_coordinates_reply_t {
    uint8_t      response_type;
    uint8_t      same_screen;
    uint16_t     sequence;
    uint32_t     length;
    xcb_window_t child;
    int16_t      dst_x;
    int16_t      dst_y;
} xcb_translate_coordinates_reply_t;

typedef struct xcb_query_tree_cookie_t {
    unsigned int sequence;
} xcb_query_tree_cookie_t;

typedef struct xcb_query_tree_reply_t {
    uint8_t      response_type;
    uint8_t      pad0;
    uint16_t     sequence;
    uint32_t     length;
    xcb_window_t root;
    xcb_window_t parent;
    uint16_t     children_len;
    uint8_t      pad1[14];
} xcb_query_tree_reply_t;

typedef struct xcb_enter_notify_event_t {
    uint8_t         response_type;
    uint8_t         detail;
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
    uint8_t         mode;
    uint8_t         same_screen_focus;
} xcb_enter_notify_event_t;

typedef xcb_enter_notify_event_t xcb_leave_notify_event_t;

typedef struct xcb_get_selection_owner_cookie_t {
    unsigned int sequence;
} xcb_get_selection_owner_cookie_t;

typedef struct xcb_get_selection_owner_reply_t {
    uint8_t      response_type;
    uint8_t      pad0;
    uint16_t     sequence;
    uint32_t     length;
    xcb_window_t owner;
} xcb_get_selection_owner_reply_t;

typedef struct xcb_selection_request_event_t {
    uint8_t         response_type;
    uint8_t         pad0;
    uint16_t        sequence;
    xcb_timestamp_t time;
    xcb_window_t    owner;
    xcb_window_t    requestor;
    xcb_atom_t      selection;
    xcb_atom_t      target;
    xcb_atom_t      property;
} xcb_selection_request_event_t;

typedef struct xcb_selection_notify_event_t {
    uint8_t         response_type;
    uint8_t         pad0;
    uint16_t        sequence;
    xcb_timestamp_t time;
    xcb_window_t    requestor;
    xcb_atom_t      selection;
    xcb_atom_t      target;
    xcb_atom_t      property;
} xcb_selection_notify_event_t;

typedef struct xcb_get_window_attributes_cookie_t {
    unsigned int sequence;
} xcb_get_window_attributes_cookie_t;

typedef struct xcb_get_window_attributes_request_t {
    uint8_t      major_opcode;
    uint8_t      pad0;
    uint16_t     length;
    xcb_window_t window;
} xcb_get_window_attributes_request_t;

typedef struct xcb_get_window_attributes_reply_t {
    uint8_t        response_type;
    uint8_t        backing_store;
    uint16_t       sequence;
    uint32_t       length;
    xcb_visualid_t visual;
    uint16_t       _class;
    uint8_t        bit_gravity;
    uint8_t        win_gravity;
    uint32_t       backing_planes;
    uint32_t       backing_pixel;
    uint8_t        save_under;
    uint8_t        map_is_installed;
    uint8_t        map_state;
    uint8_t        override_redirect;
    xcb_colormap_t colormap;
    uint32_t       all_event_masks;
    uint32_t       your_event_mask;
    uint16_t       do_not_propagate_mask;
    uint8_t        pad0[2];
} xcb_get_window_attributes_reply_t;

typedef struct xcb_get_geometry_cookie_t {
    unsigned int sequence;
} xcb_get_geometry_cookie_t;

typedef struct xcb_get_geometry_reply_t {
    uint8_t      response_type;
    uint8_t      depth;
    uint16_t     sequence;
    uint32_t     length;
    xcb_window_t root;
    int16_t      x;
    int16_t      y;
    uint16_t     width;
    uint16_t     height;
    uint16_t     border_width;
    uint8_t      pad0[2];
} xcb_get_geometry_reply_t;

typedef struct xcb_motion_notify_event_t {
    uint8_t         response_type;
    uint8_t         detail;
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
} xcb_motion_notify_event_t;

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








typedef uint16_t xcb_xkb_device_spec_t;
typedef uint16_t xcb_xkb_id_spec_t;
typedef uint16_t xcb_xkb_led_class_spec_t;

typedef struct xcb_xkb_get_map_cookie_t {
    unsigned int sequence;
} xcb_xkb_get_map_cookie_t;

typedef struct xcb_xkb_get_names_cookie_t {
    unsigned int sequence;
} xcb_xkb_get_names_cookie_t;

typedef struct xcb_xkb_per_client_flags_cookie_t {
    unsigned int sequence;
} xcb_xkb_per_client_flags_cookie_t;

typedef struct xcb_xkb_use_extension_cookie_t {
    unsigned int sequence;
} xcb_xkb_use_extension_cookie_t;

#define XCB_XKB_NEW_KEYBOARD_NOTIFY 0
#define XCB_XKB_MAP_NOTIFY 1
#define XCB_XKB_STATE_NOTIFY 2

typedef enum xcb_xkb_id_t {
    XCB_XKB_ID_USE_CORE_KBD = 256,
    XCB_XKB_ID_USE_CORE_PTR = 512,
    XCB_XKB_ID_DFLT_XI_CLASS = 768,
    XCB_XKB_ID_DFLT_XI_ID = 1024,
    XCB_XKB_ID_ALL_XI_CLASS = 1280,
    XCB_XKB_ID_ALL_XI_ID = 1536,
    XCB_XKB_ID_XI_NONE = 65280
} xcb_xkb_id_t;

typedef enum xcb_xkb_event_type_t {
    XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY = 1,
    XCB_XKB_EVENT_TYPE_MAP_NOTIFY = 2,
    XCB_XKB_EVENT_TYPE_STATE_NOTIFY = 4,
    XCB_XKB_EVENT_TYPE_CONTROLS_NOTIFY = 8,
    XCB_XKB_EVENT_TYPE_INDICATOR_STATE_NOTIFY = 16,
    XCB_XKB_EVENT_TYPE_INDICATOR_MAP_NOTIFY = 32,
    XCB_XKB_EVENT_TYPE_NAMES_NOTIFY = 64,
    XCB_XKB_EVENT_TYPE_COMPAT_MAP_NOTIFY = 128,
    XCB_XKB_EVENT_TYPE_BELL_NOTIFY = 256,
    XCB_XKB_EVENT_TYPE_ACTION_MESSAGE = 512,
    XCB_XKB_EVENT_TYPE_ACCESS_X_NOTIFY = 1024,
    XCB_XKB_EVENT_TYPE_EXTENSION_DEVICE_NOTIFY = 2048
} xcb_xkb_event_type_t;

typedef enum xcb_xkb_per_client_flag_t {
    XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT = 1,
    XCB_XKB_PER_CLIENT_FLAG_GRABS_USE_XKB_STATE = 2,
    XCB_XKB_PER_CLIENT_FLAG_AUTO_RESET_CONTROLS = 4,
    XCB_XKB_PER_CLIENT_FLAG_LOOKUP_STATE_WHEN_GRABBED = 8,
    XCB_XKB_PER_CLIENT_FLAG_SEND_EVENT_USES_XKB_STATE = 16
} xcb_xkb_per_client_flag_t;

typedef enum xcb_xkb_map_part_t {
    XCB_XKB_MAP_PART_KEY_TYPES = 1,
    XCB_XKB_MAP_PART_KEY_SYMS = 2,
    XCB_XKB_MAP_PART_MODIFIER_MAP = 4,
    XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS = 8,
    XCB_XKB_MAP_PART_KEY_ACTIONS = 16,
    XCB_XKB_MAP_PART_KEY_BEHAVIORS = 32,
    XCB_XKB_MAP_PART_VIRTUAL_MODS = 64,
    XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP = 128
} xcb_xkb_map_part_t;

typedef enum xcb_xkb_name_detail_t {
    XCB_XKB_NAME_DETAIL_KEYCODES = 1,
    XCB_XKB_NAME_DETAIL_GEOMETRY = 2,
    XCB_XKB_NAME_DETAIL_SYMBOLS = 4,
    XCB_XKB_NAME_DETAIL_PHYS_SYMBOLS = 8,
    XCB_XKB_NAME_DETAIL_TYPES = 16,
    XCB_XKB_NAME_DETAIL_COMPAT = 32,
    XCB_XKB_NAME_DETAIL_KEY_TYPE_NAMES = 64,
    XCB_XKB_NAME_DETAIL_KT_LEVEL_NAMES = 128,
    XCB_XKB_NAME_DETAIL_INDICATOR_NAMES = 256,
    XCB_XKB_NAME_DETAIL_KEY_NAMES = 512,
    XCB_XKB_NAME_DETAIL_KEY_ALIASES = 1024,
    XCB_XKB_NAME_DETAIL_VIRTUAL_MOD_NAMES = 2048,
    XCB_XKB_NAME_DETAIL_GROUP_NAMES = 4096,
    XCB_XKB_NAME_DETAIL_RG_NAMES = 8192
} xcb_xkb_name_detail_t;

typedef struct xcb_xkb_get_map_reply_t {
    uint8_t       response_type;
    uint8_t       deviceID;
    uint16_t      sequence;
    uint32_t      length;
    uint8_t       pad0[2];
    xcb_keycode_t minKeyCode;
    xcb_keycode_t maxKeyCode;
    uint16_t      present;
    uint8_t       firstType;
    uint8_t       nTypes;
    uint8_t       totalTypes;
    xcb_keycode_t firstKeySym;
    uint16_t      totalSyms;
    uint8_t       nKeySyms;
    xcb_keycode_t firstKeyAction;
    uint16_t      totalActions;
    uint8_t       nKeyActions;
    xcb_keycode_t firstKeyBehavior;
    uint8_t       nKeyBehaviors;
    uint8_t       totalKeyBehaviors;
    xcb_keycode_t firstKeyExplicit;
    uint8_t       nKeyExplicit;
    uint8_t       totalKeyExplicit;
    xcb_keycode_t firstModMapKey;
    uint8_t       nModMapKeys;
    uint8_t       totalModMapKeys;
    xcb_keycode_t firstVModMapKey;
    uint8_t       nVModMapKeys;
    uint8_t       totalVModMapKeys;
    uint8_t       pad1;
    uint16_t      virtualMods;
} xcb_xkb_get_map_reply_t;

typedef struct xcb_xkb_get_names_reply_t {
    uint8_t       response_type;
    uint8_t       deviceID;
    uint16_t      sequence;
    uint32_t      length;
    uint32_t      which;
    xcb_keycode_t minKeyCode;
    xcb_keycode_t maxKeyCode;
    uint8_t       nTypes;
    uint8_t       groupNames;
    uint16_t      virtualMods;
    xcb_keycode_t firstKey;
    uint8_t       nKeys;
    uint32_t      indicators;
    uint8_t       nRadioGroups;
    uint8_t       nKeyAliases;
    uint16_t      nKTLevels;
    uint8_t       pad0[4];
} xcb_xkb_get_names_reply_t;

typedef struct xcb_xkb_use_extension_reply_t {
    uint8_t  response_type;
    uint8_t  supported;
    uint16_t sequence;
    uint32_t length;
    uint16_t serverMajor;
    uint16_t serverMinor;
    uint8_t  pad0[20];
} xcb_xkb_use_extension_reply_t;

typedef enum xcb_xkb_const_t {
    XCB_XKB_CONST_MAX_LEGAL_KEY_CODE = 255,
    XCB_XKB_CONST_PER_KEY_BIT_ARRAY_SIZE = 32,
    XCB_XKB_CONST_KEY_NAME_LENGTH = 4
} xcb_xkb_const_t;

typedef struct xcb_xkb_key_type_t {
    uint8_t  mods_mask;
    uint8_t  mods_mods;
    uint16_t mods_vmods;
    uint8_t  numLevels;
    uint8_t  nMapEntries;
    uint8_t  hasPreserve;
    uint8_t  pad0;
} xcb_xkb_key_type_t;

typedef struct xcb_xkb_key_sym_map_t {
    uint8_t  kt_index[4];
    uint8_t  groupInfo;
    uint8_t  width;
    uint16_t nSyms;
} xcb_xkb_key_sym_map_t;

typedef struct xcb_xkb_sa_no_action_t {
    uint8_t type;
    uint8_t pad0[7];
} xcb_xkb_sa_no_action_t;

typedef struct xcb_xkb_sa_set_mods_t {
    uint8_t type;
    uint8_t flags;
    uint8_t mask;
    uint8_t realMods;
    uint8_t vmodsHigh;
    uint8_t vmodsLow;
    uint8_t pad0[2];
} xcb_xkb_sa_set_mods_t;

typedef struct xcb_xkb_sa_latch_mods_t {
    uint8_t type;
    uint8_t flags;
    uint8_t mask;
    uint8_t realMods;
    uint8_t vmodsHigh;
    uint8_t vmodsLow;
    uint8_t pad0[2];
} xcb_xkb_sa_latch_mods_t;

typedef struct xcb_xkb_sa_lock_mods_t {
    uint8_t type;
    uint8_t flags;
    uint8_t mask;
    uint8_t realMods;
    uint8_t vmodsHigh;
    uint8_t vmodsLow;
    uint8_t pad0[2];
} xcb_xkb_sa_lock_mods_t;

typedef struct xcb_xkb_sa_set_group_t {
    uint8_t type;
    uint8_t flags;
    int8_t  group;
    uint8_t pad0[5];
} xcb_xkb_sa_set_group_t;

typedef struct xcb_xkb_sa_latch_group_t {
    uint8_t type;
    uint8_t flags;
    int8_t  group;
    uint8_t pad0[5];
} xcb_xkb_sa_latch_group_t;

typedef struct xcb_xkb_sa_lock_group_t {
    uint8_t type;
    uint8_t flags;
    int8_t  group;
    uint8_t pad0[5];
} xcb_xkb_sa_lock_group_t;

typedef struct xcb_xkb_sa_move_ptr_t {
    uint8_t type;
    uint8_t flags;
    int8_t  xHigh;
    uint8_t xLow;
    int8_t  yHigh;
    uint8_t yLow;
    uint8_t pad0[2];
} xcb_xkb_sa_move_ptr_t;

typedef struct xcb_xkb_sa_ptr_btn_t {
    uint8_t type;
    uint8_t flags;
    uint8_t count;
    uint8_t button;
    uint8_t pad0[4];
} xcb_xkb_sa_ptr_btn_t;

typedef struct xcb_xkb_sa_lock_ptr_btn_t {
    uint8_t type;
    uint8_t flags;
    uint8_t pad0;
    uint8_t button;
    uint8_t pad1[4];
} xcb_xkb_sa_lock_ptr_btn_t;

typedef struct xcb_xkb_sa_set_ptr_dflt_t {
    uint8_t type;
    uint8_t flags;
    uint8_t affect;
    int8_t  value;
    uint8_t pad0[4];
} xcb_xkb_sa_set_ptr_dflt_t;

typedef struct xcb_xkb_sa_iso_lock_t {
    uint8_t type;
    uint8_t flags;
    uint8_t mask;
    uint8_t realMods;
    int8_t  group;
    uint8_t affect;
    uint8_t vmodsHigh;
    uint8_t vmodsLow;
} xcb_xkb_sa_iso_lock_t;

typedef struct xcb_xkb_sa_terminate_t {
    uint8_t type;
    uint8_t pad0[7];
} xcb_xkb_sa_terminate_t;

typedef struct xcb_xkb_sa_switch_screen_t {
    uint8_t type;
    uint8_t flags;
    int8_t  newScreen;
    uint8_t pad0[5];
} xcb_xkb_sa_switch_screen_t;

typedef struct xcb_xkb_sa_set_controls_t {
    uint8_t type;
    uint8_t pad0[3];
    uint8_t boolCtrlsHigh;
    uint8_t boolCtrlsLow;
    uint8_t pad1[2];
} xcb_xkb_sa_set_controls_t;

typedef struct xcb_xkb_sa_lock_controls_t {
    uint8_t type;
    uint8_t pad0[3];
    uint8_t boolCtrlsHigh;
    uint8_t boolCtrlsLow;
    uint8_t pad1[2];
} xcb_xkb_sa_lock_controls_t;

typedef struct xcb_xkb_sa_action_message_t {
    uint8_t type;
    uint8_t flags;
    uint8_t message[6];
} xcb_xkb_sa_action_message_t;

typedef struct xcb_xkb_sa_redirect_key_t {
    uint8_t       type;
    xcb_keycode_t newkey;
    uint8_t       mask;
    uint8_t       realModifiers;
    uint8_t       vmodsMaskHigh;
    uint8_t       vmodsMaskLow;
    uint8_t       vmodsHigh;
    uint8_t       vmodsLow;
} xcb_xkb_sa_redirect_key_t;

typedef struct xcb_xkb_sa_device_btn_t {
    uint8_t type;
    uint8_t flags;
    uint8_t count;
    uint8_t button;
    uint8_t device;
    uint8_t pad0[3];
} xcb_xkb_sa_device_btn_t;

typedef struct xcb_xkb_sa_lock_device_btn_t {
    uint8_t type;
    uint8_t flags;
    uint8_t pad0;
    uint8_t button;
    uint8_t device;
    uint8_t pad1[3];
} xcb_xkb_sa_lock_device_btn_t;

typedef struct xcb_xkb_sa_device_valuator_t {
    uint8_t type;
    uint8_t device;
    uint8_t val1what;
    uint8_t val1index;
    uint8_t val1value;
    uint8_t val2what;
    uint8_t val2index;
    uint8_t val2value;
} xcb_xkb_sa_device_valuator_t;

typedef union xcb_xkb_action_t {
    xcb_xkb_sa_no_action_t       noaction;
    xcb_xkb_sa_set_mods_t        setmods;
    xcb_xkb_sa_latch_mods_t      latchmods;
    xcb_xkb_sa_lock_mods_t       lockmods;
    xcb_xkb_sa_set_group_t       setgroup;
    xcb_xkb_sa_latch_group_t     latchgroup;
    xcb_xkb_sa_lock_group_t      lockgroup;
    xcb_xkb_sa_move_ptr_t        moveptr;
    xcb_xkb_sa_ptr_btn_t         ptrbtn;
    xcb_xkb_sa_lock_ptr_btn_t    lockptrbtn;
    xcb_xkb_sa_set_ptr_dflt_t    setptrdflt;
    xcb_xkb_sa_iso_lock_t        isolock;
    xcb_xkb_sa_terminate_t       terminate;
    xcb_xkb_sa_switch_screen_t   switchscreen;
    xcb_xkb_sa_set_controls_t    setcontrols;
    xcb_xkb_sa_lock_controls_t   lockcontrols;
    xcb_xkb_sa_action_message_t  message;
    xcb_xkb_sa_redirect_key_t    redirect;
    xcb_xkb_sa_device_btn_t      devbtn;
    xcb_xkb_sa_lock_device_btn_t lockdevbtn;
    xcb_xkb_sa_device_valuator_t devval;
    uint8_t                      type;
} xcb_xkb_action_t;

typedef struct xcb_xkb_common_behavior_t {
    uint8_t type;
    uint8_t data;
} xcb_xkb_common_behavior_t;

typedef struct xcb_xkb_default_behavior_t {
    uint8_t type;
    uint8_t pad0;
} xcb_xkb_default_behavior_t;

typedef struct xcb_xkb_lock_behavior_t {
    uint8_t type;
    uint8_t pad0;
} xcb_xkb_lock_behavior_t;

typedef struct xcb_xkb_radio_group_behavior_t {
    uint8_t type;
    uint8_t group;
} xcb_xkb_radio_group_behavior_t;

typedef struct xcb_xkb_overlay_behavior_t {
    uint8_t       type;
    xcb_keycode_t key;
} xcb_xkb_overlay_behavior_t;

typedef struct xcb_xkb_permament_lock_behavior_t {
    uint8_t type;
    uint8_t pad0;
} xcb_xkb_permament_lock_behavior_t;

typedef struct xcb_xkb_permament_radio_group_behavior_t {
    uint8_t type;
    uint8_t group;
} xcb_xkb_permament_radio_group_behavior_t;

typedef struct xcb_xkb_permament_overlay_behavior_t {
    uint8_t       type;
    xcb_keycode_t key;
} xcb_xkb_permament_overlay_behavior_t;

typedef union xcb_xkb_behavior_t {
    xcb_xkb_common_behavior_t                common;
    xcb_xkb_default_behavior_t               _default;
    xcb_xkb_lock_behavior_t                  lock;
    xcb_xkb_radio_group_behavior_t           radioGroup;
    xcb_xkb_overlay_behavior_t               overlay1;
    xcb_xkb_overlay_behavior_t               overlay2;
    xcb_xkb_permament_lock_behavior_t        permamentLock;
    xcb_xkb_permament_radio_group_behavior_t permamentRadioGroup;
    xcb_xkb_permament_overlay_behavior_t     permamentOverlay1;
    xcb_xkb_permament_overlay_behavior_t     permamentOverlay2;
    uint8_t                                  type;
} xcb_xkb_behavior_t;

typedef struct xcb_xkb_state_notify_event_t {
    uint8_t         response_type;
    uint8_t         xkbType;
    uint16_t        sequence;
    xcb_timestamp_t time;
    uint8_t         deviceID;
    uint8_t         mods;
    uint8_t         baseMods;
    uint8_t         latchedMods;
    uint8_t         lockedMods;
    uint8_t         group;
    int16_t         baseGroup;
    int16_t         latchedGroup;
    uint8_t         lockedGroup;
    uint8_t         compatState;
    uint8_t         grabMods;
    uint8_t         compatGrabMods;
    uint8_t         lookupMods;
    uint8_t         compatLoockupMods;
    uint16_t        ptrBtnState;
    uint16_t        changed;
    xcb_keycode_t   keycode;
    uint8_t         eventType;
    uint8_t         requestMajor;
    uint8_t         requestMinor;
} xcb_xkb_state_notify_event_t;

typedef struct xcb_xkb_set_behavior_t {
    xcb_keycode_t      keycode;
    xcb_xkb_behavior_t behavior;
    uint8_t            pad0;
} xcb_xkb_set_behavior_t;

typedef struct xcb_xkb_key_mod_map_t {
    xcb_keycode_t keycode;
    uint8_t       mods;
} xcb_xkb_key_mod_map_t;

typedef struct xcb_xkb_set_explicit_t {
    xcb_keycode_t keycode;
    uint8_t       explicit;
} xcb_xkb_set_explicit_t;

typedef struct xcb_xkb_key_v_mod_map_t {
    xcb_keycode_t keycode;
    uint8_t       pad0;
    uint16_t      vmods;
} xcb_xkb_key_v_mod_map_t;

typedef struct xcb_xkb_key_sym_map_iterator_t {
    xcb_xkb_key_sym_map_t *data;
    int                    rem;
    int                    index;
} xcb_xkb_key_sym_map_iterator_t;

typedef struct xcb_xkb_get_map_map_t {
    xcb_xkb_key_type_t      *types_rtrn;
    xcb_xkb_key_sym_map_t   *syms_rtrn;
    uint8_t                 *acts_rtrn_count;
    uint8_t                 *pad2;
    xcb_xkb_action_t        *acts_rtrn_acts;
    xcb_xkb_set_behavior_t  *behaviors_rtrn;
    uint8_t                 *vmods_rtrn;
    uint8_t                 *pad3;
    xcb_xkb_set_explicit_t  *explicit_rtrn;
    uint8_t                 *pad4;
    xcb_xkb_key_mod_map_t   *modmap_rtrn;
    uint8_t                 *pad5;
    xcb_xkb_key_v_mod_map_t *vmodmap_rtrn;
} xcb_xkb_get_map_map_t;

typedef struct xcb_xkb_get_device_info_cookie_t {
    unsigned int sequence;
} xcb_xkb_get_device_info_cookie_t;

typedef struct xcb_xkb_get_device_info_reply_t {
    uint8_t    response_type;
    uint8_t    deviceID;
    uint16_t   sequence;
    uint32_t   length;
    uint16_t   present;
    uint16_t   supported;
    uint16_t   unsupported;
    uint16_t   nDeviceLedFBs;
    uint8_t    firstBtnWanted;
    uint8_t    nBtnsWanted;
    uint8_t    firstBtnRtrn;
    uint8_t    nBtnsRtrn;
    uint8_t    totalBtns;
    uint8_t    hasOwnState;
    uint16_t   dfltKbdFB;
    uint16_t   dfltLedFB;
    uint8_t    pad0[2];
    xcb_atom_t devType;
    uint16_t   nameLen;
} xcb_xkb_get_device_info_reply_t;

/*typedef struct xcb_xkb_get_device_info_request_t {
    uint8_t                  major_opcode;
    uint8_t                  minor_opcode;
    uint16_t                 length;
    xcb_xkb_device_spec_t    deviceSpec;
    uint16_t                 wanted;
    uint8_t                  allButtons;
    uint8_t                  firstButton;
    uint8_t                  nButtons;
    uint8_t                  pad0;
    xcb_xkb_led_class_spec_t ledClass;
    xcb_xkb_id_spec_t        ledID;
} xcb_xkb_get_device_info_request_t;*/

enum xkb_keymap_compile_flags {
    /** Do not apply any flags. */
    XKB_KEYMAP_COMPILE_NO_FLAGS = 0
};

struct xkb_context;

typedef uint32_t xkb_keysym_t;
typedef uint32_t xkb_keycode_t;
typedef uint32_t xkb_layout_index_t;
typedef uint32_t xkb_mod_mask_t;

enum xkb_context_flags {
    /** Do not apply any context flags. */
    XKB_CONTEXT_NO_FLAGS = 0,
    /** Create this context with an empty include path. */
    XKB_CONTEXT_NO_DEFAULT_INCLUDES = (1 << 0),
    /**
     * Don't take RMLVO names from the environment.
     *
     * @since 0.3.0
     */
    XKB_CONTEXT_NO_ENVIRONMENT_NAMES = (1 << 1),
    /**
     * Disable the use of secure_getenv for this context, so that privileged
     * processes can use environment variables. Client uses at their own risk.
     *
     * @since 1.5.0
     */
    XKB_CONTEXT_NO_SECURE_GETENV = (1 << 2)
};

enum xkb_state_component {
    /** Depressed modifiers, i.e. a key is physically holding them. */
    XKB_STATE_MODS_DEPRESSED = (1 << 0),
    /** Latched modifiers, i.e. will be unset after the next non-modifier
     *  key press. */
    XKB_STATE_MODS_LATCHED = (1 << 1),
    /** Locked modifiers, i.e. will be unset after the key provoking the
     *  lock has been pressed again. */
    XKB_STATE_MODS_LOCKED = (1 << 2),
    /** Effective modifiers, i.e. currently active and affect key
     *  processing (derived from the other state components).
     *  Use this unless you explicitly care how the state came about. */
    XKB_STATE_MODS_EFFECTIVE = (1 << 3),
    /** Depressed layout, i.e. a key is physically holding it. */
    XKB_STATE_LAYOUT_DEPRESSED = (1 << 4),
    /** Latched layout, i.e. will be unset after the next non-modifier
     *  key press. */
    XKB_STATE_LAYOUT_LATCHED = (1 << 5),
    /** Locked layout, i.e. will be unset after the key provoking the lock
     *  has been pressed again. */
    XKB_STATE_LAYOUT_LOCKED = (1 << 6),
    /** Effective layout, i.e. currently active and affects key processing
     *  (derived from the other state components).
     *  Use this unless you explicitly care how the state came about. */
    XKB_STATE_LAYOUT_EFFECTIVE = (1 << 7),
    /** LEDs (derived from the other state components). */
    XKB_STATE_LEDS = (1 << 8)
};

#endif // GROUNDED_XCB_TYPES_H
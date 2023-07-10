/*
 * This file uses the X-List pattern to define all required xcb render functions
 */

X(xcb_render_create_cursor, xcb_void_cookie_t, (xcb_connection_t     *c, xcb_cursor_t          cid,xcb_render_picture_t  source,uint16_t              x,uint16_t              y))
X(xcb_render_create_picture, xcb_void_cookie_t, (xcb_connection_t        *c,xcb_render_picture_t     pid,xcb_drawable_t           drawable,xcb_render_pictformat_t  format,uint32_t                 value_mask,const void              *value_list))
X(xcb_render_query_pict_formats, xcb_render_query_pict_formats_cookie_t, (xcb_connection_t *c))
X(xcb_render_query_pict_formats_reply, xcb_render_query_pict_formats_reply_t*, (xcb_connection_t                        *c, xcb_render_query_pict_formats_cookie_t   cookie  /**< */, xcb_generic_error_t                    **e))
X(xcb_render_query_pict_formats_formats, xcb_render_pictforminfo_t *, (const xcb_render_query_pict_formats_reply_t *R))
X(xcb_render_free_picture, xcb_void_cookie_t, (xcb_connection_t *c, xcb_render_picture_t  picture))

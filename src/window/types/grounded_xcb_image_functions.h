/*
 * This file uses the X-List pattern to define all required xcb image functions
 */

X(xcb_create_pixmap_from_bitmap_data, xcb_pixmap_t, (xcb_connection_t *  display,xcb_drawable_t      d,uint8_t *           data,uint32_t            width,uint32_t            height,uint32_t            depth,uint32_t            fg,uint32_t            bg,xcb_gcontext_t *    gcp))

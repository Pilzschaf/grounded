/*
 * This file uses the X-List pattern to define all required xcb shm functions
 */

X(xcb_shm_query_version_reply, xcb_shm_query_version_reply_t*, (xcb_connection_t* c, xcb_shm_query_version_cookie_t cookie,xcb_generic_error_t** e))
X(xcb_shm_query_version, xcb_shm_query_version_cookie_t, (xcb_connection_t *c))
X(xcb_shm_attach, xcb_void_cookie_t, (xcb_connection_t *c, xcb_shm_seg_t shmseg, uint32_t shmid, uint8_t read_only))
X(xcb_shm_create_pixmap, xcb_void_cookie_t, (xcb_connection_t *c, xcb_pixmap_t pid, xcb_drawable_t drawable, uint16_t width, uint16_t height, uint8_t depth, xcb_shm_seg_t shmseg, uint32_t offset))
X(xcb_shm_attach_checked, xcb_void_cookie_t, (xcb_connection_t *c, xcb_shm_seg_t shmseg, uint32_t shmid, uint8_t read_only))
X(xcb_shm_create_segment, xcb_shm_create_segment_cookie_t, (xcb_connection_t *c, xcb_shm_seg_t shmseg, uint32_t size, uint8_t read_only))

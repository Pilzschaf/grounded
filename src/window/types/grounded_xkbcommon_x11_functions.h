

X(xkb_x11_keymap_new_from_device, struct xkb_keymap *,(struct xkb_context *context,xcb_connection_t *connection,int32_t device_id,enum xkb_keymap_compile_flags flags))
X(xkb_x11_state_new_from_device, struct xkb_state *,(struct xkb_keymap *keymap,xcb_connection_t *connection,int32_t device_id))

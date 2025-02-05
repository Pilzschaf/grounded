

X(xkb_context_new, struct xkb_context *,(enum xkb_context_flags flags))
X(xkb_state_update_mask, enum xkb_state_component,(struct xkb_state *state,xkb_mod_mask_t depressed_mods,xkb_mod_mask_t latched_mods,xkb_mod_mask_t locked_mods,xkb_layout_index_t depressed_layout,xkb_layout_index_t latched_layout,xkb_layout_index_t locked_layout))
X(xkb_state_key_get_one_sym, xkb_keysym_t, (struct xkb_state *state, xkb_keycode_t key))
X(xkb_keysym_to_utf32, uint32_t, (xkb_keysym_t keysym))
X(xkb_keysym_to_utf8, int, (xkb_keysym_t keysym, char *buffer, size_t size))
X(xkb_state_mod_name_is_active, int, (struct xkb_state *state, const char *name,enum xkb_state_component type))
X(xkb_keymap_new_from_string, struct xkb_keymap*, (struct xkb_context *context, const char *string,enum xkb_keymap_format format,enum xkb_keymap_compile_flags flags))
X(xkb_state_unref, void,(struct xkb_state *state))
X(xkb_state_new, struct xkb_state *,(struct xkb_keymap *keymap))
X(xkb_keymap_unref, void,(struct xkb_keymap *keymap))
X(xkb_keymap_key_repeats, int, (struct xkb_keymap *keymap, xkb_keycode_t key))
X(xkb_keysym_get_name, int, (xkb_keysym_t keysym, char* buffer, size_t size ))
X(xkb_keysym_to_upper, xkb_keysym_t, 	 (xkb_keysym_t ks))

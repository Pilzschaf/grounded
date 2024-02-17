/*
 * This file uses the X-List pattern to define all required xcb xkb functions
 */

X(xcb_xkb_select_events, xcb_void_cookie_t, (xcb_connection_t      *c,xcb_xkb_device_spec_t  deviceSpec,uint16_t               affectWhich,uint16_t               clear,uint16_t               selectAll,uint16_t               affectMap,uint16_t               map,const void            *details))
X(xcb_xkb_get_map, xcb_xkb_get_map_cookie_t, (xcb_connection_t      *c,xcb_xkb_device_spec_t  deviceSpec,uint16_t               full,uint16_t               partial,uint8_t                firstType,uint8_t                nTypes,xcb_keycode_t          firstKeySym,uint8_t                nKeySyms,xcb_keycode_t          firstKeyAction,uint8_t                nKeyActions,xcb_keycode_t          firstKeyBehavior,uint8_t                nKeyBehaviors,uint16_t               virtualMods,xcb_keycode_t          firstKeyExplicit,uint8_t                nKeyExplicit,xcb_keycode_t          firstModMapKey,uint8_t                nModMapKeys,xcb_keycode_t          firstVModMapKey,uint8_t                nVModMapKeys))
X(xcb_xkb_get_map_reply, xcb_xkb_get_map_reply_t *, (xcb_connection_t          *c,xcb_xkb_get_map_cookie_t   cookie  /**< */,xcb_generic_error_t      **e))
X(xcb_xkb_get_names, xcb_xkb_get_names_cookie_t, (xcb_connection_t      *c,xcb_xkb_device_spec_t  deviceSpec,uint32_t               which))
X(xcb_xkb_per_client_flags, xcb_xkb_per_client_flags_cookie_t,(xcb_connection_t      *c,xcb_xkb_device_spec_t  deviceSpec,uint32_t               change,uint32_t               value,uint32_t               ctrlsToChange,uint32_t               autoCtrls,uint32_t               autoCtrlsValues))
X(xcb_xkb_use_extension, xcb_xkb_use_extension_cookie_t, (xcb_connection_t *c,uint16_t          wantedMajor,uint16_t          wantedMinor))
X(xcb_xkb_get_names_reply, xcb_xkb_get_names_reply_t *, (xcb_connection_t            *c,xcb_xkb_get_names_cookie_t   cookie  /**< */, xcb_generic_error_t        **e))
X(xcb_xkb_use_extension_reply, xcb_xkb_use_extension_reply_t *, (xcb_connection_t                *c,xcb_xkb_use_extension_cookie_t   cookie  /**< */,xcb_generic_error_t            **e))
X(xcb_xkb_get_map_map, void *,(const xcb_xkb_get_map_reply_t *R))
X(xcb_xkb_get_map_map_unpack, int, (const void             *_buffer,uint8_t                 nTypes,uint8_t                 nKeySyms,uint8_t                 nKeyActions,uint16_t                totalActions,uint8_t                 totalKeyBehaviors,uint16_t                virtualMods,uint8_t                 totalKeyExplicit,uint8_t                 totalModMapKeys,uint8_t                 totalVModMapKeys,uint16_t                present,xcb_xkb_get_map_map_t  *_aux))
X(xcb_xkb_get_map_map_syms_rtrn_length, int, (const xcb_xkb_get_map_reply_t *R,const xcb_xkb_get_map_map_t *S))
X(xcb_xkb_get_map_map_syms_rtrn_iterator, xcb_xkb_key_sym_map_iterator_t, (const xcb_xkb_get_map_reply_t *R,const xcb_xkb_get_map_map_t *S))
X(xcb_xkb_key_sym_map_next, void, (xcb_xkb_key_sym_map_iterator_t *i))
X(xcb_xkb_get_device_info, xcb_xkb_get_device_info_cookie_t, (xcb_connection_t         *c,xcb_xkb_device_spec_t     deviceSpec,uint16_t                  wanted,uint8_t                   allButtons,uint8_t                   firstButton,uint8_t                   nButtons,xcb_xkb_led_class_spec_t  ledClass,xcb_xkb_id_spec_t         ledID))
X(xcb_xkb_get_device_info_reply, xcb_xkb_get_device_info_reply_t *,(xcb_connection_t                  *c,xcb_xkb_get_device_info_cookie_t   cookie  /**< */,xcb_generic_error_t              **e))
#ifndef GROUNDED_XKB_TYPES_H
#define GROUNDED_XKB_TYPES_H

struct xkb_context;
struct xkb_state;
struct xkb_keymap;

typedef uint32_t xkb_keysym_t;
typedef uint32_t xkb_keycode_t;
typedef uint32_t xkb_layout_index_t;
typedef uint32_t xkb_mod_mask_t;

enum xkb_keymap_compile_flags {
    /** Do not apply any flags. */
    XKB_KEYMAP_COMPILE_NO_FLAGS = 0
};

enum xkb_keymap_format {
    /** The current/classic XKB text format, as generated by xkbcomp -xkb. */
    XKB_KEYMAP_FORMAT_TEXT_V1 = 1
};

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

#define XKB_MOD_NAME_SHIFT      "Shift"
#define XKB_MOD_NAME_CAPS       "Lock"
#define XKB_MOD_NAME_CTRL       "Control"
#define XKB_MOD_NAME_ALT        "Mod1"
#define XKB_MOD_NAME_NUM        "Mod2"
#define XKB_MOD_NAME_LOGO       "Mod4"

#define XKB_LED_NAME_CAPS       "Caps Lock"
#define XKB_LED_NAME_NUM        "Num Lock"
#define XKB_LED_NAME_SCROLL     "Scroll Lock"

#endif // GROUNDED_XKB_TYPES_H

#ifndef GROUNDED_EGL_TYPES_H
#define GROUNDED_EGL_TYPES_H

typedef int32_t EGLint;
typedef unsigned int EGLenum;
typedef void *EGLContext;
typedef void *EGLSurface;
typedef void *EGLDisplay;
typedef void *EGLConfig;
typedef void *EGLNativeDisplayType;
typedef void *EGLNativeWindowType;
typedef unsigned int EGLBoolean;
#define EGL_CAST(type, value) ((type) (value))
#define EGL_NO_CONTEXT EGL_CAST(EGLContext,0)
#define EGL_NO_DISPLAY EGL_CAST(EGLDisplay,0)
#define EGL_NO_SURFACE EGL_CAST(EGLSurface,0)
#define EGL_DEFAULT_DISPLAY EGL_CAST(EGLNativeDisplayType,0)
#define EGL_OPENGL_API 0x30A2
#define EGL_RED_SIZE 0x3024
#define EGL_GREEN_SIZE 0x3023
#define EGL_BLUE_SIZE 0x3022
#define EGL_NONE 0x3038

#endif // GROUNDED_EGL_TYPES_H
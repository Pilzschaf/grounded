/*
 * This file uses the X-List pattern to define all required egl functions
 * Format: X(name, return, (parameters))
 */

X(eglBindAPI, EGLBoolean, (EGLenum api))
X(eglSwapInterval, EGLBoolean, (EGLDisplay dpy, EGLint interval))
X(eglTerminate, EGLBoolean, (EGLDisplay dpy))
X(eglMakeCurrent, EGLBoolean, (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx))
X(eglSwapBuffers, EGLBoolean, (EGLDisplay dpy, EGLSurface surface))
X(eglInitialize, EGLBoolean, (EGLDisplay dpy, EGLint *major, EGLint *minor))
X(eglGetDisplay, EGLDisplay, (EGLNativeDisplayType display_id))
X(eglCreateWindowSurface, EGLSurface, (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list))
X(eglCreateContext, EGLContext, (EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list))
X(eglChooseConfig, EGLBoolean, (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config))
X(eglDestroyContext, EGLBoolean,(EGLDisplay display, EGLContext context))
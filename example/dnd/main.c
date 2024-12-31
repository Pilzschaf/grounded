#include <grounded/window/grounded_window.h>
#include <grounded/threading/grounded_threading.h>
#include <grounded/memory/grounded_memory.h>
#include <grounded/math/grounded_math.h>

#include <stdio.h>
//#include <GL/gl.h>

#define GL_COLOR_BUFFER_BIT     0x00004000
#define GL_MODELVIEW			0x1700
#define GL_PROJECTION			0x1701
#define GL_BLEND				0x0BE2
#define GL_SRC_ALPHA			0x0302
#define GL_ONE_MINUS_SRC_ALPHA	0x0303
#define GL_TRIANGLES			0x0004
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 33346
#define GL_DEBUG_OUTPUT 37600

typedef struct Box {
    vec4 color;
    vec2 position;
    float size;
    GroundedWindow* associatedWindow;
} Box;

Box boxes[] = {
    {.color = {{1.0f, 0.0f, 0.0f, 1.0f}}, .position = {{100, 100}}, .size = 150.0f},
    {.color = {{0.0f, 0.0f, 1.0f, 1.0f}}, .position = {{300, 200}}, .size = 100.0f},
};

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLsizei;

/*typedef void (APIENTRY* DEBUGPROC)(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const char* message,
    const void* userParam);*/


static void openGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    String8 outputMessage = str8FromFormat(scratch, "[OpenGL Error] %s", message);
    GROUNDED_LOG_ERROR((const char*)outputMessage.base);
    arenaEndTemp(temp);
}

void (*glClearColor)(float, float, float, float);
void (*glClear)(int);
void (*glBegin)(unsigned int mode);
void (*glEnd)(void);
void (*glMatrixMode)(unsigned int mode);
void (*glLoadIdentity)(void);
void (*glLoadMatrixf)(const float *m);
void (*glViewport)(int x, int y, int width, int height);
void (*glEnable)(unsigned int cap);
void (*glBlendFunc)(unsigned int sfactor, unsigned int dfactor);
void (*glColor4f)(float red, float green, float blue, float alpha);
void (*glVertex2i)(int x, int y);
//void (*glDebugMessageCallback)(DEBUGPROC callback, const void* userParam);

GroundedOpenGLContext* openGLContext;

void updateAndRenderWindow(GroundedWindow* window) {
    if(!window) return;
    groundedMakeOpenGLContextCurrent(window, openGLContext);
    u32 windowWidth = groundedWindowGetWidth(window);
    u32 windowHeight = groundedWindowGetHeight(window);
    glViewport(0, 0, windowWidth, windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //glMatrixMode(GL_PROJECTION);
    glMatrixMode(5889);
    float a = 2.0f/windowWidth;
    float b = 2.0f / windowHeight;
    float proj[] = {
        a, 0.0f, 0.0f, 0.0f,
        0.0f, b, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
    };
    glLoadMatrixf(proj);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_TRIANGLES);

    for(u32 i = 0; i < ARRAY_COUNT(boxes); ++i) {
        if(boxes[i].associatedWindow == window) {
            glColor4f(boxes[i].color.r, boxes[i].color.g, boxes[i].color.b, boxes[i].color.a);
            vec2 position = boxes[i].position;
            float size = boxes[i].size;

            glVertex2i(position.x, position.y);
            glVertex2i(position.x + size, position.y);
            glVertex2i(position.x + size, position.y + size);

            glVertex2i(position.x, position.y);
            glVertex2i(position.x + size, position.y + size);
            glVertex2i(position.x, position.y + size);
        }
    }
    
    glEnd();

    groundedWindowGlSwapBuffers(window);
}

GROUNDED_WINDOW_DND_DROP_CALLBACK(boxDropCallback) {
    printf("Box drop\n");
    for(u32 i = 0; i < ARRAY_COUNT(boxes); ++i) {
        if(boxes[i].color.a < 1.0f) {
            boxes[i].associatedWindow = window;
            boxes[i].position = VEC2(x, groundedWindowGetHeight(window) - y - boxes[i].size);
            boxes[i].color.a = 1.0f;
        }
    }
}

GROUNDED_WINDOW_DND_CALLBACK(dndCallback) {
    for(u32 i = 0; i < mimeTypeCount; ++i) {
        if(str8IsEqual(mimeTypes[i], STR8_LITERAL("application/box"))) {
            *onDropCallback = boxDropCallback;
            return i;
        }
    }
    
    return 0xFFFFFFFF;
}

GROUNDED_WINDOW_DND_DATA_CALLBACK(dataCallback) {
    return STR8_LITERAL("Box");
}

GROUNDED_WINDOW_DND_DRAG_FINISH_CALLBACK(dragFinishCallback) {
    struct Box* box = (struct Box*)userData;
    box->color.a = 1.0f;
}

int main() {
    { // Thread context initialization
        MemoryArena arena1 = createGrowingArena(osGetMemorySubsystem(), KB(256));
        MemoryArena arena2 = createGrowingArena(osGetMemorySubsystem(), KB(16));

        threadContextInit(arena1, arena2, &groundedDefaultConsoleLogger);
    }

    groundedInitWindowSystem();

    // Create window1
    GroundedWindow* window1 = groundedCreateWindow(threadContextGetScratch(0), &(struct GroundedWindowCreateParameters){
        .title = STR8_LITERAL("DND window1"),
        .minWidth = 320,
        .minHeight = 240,
        .width = 1240,
        .height = 720,
        .dndCallback = dndCallback,
    });

    // Create window2
    GroundedWindow* window2 = groundedCreateWindow(threadContextGetScratch(threadContextGetScratch(0)), &(struct GroundedWindowCreateParameters) {
        .title = STR8_LITERAL("DND window2"),
        .minWidth = 320,
        .minHeight = 240,
        .width = 1240,
        .height = 720,
        .dndCallback = dndCallback,
    });

    boxes[0].associatedWindow = window1;
    boxes[1].associatedWindow = window2;

    u64 lastButtonDownCounter = 0;
    u64 lastButtonUpCounter = 0;

    openGLContext = groundedCreateOpenGLContext(threadContextGetScratch(0), 0);
    groundedMakeOpenGLContextCurrent(window1, openGLContext);

    glClearColor = groundedWindowLoadGlFunction("glClearColor");
    glClear = groundedWindowLoadGlFunction("glClear");
    glBegin = groundedWindowLoadGlFunction("glBegin");
    glEnd = groundedWindowLoadGlFunction("glEnd");
    glMatrixMode = groundedWindowLoadGlFunction("glMatrixMode");
    glLoadIdentity = groundedWindowLoadGlFunction("glMatrixMode");
    glLoadMatrixf = groundedWindowLoadGlFunction("glLoadMatrixf");
    glViewport = groundedWindowLoadGlFunction("glViewport");
    glEnable = groundedWindowLoadGlFunction("glEnable");
    glBlendFunc = groundedWindowLoadGlFunction("glBlendFunc");
    glColor4f = groundedWindowLoadGlFunction("glColor4f");
    glVertex2i = groundedWindowLoadGlFunction("glVertex2i");
    //glDebugMessageCallback = groundedWindowLoadGlFunction("glDebugMessageCallback");

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    //glDebugMessageCallback(openGLDebugCallback, 0);

    // Message loop
    u32 eventCount = 0;
    bool running = true;
    while(running) {
        GroundedEvent* events = groundedWindowPollEvents(&eventCount);
        for(u32 i = 0; i < eventCount; ++i) {
            if(events[i].type == GROUNDED_EVENT_TYPE_CLOSE_REQUEST) {
                if(events[i].window == window1) {
                    groundedDestroyWindow(events[i].window);
                    window1 = 0;
                } else if(events[i].window == window2) {
                    groundedDestroyWindow(events[i].window);
                    window2 = 0;
                }
                // Completely close application when one of the window gets closed
                if(!window1 && !window2) {
                    running = false;
                }
                break;
            }
            if(events[i].type == GROUNDED_EVENT_TYPE_BUTTON_DOWN) {
                if(events[i].buttonDown.button == GROUNDED_MOUSE_BUTTON_LEFT) {
                    for(u32 j = 0; j < ARRAY_COUNT(boxes); ++j) {
                        if(boxes[j].associatedWindow == events[i].window) {
                            vec2 minPoint = boxes[j].position;
                            vec2 maxPoint = v2Add(minPoint, VEC2(boxes[j].size, boxes[j].size));
                            vec2 mousePosition = VEC2(events[i].buttonDown.mousePositionX, events[i].buttonDown.mousePositionY);
                            mousePosition.y = groundedWindowGetHeight(boxes[j].associatedWindow) - mousePosition.y;
                            if(v2InRect(minPoint, maxPoint, mousePosition)) {
                                String8 mimeType = STR8_LITERAL("application/box");
                                MemoryArena* scratch = threadContextGetScratch(0);
                                ArenaTempMemory temp = arenaBeginTemp(scratch);
                                u32* pixelData = ARENA_PUSH_ARRAY(scratch, boxes[j].size * boxes[j].size, u32);
                                vec4 color = boxes[j].color;
                                for(u32 i = 0; i < (boxes[j].size * boxes[j].size); ++i) {
                                    pixelData[i] = ((u32)(color.a * 255) << 24) | (u32)(color.r * 255) << 16 | (u32)(color.g * 255) << 8 | (u32)(color.b * 255) << 0;
                                }
                                GroundedWindowDragPayloadDescription* desc = groundedWindowPrepareDragPayload(boxes[j].associatedWindow);
                                groundedWindowDragPayloadSetImage(desc, (u8*)pixelData, boxes[j].size, boxes[j].size);
                                groundedWindowDragPayloadSetMimeTypes(desc, 1, &mimeType);
                                groundedWindowDragPayloadSetDataCallback(desc, dataCallback);
                                groundedWindowDragPayloadSetDragFinishCallback(desc, dragFinishCallback);
                                groundedWindowBeginDragAndDrop(desc, &boxes[j]);
                                arenaEndTemp(temp);
                                //boxes[j].associatedWindow = 0;
                                boxes[j].color.a = 0.5f;
                            }
                        }
                    }
                }
            }
        }

        updateAndRenderWindow(window1);
        updateAndRenderWindow(window2);
    }

    // Release resources
    groundedWindowDestroyOpenglGontext(openGLContext);
    if(window1) {
        groundedDestroyWindow(window1);
    }
    if(window2) {
        groundedDestroyWindow(window2);
    }
    groundedShutdownWindowSystem();

    return 0;
}
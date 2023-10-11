#include <grounded/window/grounded_window.h>
#include <grounded/threading/grounded_threading.h>
#include <grounded/memory/grounded_memory.h>
#include <grounded/math/grounded_math.h>

#include <stdio.h>
#include <GL/gl.h>

#define GL_COLOR_BUFFER_BIT 0x00004000

//TODO: We need a way to know when a drag has been canceled! In this case we have to fill back the window!
//TODO: We need a major rewrite of DragPayload creation and respecitve memory management.
//TODO: Some kind of prepare/submit style API might be best so we can reuse the arena that has to be created by the backend for the payload
//TODO: Something like GroundedPayloadDescription which we initialize and can get a MemoryArena from.
//TODO: GroundedPayloadDescription is an opque struct and must be passed to startDragAndDrop function

typedef struct Box {
    vec4 color;
    vec2 position;
    float size;
    GroundedWindow* associatedWindow;
} Box;

Box boxes[] = {
    {.color = VEC4(1.0f, 0.0f, 0.0f, 1.0f), .position = VEC2(100, 100), .size = 150.0f},
    {.color = VEC4(0.0f, 0.0f, 1.0f, 1.0f), .position = VEC2(300, 200), .size = 100.0f},
};

GroundedOpenGLContext* openGLContext;

void updateAndRenderWindow(GroundedWindow* window) {
    groundedMakeOpenGLContextCurrent(window, openGLContext);
    u32 windowWidth = groundedWindowGetWidth(window);
    u32 windowHeight = groundedWindowGetHeight(window);
    glViewport(0, 0, windowWidth, windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
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

GROUNDED_WINDOW_DND_SEND_CALLBACK(sendCallback) {
    return STR8_LITERAL("Box");
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
    });

    // Create window2
    GroundedWindow* window2 = groundedCreateWindow(threadContextGetScratch(threadContextGetScratch(0)), &(struct GroundedWindowCreateParameters){
        .title = STR8_LITERAL("DND window2"),
        .minWidth = 320,
        .minHeight = 240,
        .width = 1240,
        .height = 720,
    });

    boxes[0].associatedWindow = window1;
    boxes[1].associatedWindow = window2;

    u64 lastButtonDownCounter = 0;
    u64 lastButtonUpCounter = 0;

    openGLContext = groundedCreateOpenGLContext(threadContextGetScratch(0), 0);
    groundedMakeOpenGLContextCurrent(window1, openGLContext);

    void (*glClearColor)(float, float, float, float) = groundedWindowLoadGlFunction("glClearColor");
    void (*glClear)(int) = groundedWindowLoadGlFunction("glClear");

    // Message loop
    u32 eventCount = 0;
    bool running = true;
    while(running) {
        GroundedEvent* events = groundedPollEvents(&eventCount);
        for(u32 i = 0; i < eventCount; ++i) {
            if(events[i].type == GROUNDED_EVENT_TYPE_CLOSE_REQUEST) {
                // Completely close application when one of the window gets closed
                running = false;
                break;
            }
            if(events[i].type == GROUNDED_EVENT_TYPE_BUTTON_DOWN) {
                if(events[i].buttonDown.button == GROUNDED_MOUSE_BUTTON_LEFT) {
                    for(u32 j = 0; j < ARRAY_COUNT(boxes); ++j) {
                        if(boxes[j].associatedWindow == events[i].buttonDown.window) {
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
                                GroundedWindowDragPayloadImage* payloadImage = groundedWindowCreateDragImage(scratch, (u8*)pixelData, boxes[j].size, boxes[j].size);
                                groundedStartDragAndDrop(events[i].buttonDown.window, 1, &mimeType, sendCallback, payloadImage, 0);
                                arenaEndTemp(temp);
                                boxes[j].associatedWindow = 0;
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
    groundedDestroyWindow(window1);
    groundedDestroyWindow(window2);
    groundedShutdownWindowSystem();

    return 0;
}
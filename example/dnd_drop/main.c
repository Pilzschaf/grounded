#include <grounded/window/grounded_window.h>
#include <grounded/memory/grounded_memory.h>
#include <grounded/logger/grounded_logger.h>
#include <grounded/threading/grounded_threading.h>

#include <stdio.h>

void (*glClearColor)(float, float, float, float);
void (*glClear)(int);
#define GL_COLOR_BUFFER_BIT     0x00004000

GROUNDED_WINDOW_DND_DROP_CALLBACK(uriListCallback) {
    //TODO: Fix data retrieval in xcb
    GROUNDED_LOG_INFOF("Dropped an text/uri-list\n");
    GROUNDED_LOG_INFOF("Data: %.*s\n", (int)data.size, (const char*)data.base);
}

GROUNDED_WINDOW_DND_CALLBACK(dndCallback) {
    u32 acceptedIndex = UINT32_MAX;
    for(u32 i = 0; i < mimeTypeCount; ++i) {
        GROUNDED_LOG_INFOF("Mime type[%u]: %s\n", i, (const char*)mimeTypes[i].base);
        if(str8Compare(mimeTypes[i], STR8_LITERAL("text/uri-list"))) {
            *onDropCallback = &uriListCallback;
            acceptedIndex = i;
        }
    }
    return acceptedIndex;
}

int main() {
    { // Thread context initialization
        MemoryArena arena1 = createGrowingArena(osGetMemorySubsystem(), KB(256));
        MemoryArena arena2 = createGrowingArena(osGetMemorySubsystem(), KB(16));
        enableDebugMemoryOverflowDetectForArena(&arena1);
        enableDebugMemoryOverflowDetectForArena(&arena2);

        threadContextInit(arena1, arena2, &groundedDefaultConsoleLogger);
    }

    groundedInitWindowSystem();

    GroundedWindow* window = groundedCreateWindow(threadContextGetScratch(0), &(struct GroundedWindowCreateParameters) {
        .title = STR8_LITERAL("Drop here"),
        .width = 1240,
        .height = 720,
        .minWidth = 320,
        .minHeight = 240,
        .dndCallback = dndCallback,
    });

    GroundedOpenGLContext* openGLContext = groundedCreateOpenGLContext(threadContextGetScratch(0), 0);
    groundedMakeOpenGLContextCurrent(window, openGLContext);
    glClearColor = groundedWindowLoadGlFunction("glClearColor");
    glClear = groundedWindowLoadGlFunction("glClear");

    bool running = true;
    while(running) {
        u32 eventCount = 0;
        GroundedEvent* events = groundedWindowGetEvents(&eventCount, 0);
        for(u32 i = 0; i < eventCount; ++i) {
            if(events[i].type == GROUNDED_EVENT_TYPE_CLOSE_REQUEST) {
                running = false;
            }
        }

        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        groundedWindowGlSwapBuffers(window);
    }

    groundedWindowDestroyOpenglGontext(openGLContext);
    groundedDestroyWindow(window);
    groundedShutdownWindowSystem();
    return 0;
}
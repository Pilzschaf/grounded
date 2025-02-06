#include <grounded/window/grounded_window.h>
#include <grounded/threading/grounded_threading.h>
#include <grounded/memory/grounded_memory.h>

//#include <GL/gl.h>

#define GL_COLOR_BUFFER_BIT 0x00004000

int main() {
    { // Thread context initialization
        MemoryArena arena1 = createGrowingArena(osGetMemorySubsystem(), KB(256));
        MemoryArena arena2 = createGrowingArena(osGetMemorySubsystem(), KB(16));

        threadContextInit(arena1, arena2, &groundedDefaultConsoleLogger);
    }

    groundedInitWindowSystem();

    // Create window
    GroundedWindow* window = groundedCreateWindow(threadContextGetScratch(0), &(struct GroundedWindowCreateParameters){
        .title = STR8_LITERAL("Double click window"),
        .minWidth = 320,
        .minHeight = 240,
        .width = 1920,
        .height = 1080,
    });

    u64 lastButtonDownCounter = 0;
    u64 lastButtonUpCounter = 0;

    GroundedOpenGLContext* openGLContext = groundedCreateOpenGLContext(threadContextGetScratch(0), 0);
    groundedMakeOpenGLContextCurrent(window, openGLContext);

    void (*glClearColor)(float, float, float, float) = groundedWindowLoadGlFunction("glClearColor");
    void (*glClear)(int) = groundedWindowLoadGlFunction("glClear");

    // Message loop
    u32 eventCount = 0;
    bool running = true;
    while(running) {
        GroundedEvent* events = groundedWindowPollEvents(&eventCount);
        for(u32 i = 0; i < eventCount; ++i) {
            if(events[i].type == GROUNDED_EVENT_TYPE_CLOSE_REQUEST) {
                running = false;
                break;
            }
            if(events[i].type == GROUNDED_EVENT_TYPE_BUTTON_DOWN && events[i].buttonDown.button == GROUNDED_MOUSE_BUTTON_LEFT) {
                u64 counter = groundedGetCounter();

                double deltaDown = (counter - lastButtonDownCounter) / 1000000.0;
                double deltaUp = (counter - lastButtonUpCounter) / 1000000.0;
                GROUNDED_LOG_INFOF("Button down\n");
                //GROUNDED_LOG_INFOF("\t\tdeltaDown: %f\n", deltaDown);
                //GROUNDED_LOG_INFOF("\t\tdeltaUp: %f\n", deltaUp);
                lastButtonDownCounter = counter;
            }
            if(events[i].type == GROUNDED_EVENT_TYPE_BUTTON_UP && events[i].buttonUp.button == GROUNDED_MOUSE_BUTTON_LEFT) {
                u64 counter = groundedGetCounter();

                double deltaDown = (counter - lastButtonDownCounter) / 1000000.0;
                double deltaUp = (counter - lastButtonUpCounter) / 1000000.0;
                GROUNDED_LOG_INFOF("Button up\n");
                GROUNDED_LOG_INFOF("\t\tdeltaDown: %f\n", deltaDown);
                GROUNDED_LOG_INFOF("\t\tdeltaUp: %f\n", deltaUp);
                if(deltaUp < 250) {
                    GROUNDED_LOG_INFOF("Double click\n");
                }

                lastButtonUpCounter = counter;
            }
        }

        // Do your per-frame work here
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        groundedWindowGlSwapBuffers(window);
    }

    // Release resources
    groundedWindowDestroyOpenglGontext(openGLContext);
    groundedDestroyWindow(window);
    groundedShutdownWindowSystem();

    return 0;
}
#include <grounded/window/grounded_window.h>
#include <grounded/threading/grounded_threading.h>
#include <grounded/memory/grounded_memory.h>

#include <stdio.h>

// Switch mouse cursor by pressing C
int main() {
    { // Thread context initialization
        MemoryArena arena1 = createGrowingArena(osGetMemorySubsystem(), KB(256));
        MemoryArena arena2 = createGrowingArena(osGetMemorySubsystem(), KB(16));

        threadContextInit(arena1, arena2, &groundedDefaultConsoleLogger);
    }

    groundedInitWindowSystem();

    // Create window
    GroundedWindow* window = groundedCreateWindow(&(struct GroundedWindowCreateParameters){
        .title = STR8_LITERAL("Simple grounded window"),
        .minWidth = 320,
        .minHeight = 240,
    });

    // Message loop
    u32 eventCount = 0;
    bool running = true;
    u32 cursorType = GROUNDED_MOUSE_CURSOR_DEFAULT;
    groundedSetCursorType(cursorType);
    while(running) {
        GroundedEvent* events = groundedPollEvents(&eventCount);
        for(u32 i = 0; i < eventCount; ++i) {
            if(events[i].type == GROUNDED_EVENT_TYPE_CLOSE_REQUEST) {
                running = false;
                break;
            } else if(events[i].type == GROUNDED_EVENT_TYPE_KEY_DOWN && events[i].keyDown.keycode == GROUNDED_KEY_C) {
                cursorType++;
                if(cursorType == GROUNDED_MOUSE_CURSOR_CUSTOM) {
                    cursorType++;
                }
                if(cursorType >= GROUNDED_MOUSE_CURSOR_COUNT) {
                    cursorType = 0;
                }
                printf("Switching to cursor %u\n", cursorType);
                groundedSetCursorType(cursorType);
            }
        }

        // Do your per-frame work here
    }

    // Release resources
    groundedDestroyWindow(window);
    groundedShutdownWindowSystem();

    return 0;
}
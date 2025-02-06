#include <grounded/logger/grounded_logger.h>

#include <stdio.h>

GROUNDED_FUNCTION GROUNDED_LOG_FUNCTION(groundedDefaultConsoleLogger) {
    const char* colorStart = "";
    const char* colorEnd = "";
    if(level == GROUNDED_LOG_LEVEL_WARNING) {
        colorStart = "\033[33m";
        colorEnd = "\033[0m";
    } else if(level >= GROUNDED_LOG_LEVEL_ERROR) {
        colorStart = "\033[31m";
        colorEnd = "\033[0m";
    }
    printf("%s[%.*s:%lu] %s%s\n", colorStart, (int)filename.size, (const char*)filename.base, lineNumber, message, colorEnd);
}

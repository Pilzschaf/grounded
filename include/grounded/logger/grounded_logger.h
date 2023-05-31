#ifndef GROUNDED_LOGGER_H
#define GROUNDED_LOGGER_H

#include "../grounded.h"

typedef enum GroundedLogLevel {
    GROUNDED_LOG_LEVEL_VERBOSE,
    GROUNDED_LOG_LEVEL_DEBUG,
    GROUNDED_LOG_LEVEL_INFO,
    GROUNDED_LOG_LEVEL_WARNING,
    GROUNDED_LOG_LEVEL_ERROR,
    GROUNDED_LOG_LEVEL_FATAL,
    GROUNDED_LOG_LEVEL_COUNT,
} GroundedLogLevel;

#define GROUNDED_LOG_FUNCTION(name) void name(const char* message, GroundedLogLevel level, const char* filename, u64 lineNumber)
typedef GROUNDED_LOG_FUNCTION(GroundedLogFunction);

GROUNDED_FUNCTION GroundedLogFunction* threadContextGetLogFunction();
#define GROUNDED_LOG(message, level)  (threadContextGetLogFunction()(message, level, __FILE__, __LINE__))
#define GROUNDED_LOG_VERBOSE(message) GROUNDED_LOG(message, GROUNDED_LOG_LEVEL_VERBOSE)
#define GROUNDED_LOG_DEBUG(message) GROUNDED_LOG(message, GROUNDED_LOG_LEVEL_DEBUG)
#define GROUNDED_LOG_INFO(message) GROUNDED_LOG(message, GROUNDED_LOG_LEVEL_INFO)
#define GROUNDED_LOG_WARNING(message) GROUNDED_LOG(message, GROUNDED_LOG_LEVEL_WARNING)
#define GROUNDED_LOG_ERROR(message) GROUNDED_LOG(message, GROUNDED_LOG_LEVEL_ERROR)
#define GROUNDED_LOG_FATAL(message) GROUNDED_LOG(message, GROUNDED_LOG_LEVEL_FATAL)

GROUNDED_FUNCTION  GROUNDED_LOG_FUNCTION(groundedDefaultConsoleLogger);

#endif // GROUNDED_LOGGER_H
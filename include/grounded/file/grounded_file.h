#ifndef GROUNDED_FILE_H
#define GROUNDED_FILE_H
#include "../memory/grounded_arena.h"
#include "../memory/grounded_stream.h"

GROUNDED_FUNCTION u8* groundedReadFile(MemoryArena* arena, String8 filename, u64* size);
GROUNDED_FUNCTION u8* groundedReadFileImmutable(String8 filename, u64* size);
GROUNDED_FUNCTION void groundedFreeFileImmutable(u8* file, u64 size);

GROUNDED_FUNCTION bool groundedWriteFile(String8 filename, const void* data, u64 size);

enum FileMode {
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_READ_WRITE,
    FILE_MODE_COUNT,
};
typedef struct GroundedFile {
    #ifdef _WIN32
    // typedef void* HANDLE;
    void* handle;
    #else
    int fd;
    #endif
} GroundedFile;
GROUNDED_FUNCTION GroundedFile groundedOpenFile(String8 filename, enum FileMode);
GROUNDED_FUNCTION u64 groundedFileRead(GroundedFile file, u8* buffer, u64 size);
GROUNDED_FUNCTION u64 groundedFileWrite(GroundedFile file, u8* buffer, u64 size);
GROUNDED_FUNCTION BufferedStreamWriter groundedFileGetStreamWriterFromFilename(MemoryArena* arena, String8 filename, u64 bufferSize);
GROUNDED_FUNCTION BufferedStreamWriter groundedFileGetStreamWriterFromFile(MemoryArena* arena, GroundedFile* file, u64 bufferSize);
GROUNDED_FUNCTION BufferedStreamReader groundedFileGetStreamReaderFromFilename(MemoryArena* arena, String8 filename, u64 bufferSize);
GROUNDED_FUNCTION BufferedStreamReader groundedFileGetStreamReaderFromFile(MemoryArena* arena, GroundedFile* file, u64 bufferSize);
GROUNDED_FUNCTION void groundedCloseFile(GroundedFile* file);

GROUNDED_FUNCTION bool groundedDoesFileExist(String8 filename);
GROUNDED_FUNCTION bool groundedDoesDirectoryExist(String8 directory);
GROUNDED_FUNCTION bool groundedCreateDirectory(String8 directory);
GROUNDED_FUNCTION bool groundedEnsureDirectoryExists(String8 directory);
GROUNDED_FUNCTION String8 groundedGetAbsoluteDirectory(MemoryArena* arena, String8 directory);

GROUNDED_FUNCTION String8 groundedGetLinkTarget(MemoryArena* arena, String8 filename);

// Directory iteration
typedef struct GroundedDirectoryIterator GroundedDirectoryIterator;
enum GroundedDirectoryEntryType {
    GROUNDED_DIRECTORY_ENTRY_TYPE_NONE,
    GROUNDED_DIRECTORY_ENTRY_TYPE_FILE,
    GROUNDED_DIRECTORY_ENTRY_TYPE_DIRECTORY,
    GROUNDED_DIRECTORY_ENTRY_TYPE_LINK,
    GROUNDED_DIRECTORY_ENTRY_TYPE_COUNT,
};
enum GroundedDirectoryEntryFlags {
    GROUNDED_DIRECTORY_ENTRY_FLAG_NONE = 0,
    GROUNDED_DIRECTORY_ENTRY_FLAG_HIDDEN = 1<<0,
};
typedef struct GroundedDirectoryEntry {
    String8 name;
    enum GroundedDirectoryEntryType type;
    enum GroundedDirectoryEntryFlags flags;
} GroundedDirectoryEntry;
GROUNDED_FUNCTION GroundedDirectoryIterator* groundedCreateDirectoryIterator(MemoryArena* arena, String8 directory);
GROUNDED_FUNCTION struct GroundedDirectoryEntry groundedGetNextDirectoryEntry(GroundedDirectoryIterator* iterator);
GROUNDED_FUNCTION void groundedDestroyDirectoryIterator(GroundedDirectoryIterator* iterator);
typedef struct GroundedListFilesParameters {
    bool allowDot;
    bool allowDoubleDot;
    bool ignoreHiddenFiles;
} GroundedListFilesParameters;
GROUNDED_FUNCTION GroundedDirectoryEntry* groundedListFilesOfDirectory(MemoryArena* arena, String8 directory, u64* resultCount, GroundedListFilesParameters* parameters);


enum GroundedDirectoryWatchEventType {
    GROUNDED_DIRECTORY_WATCH_EVENT_TYPE_CREATE,
    GROUNDED_DIRECTORY_WATCH_EVENT_TYPE_MODIFY,
    GROUNDED_DIRECTORY_WATCH_EVENT_TYPE_DELETE,
};

typedef struct GroundedDirectoryWatch GroundedDirectoryWatch;
typedef struct GroundedDirectoryWatchEvent {
    String8 filename;
    String8 directory;
    enum GroundedDirectoryWatchEventType type;
} GroundedDirectoryWatchEvent;

#define WATCH_FILE_CALLBACK(name) void name(GroundedDirectoryWatchEvent event)
typedef WATCH_FILE_CALLBACK(WatchFileCallback);

GROUNDED_FUNCTION GroundedDirectoryWatch* groundedDirectoryWatchCreate(MemoryArena* arena, String8 directory, bool watchSubdirectories);
GROUNDED_FUNCTION GroundedDirectoryWatchEvent* groundedDirectoryWatchPollEvents(GroundedDirectoryWatch* watch, MemoryArena* arena, u64* eventCount);
GROUNDED_FUNCTION GroundedDirectoryWatchEvent* groundedDirectoryWatchWaitForEvents(GroundedDirectoryWatch* watch, MemoryArena* arena, u64* eventCount, u32 timeoutInMs);
GROUNDED_FUNCTION void groundedDirectoryWatchPollEventsCallback(GroundedDirectoryWatch* watch, WatchFileCallback callback);
GROUNDED_FUNCTION void groundedDirectoryWatchWaitForEventsCallback(GroundedDirectoryWatch* watch, WatchFileCallback callback, u32 timeoutInMs);
GROUNDED_FUNCTION void groundedDirectoryWatchDestroy(GroundedDirectoryWatch* directoryWatch);


// Timestamp in seconds
GROUNDED_FUNCTION u64 groundedGetCreationTimestamp(String8 filename);

// Timestamp in seconds
GROUNDED_FUNCTION u64 groundedGetModificationTimestamp(String8 filename);

GROUNDED_FUNCTION String8 groundedGetUserConfigDirectory(MemoryArena* arena);
GROUNDED_FUNCTION String8 groundedGetCacheDirectory(MemoryArena* arena);
GROUNDED_FUNCTION String8 groundedGetCurrentWorkingDirectory(MemoryArena* arena);
GROUNDED_FUNCTION String8 groundedGetBinaryDirectory(MemoryArena* arena);

#endif // GROUNDED_FILE_H

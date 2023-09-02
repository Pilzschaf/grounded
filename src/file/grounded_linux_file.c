#include <grounded/file/grounded_file.h>
#include <grounded/threading/grounded_threading.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <stdlib.h> // getenv
#include <pwd.h> // getpwuid
#include <errno.h>

//#include <liburing.h>

GROUNDED_FUNCTION  u8* groundedReadFile(MemoryArena* arena, String8 filename, u64* size) {
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    u8* result = 0;
    ArenaMarker releaseMarker = arenaCreateMarker(arena);

    int fileHandle = openat(AT_FDCWD, str8GetCstr(scratch, filename), O_RDONLY);
    if(fileHandle >= 0) {
        struct stat fileStats;
        if(fstat(fileHandle, &fileStats) >= 0) {
            u64 fileSize = fileStats.st_size;
            if(fileSize > 0) {
                u64 sizeLeft = fileSize;
                result = ARENA_PUSH_ARRAY_NO_CLEAR(arena, fileSize, u8);
                u8* cursor = result;
                if(result) {
                    while(sizeLeft > 0) {
                        ssize_t bytesRead = read(fileHandle, cursor, sizeLeft);
                        if(bytesRead < 0) {
                            result = 0;
                            break;
                        } else if(bytesRead == 0) {
                            // Unexpected file end
                            GROUNDED_LOG_ERROR("Unexpected end of file while reading file");
                            fileSize = fileSize - sizeLeft;
                            arenaPopTo(arena, cursor);
                            break;
                        }
                        sizeLeft -= bytesRead;
                        cursor += bytesRead;
                    }
                    
                    // Report size to caller
                    if(size && result) {
                        *size = fileSize;
                    }
                    
                    if(close(fileHandle) < 0) {
                        GROUNDED_LOG_ERROR("Error closing file handle after read");
                    }
                }

                // Result == 0 means there was an error reading the file
                if(!result) {
                    arenaResetToMarker(releaseMarker);
                }
            }
        } else {
            GROUNDED_LOG_ERROR("Could not get filestats");
        }
    }

    arenaEndTemp(temp);
    return result;
}

GROUNDED_FUNCTION  u8* groundedReadFileImmutable(String8 filename, u64* size) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    u8* result = 0;
    *size = 0;

    // Map the file directly into address space
    int fileHandle = openat(AT_FDCWD, str8GetCstr(scratch, filename), O_RDONLY);
    if(fileHandle >= 0) {
        struct stat fileStats;
        if(fstat(fileHandle, &fileStats) >= 0) {
            // MAP_PRIVATE as we do not want a shared mapping
            // MAP_POPULATE could be used to prefault eg. read the whole file immediately
            void* memory = mmap(0, fileStats.st_size, PROT_READ, MAP_PRIVATE, fileHandle, 0);
            if(memory) {
                // Advise that this memory is soon to be accessed so it might read ahead a bit
                if(madvise(memory, fileStats.st_size, MADV_WILLNEED) < 0) {
                    GROUNDED_LOG_WARNING("Could not set madvise on mapped file");
                }
                // Advise that this memory is probably read sequentially
                //madvise(memory, fileStats.st_size, MADV_SEQUENTIAL);
                *size = fileStats.st_size;
            } else {
                GROUNDED_LOG_ERROR("Could not map file to memory");
            }
            result = memory;
        } else {
            GROUNDED_LOG_ERROR("Could not get filestats");
        }
    }

    arenaEndTemp(temp);
    return result;
}

GROUNDED_FUNCTION  void groundedFreeFileImmutable(u8* file, u64 size) {
    if(munmap(file, size)) {
        GROUNDED_LOG_ERROR("Failed to unmap immutable file");
    }
}

GROUNDED_FUNCTION  bool groundedWriteFile(String8 filename, const void* data, u64 size) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    int fileHandle = openat(AT_FDCWD, str8GetCstr(scratch, filename), O_WRONLY | O_CREAT | O_TRUNC, 0664);
    arenaEndTemp(temp);

    if(fileHandle >= 0) {
        u64 sizeLeft = size;
        const u8* cursor = data;
        while(sizeLeft > 0) {
            ssize_t bytesWritten = write(fileHandle, cursor, sizeLeft);
            if(bytesWritten <= 0) {
                GROUNDED_LOG_ERROR("Error while writing data to file");
                close(fileHandle);
                return false;
            }
            sizeLeft -= bytesWritten;
            cursor += bytesWritten;
        }

        if(close(fileHandle) < 0) {
            GROUNDED_LOG_ERROR("Error closing file");
        }
    } else {
        GROUNDED_LOG_ERROR("Error opening file for writing");
    }

    return fileHandle >= 0;
}


GROUNDED_FUNCTION  bool groundedCreateDirectory(String8 directory) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    bool result = mkdir(str8GetCstr(scratch, directory), 0755) == 0;
    if(!result) {
        GROUNDED_LOG_ERROR("Error creating directory");
    }

    arenaEndTemp(temp);
    return result;
}

GROUNDED_FUNCTION String8 groundedGetAbsoluteDirectory(MemoryArena* arena, String8 directory) {
    char* buffer = ARENA_PUSH_ARRAY_NO_CLEAR(arena, PATH_MAX, char);

    MemoryArena* tempArena = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(tempArena);
    const char* cPath = str8GetCstr(tempArena, directory);
    buffer[0] = '\0';
    realpath(cPath, buffer);
    arenaEndTemp(temp);

    String8 result = str8FromCstr(buffer);
    arenaPopTo(arena, result.base + result.size);
    return result;
}


struct GroundedFile {
    MemoryArena* arena;
    int fd;
    u8* buffer;
    u64 bufferSize;
};
GROUNDED_FUNCTION  GroundedFile* groundedOpenFile(MemoryArena* arena, String8 filename, enum FileMode fileMode) {
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    ArenaMarker errorMarker = arenaCreateMarker(arena);

    struct GroundedFile* result = ARENA_PUSH_STRUCT(arena, struct GroundedFile);
    if(result) {
        int flags = O_CREAT;
        if(fileMode == FILE_MODE_READ) {
            flags |= O_RDONLY;
        } else if(fileMode == FILE_MODE_READ_WRITE) {
            flags |= O_RDWR;
        } else if(fileMode == FILE_MODE_WRITE) {
            flags |= O_WRONLY | O_TRUNC;
        }

        result->fd = openat(AT_FDCWD, str8GetCstr(scratch, filename), flags, 0664);
        result->arena = arena;
        if(result->fd < 0) {
            GROUNDED_LOG_ERROR("Error opening file");
            result = 0;
        } else {
            result->bufferSize = KB(4);
            result->buffer = ARENA_PUSH_ARRAY(arena, result->bufferSize, u8);
            if(!result->buffer) {
                GROUNDED_LOG_ERROR("Could not allocate buffer for file");
                close(result->fd);
                result = 0;
            }
        }
    }
    if(!result) {
        arenaResetToMarker(errorMarker);
    }
    arenaEndTemp(temp);
    return result;
}

static enum GroundedStreamErrorCode fileRefill(BufferedStreamReader* r) {
    struct GroundedFile* f = (struct GroundedFile*)r->implementationPointer;
    s64 bytesRead = read(f->fd, f->buffer, f->bufferSize);
    if(bytesRead == 0) {
        refillZeros(r);
        r->refill = refillZeros;
        r->error = GROUNDED_STREAM_PAST_EOF;
        return GROUNDED_STREAM_PAST_EOF;
    } else if(bytesRead < 0) {
        refillZeros(r);
        r->refill = refillZeros;
        r->error = GROUNDED_STREAM_IO_ERROR;
        return r->error;
    } else {
        r->start = f->buffer;
        r->end = f->buffer + bytesRead;
        r->cursor = r->start;
        return GROUNDED_STREAM_SUCCESS;
    }
}

static void groundedFileStreamReaderClose(BufferedStreamReader* reader) {
    GroundedFile* file = reader->implementationPointer;
    groundedCloseFile(file);
}

GROUNDED_FUNCTION BufferedStreamReader groundedFileGetStreamReaderFromFile(GroundedFile* file) {
    struct GroundedFile* f = (struct GroundedFile*)file;
    BufferedStreamReader result = {
        .start = f->buffer,
        .cursor = f->buffer,
        .end = f->buffer + f->bufferSize,
        .implementationPointer = file,
        .error = GROUNDED_STREAM_SUCCESS,
        .refill = fileRefill,
        .close = groundedFileStreamReaderClose,
    };
    result.refill(&result);
    return result;
}

GROUNDED_FUNCTION BufferedStreamReader groundedFileGetStreamReaderFromFilename(MemoryArena* arena, String8 filename) {
    GroundedFile* file = groundedOpenFile(arena, filename, FILE_MODE_READ);
    BufferedStreamReader result = groundedFileGetStreamReaderFromFile(file);
    return result;
}

static enum GroundedStreamErrorCode fileSubmit(BufferedStreamWriter* w, u8* opl) {
    struct GroundedFile* f = (struct GroundedFile*)w->implementationPointer;
    u64 size = opl - f->buffer;
    //TODO: Error handling
    write(f->fd, f->buffer, size);
    return GROUNDED_STREAM_SUCCESS;
}

static void groundedFileStreamWriterClose(BufferedStreamWriter* writer) {
    GroundedFile* file = writer->implementationPointer;
    groundedCloseFile(file);
}

GROUNDED_FUNCTION BufferedStreamWriter groundedFileGetStreamWriterFromFile(GroundedFile* file) {
    struct GroundedFile* f = (struct GroundedFile*)file;
    BufferedStreamWriter result = {
        .start = f->buffer,
        .end = f->buffer + f->bufferSize,
        .implementationPointer = file,
        .error = GROUNDED_STREAM_SUCCESS,
        .submit = fileSubmit,
        .close = groundedFileStreamWriterClose,
    };
    return result;
}

GROUNDED_FUNCTION BufferedStreamWriter groundedFileGetStreamWriterFromFilename(MemoryArena* arena, String8 filename) {
    GroundedFile* file = groundedOpenFile(arena, filename, FILE_MODE_WRITE);
    BufferedStreamWriter result = groundedFileGetStreamWriterFromFile(file);
    return result;
}

GROUNDED_FUNCTION void groundedCloseFile(GroundedFile* file) {
    struct GroundedFile* f = (struct GroundedFile*)file;
    close(f->fd);
}

struct WatchEntry {
    int watchHandle;
    String8 directory;
    struct WatchEntry* nextWatch;
};

struct GroundedLinuxDirectoryWatch {
    int inotifyHandle;
    struct WatchEntry sentinel;
};


static void _handleWatchDirectory(struct GroundedLinuxDirectoryWatch* linuxWatch, MemoryArena* arena, String8 directory) {
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    GroundedDirectoryIterator* iterator = createDirectoryIterator(scratch, directory);
    GroundedDirectoryEntry entry = getNextDirectoryEntry(iterator);
    int handle = inotify_add_watch(linuxWatch->inotifyHandle, str8GetCstr(scratch, directory), IN_CLOSE_WRITE | IN_CREATE | IN_DELETE);
    
    // Add watch
    struct WatchEntry* watchEntry = ARENA_PUSH_STRUCT_NO_CLEAR(arena, struct WatchEntry);
    watchEntry->watchHandle = handle;
    watchEntry->directory = directory;
    watchEntry->nextWatch = linuxWatch->sentinel.nextWatch;
    linuxWatch->sentinel.nextWatch = watchEntry;

    // Check subdirectories
    while(entry.name.size) {
        if(entry.type == GROUNDED_DIRECTORY_ENTRY_TYPE_DIRECTORY) {
            String8List list = {0};
            str8ListPush(scratch, &list, directory);
            str8ListPush(scratch, &list, STR8_LITERAL("/"));
            str8ListPush(scratch, &list, entry.name);
            String8 nextDirectoryString = str8ListJoin(scratch, &list, 0);
            _handleWatchDirectory(linuxWatch, arena, nextDirectoryString);
        }
        entry = getNextDirectoryEntry(iterator);
    }

    arenaEndTemp(temp);
}


GROUNDED_FUNCTION void groundedWatchDirectory(GroundedDirectoryWatch* directoryWatch, MemoryArena* arena, String8 directory, bool watchSubdirectories) {
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    // Supported events are create, modify, delete
    // IN_CLOSE_WRITE, IN_CREATE, IN_DELETE, IN_DELETE_SELF?, IN_MOVE_SELF?, IN_MOVED_FROM, IN_MOVED_TO
    struct GroundedLinuxDirectoryWatch* linuxWatch = ARENA_PUSH_STRUCT(arena, struct GroundedLinuxDirectoryWatch);
    directoryWatch->implementationPointer = linuxWatch;

    linuxWatch->inotifyHandle = inotify_init1(IN_NONBLOCK);
    if(watchSubdirectories) {
        _handleWatchDirectory(linuxWatch, arena, directory);
    } else {
        //TODO: More masks
        int handle = inotify_add_watch(linuxWatch->inotifyHandle, str8GetCstr(scratch, directory), IN_CLOSE_WRITE | IN_CREATE | IN_DELETE);
        struct WatchEntry* watchEntry = ARENA_PUSH_STRUCT(arena, struct WatchEntry);
        watchEntry->watchHandle = handle;
        watchEntry->directory = directory;
        watchEntry->nextWatch = linuxWatch->sentinel.nextWatch;
        linuxWatch->sentinel.nextWatch = watchEntry;
    }
    arenaEndTemp(temp);
}

GROUNDED_FUNCTION void groundedDestroyDirectoryWatch(GroundedDirectoryWatch* directoryWatch) {
    struct GroundedLinuxDirectoryWatch* linuxWatch = (struct GroundedLinuxDirectoryWatch*)directoryWatch->implementationPointer;
    close(linuxWatch->inotifyHandle);
}

// Move events are encoded as create and delete events.
GROUNDED_FUNCTION void groundedHandleWatchDirectoryEvents(GroundedDirectoryWatch* directoryWatch) {
    struct GroundedLinuxDirectoryWatch* linuxWatch = (struct GroundedLinuxDirectoryWatch*)directoryWatch->implementationPointer;
    ASSERT(directoryWatch);
    ASSERT(linuxWatch);

    char buffer[KB(16)];
    struct inotify_event* inotifyEvent;
    
    int len = read(linuxWatch->inotifyHandle, buffer, KB(16));
    int i = 0;
    if(len > 0) {
        while(i < len) {
            inotifyEvent = (struct inotify_event*) &buffer[i];
            i += inotifyEvent->len + sizeof(struct inotify_event);
            
            String8 filename = str8FromCstr(inotifyEvent->name); // Name is always 0-terminated
            String8 directory = EMPTY_STRING8;
            
            struct WatchEntry* entry = linuxWatch->sentinel.nextWatch;
            while(entry) {
                if(entry->watchHandle == inotifyEvent->wd) {
                    directory = entry->directory;
                    break;
                }
                entry = entry->nextWatch;
            }
            
            // Create
            if(inotifyEvent->mask & IN_CREATE) {
                if(inotifyEvent->mask & IN_ISDIR) {
                    bool shouldWatch = true;
                    //TODO:
                } else {
                    if(directoryWatch->onFileCreate) {
                        directoryWatch->onFileCreate(filename, directory);
                    }
                }
            }
            //TODO: MOVE_TO
            
            // Modify
            if(inotifyEvent->mask & IN_CLOSE_WRITE) {
                if(directoryWatch->onFileModify) {
                    directoryWatch->onFileModify(filename, directory);
                }
            }
            
            // Delete
            if(inotifyEvent->mask & IN_DELETE) {
                if(inotifyEvent->mask & IN_ISDIR) {
                    //TODO: Delete directory watch?
                } else {
                    if(directoryWatch->onFileModify) {
                        directoryWatch->onFileModify(filename, directory);
                    }
                }
            }
            //TODO: MOVE_FROM
        }
    }
}


struct GroundedDirectoryIterator {
    DIR* dir;
};
GROUNDED_FUNCTION GroundedDirectoryIterator* createDirectoryIterator(MemoryArena* arena, String8 directory) {
    struct GroundedDirectoryIterator* result = ARENA_PUSH_STRUCT(arena, struct GroundedDirectoryIterator);
    ArenaTempMemory temp = arenaBeginTemp(arena);
    result->dir = opendir(str8GetCstr(arena, directory));
    arenaEndTemp(temp);
    return result;
}

GROUNDED_FUNCTION GroundedDirectoryEntry getNextDirectoryEntry(GroundedDirectoryIterator* iterator) {
    ASSERT(iterator);
    GroundedDirectoryEntry result = {0};
    if(iterator->dir == 0) {
        //result.entry = 0;
        return result;
    }

    struct dirent* entry = 0;
    do {
        entry = readdir(iterator->dir);
        if(entry) {
            if(entry->d_type == DT_DIR && entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
                // This is . or .. so just read the next entry
                continue;
            } else {
                // Valid entry so exit loop
                break;
            }
        }
    } while(entry);

    if(entry) {
        result.name = str8FromCstr(entry->d_name);
        //TODO: Actually not every filesystem supports returning the type here. It might be DT_UNKNOWN
        switch(entry->d_type) {
            case DT_DIR:
                result.type = GROUNDED_DIRECTORY_ENTRY_TYPE_DIRECTORY;
                break;
            case DT_REG:
                result.type = GROUNDED_DIRECTORY_ENTRY_TYPE_FILE;
                break;
            default:
                result.type = GROUNDED_DIRECTORY_ENTRY_TYPE_NONE;
                break;
        }
    }
    return result;
}

GROUNDED_FUNCTION void destroyDirectoryIterator(GroundedDirectoryIterator* iterator) {
    ASSERT(iterator);
    closedir(iterator->dir);
}

GROUNDED_FUNCTION u64 groundedGetCreationTimestamp(String8 filename) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    int fileHandle = openat(AT_FDCWD, str8GetCstr(scratch, filename), O_RDONLY);
    arenaEndTemp(temp);
    if(fileHandle < 0) {
        return 0;
    }
    
    struct stat fileStats;
    if(fstat(fileHandle, &fileStats) < 0) {
        if(close(fileHandle) < 0) {
        }
        return 0;
    }
    
    if(close(fileHandle) < 0) {
        
    }
    return fileStats.st_ctime;
}

// Directory modification timestamps also reflect files added to and removed from the directory
GROUNDED_FUNCTION u64 groundedGetModificationTimestamp(String8 filename) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    int fileHandle = openat(AT_FDCWD, str8GetCstr(scratch, filename), O_RDONLY);
    arenaEndTemp(temp);
    if(fileHandle < 0) {
        
        return 0;
    }
    
    struct stat fileStats;
    if(fstat(fileHandle, &fileStats) < 0) {
        if(close(fileHandle) < 0) {
        }
        return 0;
    }
    
    if(close(fileHandle) < 0) {

    }
    return fileStats.st_mtime;
}


// All paths are returned without / at the end.
//TODO: What about just using ~ ?
String8 getUserDirectory(MemoryArena* arena) {
    const char* homeDirectory = 0;
    homeDirectory = getenv("HOME");
    if(!homeDirectory) {
        homeDirectory = getpwuid(getuid())->pw_dir;
    }
    String8 result = str8FromCstr(homeDirectory);
    return result;
}

// https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
GROUNDED_FUNCTION String8 groundedGetUserConfigDirectory(MemoryArena* arena) {
    const char* configDirectory = 0;
    configDirectory = getenv("XDG_CONFIG_HOME");
    String8 result;
    if(configDirectory) {
        result = str8FromCstr(configDirectory);
    } else {
        String8 home = getUserDirectory(arena);
        String8List list = {};
        str8ListPush(arena, &list, home);
        str8ListPush(arena, &list, STR8_LITERAL("/.config"));
        result = str8ListJoin(arena, &list, 0);

        // Alternative would be just
        //result = STR8_LITERAL("~/.config");
    }
    return result;
}

GROUNDED_FUNCTION String8 groundedGetCacheDirectory(MemoryArena* arena) {
    const char* cacheDirectory = 0;
    cacheDirectory = getenv("XDG_CACHE_HOME");
    String8 result;
    if(cacheDirectory) {
        result = str8FromCstr(cacheDirectory);
    } else {
        String8 home = getUserDirectory(arena);
        String8List list = {};
        str8ListPush(arena, &list, home);
        str8ListPush(arena, &list, STR8_LITERAL("/.cache"));
        result = str8ListJoin(arena, &list, 0);
    }
    
    return result;
}

GROUNDED_FUNCTION String8 groundedGetCurrentWorkingDirectory(MemoryArena* arena) {
    u32 bufferSize = 256;
    ASSERT(bufferSize > 0);
    ArenaMarker marker = arenaCreateMarker(arena);
    char* buffer = ARENA_PUSH_ARRAY_NO_CLEAR(arena, bufferSize, char);
    char* result = 0;
    
    while(!result) {
        result = getcwd(buffer, bufferSize);
        if(result == buffer) {
            break;
        } else if(errno != ERANGE || bufferSize > KB(256)) {
            arenaResetToMarker(marker);
            break;
        }
        arenaResetToMarker(marker);
        bufferSize *= 2;
        buffer = ARENA_PUSH_ARRAY_NO_CLEAR(arena, bufferSize, char);
    }

    u32 len = strlen(buffer);
    ASSERT(bufferSize >= len);
    arenaPopTo(arena, (u8*)buffer + len);
    return str8FromRange((u8*)buffer, (u8*)buffer + len);
}

GROUNDED_FUNCTION String8 groundedGetBinaryDirectory(MemoryArena* arena) {
    //TODO: Currently assumes virtual memory arena
    ASSERT(false);

    u32 sizeIncrease = 4;
    u32 bufferSize = sizeIncrease;
    char* buffer = ARENA_PUSH_ARRAY(arena, sizeIncrease, char);
    int written = 0;

    while(true) {
        // readlink does not null terminate!
        written = readlink("/proc/self/exe", buffer, bufferSize);

        // readlink trunkates so if written is equal to buffersize we assume that the output has been truncated
        if(written == bufferSize || (written == -1 && errno == ENAMETOOLONG)) {
            ARENA_PUSH_ARRAY_NO_CLEAR(arena, sizeIncrease, char);
            bufferSize += sizeIncrease;
            continue;
        } else {
            break;
        }
    }

    //TODO: Actually we have the binary at this point. We still have to remove the binary name now.
    ASSERT(false);

    if(written < 0) written = 0;
    ASSERT(bufferSize >= written);
    arenaPopTo(arena, (u8*)buffer + written);
    String8 result = str8FromRange((u8*)buffer, (u8*)buffer + written);
    return result;
}
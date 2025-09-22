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
        close(fileHandle);
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

GROUNDED_FUNCTION bool groundedDoesDirectoryExist(String8 directory) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    bool result = true;
    DIR* dir = opendir(str8GetCstr(scratch, directory));
    if(dir) {
        closedir(dir);
    } else {
        // Does not exist if ENOENT == errno
        // But we want to also return false in other cases
        result = false;
    }

    arenaEndTemp(temp);
    return result;
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

GROUNDED_FUNCTION bool groundedEnsureDirectoryExists(String8 directory) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    bool result = mkdir(str8GetCstr(scratch, directory), 0755) == 0;
    if(!result && errno != EEXIST) {
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

GROUNDED_FUNCTION GroundedFile groundedOpenFile(String8 filename, enum FileMode fileMode) {
    struct GroundedFile result = {.fd = -1};
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    int flags = O_CREAT;
    if(fileMode == FILE_MODE_READ) {
        flags |= O_RDONLY;
    } else if(fileMode == FILE_MODE_READ_WRITE) {
        flags |= O_RDWR;
    } else if(fileMode == FILE_MODE_WRITE) {
        flags |= O_WRONLY | O_TRUNC;
    }

    result.fd = openat(AT_FDCWD, str8GetCstr(scratch, filename), flags, 0664);
    if(result.fd < 0) {
        GROUNDED_LOG_ERROR("Error opening file");
    }

    arenaEndTemp(temp);
    return result;
}

GROUNDED_FUNCTION u64 groundedFileRead(GroundedFile file, u8* buffer, u64 size) {
    s64 bytesRead = read(file.fd, buffer, size);
    if(bytesRead < 0) {
        bytesRead = 0;
    }
    return (u64) bytesRead;
}

GROUNDED_FUNCTION u64 groundedFileWrite(GroundedFile file, u8* buffer, u64 size) {
    s64 bytesWritten = write(file.fd, buffer, size);
    if(bytesWritten < 0) {
        bytesWritten = 0;
    }
    return (u64) bytesWritten;
}

static enum GroundedStreamErrorCode fileRefill(BufferedStreamReader* r) {
    struct GroundedFile* f = (struct GroundedFile*)r->implementationPointer;
    s64 bytesRead = read(f->fd, (void*)r->start, r->end - r->start);
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
        r->start = r->start;
        r->end = r->start + bytesRead;
        r->cursor = r->start;
        return GROUNDED_STREAM_SUCCESS;
    }
}

static void groundedFileStreamReaderClose(BufferedStreamReader* reader) {
    GroundedFile* file = reader->implementationPointer;
    groundedCloseFile(file);
}

GROUNDED_FUNCTION BufferedStreamReader groundedFileGetStreamReaderFromFile(MemoryArena* arena, GroundedFile* file, u64 bufferSize) {
    // Default buffer size of 4KB
    if(!bufferSize) {
        bufferSize = KB(4);
    }

    u8* buffer = ARENA_PUSH_ARRAY(arena, bufferSize, u8);
    if(!buffer) {
        bufferSize = 0;
        GROUNDED_LOG_ERROR("Could not allocate buffer for file");
    }

    BufferedStreamReader result = {
        .start = buffer,
        .cursor = buffer,
        .end = buffer + bufferSize,
        .implementationPointer = file,
        .error = GROUNDED_STREAM_SUCCESS,
        .refill = fileRefill,
        .close = groundedFileStreamReaderClose,
    };
    result.refill(&result);
    
    return result;
}

GROUNDED_FUNCTION BufferedStreamReader groundedFileGetStreamReaderFromFilename(MemoryArena* arena, String8 filename, u64 bufferSize) {
    GroundedFile file = groundedOpenFile(filename, FILE_MODE_READ);
    GroundedFile* filePointer = ARENA_PUSH_COPY(arena, GroundedFile, &file);
    BufferedStreamReader result = groundedFileGetStreamReaderFromFile(arena, filePointer, bufferSize);
    return result;
}

static enum GroundedStreamErrorCode fileSubmit(BufferedStreamWriter* w, u8* opl) {
    struct GroundedFile* f = (struct GroundedFile*)w->implementationPointer;
    u64 size = opl - w->start;
    //TODO: Error handling
    write(f->fd, w->start, size);
    return GROUNDED_STREAM_SUCCESS;
}

static void groundedFileStreamWriterClose(BufferedStreamWriter* writer) {
    GroundedFile* file = writer->implementationPointer;
    groundedCloseFile(file);
}

GROUNDED_FUNCTION BufferedStreamWriter groundedFileGetStreamWriterFromFile(MemoryArena* arena, GroundedFile* file, u64 bufferSize) {
    struct GroundedFile* f = (struct GroundedFile*)file;

    // Default buffer size of 4KB
    if(!bufferSize) {
        bufferSize = KB(4);
    }

    u8* buffer = ARENA_PUSH_ARRAY(arena, bufferSize, u8);
    if(!buffer) {
        bufferSize = 0;
        GROUNDED_LOG_ERROR("Could not allocate buffer for file");
    }

    BufferedStreamWriter result = {
        .start = buffer,
        .head = buffer,
        .end = buffer + bufferSize,
        .implementationPointer = file,
        .error = GROUNDED_STREAM_SUCCESS,
        .submit = fileSubmit,
        .close = groundedFileStreamWriterClose,
    };
    return result;
}

GROUNDED_FUNCTION BufferedStreamWriter groundedFileGetStreamWriterFromFilename(MemoryArena* arena, String8 filename, u64 bufferSize) {
    GroundedFile file = groundedOpenFile(filename, FILE_MODE_WRITE);
    GroundedFile* filePointer = ARENA_PUSH_COPY(arena, GroundedFile, &file);
    BufferedStreamWriter result = groundedFileGetStreamWriterFromFile(arena, filePointer, bufferSize);
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

struct GroundedDirectoryWatch {
    MemoryArena arena; // Only initialized if we recurse into submodules
    int inotifyHandle;
    bool recurseSubdirectories;
    struct WatchEntry sentinel;
    struct WatchEntry* firstFree;
};

static void addWatch(struct GroundedDirectoryWatch* linuxWatch, String8 directory, struct WatchEntry* watchEntry) {
    MemoryArena* scratch = threadContextGetScratch(&linuxWatch->arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    u32 eventMask = IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF | IN_MOVE;
    int handle = inotify_add_watch(linuxWatch->inotifyHandle, str8GetCstr(scratch, directory), eventMask);
    watchEntry->watchHandle = handle;
    watchEntry->directory = directory;

    arenaEndTemp(temp);
}

static void _handleRecursiveWatchDirectory(struct GroundedDirectoryWatch* linuxWatch, String8 directory) {
    MemoryArena* scratch = threadContextGetScratch(&linuxWatch->arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    GroundedDirectoryIterator* iterator = groundedCreateDirectoryIterator(scratch, directory);
    GroundedDirectoryEntry entry = groundedGetNextDirectoryEntry(iterator);
    
    // Add watch
    struct WatchEntry* watchEntry = ARENA_PUSH_STRUCT_NO_CLEAR(&linuxWatch->arena, struct WatchEntry);
    addWatch(linuxWatch, directory, watchEntry);
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
            _handleRecursiveWatchDirectory(linuxWatch, nextDirectoryString);
        }
        entry = groundedGetNextDirectoryEntry(iterator);
    }

    arenaEndTemp(temp);
}

GROUNDED_FUNCTION GroundedDirectoryWatch* groundedDirectoryWatchCreate(MemoryArena* arena, String8 directory, bool watchSubdirectories) {
    struct GroundedDirectoryWatch* result = 0;

    if(watchSubdirectories) {
        result = ARENA_BOOTSTRAP_PUSH_STRUCT(createGrowingArena(osGetMemorySubsystem(), KB(16)), GroundedDirectoryWatch, arena);
    } else {
        result = ARENA_PUSH_STRUCT(arena, struct GroundedDirectoryWatch);
    }

    result->inotifyHandle = inotify_init1(IN_NONBLOCK);
    result->recurseSubdirectories = watchSubdirectories;

    if(watchSubdirectories) {
        // We need our own arena as we want to handle recurse subdirectories
        _handleRecursiveWatchDirectory(result, directory);
    } else {
        addWatch(result, directory, &result->sentinel);
    }

    return result;
}

GROUNDED_FUNCTION void groundedDestroyDirectoryWatch(GroundedDirectoryWatch* directoryWatch) {
    ASSUME(directoryWatch) {
        close(directoryWatch->inotifyHandle);
        if(directoryWatch->recurseSubdirectories) {
            arenaRelease(&directoryWatch->arena);  
        }
    }
}

// Move events are encoded as create and delete events.
GROUNDED_FUNCTION GroundedDirectoryWatchEvent* groundedDirectoryWatchPollEvents(GroundedDirectoryWatch* watch, MemoryArena* arena, u64* eventCount) {
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    
    GroundedDirectoryWatchEvent* result = 0;
    ASSUME(watch && eventCount) {
        *eventCount = 0;
        // Buffer size should be at least sizeof(struct inotify_event) + NAME_MAX + 1.
        u8 buffer[KB(8)];
        int len = read(watch->inotifyHandle, buffer, ARRAY_COUNT(buffer));
        if(len > 0) {
            int i = 0;
            int maxCount = len / sizeof(struct inotify_event);
            GroundedDirectoryWatchEvent* events = ARENA_PUSH_ARRAY(scratch, maxCount, GroundedDirectoryWatchEvent);
            while(i < len) {
                struct inotify_event* inotifyEvent;
                inotifyEvent = (struct inotify_event*) &buffer[i];
                i += inotifyEvent->len + sizeof(struct inotify_event);
                
                String8 filename = str8FromCstr(inotifyEvent->name); // Name is always 0-terminated
                String8 directory = watch->sentinel.directory;
                bool isDirectory = inotifyEvent->mask & IN_ISDIR;
                
                // Lookup directory watch
                if(watch->recurseSubdirectories) {
                    struct WatchEntry* entry = watch->sentinel.nextWatch;
                    while(entry) {
                        if(entry->watchHandle == inotifyEvent->wd) {
                            directory = entry->directory;
                            break;
                        }
                        entry = entry->nextWatch;
                    }
                }
                
                // Create
                if(inotifyEvent->mask & IN_CREATE) {
                    if(isDirectory && watch->recurseSubdirectories) {
                        //TODO: Watch subdirectory
                        ASSERT(false);
                    }
                    events[*eventCount++] = (GroundedDirectoryWatchEvent) {
                        .filename = filename,
                        .directory = directory,
                        .type = GROUNDED_DIRECTORY_WATCH_EVENT_TYPE_CREATE,
                    };
                }
                //TODO: MOVE_TO
                //TODO: MOVE_FROM
                //TODO: With cookie is rename
                
                // Modify
                if(inotifyEvent->mask & IN_CLOSE_WRITE) {
                    events[*eventCount++] = (GroundedDirectoryWatchEvent) {
                        .filename = filename,
                        .directory = directory,
                        .type = GROUNDED_DIRECTORY_WATCH_EVENT_TYPE_MODIFY,
                    };
                }
                
                // Delete
                if(inotifyEvent->mask & IN_DELETE) {
                    if(isDirectory && watch->recurseSubdirectories) {
                        //TODO: Delete directory watch?
                    } 
                    events[*eventCount++] = (GroundedDirectoryWatchEvent) {
                        .filename = filename,
                        .directory = directory,
                        .type = GROUNDED_DIRECTORY_WATCH_EVENT_TYPE_DELETE,
                    };
                }
            }
            result = ARENA_PUSH_ARRAY(arena, *eventCount, GroundedDirectoryWatchEvent);
            for(u64 i = 0; i < *eventCount; ++i) {
                result[i] = events[i];
            }
        }   
    }
    
    arenaEndTemp(temp);
    return result;
}


struct GroundedDirectoryIterator {
    DIR* dir;
};
GROUNDED_FUNCTION GroundedDirectoryIterator* groundedCreateDirectoryIterator(MemoryArena* arena, String8 directory) {
    struct GroundedDirectoryIterator* result = ARENA_PUSH_STRUCT(arena, struct GroundedDirectoryIterator);
    ArenaTempMemory temp = arenaBeginTemp(arena);
    result->dir = opendir(str8GetCstr(arena, directory));
    arenaEndTemp(temp);
    return result;
}

GROUNDED_FUNCTION GroundedDirectoryEntry groundedGetNextDirectoryEntry(GroundedDirectoryIterator* iterator) {
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
        if(entry->d_name[0] == '.') {
            result.flags |= GROUNDED_DIRECTORY_ENTRY_FLAG_HIDDEN;
        }
        //TODO: Actually not every filesystem supports returning the type here. It might be DT_UNKNOWN
        switch(entry->d_type) {
            case DT_DIR:
                result.type = GROUNDED_DIRECTORY_ENTRY_TYPE_DIRECTORY;
                break;
            case DT_REG:
                result.type = GROUNDED_DIRECTORY_ENTRY_TYPE_FILE;
                break;
            case DT_UNKNOWN:
                //TODO: Need to check with statat
                result.type = GROUNDED_DIRECTORY_ENTRY_TYPE_NONE;
                break;
            default:
                result.type = GROUNDED_DIRECTORY_ENTRY_TYPE_NONE;
                break;
        }
    }
    return result;
}

GROUNDED_FUNCTION void groundedDestroyDirectoryIterator(GroundedDirectoryIterator* iterator) {
    ASSERT(iterator);
    closedir(iterator->dir);
}

static int compareFilenames(GroundedDirectoryEntry* a, GroundedDirectoryEntry* b) {
    return str8CompareCaseInsensitive(a->name, b->name);
}

GROUNDED_FUNCTION GroundedDirectoryEntry* groundedListFilesOfDirectory(MemoryArena* arena, String8 directory, u64* resultCount, GroundedListFilesParameters* parameters) {
    if(!parameters) {
        static struct GroundedListFilesParameters defaultParameters = {0};
        parameters = &defaultParameters;
    }
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    GroundedDirectoryEntry* result = 0;

    int dirfd = -1;
    {
        ArenaTempMemory temp = arenaBeginTemp(arena);
        dirfd = open(str8GetCstr(arena, directory), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if(dirfd < 0) {
            //TODO: Check errno and return matching error
        }
        arenaEndTemp(temp);
    }

    DIR* dir = 0;
    if(dirfd >= 0) {
        dir = fdopendir(dirfd);
        if(!dir) {
            //TODO: Error
            close(dirfd);
            dirfd = -1;
        }
    }

    struct DirectoryNode {
        struct DirectoryNode* next;
        GroundedDirectoryEntry entry;
    };

    struct DirectoryNode* head;
    if(dir) {
        struct dirent* d = 0;
        u64 fileCount = 0;
        while((d = readdir(dir))) {
            String8 name = str8FromCstr(d->d_name);
            enum GroundedDirectoryEntryType type = GROUNDED_DIRECTORY_ENTRY_TYPE_NONE;
            enum GroundedDirectoryEntryFlags flags = 0;

            if(*name.base == '.') {
                flags |= GROUNDED_DIRECTORY_ENTRY_FLAG_HIDDEN;
            }

            // Skip hidden files if it was requested
            if(parameters->ignoreHiddenFiles && (flags & GROUNDED_DIRECTORY_ENTRY_FLAG_HIDDEN)) {
                continue;
            }

            // Ignore . and ..
            if(*name.base == '.' && name.base[1] == '\0' && !parameters->allowDot) {
                continue;
            }
            if(*name.base == '.' && name.base[1] == '.' && name.base[2] == '\0' && !parameters->allowDoubleDot) {
                continue;
            }

            if(d->d_type == DT_DIR) {
                type = GROUNDED_DIRECTORY_ENTRY_TYPE_DIRECTORY;
            } else if(d->d_type == DT_REG) {
                type = GROUNDED_DIRECTORY_ENTRY_TYPE_FILE;
            } else if(d->d_type == DT_LNK) {
                type = GROUNDED_DIRECTORY_ENTRY_TYPE_LINK;
            } else if(d->d_type == DT_UNKNOWN) {
                // Explicitly stat the file to check type
                struct stat stats = {0};
                if(fstatat(dirfd, d->d_name, &stats, 0) == 0) {
                    if(S_ISREG(stats.st_mode)) {
                        type = GROUNDED_DIRECTORY_ENTRY_TYPE_FILE;
                    } else if(S_ISDIR(stats.st_mode)) {
                        type = GROUNDED_DIRECTORY_ENTRY_TYPE_DIRECTORY;
                    } else if(S_ISLNK(stats.st_mode)) {
                        type = GROUNDED_DIRECTORY_ENTRY_TYPE_LINK;
                    }
                }
            }

            GroundedDirectoryEntry entry = {
                .name = str8CopyAndNullTerminate(arena, name), // Copy name onto final arena
                .type = type,
                .flags = flags,
            };
            
            // Add to list
            struct DirectoryNode* node = ARENA_PUSH_STRUCT(scratch, struct DirectoryNode);
            ASSUME(node) {
                node->entry = entry;
                node->next = head;
                head = node;
                fileCount++;
            }
        }

        result = ARENA_PUSH_ARRAY_NO_CLEAR(arena, fileCount, GroundedDirectoryEntry);
        if(result) {
            if(resultCount) {
                *resultCount = fileCount;
            }

            struct DirectoryNode* node = head;
            for(u64 i = 0; i < fileCount; ++i) {
                ASSUME(node) {
                    result[i] = node->entry;
                    node = node->next;
                } else {
                    //TODO: List is shorter if we are here
                }
            }
        }

        // Sort the list
        qsort(result, fileCount, sizeof(GroundedDirectoryEntry), (__compar_fn_t)&compareFilenames);

        // Also closes fd
        closedir(dir);
        dir = 0;
    }

    arenaEndTemp(temp);
    return result;
}

GROUNDED_FUNCTION String8 groundedGetLinkTarget(MemoryArena* arena, String8 filename) {
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    String8 result = EMPTY_STRING8;

    struct stat stats = {0};
    const char* cFilename = str8GetCstr(scratch, filename);
    if(lstat(cFilename, &stats) == 0) {
        bool hardLink = stats.st_nlink > 1;
        u64 bufferSize = 256;
        u8* buffer = ARENA_PUSH_ARRAY(arena, bufferSize, u8);
        ssize_t length = readlink(cFilename, (char*)buffer, bufferSize);
        if(length < (s64)bufferSize) {
            result = str8FromBlock(buffer, length);
        } else {
            //TODO: Do again with new size
        }
        arenaPopTo(arena, buffer + length);
    }
    
    arenaEndTemp(temp);
    return result;
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
        if(written == (s32)bufferSize || (written == -1 && errno == ENAMETOOLONG)) {
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
    ASSERT((s32)bufferSize >= written);
    arenaPopTo(arena, (u8*)buffer + written);
    String8 result = str8FromRange((u8*)buffer, (u8*)buffer + written);
    return result;
}
#include <grounded/file/grounded_file.h>
#include <grounded/threading/grounded_threading.h>

#include <windows.h>

//TODO: Remove the UTF conversions from here
inline static const char* UTF16ToUTF8(MemoryArena* arena, const wchar_t* s) {
	int requiredLength = WideCharToMultiByte(CP_UTF8, 0, s, -1, 0, 0, 0, 0);
	ASSERT(requiredLength >= 0);
    char* result = ARENA_PUSH_ARRAY(arena, (u64)requiredLength, char);
	ASSERT(result);
	WideCharToMultiByte(CP_UTF8, 0, s, -1, result, requiredLength, 0, 0);
	return result;
}

inline static wchar_t* UTF8ToUTF16(MemoryArena* arena, const char* s) {
	int requiredBufferSize = MultiByteToWideChar(CP_UTF8, 0, s, -1, 0, 0);
	ASSERT(requiredBufferSize >= 0);
    wchar_t* result = ARENA_PUSH_ARRAY(arena, (u64)requiredBufferSize, wchar_t);
	ASSERT(result);
	int usedBufferSize = MultiByteToWideChar(CP_UTF8, 0, s, -1, result, requiredBufferSize);
    ASSERT(usedBufferSize == requiredBufferSize);
	return result;
}


//TODO: Error cases do not work at all!
GROUNDED_FUNCTION u8* groundedReadFile(MemoryArena* arena, String8 filename, u64* size) {
	MemoryArena* scratch = threadContextGetScratch(arena);
	ArenaTempMemory temp = arenaBeginTemp(scratch);
    u8* result = 0;
    HANDLE fileHandle = CreateFileA(str8GetCstr(scratch, filename), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	arenaEndTemp(temp);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		return result;
	}
	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(fileHandle, &fileSize)) {
		return result;
	}
    *size = fileSize.QuadPart;
    result = ARENA_PUSH_ARRAY_NO_CLEAR(arena, fileSize.QuadPart, u8);
    if(result) {
        DWORD bytesRead;
        //TODO: Read larger files by issuing multiple ReadFile commands
        ASSERT(fileSize.QuadPart < 0xFFFFFFFF);
        if (!ReadFile(fileHandle, result, (u32)fileSize.QuadPart, &bytesRead, 0) || fileSize.QuadPart != bytesRead) {
            return 0;
        }
    }
	CloseHandle(fileHandle);

    return result;
}

GROUNDED_FUNCTION u8* groundedReadFileImmutable(String8 filename, u64* size) {
#if 1
	MemoryArena* scratch = threadContextGetScratch(0);
	ArenaTempMemory temp = arenaBeginTemp(scratch);
    u8* result = 0;
    HANDLE fileHandle = CreateFileA(str8GetCstr(scratch, filename), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	arenaEndTemp(temp);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        return result;
    }
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(fileHandle, &fileSize)) {
        return result;
    }
    *size = fileSize.QuadPart;
    result = VirtualAlloc(0, fileSize.QuadPart, MEM_COMMIT, PAGE_READWRITE);
    if (result) {
        DWORD bytesRead;
        //TODO: Read larger files by issuing multiple ReadFile commands
        ASSERT(fileSize.QuadPart < 0xFFFFFFFF);
        if (!ReadFile(fileHandle, result, (u32)fileSize.QuadPart, &bytesRead, 0) || fileSize.QuadPart != bytesRead) {
            //TODO: Handle leak
            return 0;
        }
		// Old protect must be retrieved for successful call. See doc
		DWORD oldProtect;
        VirtualProtect(result, fileSize.QuadPart, PAGE_READONLY, &oldProtect);
    }
    CloseHandle(fileHandle);
    return result;
#else
    HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    //TODO: This leaks the mapping object
    HANDLE mappingObject = CreateFileMapping(fileHandle, 0, PAGE_READONLY, 0, 0, 0);
    void* result = MapViewOfFile(mappingObject, FILE_MAP_READ, 0, 0, 0);
    return (u8*)result;
#endif
}

GROUNDED_FUNCTION void groundedFreeFileImmutable(u8* file, u64 size) {
#if 1
    VirtualFree(file, 0, MEM_RELEASE);
#else
    UnmapViewOfFile(file);
#endif
}

struct GroundedDirectoryIterator {
    HANDLE findHandle;
	wchar_t* directory;
    MemoryArena* arena; //TODO: This is a very weird use of the arena here. A fixed buffer in here to store converted filenames might be enough as we specify, that the string of a directory entry is only valid as long as the next entry has not been retrieved?
};

GROUNDED_FUNCTION GroundedDirectoryIterator* createDirectoryIterator(MemoryArena* arena, String8 directory) {
	// As our API doesn't return an element on iterator creation we create the real iterator on the first call of getNextEntry.
	// Instead we only do UTF-16 conversion in here and save the directory name in the directory iterator
	MemoryArena* scratch = threadContextGetScratch(arena);
	ArenaTempMemory temp = arenaBeginTemp(scratch);

    struct GroundedDirectoryIterator* result = ARENA_PUSH_STRUCT(arena, struct GroundedDirectoryIterator);
    result->arena = arena;
    
	int requiredBufferSize = MultiByteToWideChar(CP_UTF8, 0, str8GetCstr(scratch, directory), -1, 0, 0);
	ASSERT(requiredBufferSize >= 2); // At least a single character + \0
	wchar_t* utf16Directory = (wchar_t*)malloc(((u64)requiredBufferSize) * 2 + 4);
	ASSERT(utf16Directory);
	int usedBufferSize = MultiByteToWideChar(CP_UTF8, 0, str8GetCstr(scratch, directory), -1, utf16Directory, requiredBufferSize);
	ASSERT(usedBufferSize == requiredBufferSize);
    if (utf16Directory[requiredBufferSize - 2] != L'/') {
		utf16Directory[requiredBufferSize - 1] = L'/';
		utf16Directory[requiredBufferSize] = L'*';
		utf16Directory[requiredBufferSize + 1] = L'\0';
	}
	else {
		utf16Directory[requiredBufferSize - 1] = L'*';
		utf16Directory[requiredBufferSize] = L'\0';
	}
	result->directory = utf16Directory;
	result->findHandle = INVALID_HANDLE_VALUE;

	arenaEndTemp(temp);
	return result;
}

GROUNDED_FUNCTION GroundedDirectoryEntry getNextDirectoryEntry(GroundedDirectoryIterator* iterator) {
	GroundedDirectoryEntry result = {0};
    WIN32_FIND_DATAW entry;

	if (iterator->findHandle == INVALID_HANDLE_VALUE) {
		HANDLE handle = FindFirstFileW(iterator->directory, &entry);
		if (handle == INVALID_HANDLE_VALUE) {
			// Invalid
			return result;
		}
		iterator->findHandle = handle;
	}
	else {
	A:
		if (!FindNextFileW(iterator->findHandle, &entry)) {
			// Invalid
			result = (GroundedDirectoryEntry){0};
			return result;
		}
	}

	if (entry.cFileName[0] == '.' && (entry.cFileName[1] == '\0' || (entry.cFileName[1] == '.' && entry.cFileName[2] == '\0'))) {
		// Check that we have no .. as this results in inifnity paths ;)
		goto A;
	}
    String16 entryFilename16 = str16FromBlock(entry.cFileName, lstrlenW(entry.cFileName));
    result.name = str8FromStr16(iterator->arena, entryFilename16);
    //result.name = UTF16ToUTF8(iterator->arena, entry.cFileName);
    if(entry.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
        result.type = GROUNDED_DIRECTORY_ENTRY_TYPE_DIRECTORY;
    } else {
        result.type = GROUNDED_DIRECTORY_ENTRY_TYPE_FILE;
    }
    
	return result;
}

GROUNDED_FUNCTION void destroyDirectoryIterator(GroundedDirectoryIterator* iterator) {
    ASSERT(iterator);
	FindClose(iterator->findHandle);
}

static int compareFilenames(const void* a, const void* b) {
    const GroundedDirectoryEntry* entryA = (const GroundedDirectoryEntry*)a;
    const GroundedDirectoryEntry* entryB = (const GroundedDirectoryEntry*)b;
    return str8CompareCaseInsensitive(entryA->name, entryB->name);
}

GROUNDED_FUNCTION GroundedDirectoryEntry* groundedListFilesOfDirectory(MemoryArena* arena, String8 directory, u64* resultCount, GroundedListFilesParameters* parameters) {
    if (!parameters) {
        static struct GroundedListFilesParameters defaultParameters = {0};
        parameters = &defaultParameters;
    }
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    GroundedDirectoryEntry* result = 0;

    struct DirectoryNode {
        struct DirectoryNode* next;
        GroundedDirectoryEntry entry;
    };

    u64 fileCount = 0;
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(L"\\*", &findData);
    struct DirectoryNode* head = 0;
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            String8 name = str8FromStr16(scratch, str16FromWcstr(findData.cFileName));
            enum GroundedDirectoryEntryType type = GROUNDED_DIRECTORY_ENTRY_TYPE_NONE;
            enum GroundedDirectoryEntryFlags flags = 0;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                type = GROUNDED_DIRECTORY_ENTRY_TYPE_DIRECTORY;
            } else {
                type = GROUNDED_DIRECTORY_ENTRY_TYPE_FILE;
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

        } while (FindNextFileW(hFind, &findData));

        FindClose(hFind);
    } else {
        // TODO: Handle error cases with GetLastError()
    }

    // Copy results into the result array
    result = ARENA_PUSH_ARRAY_NO_CLEAR(arena, fileCount, GroundedDirectoryEntry);
    if (result) {
        if (resultCount) {
            *resultCount = fileCount;
        }

        struct DirectoryNode* node = head;
        for (u64 i = 0; i < fileCount; ++i) {
            ASSUME(node) {
                result[i] = node->entry;
                node = node->next;
            } else {
                // TODO: Handle truncated list case
            }
        }

        // Sort the list
        qsort(result, fileCount, sizeof(GroundedDirectoryEntry), &compareFilenames);
    }
    
    arenaEndTemp(temp);
    return result;
}

GROUNDED_FUNCTION bool groundedCreateDirectory(String8 directory) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    wchar_t* utf16Directory = UTF8ToUTF16(scratch, str8GetCstr(scratch, directory));
	int result = CreateDirectoryW(utf16Directory, 0);

    arenaEndTemp(temp);

	return result != 0;
}

GROUNDED_FUNCTION bool groundedWriteFile(String8 filename, const void* data, u64 size) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    
	//TODO: Handle sizes larger than DWORD
    wchar_t* utf16Filename = (UTF8ToUTF16(scratch, str8GetCstr(scratch, filename)));
    HANDLE handle = CreateFileW(utf16Filename, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    WriteFile(handle, data, (DWORD)size, 0, 0);
    CloseHandle(handle);

    arenaEndTemp(temp);
	return true;
}



struct GroundedFile {
    HANDLE handle;
};
GROUNDED_FUNCTION GroundedFile* groundedOpenFile(String8 filename, enum FileMode fileMode) {
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    DWORD access = GENERIC_WRITE;
    DWORD creation = CREATE_ALWAYS;
    if (fileMode == FILE_MODE_READ) {
        creation = OPEN_EXISTING;
        access = GENERIC_READ;
    } else if (fileMode == FILE_MODE_READ_WRITE) {
        creation = OPEN_ALWAYS;
        access |= GENERIC_READ;
    }

    struct GroundedFile* result = ARENA_PUSH_STRUCT(arena, struct GroundedFile);
    wchar_t* utf16Filename = (UTF8ToUTF16(scratch, str8GetCstr(scratch, filename)));
    result->handle = CreateFileW(utf16Filename, access, FILE_SHARE_READ, 0, creation, FILE_ATTRIBUTE_NORMAL, 0);

    arenaEndTemp(temp);
    return result;
}

static enum GroundedStreamErrorCode fileRefill(BufferedStreamReader* r) {
    struct GroundedFile* f = (struct GroundedFile*)r->implementationPointer;
    DWORD bytesRead = 0;
    s32 bytesToRead = (s32)CLAMP(0, f->bufferSize, INT32_MAX);
    if (!ReadFile(f->handle, f->buffer, bytesToRead, &bytesRead, 0)) {
        refillZeros(r);
        r->refill = refillZeros;
        r->error = GROUNDED_STREAM_IO_ERROR;
        return r->error;
    }
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

    //TODO: Buffer deallocation missing
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
    GroundedFile file = groundedOpenFile(arena, filename, FILE_MODE_READ);
    GroundedFile* filePointer = ARENA_PUSH_COPY(arena, GroundedFile, &file);
    BufferedStreamReader result = groundedFileGetStreamReaderFromFile(arena, filePointer, bufferSize);
    return result;
}

static enum GroundedStreamErrorCode fileSubmit(BufferedStreamWriter* w, u8* opl) {
    struct GroundedFile* f = (struct GroundedFile*)w->implementationPointer;
    u64 size = opl - w->start;
	//TODO: Handle sizes larger than DWORD
    WriteFile(f->handle, w->start, (DWORD)size, 0, 0);
    return NO_ERROR;
}

GROUNDED_FUNCTION BufferedStreamWriter groundedFileGetStreamWriterFromFile(MemoryArena* arena, GroundedFile* file, u64 bufferSize) {
    // Default buffer size of 4KB
    if(!bufferSize) {
        bufferSize = KB(4);
    }

    //TODO: Buffer deallocation missing
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
        .error = NO_ERROR,
        .submit = fileSubmit,
    };
    return result;
}

GROUNDED_FUNCTION BufferedStreamWriter groundedFileGetStreamWriterFromFilename(MemoryArena* arena, String8 filename, u64 bufferSize) {
    GroundedFile* file = groundedOpenFile(arena, filename, FILE_MODE_WRITE);
    BufferedStreamWriter result = groundedFileGetStreamWriterFromFile(arena, file, bufferSize);
    return result;
}

GROUNDED_FUNCTION void groundedCloseFile(GroundedFile* file) {
    struct GroundedFile* f = (struct GroundedFile*)file;
    CloseHandle(f->handle);
}

GROUNDED_FUNCTION bool groundedDoesDirectoryExist(String8 directory) {
    MemoryArena* scratch = threadContextGetScratch(0);
	ArenaTempMemory temp = arenaBeginTemp(scratch);

    String16 directory16 = str16FromStr8(scratch, directory);
    DWORD attributes = GetFileAttributesW(directory16.base);
    
    arenaEndTemp(temp);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

GROUNDED_FUNCTION u64 groundedGetCreationTimestamp(String8 filename) {
	MemoryArena* scratch = threadContextGetScratch(0);
	ArenaTempMemory temp = arenaBeginTemp(scratch);
    HANDLE file = CreateFileA(str8GetCstr(scratch, filename), GENERIC_READ, FILE_SHARE_READ, 0, 0, 0, 0);
	arenaEndTemp(temp);
	if (file == INVALID_HANDLE_VALUE) {
		GROUNDED_LOG_ERROR("Filetime of file that does not exist queried\n");
		return 0;
	}
	FILETIME creationTime;
	if (!GetFileTime(file, &creationTime, 0, 0)) {
		GROUNDED_LOG_ERROR("Error getting file creation timestamp\n");
		return 0;
	}
	if (!CloseHandle(file)) {
		GROUNDED_LOG_ERROR("Error closing file handle after time query\n");
	}
	// Simple cast is not allowed as it can result in alignment issues. See https://docs.microsoft.com/de-de/windows/win32/api/minwinbase/ns-minwinbase-filetime
	ULARGE_INTEGER result;
	result.HighPart = creationTime.dwHighDateTime;
	result.LowPart = creationTime.dwLowDateTime;
	return result.QuadPart;

}

// Timestamp in seconds
GROUNDED_FUNCTION u64 groundedGetModificationTimestamp(String8 filename) {
	MemoryArena* scratch = threadContextGetScratch(0);
	ArenaTempMemory temp = arenaBeginTemp(scratch);
    FILETIME lastWriteTime = {0};
	WIN32_FILE_ATTRIBUTE_DATA data;
	//TODO: Unicode
	BOOL success = GetFileAttributesExA(str8GetCstr(scratch, filename), GetFileExInfoStandard, &data);
	arenaEndTemp(temp);
	if (success) {
		lastWriteTime = data.ftLastWriteTime;
	} else {
		GROUNDED_LOG_INFO("Filetime of file that does not exist queried\n");
		return 0;
	}
	ULARGE_INTEGER result;
	result.HighPart = lastWriteTime.dwHighDateTime;
	result.LowPart = lastWriteTime.dwLowDateTime;
	return result.QuadPart;
#if 0
	HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, 0, 0, 0);
	if (file == INVALID_HANDLE_VALUE) {
		LOG_WARN("Filetime of file that does not exist queried");
		return 0;
	}
	FILETIME modificationTime;
	if (!GetFileTime(file, 0, 0, &modificationTime)) {
		LOG_ERROR("Error getting file creation timestamp");
		return 0;
	}
	if (!CloseHandle(file)) {
		LOG_ERROR("Error closing file handle after time query");
	}
	// Simple cast is not allowed as it can result in alignment issues. See https://docs.microsoft.com/de-de/windows/win32/api/minwinbase/ns-minwinbase-filetime
	ULARGE_INTEGER result;
	result.HighPart = modificationTime.dwHighDateTime;
	result.LowPart = modificationTime.dwLowDateTime;
	return result.QuadPart;
#endif
}

GROUNDED_FUNCTION String8 groundedGetUserConfigDirectory(MemoryArena* arena) {
    //SHGetFolderPath(0, CSIDL_APPDATA, 0, 0, buffer)
    ASSERT(false);
}

GROUNDED_FUNCTION String8 groundedGetCacheDirectory(MemoryArena* arena) {
    //SHGetFolderPath(0, CSIDL_LOCAL_APPDATA, 0, 0, buffer)
    ASSERT(false);
}

GROUNDED_FUNCTION String8 groundedGetCurrentWorkingDirectory(MemoryArena* arena) {
    String8 result = EMPTY_STRING8;
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    int requiredSize = GetCurrentDirectoryW(0, 0);
    while(requiredSize > 0 && !result.size) {
        u16* buffer = ARENA_PUSH_ARRAY(scratch, requiredSize, u16);
        int writtenSize = GetCurrentDirectoryW(requiredSize, buffer);
        if(writtenSize <= requiredSize) {
            result = str8FromStr16(arena, str16FromBlock(buffer, writtenSize));
            break;
        } else {
            requiredSize = writtenSize;
            arenaEndTemp(temp);
            temp = arenaBeginTemp(scratch);
        }
    }
    if(result.size == 0) {
        GROUNDED_PUSH_ERROR("Error retrieving cwd");
    }
    
    arenaEndTemp(temp);
    return result;
}

GROUNDED_FUNCTION String8 groundedGetBinaryDirectory(MemoryArena* arena) {
    String8 result = EMPTY_STRING8;
    u16 exePath[MAX_PATH];
    MemoryArena* scratch = threadContextGetScratch(arena);
    ArenaTempMemory temp = arenaBeginTemp(scratch);

    // Get the full path to the executable
    int pathLength = GetModuleFileNameW(0, exePath, MAX_PATH);
    if (!pathLength) {
        GROUNDED_PUSH_ERRORF("Failed to get executable path. Error code: %lu\n", GetLastError());
    } else {
        String8 path = str8FromStr16(scratch, str16FromBlock(exePath, pathLength));
        u64 lastSlash = str8GetLastOccurence(path, '/');
        if(lastSlash < path.size) {
            result = str8Copy(arena, str8Prefix(path, lastSlash));
        }
    }
    if(result.size == 0) {
        GROUNDED_PUSH_ERROR("Error retrieving binary directory");
    }

    arenaEndTemp(temp);
    return result;
}

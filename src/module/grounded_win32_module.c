#include <grounded/module/grounded_module.h>
#include <grounded/string/grounded_string.h>
#include <grounded/memory/grounded_arena.h>

#include <windows.h>

struct GroundedModule* groundedLoadModule(String8 moduleFilename) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    
    // str16FromStr8 guarantees 0-termination
    String16 utf16ModuleFilename = str16FromStr8(scratch, moduleFilename);
    struct GroundedModule* result = LoadLibraryW(utf16ModuleFilename.base);

    arenaEndTemp(temp);
    return result;
}

void* groundedModuleGetSymbol(struct GroundedModule* module, const char* symbolName) {
    void* result = 0;
    ASSUME(module) {
        result = GetProcAddress((HMODULE)module, symbolName);
    }
    return result;
}

void groundedFreeModule(struct GroundedModule* module) {
    ASSUME(module) {
        FreeLibrary((HMODULE)module);
    }
}

String8 groundedGetPlatformModuleName(MemoryArena* arena, String8 basename) {
    String8 result = str8FromFormat(arena, "%S.dll", basename);
    return result;
}

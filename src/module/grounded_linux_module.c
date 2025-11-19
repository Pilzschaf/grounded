#include <grounded/module/grounded_module.h>
#include <grounded/threading/grounded_threading.h>

#include <dlfcn.h>

struct GroundedModule* groundedLoadModule(String8 moduleFilename) {
    MemoryArena* scratch = threadContextGetScratch(0);
    ArenaTempMemory temp = arenaBeginTemp(scratch);
    
    struct GroundedModule* result = (struct GroundedModule*)dlopen(str8GetCstr(scratch, moduleFilename), RTLD_LAZY | RTLD_LOCAL);

    arenaEndTemp(temp);
    return result;
}

void* groundedModuleGetSymbol(struct GroundedModule* module, const char* symbolName) {
    void* result = 0;
    ASSUME(module) {
        result = dlsym(module, symbolName);
    }
    return result;
}

void groundedFreeModule(struct GroundedModule* module) {
    ASSUME(module) {
        dlclose(module);
    }
}

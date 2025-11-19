#ifndef GROUNDED_MODULE_H
#define GROUNDED_MODULE_H

#include <grounded/grounded.h>

typedef struct GroundedModule GroundedModule;

struct GroundedModule* groundedLoadModule(String8 moduleFilename);
// Get Symbol uses c strings as symbol name is in most cases just a c string literal
void* groundedModuleGetSymbol(struct GroundedModule* module, const char* symbolName);
void groundedFreeModule(struct GroundedModule* module);

#endif // GROUNDED_MODULE_H
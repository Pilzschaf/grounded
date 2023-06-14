#pragma once
#include <base/base_types.h>

typedef struct {
	const char* name;
	const char* description;
	int parameterCount; // -1 for unknown
} SyscallInfo;

extern SyscallInfo syscallInfoTable64[];
extern u64 numSyscallInfos64;

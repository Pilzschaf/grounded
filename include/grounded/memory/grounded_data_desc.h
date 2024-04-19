#ifndef GROUNDED_DATA_DESC_H
#define GROUNDED_DATA_DESC_H

#include "../grounded.h"

enum DataType {
    DATA_TYPE_INVALID,
    DATA_TYPE_CSTRING,
    DATA_TYPE_STRING8,
    DATA_TYPE_U8,
    DATA_TYPE_U16,
    DATA_TYPE_U32,
    DATA_TYPE_U64,
    DATA_TYPE_S8,
    DATA_TYPE_S16,
    DATA_TYPE_S32,
    DATA_TYPE_S64,
    DATA_TYPE_FLOAT,
    DATA_TYPE_DOUBLE,
    //TODO: DATA_TYPE_NESTED,
    //TODO: Maybe a pointer type with custom callback serialize and desrialize? Or optionally callback to data type nested
    DATA_TYPE_COUNT,
};


typedef struct DataDesc {
    const char* name;
    enum DataType dataType;
    u32 elementCount; // For array types. A value of 0 defaults to 1 element
    u32 flags;
    u32 nonStandardAlignment; // Value of 0 means default alignment for this type
    //u32 versionIntroduced;
    //u32 versionDropped;
} DataDesc ;

// This field is present in the struct but should not be serialized/deserialized
#define DATA_DESC_FLAG_UNUSED 1

//TODO: Nested data descs

// Unsized types return 0
GROUNDED_FUNCTION_INLINE u32 getSizeOfDataType(enum DataType type) {
    switch(type) {
        case DATA_TYPE_U8:
        case DATA_TYPE_S8:
        return 1;
        case DATA_TYPE_U16:
        case DATA_TYPE_S16:
        return 2;
        case DATA_TYPE_U32:
        case DATA_TYPE_S32:
        case DATA_TYPE_FLOAT:
        return 4;
        case DATA_TYPE_U64:
        case DATA_TYPE_S64:
        case DATA_TYPE_DOUBLE:
        case DATA_TYPE_CSTRING:
        return 8;
        case DATA_TYPE_STRING8:
        return 16;
        case DATA_TYPE_INVALID:
        default:
        ASSERT(false);
        return 0;
    }
}

GROUNDED_FUNCTION_INLINE bool isDataTypeInteger(enum DataType type) {
    switch(type) {
        case DATA_TYPE_U8:
        case DATA_TYPE_S8:
        case DATA_TYPE_U16:
        case DATA_TYPE_S16:
        case DATA_TYPE_U32:
        case DATA_TYPE_S32:
        case DATA_TYPE_U64:
        case DATA_TYPE_S64:
        return true;
        default:
        return false;
    }
}

GROUNDED_FUNCTION_INLINE u32 getAlignmentOfDataType(enum DataType type) {
    switch(type) {
        case DATA_TYPE_U8:
        case DATA_TYPE_S8:
        return 1;
        case DATA_TYPE_U16:
        case DATA_TYPE_S16:
        return 2;
        case DATA_TYPE_U32:
        case DATA_TYPE_S32:
        case DATA_TYPE_FLOAT:
        return 4;
        case DATA_TYPE_U64:
        case DATA_TYPE_S64:
        case DATA_TYPE_DOUBLE:
        case DATA_TYPE_CSTRING:
        case DATA_TYPE_STRING8:
        return 8;
        case DATA_TYPE_INVALID:
        default:
        ASSERT(false);
        return 0;
    }
}

#endif // GROUNDED_DATA_DESC_H
#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "common.h"
#include "chunk.h"
#include "object.h"

obj_function_t *compile(const char *src);

#endif
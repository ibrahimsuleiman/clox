#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include"common.h"
#include"chunk.h"





bool compile(const char *src, struct chunk *chunk);

#endif
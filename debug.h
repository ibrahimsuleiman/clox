#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

#include "chunk.h"
#include "common.h"

void disassemble_chunk(struct chunk *c, const char *name);
/* @ret: offset of the next instruction*/
int disassemble_instruction(struct chunk *c, int offset);

#endif
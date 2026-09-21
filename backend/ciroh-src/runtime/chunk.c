#include "../headers/common.h"
#include <stdio.h>
#include <stdlib.h>

#include "../headers/chunk.h"
#include "../headers/memory.h"
#include "../headers/value.h"
#include "../headers/vm.h"

void initChunk(Chunk *chunk) {
  chunk->count = 0;
  chunk->code = NULL;
  chunk->capacity = 0;
  chunk->lines = NULL;
  chunk->lineCount = 0;
  chunk->lineCapacity = 0;
  initValueArray(&chunk->constants);
}

int getLine(Chunk *chunk, int instructionIndex) {
  for (int i = 0; i < chunk->lineCount; i++) {
    instructionIndex -= chunk->lines[i].frequency;
    if (instructionIndex <= 0) {
      return chunk->lines[i].lineInformation;
    }
  }
  return -1;
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {

  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code =
        GROW_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
  }

  // grow lines array separately
  if (chunk->lineCapacity < chunk->lineCount + 1) {
    int oldCapacity = chunk->lineCapacity;
    chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
    chunk->lines =
        GROW_ARRAY(LineRun, chunk->lines, oldCapacity, chunk->lineCapacity);
  }
  chunk->code[chunk->count] = byte;
  chunk->count++;
  if (chunk->lineCount > 0 &&
      chunk->lines[chunk->lineCount - 1].lineInformation == line) {
    chunk->lines[chunk->lineCount - 1].frequency++;
  } else {
    chunk->lines[chunk->lineCount].lineInformation = line;
    chunk->lines[chunk->lineCount].frequency = 1;
    chunk->lineCount++;
  }
}

void freeChunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(LineRun, chunk->lines, chunk->lineCapacity);
  freeValueArray(&chunk->constants);
  initChunk(chunk);
}

void writeConstant(Chunk *chunk, Value value, int line) {
  int index = addConstant(chunk, value);
  if (index < 256) {
    writeChunk(chunk, OP_CONSTANT, line);
    writeChunk(chunk, index, line);
  } else {
    writeChunk(chunk, OP_CONSTANT_LONG, line);
    writeChunk(chunk, (index >> 16) & 0xFF, line); // byte 1
    writeChunk(chunk, (index >> 8) & 0xFF, line);  // byte 2
    writeChunk(chunk, index & 0xFF, line);         // byte 3
  }
}

int addConstant(Chunk *chunk, Value value) {
  push(value);

  writeValueArray(&chunk->constants, value);
  pop();

  return chunk->constants.count - 1;
}

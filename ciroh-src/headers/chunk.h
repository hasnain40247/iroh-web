#ifndef iroh_chunk_h
#define iroh_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_INVOKE,
  OP_BUILD_LIST,
  OP_INDEX_GET,
  OP_INDEX_SET,
  OP_GET_SUPER,
  OP_SUPER_INVOKE,
  OP_INHERIT,
  OP_GET_UPVALUE,
  OP_METHOD,
  OP_CLOSE_UPVALUE,
  OP_CLASS,
  OP_GET_PROPERTY,
  OP_SET_PROPERTY,
  OP_SET_UPVALUE,
  OP_RETURN,
  OP_CLOSURE,
  OP_CALL,
  OP_DUP,
  OP_LOOP,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_SET_GLOBAL,
  OP_GET_GLOBAL,
  OP_CONSTANT,
  OP_CONSTANT_LONG,
  OP_DEFINE_GLOBAL,
  OP_NEGATE,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_TRUE,
  OP_FALSE,
  OP_NIL,
  OP_NOT,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_PRINT,
  OP_POP
} OpCode;

typedef struct {
  int frequency;
  int lineInformation;
} LineRun;

typedef struct {
  int count;
  int capacity;
  uint8_t *code;
  LineRun *lines;
  int lineCount;
  int lineCapacity;
  ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
void freeChunk(Chunk *chunk);
int addConstant(Chunk *chunk, Value value);
int getLine(Chunk *chunk, int instructionIndex);
void writeConstant(Chunk *chunk, Value value, int line);

#endif

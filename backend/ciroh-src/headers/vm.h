#ifndef iroh_vm_h
#define iroh_vm_h
#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_SIZE (FRAMES_MAX * UINT8_COUNT)
typedef struct {

  ObjClosure *closure;

  uint8_t *ip;
  Value *slots; // points to value stack

} CallerFrame;

typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  Value *stack;
  Value *stackTop;
  int stackSize;
  Obj *objects;
  Table strings;
  ObjString *initString;

  Table globals;
  CallerFrame frames[FRAMES_MAX];
  int frameCount;
  ObjUpvalue *openUpvalues;

  int grayCount;
  int grayCapacity;
  Obj **grayStack;
  size_t bytesAllocated;
  size_t nextGC;

} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char *source);
void push(Value value);
Value pop();
#endif

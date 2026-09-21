#ifndef iroh_object_h
#define iroh_object_h

#include "chunk.h"
#include "common.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)
#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_CLOSURE(value) isObjType(value, OBJ_CLOSURE)
#define IS_CLASS(value) isObjType(value, OBJ_CLASS)
#define IS_INSTANCE(value) isObjType(value, OBJ_INSTANCE)
#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_LIST(value) isObjType(value, OBJ_LIST)
#define IS_MAP(value) isObjType(value, OBJ_MAP)
#define IS_QUEUE(value) isObjType(value, OBJ_QUEUE)

#define AS_CLOSURE(value) ((ObjClosure *)AS_OBJ(value))
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative *)AS_OBJ(value))->function)
#define AS_CLASS(value) ((ObjClass *)AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance *)AS_OBJ(value))
#define AS_BOUND_METHOD(value) ((ObjBoundMethod *)AS_OBJ(value))
#define AS_LIST(value) ((ObjList *)AS_OBJ(value))
#define AS_MAP(value) ((ObjMap *)AS_OBJ(value))
#define AS_QUEUE(value) ((ObjQueue *)AS_OBJ(value))

typedef Value (*NativeFn)(int argCount, Value *args);

typedef enum {
  OBJ_STRING,
  OBJ_BOUND_METHOD,
  OBJ_INSTANCE,
  OBJ_CLASS,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_CLOSURE,
  OBJ_UPVALUE,
  OBJ_LIST,
  OBJ_MAP,
  OBJ_QUEUE,
} ObjType;

struct Obj {
  ObjType type;
  bool isMarked;

  struct Obj *next;
};

struct ObjString {
  Obj obj;
  int length;
  // char* chars;
  uint32_t hash;
  char chars[];
};

typedef struct ObjUpvalue {
  Obj obj;
  Value *location;
  struct ObjUpvalue *next;
  Value closed;

} ObjUpvalue;

typedef struct {
  Obj obj;
  int arity;
  int upvalueCount;
  Chunk chunk;
  ObjString *name;
} ObjFunction;

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;

typedef struct {
  Obj obj;
  ObjString *name;
  Table methods;
} ObjClass;

typedef struct {
  Obj obj;
  ObjClass *classObject;
  Table fields;
} ObjInstance;

typedef struct {
  Obj obj;
  ObjFunction *function;
  ObjUpvalue **upvalues;
  int upvalueCount;
} ObjClosure;

typedef struct {
  Obj obj;
  Value receiver;
  ObjClosure *method;
} ObjBoundMethod;

typedef struct {
  Obj obj;
  ValueArray items;
} ObjList;

typedef struct {
  Obj obj;
  ValueArray keys;
  ValueArray vals;
} ObjMap;

typedef struct {
  Obj obj;
  ValueArray items;
  int head;
} ObjQueue;
static inline bool isObjType(Value value, ObjType type) {
  return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

ObjString *copyString(const char *chars, int length);
void printObject(Value value);
ObjString *takeString(char *chars, int length);

ObjFunction *newFunction();
ObjNative *newNative(NativeFn function);
ObjClosure *newClosure(ObjFunction *function);
ObjClass *newClass(ObjString *name);
ObjUpvalue *newUpvalue(Value *slot);
ObjInstance *newInstance(ObjClass *classObject);
ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method);
ObjList *newList();
ObjMap *newMap();
ObjQueue *newQueue();
#endif
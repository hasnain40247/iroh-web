#include "../headers/table.h"
#include <stdio.h>
#include <string.h>

#include "../headers/memory.h"
#include "../headers/object.h"
#include "../headers/value.h"
#include "../headers/vm.h"

static uint32_t hashString(const char *key, int length) {

  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

#define ALLOCATE_OBJ(type, objectType)                                         \
  (type *)allocateObject(sizeof(type), objectType)

static Obj *allocateObject(size_t size, ObjType type) {
  Obj *object = (Obj *)reallocate(NULL, 0, size);
  object->type = type;
  object->isMarked = false;
  object->next = vm.objects;
  vm.objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void *)object, size, type);
#endif
  return object;
}

// static ObjString* allocateString(char* chars,int length, uint32_t hash){
//         ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
//         string->length = length;
//         string->chars = chars;
//         string->hash = hash;
//         tableSet(&vm.strings, string, NIL_VAL);
//         return string;

// }

static ObjString *allocateString(int length, uint32_t hash) {
  ObjString *string =
      (ObjString *)allocateObject(sizeof(ObjString) + length + 1, OBJ_STRING);
  string->length = length;
  string->hash = hash;
  push(OBJ_VAL(string));

  tableSet(&vm.strings, string, NIL_VAL);
  pop();

  return string;
}

// ObjString* takeString(char* chars, int length) {
//     uint32_t hash = hashString(chars, length);
//      ObjString* interned = tableFindString(&vm.strings, chars, length,
//                                         hash);
//   if (interned != NULL) {
//     FREE_ARRAY(char, chars, length + 1);
//     return interned;
//   }
//   return allocateString(chars, length, hash);
// }

ObjString *takeString(char *chars, int length) {
  uint32_t hash = hashString(chars, length);
  ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, length + 1);
    return interned;
  }
  ObjString *string = allocateString(length, hash);
  memcpy(string->chars, chars, length);
  string->chars[length] = '\0';
  FREE_ARRAY(char, chars, length + 1); // ← free original since we copied it in
  return string;
}

// ObjString* copyString(const char* chars, int length){

//   uint32_t hash = hashString(chars, length);
//   ObjString* interned = tableFindString(&vm.strings, chars, length,
//                                           hash);
//   if (interned != NULL) return interned;
//     char* heapChars=ALLOCATE(char,length+1);
//     memcpy(heapChars,chars,length);
//     heapChars[length]='\0';
//     return allocateString(heapChars, length, hash);
// }

ObjString *copyString(const char *chars, int length) {

  uint32_t hash = hashString(chars, length);
  ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
  if (interned != NULL)
    return interned;
  ObjString *string = allocateString(length, hash);
  memcpy(string->chars, chars, length);
  string->chars[length] = '\0';
  return string;
}

ObjClosure *newClosure(ObjFunction *function) {
  ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue *, function->upvalueCount);
  for (int i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }
  ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}

ObjNative *newNative(NativeFn function) {
  ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}
ObjFunction *newFunction() {
  ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->upvalueCount = 0;

  function->name = NULL;
  initChunk(&function->chunk);
  return function;
}
static void printFunction(ObjFunction *function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

ObjUpvalue *newUpvalue(Value *slot) {
  ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;

  upvalue->location = slot;
  upvalue->next = NULL;

  return upvalue;
}
ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method) {
  ObjBoundMethod *bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
  bound->receiver = receiver;
  bound->method = method;
  return bound;
}

ObjClass *newClass(ObjString *name) {
  ObjClass *classObject = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  classObject->name = name;
  initTable(&classObject->methods);
  return classObject;
}
ObjInstance *newInstance(ObjClass *klass) {
  ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
  instance->classObject = klass;
  initTable(&instance->fields);
  return instance;
}

ObjList *newList() {
  ObjList *list = ALLOCATE_OBJ(ObjList, OBJ_LIST);
  initValueArray(&list->items);
  return list;
}

ObjMap *newMap() {
  ObjMap *map = ALLOCATE_OBJ(ObjMap, OBJ_MAP);
  initValueArray(&map->keys);
  initValueArray(&map->vals);
  return map;
}

ObjQueue *newQueue() {
  ObjQueue *queue = ALLOCATE_OBJ(ObjQueue, OBJ_QUEUE);
  initValueArray(&queue->items);
  queue->head = 0;
  return queue;
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
  case OBJ_STRING:
    printf("%s", AS_CSTRING(value));
    break;

  case OBJ_FUNCTION:
    printFunction(AS_FUNCTION(value));
    break;
  case OBJ_INSTANCE:
    printf("%s instance", AS_INSTANCE(value)->classObject->name->chars);
  case OBJ_CLOSURE:
    printFunction(AS_CLOSURE(value)->function);
    break;
  case OBJ_BOUND_METHOD:
    printFunction(AS_BOUND_METHOD(value)->method->function);
    break;
  case OBJ_NATIVE:
    printf("<native fn>");
    break;
  case OBJ_UPVALUE:
    printf("upvalue");
    break;
  case OBJ_CLASS:
    printf("%s", AS_CLASS(value)->name->chars);
    break;
  case OBJ_LIST: {
    ObjList *list = AS_LIST(value);
    printf("[");
    for (int i = 0; i < list->items.count; i++) {
      printValue(list->items.values[i]);
      if (i < list->items.count - 1) printf(", ");
    }
    printf("]");
    break;
  }
  case OBJ_MAP: {
    ObjMap *map = AS_MAP(value);
    printf("{");
    for (int i = 0; i < map->keys.count; i++) {
      printValue(map->keys.values[i]);
      printf(": ");
      printValue(map->vals.values[i]);
      if (i < map->keys.count - 1) printf(", ");
    }
    printf("}");
    break;
  }
  case OBJ_QUEUE: {
    ObjQueue *queue = AS_QUEUE(value);
    printf("Queue[");
    for (int i = queue->head; i < queue->items.count; i++) {
      printValue(queue->items.values[i]);
      if (i < queue->items.count - 1) printf(", ");
    }
    printf("]");
    break;
  }
  }
}
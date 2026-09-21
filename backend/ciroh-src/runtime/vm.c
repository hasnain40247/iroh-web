#include "../headers/vm.h"
#include "../headers/chunk.h"
#include "../headers/common.h"
#include "../headers/compiler.h"
#include "../headers/debug.h"
#include "../headers/memory.h"
#include "../headers/object.h"
#include "../headers/value.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

VM vm;

static Value clockNative(int argCount, Value *args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static Value sqrtNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_NUMBER(args[0]))
    return NIL_VAL;
  return NUMBER_VAL(sqrt(AS_NUMBER(args[0])));
}

static Value floorNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_NUMBER(args[0]))
    return NIL_VAL;
  return NUMBER_VAL(floor(AS_NUMBER(args[0])));
}

static Value ceilNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_NUMBER(args[0]))
    return NIL_VAL;
  return NUMBER_VAL(ceil(AS_NUMBER(args[0])));
}

static Value absNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_NUMBER(args[0]))
    return NIL_VAL;
  return NUMBER_VAL(fabs(AS_NUMBER(args[0])));
}

static Value powNative(int argCount, Value *args) {
  if (argCount != 2 || !IS_NUMBER(args[0]) || !IS_NUMBER(args[1]))
    return NIL_VAL;
  return NUMBER_VAL(pow(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}

static Value strNative(int argCount, Value *args) {
  if (argCount != 1)
    return NIL_VAL;
  char buffer[64];
  int len;
  if (IS_NUMBER(args[0])) {
    len = snprintf(buffer, sizeof(buffer), "%g", AS_NUMBER(args[0]));
  } else if (IS_BOOL(args[0])) {
    len = snprintf(buffer, sizeof(buffer), "%s",
                   AS_BOOL(args[0]) ? "true" : "false");
  } else if (IS_NIL(args[0])) {
    len = snprintf(buffer, sizeof(buffer), "nil");
  } else if (IS_STRING(args[0])) {
    return args[0];
  } else {
    return NIL_VAL;
  }
  return OBJ_VAL(copyString(buffer, len));
}

static void runtimeError(const char *format, ...);

static Value lenNative(int argCount, Value *args) {
  if (argCount != 1) {
    fprintf(stderr, "len() expects 1 argument.\n");
    return NIL_VAL;
  }
  if (IS_STRING(args[0]))
    return NUMBER_VAL(AS_STRING(args[0])->length);
  if (IS_LIST(args[0]))
    return NUMBER_VAL(AS_LIST(args[0])->items.count);
  fprintf(stderr, "len() argument must be a string or list.\n");
  return NIL_VAL;
}

static Value numNative(int argCount, Value *args) {
  if (argCount != 1 || !IS_STRING(args[0]))
    return NIL_VAL;
  const char *s = AS_CSTRING(args[0]);
  char *end;
  double n = strtod(s, &end);
  if (end == s)
    return NIL_VAL;
  return NUMBER_VAL(n);
}

static Value typeNative(int argCount, Value *args) {
  if (argCount != 1) return NIL_VAL;
  Value v = args[0];
  if (IS_NIL(v))     return OBJ_VAL(copyString("empty", 5));
  if (IS_BOOL(v))    return OBJ_VAL(copyString("bool", 4));
  if (IS_NUMBER(v))  return OBJ_VAL(copyString("number", 6));
  switch (OBJ_TYPE(v)) {
    case OBJ_STRING:   return OBJ_VAL(copyString("string", 6));
    case OBJ_LIST:     return OBJ_VAL(copyString("list", 4));
    case OBJ_MAP:      return OBJ_VAL(copyString("map", 3));
    case OBJ_QUEUE:    return OBJ_VAL(copyString("queue", 5));
    case OBJ_CLOSURE:
    case OBJ_FUNCTION:
    case OBJ_NATIVE:   return OBJ_VAL(copyString("function", 8));
    case OBJ_CLASS:    return OBJ_VAL(copyString("leaf", 4));
    case OBJ_INSTANCE: return OBJ_VAL(copyString("instance", 8));
    default:           return OBJ_VAL(copyString("unknown", 7));
  }
}

static Value listNative(int argCount, Value *args) {
  return OBJ_VAL(newList());
}

static Value mapNative(int argCount, Value *args) {
  return OBJ_VAL(newMap());
}

static Value queueNative(int argCount, Value *args) {
  return OBJ_VAL(newQueue());
}

static Value inputNative(int argCount, Value *args) {
  if (argCount == 1 && IS_STRING(args[0])) {
    printf("%s", AS_CSTRING(args[0]));
    fflush(stdout);
  }
  char buffer[1024];
  if (fgets(buffer, sizeof(buffer), stdin) == NULL)
    return NIL_VAL;
  int len = strlen(buffer);
  if (len > 0 && buffer[len - 1] == '\n')
    len--;
  return OBJ_VAL(copyString(buffer, len));
}

static void resetStack() {
  vm.stack = GROW_ARRAY(Value, NULL, 0, STACK_SIZE);
  vm.stackSize = STACK_SIZE;
  vm.stackTop = vm.stack;
  vm.frameCount = 0;
  vm.openUpvalues = NULL;
}

static void runtimeError(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  CallerFrame *frame = &vm.frames[vm.frameCount - 1];

  size_t instruction = frame->ip - frame->closure->function->chunk.code - 1;

  int line = getLine(&frame->closure->function->chunk, instruction);

  fprintf(stderr, "[line %d] in script\n", line);
  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallerFrame *frame = &vm.frames[i];
    ObjFunction *function = frame->closure->function;

    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ",
            getLine(&function->chunk, instruction)); // ← use getLine
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }
  resetStack();
}
static void defineNative(const char *name, NativeFn function) {
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function)));
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}
void initVM() {
  resetStack();
  vm.objects = NULL;
  vm.bytesAllocated = 0;
  vm.nextGC = 1024 * 1024;
  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;
  initTable(&vm.strings);
  initTable(&vm.globals);
  vm.initString = NULL;

  vm.initString = copyString("init", 4);

  defineNative("clock", clockNative);
  defineNative("sqrt", sqrtNative);
  defineNative("floor", floorNative);
  defineNative("ceil", ceilNative);
  defineNative("abs", absNative);
  defineNative("pow", powNative);
  defineNative("str", strNative);
  defineNative("len", lenNative);
  defineNative("num", numNative);
  defineNative("input", inputNative);
  defineNative("type",  typeNative);
  defineNative("List",  listNative);
  defineNative("Map",   mapNative);
  defineNative("Queue", queueNative);
}

void freeVM() {
  freeTable(&vm.globals);
  freeTable(&vm.strings);
  vm.initString = NULL;

  freeObjects();
}

static Value peek(int distance) { return vm.stackTop[-1 - distance]; }

static bool call(ObjClosure *closure, int argCount) {
  if (argCount != closure->function->arity) {
    runtimeError("Expected %d arguments but got %d.", closure->function->arity,
                 argCount);
    return false;
  }

  if (vm.frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }
  CallerFrame *frame = &vm.frames[vm.frameCount++];

  frame->closure = closure;
  frame->ip = closure->function->chunk.code;

  frame->slots = vm.stackTop - argCount - 1;
  return true;
}

static bool callValue(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod *bound = AS_BOUND_METHOD(callee);
      vm.stackTop[-argCount - 1] = bound->receiver;
      return call(bound->method, argCount);
    }
    case OBJ_CLOSURE:
      return call(AS_CLOSURE(callee), argCount);
    case OBJ_NATIVE: {
      NativeFn native = AS_NATIVE(callee);
      Value result = native(argCount, vm.stackTop - argCount);
      vm.stackTop -= argCount + 1;
      push(result);
      return true;
    }
    case OBJ_CLASS: {
      ObjClass *classObject = AS_CLASS(callee);
      vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(classObject));
      Value initializer;
      if (tableGet(&classObject->methods, vm.initString, &initializer)) {
        return call(AS_CLOSURE(initializer), argCount);
      } else if (argCount != 0) {
        runtimeError("Expected 0 arguments but got %d.", argCount);
        return false;
      }
      return true;
    }

    default:
      break;
    }
  }
  runtimeError("Can only call functions and classes.");
  return false;
}

static bool isFalsy(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {

  ObjString *b = AS_STRING(peek(0));
  ObjString *a = AS_STRING(peek(1));

  int length = a->length + b->length;
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString *result = takeString(chars, length);
  pop();
  pop();
  push(OBJ_VAL(result));
}

static void closeUpvalues(Value *last) {
  while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
    ObjUpvalue *upvalue = vm.openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.openUpvalues = upvalue->next;
  }
}
static ObjUpvalue *captureUpvalue(Value *local) {
  ObjUpvalue *prevUpvalue = NULL;
  ObjUpvalue *upvalue = vm.openUpvalues;
  while (upvalue != NULL && upvalue->location > local) {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }
  ObjUpvalue *createdUpvalue = newUpvalue(local);
  createdUpvalue->next = upvalue;

  if (prevUpvalue == NULL) {
    vm.openUpvalues = createdUpvalue;
  } else {
    prevUpvalue->next = createdUpvalue;
  }
  return createdUpvalue;
}

static bool bindMethod(ObjClass *klass, ObjString *name) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }

  ObjBoundMethod *bound = newBoundMethod(peek(0), AS_CLOSURE(method));
  pop();
  push(OBJ_VAL(bound));
  return true;
}
static bool invokeFromClass(ObjClass *klass, ObjString *name, int argCount) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }
  return call(AS_CLOSURE(method), argCount);
}

static bool invokeList(ObjList *list, ObjString *name, int argCount) {
  if (strcmp(name->chars, "push") == 0) {
    if (argCount != 1) { runtimeError("push() expects 1 argument."); return false; }
    writeValueArray(&list->items, peek(0));
    vm.stackTop -= argCount + 1;
    push(NIL_VAL);
    return true;
  }
  if (strcmp(name->chars, "pop") == 0) {
    if (argCount != 0) { runtimeError("pop() expects no arguments."); return false; }
    Value result = list->items.count > 0
        ? list->items.values[--list->items.count]
        : NIL_VAL;
    vm.stackTop -= 1;
    push(result);
    return true;
  }
  runtimeError("List has no method '%s'.", name->chars);
  return false;
}

static bool invokeMap(ObjMap *map, ObjString *name, int argCount) {
  if (strcmp(name->chars, "set") == 0) {
    if (argCount != 2) { runtimeError("set() expects 2 arguments."); return false; }
    Value key = peek(1);
    Value val = peek(0);
    for (int i = 0; i < map->keys.count; i++) {
      if (valuesEqual(map->keys.values[i], key)) {
        map->vals.values[i] = val;
        vm.stackTop -= argCount + 1;
        push(NIL_VAL);
        return true;
      }
    }
    writeValueArray(&map->keys, key);
    writeValueArray(&map->vals, val);
    vm.stackTop -= argCount + 1;
    push(NIL_VAL);
    return true;
  }
  if (strcmp(name->chars, "get") == 0) {
    if (argCount != 1) { runtimeError("get() expects 1 argument."); return false; }
    Value key = peek(0);
    for (int i = 0; i < map->keys.count; i++) {
      if (valuesEqual(map->keys.values[i], key)) {
        Value result = map->vals.values[i];
        vm.stackTop -= argCount + 1;
        push(result);
        return true;
      }
    }
    vm.stackTop -= argCount + 1;
    push(NIL_VAL);
    return true;
  }
  runtimeError("Map has no method '%s'.", name->chars);
  return false;
}

static bool invokeQueue(ObjQueue *queue, ObjString *name, int argCount) {
  if (strcmp(name->chars, "enqueue") == 0) {
    if (argCount != 1) { runtimeError("enqueue() expects 1 argument."); return false; }
    writeValueArray(&queue->items, peek(0));
    vm.stackTop -= argCount + 1;
    push(NIL_VAL);
    return true;
  }
  if (strcmp(name->chars, "dequeue") == 0) {
    if (argCount != 0) { runtimeError("dequeue() expects no arguments."); return false; }
    int available = queue->items.count - queue->head;
    Value result = available > 0 ? queue->items.values[queue->head++] : NIL_VAL;
    vm.stackTop -= 1;
    push(result);
    return true;
  }
  runtimeError("Queue has no method '%s'.", name->chars);
  return false;
}

static bool invoke(ObjString *name, int argCount) {
  Value receiver = peek(argCount);
  if (IS_LIST(receiver))  return invokeList(AS_LIST(receiver), name, argCount);
  if (IS_MAP(receiver))   return invokeMap(AS_MAP(receiver), name, argCount);
  if (IS_QUEUE(receiver)) return invokeQueue(AS_QUEUE(receiver), name, argCount);
  if (IS_CLASS(receiver)) {
    ObjClass *klass = AS_CLASS(receiver);
    char prefixed[256];
    prefixed[0] = '$';
    memcpy(prefixed + 1, name->chars, name->length);
    ObjString *staticName = copyString(prefixed, name->length + 1);
    Value method;
    if (tableGet(&klass->methods, staticName, &method)) {
      vm.stackTop[-argCount - 1] = method;
      return callValue(method, argCount);
    }
    runtimeError("Undefined static method '%s'.", name->chars);
    return false;
  }
  if (!IS_INSTANCE(receiver)) {
    runtimeError("Only instances have methods.");
    return false;
  }
  ObjInstance *instance = AS_INSTANCE(receiver);
  Value value;
  if (tableGet(&instance->fields, name, &value)) {
    vm.stackTop[-argCount - 1] = value;
    return callValue(value, argCount);
  }
  return invokeFromClass(instance->classObject, name, argCount);
}

static void defineMethod(ObjString *name) {
  Value method = peek(0);
  ObjClass *classObject = AS_CLASS(peek(1));
  tableSet(&classObject->methods, name, method);
  pop();
}
static InterpretResult run() {

  CallerFrame *frame = &vm.frames[vm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT()                                                           \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT()                                                        \
  (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())

#define BINARY_OP(valueType, op)                                               \
  do {                                                                         \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                          \
      runtimeError("Operands must be numbers.");                               \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b = AS_NUMBER(pop());                                               \
    double a = AS_NUMBER(pop());                                               \
    push(valueType(a op b));                                                   \
  } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");

    disassembleInstruction(
        &frame->closure->function->chunk,
        (int)(frame->ip - frame->closure->function->chunk.code));
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {

    case OP_GET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      push(*frame->closure->upvalues[slot]->location);
      break;
    }
    case OP_SET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      *frame->closure->upvalues[slot]->location = peek(0);
      break;
    }
    case OP_INVOKE: {
      ObjString *method = READ_STRING();
      int argCount = READ_BYTE();
      if (!invoke(method, argCount)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frameCount - 1];
      break;
    }

    case OP_RETURN: {

      Value result = pop();
      closeUpvalues(frame->slots);

      vm.frameCount--;
      if (vm.frameCount == 0) {
        pop();
        return INTERPRET_OK;
      }

      vm.stackTop = frame->slots;
      push(result);
      frame = &vm.frames[vm.frameCount - 1];
      break;
    }
    case OP_GET_SUPER: {
      ObjString *name = READ_STRING();
      ObjClass *superclass = AS_CLASS(pop());

      if (!bindMethod(superclass, name)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SUPER_INVOKE: {
      ObjString *method = READ_STRING();
      int argCount = READ_BYTE();
      ObjClass *superclass = AS_CLASS(pop());
      if (!invokeFromClass(superclass, method, argCount)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frameCount - 1];
      break;
    }
    case OP_INHERIT: {
      Value superclass = peek(1);
      if (!IS_CLASS(superclass)) {
        runtimeError("Superclass must be a class.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjClass *subclass = AS_CLASS(peek(0));
      tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
      pop(); // Subclass.
      break;
    }
    case OP_CALL: {
      int argCount = READ_BYTE();
      if (!callValue(peek(argCount), argCount)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frameCount - 1];

      break;
    }
    case OP_CLOSE_UPVALUE:
      closeUpvalues(vm.stackTop - 1);
      pop();
      break;
    case OP_CLOSURE: {
      ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
      ObjClosure *closure = newClosure(function);
      push(OBJ_VAL(closure));
      for (int i = 0; i < closure->upvalueCount; i++) {
        uint8_t isLocal = READ_BYTE();
        uint8_t index = READ_BYTE();
        if (isLocal) {
          closure->upvalues[i] = captureUpvalue(frame->slots + index);
        } else {
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }
      break;
    }
    case OP_ADD: {
      if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
        concatenate();
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      } else {
        runtimeError("Operands must be two numbers or two strings.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SUBTRACT:
      BINARY_OP(NUMBER_VAL, -);
      break;
    case OP_MULTIPLY:
      BINARY_OP(NUMBER_VAL, *);
      break;
    case OP_DIVIDE:
      BINARY_OP(NUMBER_VAL, /);
      break;
    case OP_PRINT: {
      printValue(pop());
      printf("\n");
      break;
    }
    case OP_SET_PROPERTY: {
      if (!IS_INSTANCE(peek(1))) {
        runtimeError("Only instances have fields.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjInstance *instance = AS_INSTANCE(peek(1));
      tableSet(&instance->fields, READ_STRING(), peek(0));
      Value value = pop();
      pop();
      push(value);
      break;
    }
    case OP_GET_PROPERTY: {
      if (IS_LIST(peek(0))) {
        ObjList *list = AS_LIST(peek(0));
        ObjString *name = READ_STRING();
        if (strcmp(name->chars, "size") == 0) {
          pop();
          push(NUMBER_VAL(list->items.count));
          break;
        }
        runtimeError("List has no property '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      if (IS_MAP(peek(0))) {
        ObjMap *map = AS_MAP(peek(0));
        ObjString *name = READ_STRING();
        if (strcmp(name->chars, "size") == 0) {
          pop();
          push(NUMBER_VAL(map->keys.count));
          break;
        }
        runtimeError("Map has no property '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      if (IS_QUEUE(peek(0))) {
        ObjQueue *queue = AS_QUEUE(peek(0));
        ObjString *name = READ_STRING();
        if (strcmp(name->chars, "size") == 0) {
          pop();
          push(NUMBER_VAL(queue->items.count - queue->head));
          break;
        }
        runtimeError("Queue has no property '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      if (IS_CLASS(peek(0))) {
        ObjClass *klass = AS_CLASS(peek(0));
        ObjString *name = READ_STRING();
        char prefixed[256];
        prefixed[0] = '$';
        memcpy(prefixed + 1, name->chars, name->length);
        ObjString *staticName = copyString(prefixed, name->length + 1);
        Value method;
        if (tableGet(&klass->methods, staticName, &method)) {
          pop();
          push(method);
          break;
        }
        runtimeError("Undefined static method '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      if (!IS_INSTANCE(peek(0))) {
        runtimeError("Only instances have properties.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjInstance *instance = AS_INSTANCE(peek(0));
      ObjString *name = READ_STRING();

      // check for getter first
      char getterKey[256];
      memcpy(getterKey, "get$", 4);
      memcpy(getterKey + 4, name->chars, name->length);
      ObjString *getterName = copyString(getterKey, name->length + 4);
      Value getter;
      if (tableGet(&instance->classObject->methods, getterName, &getter)) {
        ObjBoundMethod *bound = newBoundMethod(peek(0), AS_CLOSURE(getter));
        pop();
        push(OBJ_VAL(bound));
        Value result;
        if (!callValue(OBJ_VAL(bound), 0)) return INTERPRET_RUNTIME_ERROR;
        frame = &vm.frames[vm.frameCount - 1];
        break;
      }

      Value value;
      if (tableGet(&instance->fields, name, &value)) {
        pop();
        push(value);
        break;
      }
      if (!bindMethod(instance->classObject, name)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_GET_GLOBAL: {
      ObjString *name = READ_STRING();
      Value value;
      if (!tableGet(&vm.globals, name, &value)) {
        runtimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      push(value);
      break;
    }
    case OP_NEGATE:
      if (!IS_NUMBER(peek(0))) {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      vm.stackTop[-1] = NUMBER_VAL(-AS_NUMBER(vm.stackTop[-1]));
      break;
    case OP_NOT:
      vm.stackTop[-1] = BOOL_VAL(isFalsy(vm.stackTop[-1]));
      break;

    case OP_GREATER:
      BINARY_OP(BOOL_VAL, >);
      break;
    case OP_LESS:
      BINARY_OP(BOOL_VAL, <);
      break;
    case OP_BUILD_LIST: {
      int count = READ_BYTE();
      ObjList *list = newList();
      // elements are on the stack in order, bottom to top
      for (int i = count - 1; i >= 0; i--) {
        writeValueArray(&list->items, peek(i));
      }
      vm.stackTop -= count;
      push(OBJ_VAL(list));
      break;
    }
    case OP_INDEX_GET: {
      if (!IS_NUMBER(peek(0))) {
        runtimeError("Index must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int index = (int)AS_NUMBER(pop());
      if (!IS_LIST(peek(0))) {
        runtimeError("Only lists can be indexed.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjList *list = AS_LIST(pop());
      if (index < 0 || index >= list->items.count) {
        runtimeError("Index %d out of bounds (size %d).", index, list->items.count);
        return INTERPRET_RUNTIME_ERROR;
      }
      push(list->items.values[index]);
      break;
    }
    case OP_INDEX_SET: {
      Value value = pop();
      if (!IS_NUMBER(peek(0))) {
        runtimeError("Index must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int index = (int)AS_NUMBER(pop());
      if (!IS_LIST(peek(0))) {
        runtimeError("Only lists can be indexed.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjList *list = AS_LIST(pop());
      if (index < 0 || index >= list->items.count) {
        runtimeError("Index %d out of bounds (size %d).", index, list->items.count);
        return INTERPRET_RUNTIME_ERROR;
      }
      list->items.values[index] = value;
      push(value);
      break;
    }
    case OP_POP:
      pop();
      break;
    case OP_DEFINE_GLOBAL: {
      ObjString *name = READ_STRING();
      tableSet(&vm.globals, name, peek(0));
      pop();
      break;
    }
    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();
      if (tableSet(&vm.globals, name, peek(0))) {
        tableDelete(&vm.globals, name);
        runtimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_GET_LOCAL: {
      uint8_t slot = READ_BYTE();
      push(frame->slots[slot]);
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t slot = READ_BYTE();
      frame->slots[slot] = peek(0);
      break;
    }
    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (isFalsy(peek(0)))
        frame->ip += offset;
      break;
    }
    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      frame->ip += offset;
      break;
    }
    case OP_DUP: {
      push(peek(0));
      break;
    }
    case OP_CLASS:
      push(OBJ_VAL(newClass(READ_STRING())));
      break;
    case OP_EQUAL: {
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(valuesEqual(a, b)));
      break;
    }

    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }
    case OP_CONSTANT_LONG: {
      int index = (READ_BYTE() << 16) | (READ_BYTE() << 8) | READ_BYTE();
      Value constant = frame->closure->function->chunk.constants.values[index];

      push(constant);
      break;
    }
    case OP_NIL:
      push(NIL_VAL);
      break;
    case OP_TRUE:
      push(BOOL_VAL(true));
      break;
    case OP_METHOD:
      defineMethod(READ_STRING());
      break;
    case OP_FALSE:
      push(BOOL_VAL(false));
      break;
    case OP_LOOP: {
      uint16_t offset = READ_SHORT();
      frame->ip -= offset;
      break;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
#undef READ_SHORT
#undef READ_STRING
}

InterpretResult interpret(const char *source) {

  ObjFunction *function = compile(source);
  if (function == NULL)
    return INTERPRET_COMPILE_ERROR;
  push(OBJ_VAL(function));
  ObjClosure *closure = newClosure(function);
  pop();
  push(OBJ_VAL(closure));
  call(closure, 0);

  return run();
}

void push(Value value) {

  if (vm.stackTop - vm.stack >= vm.stackSize) {
    int oldSize = vm.stackSize;
    vm.stackSize = GROW_CAPACITY(oldSize);
    vm.stack = GROW_ARRAY(Value, vm.stack, oldSize, vm.stackSize);
    vm.stackTop = vm.stack + oldSize;
  }

  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

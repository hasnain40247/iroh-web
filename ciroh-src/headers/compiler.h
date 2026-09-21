#ifndef iroh_compiler_h
#define iroh_compiler_h
#include "vm.h"
#include "object.h"


extern bool replMode;

ObjFunction* compile(const char* source);

void markCompilerRoots();



#endif

#ifndef iroh_memory_h
#define iroh_memory_h
#include "object.h"
#include "common.h"
#include "table.h"

#define GROW_CAPACITY(capacity) \
	((capacity) < 8? 8 :(capacity)*2)

#define GROW_ARRAY(type,pointer,oldcapacity,newcapacity) \
	(type*)reallocate(pointer, sizeof(type)*oldcapacity, sizeof(type)*newcapacity)


#define FREE_ARRAY(type,pointer,capacity) \
	reallocate(pointer, sizeof(type)*capacity,0)

#define ALLOCATE(type,count) \
	(type*)reallocate(NULL,0,sizeof(type)*count)

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)
void markValue(Value value);

void* reallocate(void* pointer, size_t oldSize,size_t newSize);
void freeObjects();
void collectGarbage();
void markObject(Obj* object);
void markTable(Table* table);





#endif

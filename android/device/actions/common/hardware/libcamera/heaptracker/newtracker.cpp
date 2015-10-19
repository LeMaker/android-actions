

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

extern void (*malloc_log)(const char *fmt, ...);
#define ARRAY_MAGIC (('A'<<24)|('R'<<16)|('R'<<8)|('Y'))
void* operator new(size_t size) {
    return malloc(size);
}

void* operator new[] (size_t size) {
    void *p=NULL;
    p = malloc(size+sizeof(int));

    *(int *)p = ARRAY_MAGIC;
    return (void *)((unsigned int)p+sizeof(int));
}

void operator delete (void *p) {
    free(p);
}

void operator delete[] (void *p) {
    void *f = (void *)((unsigned int)p - sizeof(int));
    if(*(int*)f != ARRAY_MAGIC)
    {
        malloc_log("delete[] error!");
    }
    free(f);
}                  

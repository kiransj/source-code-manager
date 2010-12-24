#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <errno.h>
typedef enum 
{
	false = 0,
	true = 1,
}bool;

#define MAX_BUFFER_SIZE		4096

#define OBJECT_FILE_LIST	0xFF000001


#define LOG_ERROR(format, ...) fprintf(stderr, format "\n", ## __VA_ARGS__)
#define LOG_INFO(format, ...)  fprintf(stdout, format "\n", ## __VA_ARGS__)

typedef unsigned int uint32_t;
typedef unsigned short int uint16_t;


void* XMALLOC(size_t size);
void* XREALLOC(void *p, size_t size);
void PrintAllocatedBytes(void);
#define XFREE(x)  do { if(NULL != x) free(x), x = NULL; }while(0)

extern void *memcpy (void *dest, const void *src, size_t n);

int Nstrlen(const char *str);
bool isItFile(const char *name);
bool isItFolder(const char *name);
#endif

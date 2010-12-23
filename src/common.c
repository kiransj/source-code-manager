#include <stdio.h>
#include <time.h>
#include <malloc.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "common.h"

#define MIN_MEMORY_TO_ALLOCATE	16

static int numOfBytesAllocated = 0;
void* XMALLOC(size_t size)
{
	void *ptr;
	if(MIN_MEMORY_TO_ALLOCATE > size)
		size = MIN_MEMORY_TO_ALLOCATE;
	numOfBytesAllocated += size;		
	ptr = malloc(size);
	if(NULL == ptr)
	{
		LOG_ERROR("unable to allocate memory of size %d", size);
		exit(1);
	}
	return ptr;
}

void PrintAllocatedBytes(void)
{	
	LOG_INFO("numOfBytesAllocated :%d", numOfBytesAllocated);
	return;
}
void* XREALLOC(void *p, size_t size)
{
	void *ptr;
	ptr = realloc(p, size);
	if(NULL == ptr)
	{
		LOG_ERROR("unable to [Re]allocate memory of size %d", size);
		exit(1);
	}
	return ptr;
}


int Nstrlen(const char *str)
{
	int len = 0 ;
	if(NULL != str)
	{
		while(str[len] != 0)
			len++;
	}
	return len;
}
int getTime(char buffer[64])
{
	time_t t;
	struct tm *tim;
	t = time(NULL);
	tim = localtime(&t);
	strftime(buffer, 64, "%a %b %d  %H:%M:%S  %Y", tim);
	return 0;
}

bool isItFolder(const char *name)
{
	struct stat s;
	if(0 == stat(name, &s))
	{
		if(S_ISDIR(s.st_mode))
			return true;
	}
	return false;
}

bool isItFile(const char *name)
{
	struct stat s;
	if(0 == stat(name, &s))
	{
		if(S_ISREG(s.st_mode))
			return true;
	}
	return false;
}


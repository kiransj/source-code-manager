#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <malloc.h>
#include <stdlib.h>
#include <zlib.h>
#include <assert.h>

#include "common.h"
#define MIN_MEMORY_TO_ALLOCATE	16

void* XMALLOC(size_t size)
{
	void *ptr;
	if(MIN_MEMORY_TO_ALLOCATE > size)
		size = MIN_MEMORY_TO_ALLOCATE;
	ptr = malloc(size);
	if(NULL == ptr)
	{
		LOG_ERROR("unable to allocate memory of size %d", size);
		exit(1);
	}
	return ptr;
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


int getTime(char buffer[64])
{
	time_t t;
	struct tm *tim;
	t = time(NULL);
	tim = localtime(&t);
	strftime(buffer, 64, "%a %b %d  %H:%M:%S  %Y", tim);
	return 0;
}



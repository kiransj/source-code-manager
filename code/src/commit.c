#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "scm.h"
#include "commit.h"
#include "common.h"


Commit Commit_Create(void)
{
	Commit c;
	c = (Commit)XMALLOC(sizeof(struct _Commit));
	sha_reset(c->parent0);
	sha_reset(c->parent1);
	sha_reset(c->tree);
	c->author = String_Create();
	c->message = String_Create();
	getTime(c->dateTime);
	return c;
}

void commit_Delete(Commit c)
{
	String_Delete(c->author);
	String_Delete(c->message);
	XFREE(c);
	return ;
}

void Commit_Reset(Commit c)
{
	sha_reset(c->parent0);
	sha_reset(c->parent1);
	sha_reset(c->tree);	
	getTime(c->dateTime);
	String_strcpy(c->author, "\0");
	String_strcpy(c->message, "\0");
	return;
}



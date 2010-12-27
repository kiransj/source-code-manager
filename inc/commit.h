#include "scm.h"
#include "sha.h"
#include "common.h"


struct _Commit
{
	ShaBuffer 	tree;
	ShaBuffer 	parent0, parent1;
	char	  	dateTime[64];
	char		*author;
	char		*message;
};

typedef struct _Commit *Commit;

void Commit_Reset(Commit s);
Commit Commit_Create(void);
void Commit_Delete(Commit c);
bool Commit_SetTree(Commit c, ShaBuffer tree);
bool Commit_SetParent0(Commit c, ShaBuffer parent);
bool Commit_SetParent1(Commit c, ShaBuffer parent);
bool Commit_ReadCommitFile(Commit d, ShaBuffer commit);
bool Commit_SetMessage(Commit c, char *message);
bool Commit_SetAuthor(Commit c, const char *authorName, const char *authorEmailId);

bool WriteCommitFile(Commit s, ShaBuffer commit);
void PrintCommit(Commit s);
bool SetCurrentCommit(ShaBuffer commitSha);
bool GetCurrentCommit(Commit commit, ShaBuffer commitSha);

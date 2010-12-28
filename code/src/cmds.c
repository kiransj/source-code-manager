#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "cmds.h"
#include "sha.h"
#include "strings.h"
#include "filelist.h"
#include "common.h"
#include "scm.h"
#include "obj.h"
#include "commit.h"

int cmd_version(int argc, char *argv[])
{
	LOG_INFO("%s Version 0.01", argv[0]);
	return 0;
}

int cmd_sha(int argc, char *argv[])
{
	int i;
	ShaBuffer sha;

	if(argc <= 2)
	{
		LOG_INFO("Usage %s %s <filenames>", argv[0], argv[1]);
		return 1;
	}
	for(i = 2; i < argc; i++)
	{
		if(true == sha_file(argv[i], sha))
		{
			LOG_INFO("%s %s", sha, argv[i]);
		}
		else
		{
			LOG_INFO("fatal: unable to hash '%s'", argv[i]);
			return 1;
		}
	}
	return 0;
}

int cmd_branch(int argc, char *argv[])
{
	String s;
	s = String_Create();
	if(true == getBranchName(s))
	{
		LOG_INFO("%s", s_getstr(s));
	}
	String_Delete(s);
	return 0;
}

static int differences(File ref, File n, DifferenceType type, void *data)
{
	bool folder = false;
	switch(type)
	{
		case FILE_NEW:
			folder = isItFolder(s_getstr(n->filename));
			LOG_INFO("?? %s%c", s_getstr(n->filename), folder? '/':' ');
			break;
		case FILE_DELETED:
			folder = isItFolder(s_getstr(ref->filename));
			LOG_INFO(" D %s%c", s_getstr(ref->filename), folder? '/' : ' ');
			break;
		case FILE_MODIFIED:
			if(isItFile(s_getstr(n->filename)))
			{
				char p[3] = {' ', ' ', '\0'};
				if(ref->mode !=  n->mode)
					p[0] = 'P';
				if(S_ISREG(ref->mode))
				{
					if((strlen((char*)n->sha)+1) != SHA_HASH_LENGTH)
						sha_file(s_getstr(n->filename), n->sha);
					if(false == sha_compare(ref->sha, n->sha))
					{
						p[1] = 'M';
					}
				}
				if(strlen(p))
				LOG_INFO("%2s %s", p, s_getstr(ref->filename));
			}
			break;
		case FILE_LAST_VALUE:
		default:
			LOG_ERROR("invalid type in difference");
			return 1;
	}
	return 0;
}

int cmd_status(int argc, char *argv[])
{
	int returnValue = 1;
	FileList f, f1;
	String s;
	s = String_Create();
	f = FileList_Create();
	f1 = FileList_Create();

	if(false == getCurrentIndexFile(s))
		goto EXIT;

	if(false == FileList_DeSerialize(f, s_getstr(s)))
		goto EXIT;

	FileList_GetDirectoryConents(f1, "./", true, false);

	FileList_GetDifference(f, f1, differences, NULL);
	returnValue = 0;
EXIT:
	FileList_Delete(f);
	FileList_Delete(f1);
	String_Delete(s);
	return returnValue;
}

int cmd_add(int argc, char *argv[])
{
	int i, returnValue = 0;
	FileList f = FileList_Create(), f1 = FileList_Create();
	String indexfile = String_Create(), s1 = String_Create();
	if(argc < 3)
	{
		LOG_ERROR("usage %s %s <filename | foldername>", argv[0], argv[1]);
	}

	if(false == getCurrentIndexFile(indexfile))
	{
		returnValue = 1;
		goto EXIT;
	}

	FileList_DeSerialize(f, s_getstr(indexfile));
	for(i = 2; i < argc; i++)
	{
		if(true == isItFile(argv[i]))
		{
			char *str;
			int len;
			File d = File_Create();
			String_strcpy(s1, argv[i]);
			String_NormalizeFileName(s1);
			str = (char*)s_getstr(s1);
			len = String_strlen(s1);
			LOG_INFO("adding %s", str);
			File_SetFileData(d, str, true);
			FileList_InsertFile(f, str, true);

			/*copy the file to repo..*/
			copyFileToCache(d);
			File_Delete(d);
			/* Add all the folder which leads to the current file
			 * Example if you insert scm add ./code/src/main.c the following gets inserted
			 * ./code
			 * ./code/src
			 * ./code/src/main.c
			 *
			 * Make sure we don't insert './' */
			while(len > 2)
			{
				while((len > 2) && str[len--] != '/');
				if(len > 2)
				{
					str[len+1] = 0;
					FileList_InsertFile(f, str, false);
				}
			}
		}
		else if(true == isItFolder(argv[i]))
		{
			File *list;
			char *str;
			uint32_t num = 0, j, len;
			FileList_ResetList(f1);
			String_strcpy(s1, argv[i]);

			String_NormalizeFolderName(s1);

			LOG_INFO("adding %s/", s_getstr(s1));
			if(strcmp(s_getstr(s1), ".") != 0)
				FileList_InsertFile(f, s_getstr(s1), false);

			FileList_GetDirectoryConents(f1, s_getstr(s1), true /*recursive*/, false /*computeSha*/);
			list = FileList_GetListDetails(f1, &num);

			/*Add the individual files to the list*/
			for(j = 0; j < num; j++)
			{
				uint32_t pos, temp;
				FileList_InsertFile(f, s_getstr(list[j]->filename), true);

				/*copy the file to repo..*/
				if(FileList_Find(f, s_getstr(list[j]->filename), &pos))
					copyFileToCache(FileList_GetListDetails(f, &temp)[pos]);
			}
			/*Add all the folders which leads to current folder*/
			str = (char*)s_getstr(s1);
			len = String_strlen(s1);
			while(len > 2)
			{
				while((len > 2) && str[len--] != '/');
				if(len > 2)
				{
					str[len+1] = 0;
					FileList_InsertFile(f, str, false);
				}
			}
		}
		else
		{
			LOG_ERROR("FATAL: cmd_add() '%s' is neither a file nor a folder", argv[i]);
			returnValue = 1;
			goto EXIT;
		}
	}
	/*Rewrite the index file*/
	FileList_Serialize(f, s_getstr(indexfile));
EXIT:
	FileList_Delete(f);
	FileList_Delete(f1);
	String_Delete(indexfile);
	String_Delete(s1);
	return returnValue;
}


int cmd_ls(int argc, char *argv[])
{
	int returnValue = 0;
	bool recursive = false, longlist = false;
	FileList f = FileList_Create();
	String indexfile = String_Create();
	if(argc >= 3)
	{
		int c;
		while((c = getopt(argc, argv, "rl")) != -1)
		{
			switch(c)
			{
				case 'r':
					recursive = true;
					break;
				case 'l':
					longlist = true;
					break;
				default:
					LOG_ERROR("usage %c %s %s -l \"long list\" -r \"recursive\"", c, argv[0], argv[1]);
					returnValue = 1;
					goto EXIT;
			}
		}
	}

	if(false == getCurrentIndexFile(indexfile))
	{
		returnValue = 1;
		goto EXIT;
	}
	if(true == FileList_DeSerialize(f, s_getstr(indexfile)))
		FileList_PrintList(f, recursive, longlist);
EXIT:
	String_Delete(indexfile);
	FileList_Delete(f);
	return returnValue;
}

struct diff
{
	int n, d, m;
};
static int differences_commit(File ref, File n, DifferenceType type, void *data)
{
	struct diff *p = (struct diff*)data;
	switch(type)
	{
		case FILE_NEW:
			p->n++;
			break;
		case FILE_DELETED:
			p->d++;
			break;
		case FILE_MODIFIED:

			if(ref->mode !=  n->mode)
				p->m++;
			else if(S_ISREG(ref->mode))
			{
				if((strlen((char*)n->sha)+1) != SHA_HASH_LENGTH)
					sha_file(s_getstr(n->filename), n->sha);
				if(false == sha_compare(ref->sha, n->sha))
				{
					p->m++;
				}
			}
			break;
		case FILE_LAST_VALUE:
		default:
			break;
	}
	return 1;
}

static bool proceedWithCommit(char *argv, ShaBuffer treeSha)
{
	File tree = File_Create();
	bool returnValue = false;
	FileList f, f1;
	struct diff d;
	String s;
	d.n = d.d = d.m = 0;
	s = String_Create();
	f = FileList_Create();
	f1 = FileList_Create();

	if(false == getCurrentIndexFile(s))
		goto EXIT;

	if(false == FileList_DeSerialize(f, s_getstr(s)))
		goto EXIT;

	FileList_GetDirectoryConents(f1, "./", true, false);

	FileList_GetDifference(f, f1, differences_commit, &d);
	if((d.d + d.m) > 0)
	{
		LOG_ERROR("changes not updated, run `%s status` to view the changes", argv);
		LOG_ERROR("Or just run `%s add .` to add all the changes and then commit", argv);
		goto EXIT;
	}

	File_SetFileData(tree,s_getstr(s), true);
	copyTreeToRepo(tree);
	memcpy(treeSha, tree->sha, SHA_HASH_LENGTH);
	returnValue = true;
EXIT:
	File_Delete(tree);
	FileList_Delete(f);
	FileList_Delete(f1);
	String_Delete(s);
	return returnValue;
}
int cmd_commit(int argc, char *argv[])
{
	ShaBuffer treeSha, commitSha, prevCommit;
	Commit c = Commit_Create(), prev = Commit_Create();
	int returnValue = 1;
	if(false == proceedWithCommit(argv[0], treeSha))
		goto EXIT;
	
	LOG_INFO("tree Sha : %s", treeSha);
	Commit_SetTree(c, treeSha);
	if(false == getCurrentCommit(prev, prevCommit))
	{			
		LOG_INFO("first commit");
		Commit_SetParent(c, "\0", "\0");
	}
	else
	{
		LOG_INFO("parent : %s", prevCommit);
		Commit_SetParent(c, prevCommit, "\0");
	}
		
	Commit_SetAuthor(c, "kiransj", "kiransj2@gmail.com");
	Commit_SetMessage(c, "initial Upload");
	Commit_WriteCommitFile(c, commitSha);
	setCurrentCommit(commitSha);
	LOG_INFO("commit Sha: %s", commitSha);
EXIT:
	Commit_Delete(c);
	Commit_Delete(prev);
	return returnValue;
}


int cmd_info(int argc, char *argv[])
{
	ShaBuffer sha;
	Commit c = Commit_Create();
	if(true == getCurrentCommit(c, sha))
	{
		LOG_INFO("Commit : %s", sha);
		LOG_INFO("Tree	 : %s", c->tree);
		LOG_INFO("Parent : %s", c->parent0);
		LOG_INFO("Author : %s", s_getstr(c->author));
	}
	else
	{
		LOG_INFO("commit not set, new repo!!!");
	}
	Commit_Delete(c);
	return 0;
}

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
	LOG_INFO("%s Version %s", argv[0], SCM_VERSION_NUMBER);

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
				if((p[0] != ' ') || p[1] != ' ')
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
	FileList_InsertFile(f1, ".", false);
	FileList_GetDifference(f, f1, differences, NULL);
	returnValue = 0;
EXIT:
	FileList_Delete(f);
	FileList_Delete(f1);
	String_Delete(s);
	return returnValue;
}

bool isFileModified(File f, const char *filename)
{
	struct stat s;
	bool flag = false;
	if(0 != stat(filename, &s))
	{
		return flag;
	}
	if(f->mode != s.st_mode)
	{
		f->mode = s.st_mode;
		flag = true;
	}
	if(f->mtime != s.st_mtime)
	{
		File_SetFileData(f, filename, true);
		flag = true;
	}
	return flag;
}

bool addFileIfNecessary(FileList indexlist, const String filename)
{
	uint32_t pos = 0;
	bool modified;

/*Add the file to repo*/
	if(true == FileList_Find(indexlist, s_getstr(filename), &pos))
	{
		/*check if the has modifed. if true copy it to repo and update the index*/
		if(true == isFileModified(FileList_GetListDetails(indexlist,NULL)[pos], s_getstr(filename)))
		{
			modified = true;
			LOG_INFO("Updating %s", s_getstr(filename));
			copyFileToCache(FileList_GetListDetails(indexlist,NULL)[pos]);
		}
	}
	else
	{
		uint32_t len;
		char *str;

		LOG_INFO("Adding %s", s_getstr(filename));
		/*this is a new file, add it to the list and update the index and copy it to cache*/
		FileList_InsertFile(indexlist,s_getstr(filename),true);
		FileList_Find(indexlist, s_getstr(filename), &pos);
		copyFileToCache(FileList_GetListDetails(indexlist,NULL)[pos]);
		modified = true;

		/*add all the folder that leads to current folder*/
		str = (char*)s_getstr(filename);
		len = String_strlen(filename);
		while(len > 2)
		{
			while((len > 2) && str[len--] != '/');
			if(len > 2)
			{
				str[len+1] = 0;
				FileList_InsertFile(indexlist, str, false);
			}
		}

	}
	return modified;
}

int cmd_add(int argc, char *argv[])
{
	bool modified = false;
	int i, returnValue = 0;
	FileList indexlist = FileList_Create();
	String indexfile = String_Create(), filename = String_Create();
	if(argc < 3)
	{
		LOG_ERROR("usage %s %s <filename | foldername>", argv[0], argv[1]);
	}

	if(false == getCurrentIndexFile(indexfile))
	{
		returnValue = 1;
		goto EXIT;
	}

	FileList_DeSerialize(indexlist, s_getstr(indexfile));
	for(i = 2; i < argc; i++)
	{
	
		String_strcpy(filename, argv[i]);
		String_NormalizeFolderName(filename);
		if(isItFile(argv[i]))
		{
			/*add the file if it modified*/
			modified = addFileIfNecessary(indexlist, filename) || modified;
		}
		else if(isItFolder(argv[i]))
		{
			/*read all the files in the folder recursively and add/update them if necessary*/
			File *list;
			FileList f1;
			char *str;
			uint32_t num = 0, j, len;
			f1 = FileList_Create();
				
			FileList_InsertFile(indexlist, s_getstr(filename), false);

			FileList_GetDirectoryConents(f1, s_getstr(filename), true /*recursive*/, false /*computeSha*/);
			list = FileList_GetListDetails(f1, &num);

			/*Add the individual files to the list*/
			for(j = 0; j < num; j++)
			{
				modified = addFileIfNecessary(indexlist, list[j]->filename) || modified;
			}

			/*Add all the folders which leads to current folder*/
			str = (char*)s_getstr(filename);
			len = String_strlen(filename);
			while(len > 2)
			{
				while((len > 2) && str[len--] != '/');
				if(len > 2)
				{
					str[len+1] = 0;
					FileList_InsertFile(indexlist, str, false);
				}
			}

			FileList_Delete(f1);
		}
		else
		{
			LOG_ERROR("fatal: file '%s' not found", argv[i]);
			goto EXIT;
		}
	}
	if(true == modified)
		FileList_Serialize(indexlist, s_getstr(indexfile));
EXIT:
	FileList_Delete(indexlist);

	String_Delete(indexfile);
	String_Delete(filename);
	return returnValue;
}


int cmd_rm(int argc, char *argv[])
{
	FileList f;
	String indexfile, s;
	bool recursive = false;
	int returnValue = 1;
	if(argc == 2)
	{
		LOG_ERROR("usage %s %s <filenames |  foldernames>", argv[0], argv[1]);
		return 1;
	}
	indexfile = String_Create();
	s = String_Create();
	f = FileList_Create();

	if(false == getCurrentIndexFile(indexfile))
	{
		returnValue = 1;
		goto EXIT;
	}

	if(true == FileList_DeSerialize(f, s_getstr(indexfile)))
	{
		int i;
		for(i = 2; i < argc; i++)
		{

			if(argv[i][0] != '-')
			{
				String_strcpy(s, argv[i]);
				String_NormalizeFileName(s);
				if(false == FileList_RemoveFile(f,s_getstr(s), recursive))
				{
					goto EXIT;
				}
				LOG_INFO("Deleting %s", argv[i]);
			}
			else
			{
				if((argv[i][1] == 'r') && argv[i][2] == '\0')
					recursive = true;
				else
				{

					LOG_ERROR("usage %s %s -r <filenames |  foldernames>", argv[0], argv[1]);
					goto EXIT;
				}
			}
		}

		/*Rewrite the index file, without the deleted file*/
		FileList_Serialize(f, s_getstr(indexfile));
		returnValue = 0;
	}
EXIT:
	FileList_Delete(f);
	String_Delete(indexfile);
	String_Delete(s);
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
		while((c = getopt(argc, argv, "rhl")) != -1)
		{
			switch(c)
			{
				case 'r':
					recursive = true;
					break;
				case 'l':
					longlist = true;
					break;
				case 'h':
					LOG_INFO("usage %s %s [-l \"long list\"] [-r \"recursive\"]", argv[0], argv[1]);
					returnValue = 0;
					goto EXIT;
				default:
					LOG_ERROR("usage %s %s -l \"long list\" -r \"recursive\"", argv[0], argv[1]);
					returnValue = 1;
					LOG_ERROR("usage %s %s -l \"long list\" -r \"recursive\"", argv[0], argv[1]);
					returnValue = 1;
					goto EXIT;
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
	bool copyFile;
};

/*finds what has changed between two file objects of the same file.
 * and if necessary copies content to the blob folder */
static int differences_commit(File ref, File n, DifferenceType type, void *data)
{
	struct diff *p = (struct diff*)data;
	switch(type)
	{
		case FILE_NEW:
			p->n++;
			if((true == p->copyFile) && S_ISREG(n->mode))
				moveFileFromCacheToRepo(n);
			break;
		case FILE_DELETED:
			p->d++;
			break;
		case FILE_MODIFIED:
			if(S_ISREG(ref->mode))
			{
				sha_file(s_getstr(n->filename), n->sha);
				if(false == sha_compare(ref->sha, n->sha))
				{
					p->m++;
					if(true == p->copyFile)
						moveFileFromCacheToRepo(n);
				}
				else if(ref->mode !=  n->mode)
					p->m++;
			}
			else if(ref->mode != n->mode)
				p->m++;
			break;
		case FILE_LAST_VALUE:
		default:
			break;
	}
	return 1;
}
#if 0
static bool proceedWithCommit(char *argv)
{
	bool returnValue = false;
	FileList f, f1;
	struct diff d = {0, 0, 0, false};
	String s;
	s = String_Create();
	f = FileList_Create();
	f1 = FileList_Create();

	if(false == getCurrentIndexFile(s))
		goto EXIT;

	if(false == FileList_DeSerialize(f, s_getstr(s)))
		goto EXIT;

	if(FileList_GetListLength(f) == 0)
	{
		LOG_ERROR("index file is empty!!!");
		goto EXIT;
	}
	FileList_GetDirectoryConents(f1, "./", true, false);
	d.copyFile = false;
	FileList_GetDifference(f, f1, differences_commit, &d);
	if(d.m > 0)
	{
		LOG_ERROR("changes not updated, run `%s status` to view the changes", argv);
		LOG_ERROR("Or just run `%s add .` to add all the changes and then commit", argv);
		goto EXIT;
	}
	returnValue = true;
EXIT:
	FileList_Delete(f);
	FileList_Delete(f1);
	String_Delete(s);
	return returnValue;
}
#endif

int cmd_commit(int argc, char *argv[])
{
	int returnValue = 1;
	File f = File_Create();
	String s = String_Create();
	struct diff d = {0, 0, 0, true};
	ShaBuffer commitSha, prevCommit, dummy;
	Commit c = Commit_Create(), prev = Commit_Create();
	FileList parentTree = FileList_Create(), indexTree = FileList_Create();


	if(argc >= 3)
	{
		int o;
		bool flag = false;
		while((o = getopt(argc, argv, "hm:")) != -1)
		{
			switch(o)
			{
				case 'm':
					Commit_SetMessage(c, optarg);
					flag = true;
					break;
				case 'h':
					LOG_INFO("usage %s %s -m <msg>", argv[0], argv[1]);
					returnValue = 0;
					goto EXIT;

				default:
					LOG_ERROR("usage %s %s -m <msg>", argv[0], argv[1]);
					returnValue = 1;
					goto EXIT;
			}
		}
		if(false == flag)
		{
			LOG_ERROR("usage %s %s -m <msg>", argv[0], argv[1]);
			goto EXIT;
		}
	}
	else
	{
		LOG_ERROR("usage %s %s -m <msg>", argv[0], argv[1]);
		goto EXIT;
	}

	/*check whether all the changes to the working area are added
	 * into index, if not abort commit*/
	if(true == compareIndexWithWorkingArea())
	{
		LOG_ERROR("changes not updated, run `%s status` to view the changes", argv[0]);
		LOG_ERROR("Or just run `%s add .` to add all the changes and then commit", argv[0]);
		goto EXIT;
	}

	sha_reset(dummy);

	if(false == getCurrentCommit(prev, prevCommit))
	{
		LOG_INFO("first commit.. parent commit is NULL");
		Commit_SetParent(c, dummy, dummy);
		FileList_ResetList(parentTree);
	}
	else
	{
		Commit_SetParent(c, prevCommit, dummy);
		String_format(s, "%s/%s", SCM_TREE_FOLDER, prev->tree);
		FileList_DeSerialize(parentTree, s_getstr(s));
	}
	/*Now check if any thing has been modified by comparing
	 * index with the parentTree*/
	getCurrentIndexFile(s);
	FileList_DeSerialize(indexTree, s_getstr(s));

	/*Copy the modified files from cache to repo..*/
	d.copyFile = true;
	FileList_GetDifference(parentTree, indexTree, differences_commit, &d);

	if((d.n + d.d + d.m) == 0)
	{
		LOG_INFO("nothing to comit");
		goto EXIT;
	}

	/*save the index file as tree into the tree repo*/
	getCurrentIndexFile(s);
	File_SetFileData(f, s_getstr(s), true);
	copyTreeToRepo(f);

	/*save the commit in the commit repo*/
	Commit_SetTree(c, f->sha);
	Commit_SetAuthor(c, "kiransj", "kiransj2@gmail.com");
	Commit_WriteCommitFile(c, commitSha);

	/*update the branch commit file*/
	setCurrentCommit(commitSha);
	returnValue = 0;
	/*print the info*/
	cmd_info(0, NULL);
EXIT:

	FileList_Delete(indexTree);
	FileList_Delete(parentTree);
	File_Delete(f);
	String_Delete(s);
	Commit_Delete(c);
	Commit_Delete(prev);
	return returnValue;
}


int cmd_info(int argc, char *argv[])
{
	ShaBuffer sha;
	Commit c = Commit_Create();
	bool flag = false;

	if(argc >= 3)
	{
		int o;
		while((o = getopt(argc, argv, "hc:")) != -1)
		{
			switch(o)
			{
				case 'c':
					strcpy((char*)sha, optarg);
					flag = true;
					break;
				case 'h':
					LOG_INFO("usage %s %s [-c <commitSha>]\n Default prints the current commit information", argv[0], argv[1]);
					goto EXIT;
				default:
					LOG_ERROR("usage %s %s [-c <commitSha>]", argv[0], argv[1]);
					goto EXIT;
			}
		}
	}

	if(true == flag)
		flag = Commit_ReadCommitFile(c, sha);
	else
	{
		flag = getCurrentCommit(c, sha);
		if(false == flag)
		{
			LOG_INFO("commit not set, new repo!!!");
			goto EXIT;
		}
		LOG_INFO("Commit : %s", sha);
	}


	if(true == flag)
	{
		struct tm *t;
		char buffer[64];

		t = localtime(&c->rawtime);
		strftime(buffer, 64, "%c", t);

		LOG_INFO("Tree   : %s", c->tree);
		LOG_INFO("Parent : %s", c->parent0);
		if(0 != strlen((char*)c->parent1))
		LOG_INFO("Parent1: %s", c->parent1);
		LOG_INFO("Date   : %s", buffer);
		LOG_INFO("Author : %s", s_getstr(c->author));
		LOG_INFO("\n   %s", s_getstr(c->message));
		LOG_INFO(" ");
	}
EXIT:
	Commit_Delete(c);
	return 0;
}

bool createCompletePath(FileList list, String filename, int mode)
{
	int len = String_strlen(filename), i;
	char *str = (char*)s_getstr(filename);
	uint32_t pos = 0;

	i = 2;

	LOG_INFO("%s",str);
	while(i < len)
	{
		while((i < len) && (str[i] != '/'))
			i++;

		str[i] = 0;
		if(i != len)
		{

			if(false == FileList_Find(list, str, &pos))
				LOG_ERROR("createCompletePath: folder '%s' is not part to repo..", str);
			else
			{
				mode = FileList_GetListDetails(list, NULL)[pos]->mode;
			}
			if(S_ISDIR(mode) && !isItFolder(str))
			{
				if(0 != mkdir(str, mode))
					return false;
			}

			str[i++] = '/';
		}
	}

	if(false == FileList_Find(list, str, &pos))
		LOG_ERROR("createCompletePath: folder '%s' is not part to repo..", str);
	else
	{
		mode = FileList_GetListDetails(list, NULL)[pos]->mode;
	}

	if(S_ISDIR(mode) && !isItFolder(str))
	{
		if(0 != mkdir(str, mode))
		{
			LOG_ERROR("createCompletePath: mkdir('%s') failed (%d)", str, errno);
			return false;
		}

	}
	return true;
}

int cmd_checkout(int argc, char *argv[])
{
	uint32_t i, pos;
	String filename;
	FileList indextree;
	if(argc == 2)
	{
		LOG_ERROR("usage %s %s [file|folder]name", argv[0], argv[1]);
		return 1;
	}
	filename = String_Create();
	indextree = FileList_Create();
	if((false == getCurrentIndexFile(filename)) || (false == FileList_DeSerialize(indextree, s_getstr(filename))))
		goto EXIT;
	for(i = 2; i < argc; i++)
	{
		File *list;
		pos = -1;
		String_strcpy(filename, argv[i]);
		String_NormalizeFolderName(filename);
		if(false == FileList_Find(indextree, s_getstr(filename), &pos))
		{
			LOG_ERROR("%s: '%s' is not found in the tree", argv[1], s_getstr(filename));
			goto EXIT;
		}
		list = FileList_GetListDetails(indextree,NULL);
		if(S_ISREG(list[pos]->mode))
		{
			if(false == createCompletePath(indextree, list[pos]->filename, list[pos]->mode))
				goto EXIT;
			copyFileFromRepo(list[pos]);
		}
		else  if(S_ISDIR(list[pos]->mode))
		{
			int i = pos, len = String_strlen(list[i]->filename);
			while(strncmp(s_getstr(list[i]->filename), s_getstr(list[pos]->filename), len) == 0)
			{
				if(S_ISDIR(list[i]->mode))
				{
					if(false == createCompletePath(indextree, list[i]->filename, list[i]->mode))
					{
						goto EXIT;
					}
				}
				else
				{
					copyFileFromRepo(list[i]);
				}
				i++;
			}
		}
	}
EXIT:
	FileList_Delete(indextree);
	String_Delete(filename);
	return 0;
}

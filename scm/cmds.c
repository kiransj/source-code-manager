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

int differences(File ref, File n, DifferenceType type, void *data)
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
				char p[3] = {'\0', '\0', '\0'};
				if(ref->mode !=  n->mode)
					p[0] = 'P';
				if(S_ISREG(ref->mode))
				{
					if((strlen((char*)n->sha)+1) != SHA_HASH_LENGTH)
						sha_file(s_getstr(n->filename), n->sha);
					if(false == sha_compare(ref->sha, n->sha))
					{
						p[1] = 'M';
						if('\0' == p[0])
						p[0] = ' ';
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
	FileList f, f1;
	String s;
	s = String_Create();
	f = FileList_Create();
	f1 = FileList_Create();

	getCurrentIndexFile(s);
	if(false == FileList_DeSerialize(f, s_getstr(s)))
		goto EXIT;

	FileList_GetDirectoryConents(f1, "./", true, false);

	FileList_GetDifference(f, f1, differences, NULL);
EXIT:
	FileList_Delete(f);
	FileList_Delete(f1);
	String_Delete(s);
	return 0;
}

int cmd_add(int argc, char *argv[])
{
	int i;
	FileList f = FileList_Create(), f1 = FileList_Create();
	String indexfile = String_Create(), s1 = String_Create();
	if(argc < 3)
	{
		LOG_ERROR("usage %s %s <filename | foldername>", argv[0], argv[1]);
	}

	getCurrentIndexFile(indexfile);
	FileList_DeSerialize(f, s_getstr(indexfile));
	for(i = 2; i < argc; i++)
	{
		if(true == isItFile(argv[i]))
		{
			File d = File_Create();
			String_strcpy(s1, argv[i]);
			String_NormalizeFileName(s1);
			LOG_INFO("adding %s", s_getstr(s1));
			FileList_InsertFile(f, s_getstr(s1), true);
			File_SetFileData(d, s_getstr(s1), true);

			/*copy the file to repo..*/
			copyFileToRepo(d);
			File_Delete(d);
		}
		else if(true == isItFolder(argv[i]))
		{
			File *list;
			uint32_t num = 0, j;
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
					copyFileToRepo(FileList_GetListDetails(f, &temp)[pos]);
			}
		}
	}
	/*Rewrite the index file*/
	FileList_Serialize(f, s_getstr(indexfile));
	FileList_Delete(f);
	FileList_Delete(f1);
	String_Delete(indexfile);
	String_Delete(s1);
	return 0;
}


int cmd_ls(int argc, char *argv[])
{
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
					return 1;
			}
		}
	}

	getCurrentIndexFile(indexfile);
	if(true == FileList_DeSerialize(f, s_getstr(indexfile)))
	{
		FileList_PrintList(f, recursive, longlist);
	}
	String_Delete(indexfile);
	FileList_Delete(f);
	return 0;
}

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

int cmd_version(int argc, char *argv[])
{
	LOG_INFO("scm Version 0.01");
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


static bool getBranchName(String const s)
{
	FILE *fp;
	char branchName[MAX_BUFFER_SIZE], type[MAX_BUFFER_SIZE];
	bool returnValue = false;
	if(false == isItFile(SCM_HEAD_FILE))
	{
		LOG_ERROR("fatal: getBranchName() HEAD file doesn't exist");
		goto EXIT;
	}
	fp = fopen(SCM_HEAD_FILE, "r");
	if(NULL == fp)
	{
		LOG_ERROR("fatal: getBranchName(): unable to open HEAD file");
		goto EXIT;
	}
	fscanf(fp, "%s %s", type, branchName);
	if(NULL == strstr(type, "branch:"))
	{
		LOG_ERROR("getBranchName() HEAD file format unrecogized!!");
		fclose(fp);
		goto EXIT;
	}
	String_strcpy(s, branchName);
	returnValue = true;
	fclose(fp);
EXIT:	
	return returnValue;
}

bool getCurrentIndexFile(String s)
{
	bool returnValue = false;
	String s1 = String_Create();

	if(false == getBranchName(s1))
		goto EXIT;
	String_format(s, "%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(s1), SCM_INDEX_FILENAME);
	returnValue = true;
EXIT:
	String_Delete(s1);
	return returnValue;
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
			String_strcpy(s1, argv[i]);
			String_NormalizeFileName(s1);
			LOG_INFO("adding %s", s_getstr(s1));
			FileList_InsertFile(f, s_getstr(s1), true);
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
				FileList_InsertFile(f, s_getstr(list[j]->filename), true);
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

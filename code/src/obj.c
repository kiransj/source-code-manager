#include <string.h>
#include "cmds.h"
#include "sha.h"
#include "strings.h"
#include "filelist.h"
#include "common.h"
#include "obj.h"
#include "scm.h"

bool getBranchName(String const s)
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

bool getCurrentCommitFile(String s)
{
	bool returnValue = false;
	String s1 = String_Create();

	if(false == getBranchName(s1))
		goto EXIT;
	String_format(s, "%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(s1), SCM_COMMIT_FILENAME);
	returnValue = true;
EXIT:
	String_Delete(s1);
	return returnValue;
}

bool copyFileToCache(File f)
{
	bool returnValue = false;
	if(S_ISREG(f->mode))
	{
		String s, branchName;
		if(strlen((char*)f->sha) !=  SHA_HASH_LENGTH)
		{
			LOG_ERROR("copyFileToRepo: trying to copy file whose sha is not computed %d", strlen((char*)f->sha));
			goto EXIT;
		}
		s = String_Create();
		branchName = String_Create();

		if(false == getBranchName(branchName))
		{
			String_Delete(s);
			String_Delete(branchName);
			goto EXIT;
		}
		String_format(s, "%s/%s", SCM_BLOB_FOLDER, f->sha);

		returnValue = true;
		/*Check if this file already exist in the repo*/
		if(false == isItFile(s_getstr(s)))
		{
			/*check if the file exist in the cache..
			 * if it doesn't then copy it to the cache..*/
			String_format(s, "%s/%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(branchName), SCM_BRANCH_CACHE_FOLDER, f->sha);
			if(false == isItFile(s_getstr(s)))
				returnValue = compressAndSave(s_getstr(f->filename), s_getstr(s), SCM_OBJECT_FILE_PERMISSION);
		}
		String_Delete(s);
	}
EXIT:
	return returnValue;
}


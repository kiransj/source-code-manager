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


bool createNewBranch(String branchName)
{
	FILE *fp;
	bool returnValue = false;
	String temp = String_Create();
	String currentBranch = String_Create();
	Commit currentCommit = Commit_Create();
	ShaBuffer commitSha;
	String_format(temp, "%s/%s", SCM_BRANCH_FOLDER, s_getstr(branchName));
	if(isItFolder(s_getstr(temp)))
	{
		LOG_ERROR("branch '%s' already exist's. try with another name", s_getstr(branchName));
		goto EXIT;
	}
	if(false == getCurrentCommit(currentCommit,commitSha))
	{
		LOG_ERROR("commit not set in the current branch");
		goto EXIT;
	}
	if(0 != mkdir(s_getstr(temp), SCM_FOLDER_PERMISSION))
	{
		LOG_ERROR("fatal: mkdir('%s') failed(%d)", s_getstr(temp), errno);
		goto EXIT;
	}
	String_format(temp, "%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(branchName), SCM_BRANCH_CACHE_FOLDER);	
	if(0 != mkdir(s_getstr(temp), SCM_FOLDER_PERMISSION))
	{
		LOG_ERROR("fatal: mkdir('%s') failed(%d)", s_getstr(temp), errno);
		goto EXIT;
	}

	String_format(temp, "%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(branchName), SCM_INDEX_FILENAME);	
	LOG_INFO("copy %s --> %s", currentCommit->tree, s_getstr(temp));
	copyTreeFromRepo(currentCommit->tree, s_getstr(temp), SCM_HEAD_FILE_PERMISSION);


	String_format(temp, "%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(branchName), SCM_COMMIT_FILENAME);
	fp = fopen(s_getstr(temp), "w");
	if(NULL == fp)
	{
		LOG_ERROR("unable to write into commit file '%s'", s_getstr(temp));
		goto EXIT;
	}
	fprintf(fp, "%s\n", commitSha);
	fclose(fp);
	LOG_INFO("branch '%s' pointing to '%s'", s_getstr(branchName), commitSha);
EXIT:
	Commit_Delete(currentCommit);
	String_Delete(temp);
	String_Delete(currentBranch);
	return returnValue;
}
int cmd_branch(int argc, char *argv[])
{
	String s, branchName;
	bool createBranch, setBranch;

	s = String_Create();
	branchName = String_Create();
	createBranch = setBranch = false;

	if(argc >= 3)
	{
		int o;
		while((o = getopt(argc, argv, "c:s:")) != -1)
		{
			switch(o)
			{
				case 'c':
					String_strcpy(branchName, optarg);
					createBranch = true;
					break;
				case 's':
					String_strcpy(branchName, optarg);
					setBranch = true;
					break;
				default:
					LOG_ERROR("usage %s %s [-c <create branch>] [-s <set branch>]", argv[0], argv[1]);
					goto EXIT;
			}
		}
	}
	if((true == setBranch) && (true == createBranch))
	{
		LOG_ERROR("Lets do one at a time!! first create the branch and then try to set it");
		goto EXIT;
	}
	if(true == createBranch)
	{
		createNewBranch(branchName);
	}
	else if(true == setBranch)
	{
		if(true == compareIndexWithWorkingArea())
		{
			LOG_ERROR("changes not updated, run `%s status` to view the changes", argv[0]);
			LOG_ERROR("Or just run `%s add .` to add all the changes and then try again", argv[0]);
			goto EXIT;
		}
	}
	else
	{
		File *list;
		String branch;
		uint32_t num, i, len = strlen(SCM_BRANCH_FOLDER);
		FileList f;


		branch = String_Create();
		if(getBranchName(branch) == false)
		{
			LOG_ERROR("current branch not set!!");
			String_Delete(branch);
			goto EXIT;
		}
		f = FileList_Create();
		FileList_GetDirectoryConents(f, SCM_BRANCH_FOLDER,false,false);
		list = FileList_GetListDetails(f,&num);

		for(i = 0; i < num; i++)
		{
			if(strcmp(s_getstr(list[i]->filename)+len+3, s_getstr(branch)) == 0)
				LOG_INFO("*%s", s_getstr(list[i]->filename)+len+3);
			else
				LOG_INFO(" %s", s_getstr(list[i]->filename)+len+3);
		}
		FileList_Delete(f);
		String_Delete(branch);
	}
EXIT:
	String_Delete(s);
	String_Delete(branchName);
	return 0;
}

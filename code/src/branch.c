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

struct diff
{
	int n, d, m;
};
static int differences(File ref, File n, DifferenceType type, void *data)
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
			if(S_ISREG(ref->mode))
			{
				if(ref->mode !=  n->mode)
					p->m++;
				else
				{
					sha_file(s_getstr(n->filename),n->sha);
					if(false == sha_compare(ref->sha, n->sha))
					p->m++;
				}
			}
			else if(ref->mode !=  n->mode)
				p->m++;
			break;
		case FILE_LAST_VALUE:
		default:
			break;
	}
	return 1;
}

bool compareIndexWithWorkingArea(void)
{
	bool returnValue = false;
	FileList indexlist, workingArea;
	struct diff d = {0, 0, 0};
	String filename;
	filename = String_Create();
	indexlist = FileList_Create();
	workingArea = FileList_Create();

	if(false == getCurrentIndexFile(filename))
		goto EXIT;

	if(false == FileList_DeSerialize(indexlist, s_getstr(filename)))
		goto EXIT;

	if(FileList_GetListLength(indexlist) == 0)
	{
		LOG_ERROR("index file is empty!!!");
		goto EXIT;
	}
	FileList_GetDirectoryConents(workingArea, "./", true, false);
	FileList_GetDifference(indexlist, workingArea, differences, &d);
	if(d.m > 0)
		returnValue = true;
EXIT:
	FileList_Delete(indexlist);
	FileList_Delete(workingArea);
	String_Delete(filename);
	return returnValue;
}


int cmd_branch(int argc, char *argv[])
{
	String s, branchname;
	bool createBranch, setBranch;

	s = String_Create();
	branchname = String_Create();
	createBranch = setBranch = false;

	if(argc >= 3)
	{
		int o;
		while((o = getopt(argc, argv, "c:s:")) != -1)
		{
			switch(o)
			{
				case 'c':
					String_strcpy(branchname, optarg);
					createBranch = true;
					break;
				case 's':
					String_strcpy(branchname, optarg);
					setBranch = true;
					goto EXIT;
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

EXIT:
	String_Delete(s);
	String_Delete(branchname);
	return 0;
}

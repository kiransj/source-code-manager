#include <string.h>
#include <sys/stat.h>
#include "sha.h"
#include "filelist.h"


struct listdiff
{
	int new, modified, deleted;
};
int differences(File ref, File n, DifferenceType type, void *data)
{
	switch(type)
	{
		case FILE_NEW:
			LOG_INFO("?? %s", s_getstr(n->filename));
			break;
		case FILE_DELETED:
			LOG_INFO(" D %s", s_getstr(ref->filename));
			break;
		case FILE_MODIFIED:
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
int main(int argc, char *argv[])
{
	FileList f;
	int num = 0;
	struct listdiff s;
	s.new = s.modified = s.deleted = 0;
	f = FileList_Create();
//	FileList_GetDirectoryConents(f, "./", true, false);
	FileList_DeSerialize(f, "/home/kiransj/index");
	//FileList_PrintList(f, true);
	FileList_GetListDetails(f, &num);
	LOG_INFO("%d", num);
	FileList_Delete(f);
	PrintAllocatedBytes();
	return 0;
}

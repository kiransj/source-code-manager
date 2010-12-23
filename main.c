#include "filelist.h"

int main(int argc, char *argv[])
{
	FileList f, f1;
	f = FileList_Create();
	f1 = FileList_Create();

	FileList_GetDirectoryConents(f, ".");
	FileList_GetDirectoryConents(f1, "./folder");
	LOG_INFO("========='.'============");
	FileList_PrintList(f);
	LOG_INFO("========='folder'============");
	FileList_PrintList(f1);
#if 1
	FileList_MergeSortedList(f, f1);
	LOG_INFO("========='.+folder'============");	
	FileList_PrintList(f);
#endif	
	FileList_Delete(f);
	FileList_Delete(f1);
	return 0;
}

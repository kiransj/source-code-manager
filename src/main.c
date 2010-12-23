#include "filelist.h"


int main(int argc, char *argv[])
{
	FileList f, f1, f2;
	f = FileList_Create();
	f1 = FileList_Create();
	f2 = FileList_Create();

	FileList_GetDirectoryConents(f, "./", false);


	FileList_PrintList(f);

	PrintAllocatedBytes();
	FileList_Delete(f);
	FileList_Delete(f1);
	FileList_Delete(f2);
	return 0;
}

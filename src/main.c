#include "filelist.h"


int main(int argc, char *argv[])
{
	FileList f;
	f = FileList_Create();

#if 0
	FileList_GetDirectoryConents(f, "./", true, true);
	FileList_Serialize(f, "../index");
#else
	FileList_DeSerialize(f, "../index");
#endif

	FileList_PrintList(f);

	FileList_Delete(f);
	PrintAllocatedBytes();
	return 0;
}

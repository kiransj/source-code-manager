#include "filelist.h"


int main(int argc, char *argv[])
{
	FileList f, f1, f2;
	f = FileList_Create();

	//FileList_GetDirectoryConents(f, "./", true);
	FileList_DeSerialize(f, "1");

	FileList_PrintList(f);

	FileList_Delete(f);
	return 0;
}

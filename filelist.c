#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "filelist.h"

#define MIN_LIST_SIZE	50
#define File_Swap(a, b) { File tmp = a; a = b; b = tmp;}

struct _filelist
{
	File *list;
	uint32_t length, listSize;
};


static void FileList_SetListSize(FileList f, const uint32_t size);
static bool FileList_GetPositionToInsert(const FileList f, const char* filename, uint32_t * const pos);


FileList FileList_Create(void)
{
	int i;
	FileList f;
	f = (FileList)XMALLOC(sizeof(FileList));
	f->listSize = MIN_LIST_SIZE;
	f->length = 0;

	f->list = (File*)XMALLOC(sizeof(File) * f->listSize);
	for(i = 0; i < f->listSize; i++)
	{
		f->list[i] = File_Create();
	}
	return f;
}

void FileList_Delete(FileList f)
{
	int i;
	for(i = 0; i < f->listSize; i++)
	{
		File_Delete(f->list[i]);
	}
	XFREE(f->list);
	XFREE(f);
	return;
}

static void FileList_SetListSize(FileList f, const uint32_t size)
{
	if(f->listSize < size)
	{
		int i;
		f->list = (File*)XREALLOC(f->list, size * sizeof(File));
		for(i = f->listSize; i < size; i++)
		{
			f->list[i] = File_Create();
		}
		f->listSize = size;
	}

	return;
}

static bool FileList_GetPositionToInsert(const FileList f, const char *filename, uint32_t * const pos)
{
	uint32_t mid, max, min;
	min = 0;
	max = f->length;
	*pos = 0;
	if(f->length != 0)
	{
		int temp;
		while(min <  max)
		{
			mid = (min + max)>>1;
			temp = String_strcmp(f->list[mid]->filename, filename);			
			if(0 == temp)
			{
				*pos = mid;
				return true;
			}
			else if(0 < temp)
				max = mid;
			else if(0 > temp)
				min = mid + 1;
		}
		*pos = min;
	}	
	return false;
}

File* FileList_GetListDetails(const FileList f, uint32_t * const listLength)
{
	*listLength	 = f->length;
	return f->list;
}

/*This function returns true if the filename already exist, else return's false.
* pos variable points to the position in the list if the elements exist
* if it doesn't exist it point to the pos where it should be inserted*/
uint32_t FileList_InsertFileToSortedList(FileList f, const char *filename, const bool computeSha)
{
	uint32_t pos;

	/*if the file already exist then update it*/
	if(true == FileList_GetPositionToInsert(f, filename, &pos))
	{
		File_SetFileData(f->list[pos], filename, computeSha);
	}
	else
	{
		int i;
		/*increase the list size if necessary*/
		if((f->length+1) == f->listSize)
		{
			FileList_SetListSize(f, f->listSize + MIN_LIST_SIZE);
		}
		/*Make space for the new element to moving some element down*/
		for(i = f->length; i > pos; i--)
		{
			File_Swap(f->list[i], f->list[i-1]);
		}
		File_SetFileData(f->list[pos], filename, computeSha);
		f->length++;
	}
	return pos;
}

bool FileList_MergeSortedList(FileList masterList, const FileList newList)
{
	uint32_t pos, i, j;

	if(newList->length == 0)
	{
		return true;
	}
	/*if the file already exist then update it*/
	FileList_GetPositionToInsert(masterList, String_getstr(newList->list[0]->filename), &pos);

	/*increase the list size if necessary*/
	if((masterList->length+newList->length+10) >= masterList->listSize)
	{
		FileList_SetListSize(masterList, masterList->listSize + newList->length + 10);
	}

	for(j = 0; j < newList->length; j++)
	{
		/*Make space for the new element to moving some element down*/		
		for(i = masterList->length; i > pos; i--)
		{
			File_Swap(masterList->list[i], masterList->list[i-1]);
		}

		memcpy(masterList->list[pos], newList->list[j], sizeof(FileData));
		masterList->length++;
		pos++;
	}	
	return true;
}
bool FileList_GetDirectoryConents(FileList f, const char *path)
{
	DIR *dir;
	struct dirent *d;
	struct stat s;
	char filename[MAX_BUFFER_SIZE];

	/*Set the listlength to zero..*/
	f->length = 0;

	
	if((0 != stat(path, &s) || !S_ISDIR(s.st_mode)))
	{
		LOG_ERROR("FileList_GetDirectoryConents: '%s' is not a directory/doesn't exist", path);
		return false;
	}

	dir = opendir(path);
	while(NULL != (d = readdir(dir)))
	{
		snprintf(filename, MAX_BUFFER_SIZE, "%s/%s", path, d->d_name);
		FileList_InsertFileToSortedList(f, filename, false);
	}
	closedir(dir);
	return true;
}

void FileList_PrintList(const FileList f)
{
	int i = 0;
	for(i = 0; i < f->length; i++)
	{
		LOG_INFO("%2.2d: %6.6o %s", i+1, f->list[i]->mode, String_getstr(f->list[i]->filename));
	}
	return ;
}

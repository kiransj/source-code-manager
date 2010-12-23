#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "filelist.h"

#define MIN_LIST_SIZE	10
#define File_Swap(a, b) { File tmp = a; a = b; b = tmp;}

struct _filelist
{
	File *list;
	uint32_t length, listSize;
};


static void SetListSize(FileList f, const uint32_t size);
static bool getPositionToInsert(const FileList f, const char* filename, uint32_t * const pos);


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

static void SetListSize(FileList f, const uint32_t size)
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

static bool getPositionToInsert(const FileList f, const char *filename, uint32_t * const pos)
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
uint32_t FileList_InsertFile(FileList f, const char *filename, const bool computeSha)
{
	uint32_t pos;

	/*if the file already exist then update it*/
	if(true == getPositionToInsert(f, filename, &pos))
	{
		File_SetFileData(f->list[pos], filename, computeSha);
	}
	else
	{
		int i;
		/*increase the list size if necessary*/
		if((f->length+1) == f->listSize)
		{
			SetListSize(f, f->listSize + MIN_LIST_SIZE);
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

bool FileList_MergeList(FileList masterList, const FileList newList)
{
	uint32_t pos, i, j;

	if(newList->length == 0)
	{
		return true;
	}
	/*if the file already exist then update it*/
	getPositionToInsert(masterList, s_getstr(newList->list[0]->filename), &pos);

	/*increase the list size if necessary*/
	if((masterList->length+newList->length+10) >= masterList->listSize)
	{
		SetListSize(masterList, masterList->listSize + newList->length + 10);
	}

	for(j = 0; j < newList->length; j++)
	{
		/*Make space for the new element to moving some element down*/		
		for(i = masterList->length; i > pos; i--)
		{
			File_Swap(masterList->list[i], masterList->list[i-1]);
		}
		File_SetFileData(masterList->list[pos], s_getstr(newList->list[j]->filename), false);
		masterList->length++;
		pos++;
	}	
	return true;
}

static bool excluseFile(const char *filename)
{
	int i;

	char *files[] = { ".o", ".out", ".swp", "tags"};
	char *folder[] = { ".git", ".scm", ".", "..", ".objs"}; 

	const int numOfFiles = sizeof(files)/sizeof(files[0]);
	const int numOfFolders = sizeof(folder)/sizeof(folder[0]);
	
	if(isItFile(filename))
	{
		for(i = 0;i < numOfFiles; i++)
			if(NULL != strstr(filename, files[i]))
				return true;
	}
	else if(isItFolder(filename))
	{		
		for(i = 0;i < numOfFolders; i++)
			if(0 == strcmp(filename, folder[i]))
				return true;
	}
	return false;
}
static bool GetDirectoryConents(FileList f, const char *folder)
{
	DIR *dir;
	struct dirent *d;
	String filename, path;

	/*Set the listlength to zero..*/
	f->length = 0;
	
	if(false == isItFolder(folder))
	{
		LOG_ERROR("FileList_GetDirectoryConents: '%s' is not a directory/doesn't exist", folder);
		return false;
	}
	path = String_Create();
	filename = String_Create();

	String_strcpy(path, folder);

	/*Convert folder name to ./<foldername>/ format*/
	String_NormalizeFolderName(path);

	dir = opendir(folder);
	while(NULL != (d = readdir(dir)))
	{
		if(false == excluseFile(d->d_name))
		{
			String_format(filename, "%s%s", s_getstr(path), d->d_name);
			FileList_InsertFile(f, s_getstr(filename), false);
		}
	}
	String_Delete(path);
	closedir(dir);
	String_Delete(filename);
	return true;
}

bool FileList_GetDirectoryConents(FileList f, const char *folder, const bool recursive)
{	
	if(true == recursive)
	{
		FileList l;
		int size = 20, top = 0, i;
		String *stack;

		f->length = 0; /*clean the current list*/
		/*Allocate memory for the stack, to implement recursive listing*/
		stack = (String*)XMALLOC(sizeof(String*) * size);
		for(i = 0; i < size; i++)
			stack[i] = String_Create();

		String_strcpy(stack[top++], folder); /*PUSH operation*/

		l = FileList_Create();	

		while(0 != top)
		{
			if(false == GetDirectoryConents(l, s_getstr(stack[--top])))/*pop operation*/
				continue;

			FileList_MergeList(f, l);
			/*Push all the folders into the stack*/
			for(i = 0; i < l->length; i++)
			{
				if(!S_ISDIR(l->list[i]->mode))
					continue;

				/*if it a folder then add it to the stack*/
				String_clone(stack[top++], l->list[i]->filename); /*PUSH operation*/
				/*Allocate some more memory if necessary*/
				if(top == size)
				{
					stack = (String*)XREALLOC(stack, sizeof(String*) * (size + 50));
					for(i = size; i < (size+50); i++)
						stack[i] = String_Create();
					size += 50;
				}									
			}			
		}

		FileList_Delete(l);
		for(i = 0; i < size; i++)
			String_Delete(stack[i]);
		XFREE(stack);
	}
	else
	{
		GetDirectoryConents(f, folder);
	}
	return true;
}

void FileList_PrintList(const FileList f)
{
	int i = 0;
	for(i = 0; i < f->length; i++)
	{
		LOG_INFO("%2.2d: %6.6o %s", i+1, f->list[i]->mode, s_getstr(f->list[i]->filename));
	}
	return ;
}

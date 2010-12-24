#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "filelist.h"

#define INT_SIZE		sizeof(uint32_t)
#define MIN_LIST_SIZE	10
#define File_Swap(a, b) { File tmp = a; a = b; b = tmp;}

struct _filelist
{
	File *list;
	uint32_t length, listSize;
};


static void ResetList(FileList f);
static void SetListSize(FileList f, const uint32_t size);
static bool getPositionToInsert(const FileList f, const char* filename, uint32_t * const pos);


static void ResetList(FileList f)
{
	f->length = 0;
}

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
/*This function returns true if the file is already present in the list and sets pos 
 * value to the pos of the file. If the file is not present then it returns false and 
 * set the pos value to the position where this file has to be inserted*/
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

/*Returns the list and sets the listLength to number of elements*/
File* FileList_GetListDetails(const FileList f, uint32_t * const listLength)
{
	*listLength	 = f->length;
	return f->list;
}

/*inerts the file and returns the pos where the file was inserted.
 * If the file is already present then it updates the list and returns
 * the pos of the file*/
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

/*Merges two list and */
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
		File_Clone(masterList->list[pos], newList->list[j]);
		masterList->length++;
		pos++;
	}
	return true;
}

static bool excludeFile(const char *filename)
{
	int i;

	char *files[] = { ".o", ".out", "tags", ".swp", ".git", ".scm", ".objs"};

	const int numOfFiles = sizeof(files)/sizeof(files[0]);

	if((strcmp(filename, ".") == 0) || (strcmp(filename, "..") == 0))
	{
		return true;
	}
	for(i = 0;i < numOfFiles; i++)
		if(NULL != strstr(filename, files[i]))
			return true;

	return false;
}
static bool GetDirectoryConents(FileList f, const char *folder, const bool computeSha)
{
	DIR *dir;
	struct dirent *d;
	String filename, path;

	/*Set the listlength to zero..*/
	ResetList(f);

	path = String_Create();
	filename = String_Create();

	String_strcpy(path, folder);

	/*Convert folder name to ./<foldername>/ format*/
	String_NormalizeFolderName(path);

	dir = opendir(folder);
	while(NULL != (d = readdir(dir)))
	{		
		if(false == excludeFile(d->d_name))
		{			
			String_format(filename, "%s%s", s_getstr(path), d->d_name);
			FileList_InsertFile(f, s_getstr(filename), computeSha);
		}
	}
	String_Delete(path);
	closedir(dir);
	String_Delete(filename);
	return true;
}

bool FileList_GetDirectoryConents(FileList f, const char *folder, const bool recursive, const bool computeSha)
{
	if(false == isItFolder(folder))
	{
		LOG_ERROR("FileList_GetDirectoryConents: '%s' is not a directory/doesn't exist", folder);
		return false;
	}
	if(false == recursive)
	{
		GetDirectoryConents(f, folder, computeSha);		
	}
	else
	{
		FileList l;		
		String *stack;
		int size = 20, top = 0, i;

		ResetList(f); /*clean the current list*/
		/*Allocate memory for the stack, to implement recursive listing*/
		stack = (String*)XMALLOC(sizeof(String*) * size);
		for(i = 0; i < size; i++)
			stack[i] = String_Create();

		String_strcpy(stack[top++], folder); /*PUSH operation*/

		l = FileList_Create();

		while(0 != top)
		{
			if(false == GetDirectoryConents(l, s_getstr(stack[--top]), computeSha))/*pop operation*/
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

	return true;
}

bool FileList_GetDifference(const FileList reference, const FileList newlist, fn_difference function, void *data)
{
	int temp;
	bool returnValue = false;
	uint32_t refpos, pos;
	refpos = pos = 0;
	if(NULL == function)
	{
		LOG_ERROR("FileList_GetDifference: 'function' is NULL");
		goto EXIT;
	}
	while((pos < newlist->length) && (refpos < reference->length))
	{
		temp = String_compare(reference->list[refpos]->filename, newlist->list[pos]->filename);
		if(0 == temp)
		{
			if(reference->list[refpos]->mtime != newlist->list[pos]->mtime)
			{
				function(reference->list[refpos], newlist->list[pos], FILE_MODIFIED, data);
			}
			refpos++;
			pos++;
		}
		else if(temp < 0)
		{
			/*If this filename exist in the reference list and not in the other
			 * it means this file is deleted*/
			function(reference->list[refpos], NULL, FILE_DELETED, data);
			refpos++;
		}
		else /*if(temp > 0)*/
		{
			/*If this filename doesn't exist in the reference list and is present in the other
			 * it means this file is a new one*/
			function(NULL, newlist->list[pos], FILE_NEW, data);
			pos++;
		}
	}

	/*the remaining elements in the new list are new one's*/
	while(pos < newlist->length)
	{
		function(NULL, newlist->list[pos], FILE_NEW, data);
		pos++;
	}

	while(refpos < reference->length)
	{
			function(reference->list[refpos], NULL, FILE_DELETED, data);
			refpos++;
	}
	returnValue = true;
EXIT:
	return returnValue;
}
bool FileList_DeSerialize(FileList f, const char *filename)
{
	struct stat s;
	bool returnValue = false;
	uint32_t fd, i, numOfFiles, temp;
	unsigned char buffer[MAX_BUFFER_SIZE];

	ResetList(f);
	if((0 != stat(filename, &s)) || (!S_ISREG(s.st_mode)))
	{
		LOG_ERROR("FileList_DeSerialize: '%s' is not a file/ or doesn't exist", filename);
		return false;
	}

	if(s.st_size <= 12)
	{
		LOG_ERROR("FileList_DeSerialize: File '%s' size %d bytes is too less", filename, (int)s.st_size);
		return false;
	}
	fd = open(filename, O_RDONLY);
	if(fd < 0)
	{
		LOG_ERROR("FileList_DeSerialize: open('%s') failed(%d)", filename, errno);
		return false;
	}
	read(fd, &temp, INT_SIZE);
	if(ntohl(temp) != OBJECT_FILE_LIST)
	{
		LOG_ERROR("FileList_DeSerialize: SYNC byte match failed");
		goto EXIT;
	}

	read(fd, &temp, INT_SIZE);
	numOfFiles = ntohl(temp);
	SetListSize(f, numOfFiles+10);
	for(i = 0; i < numOfFiles; i++)
	{
		if(INT_SIZE != read(fd, &temp, INT_SIZE))
			goto EXIT;

		temp = ntohl(temp);

		if(temp != read(fd, buffer, temp))
			goto EXIT;

		File_DeSerialize(f->list[i], buffer, temp);
	}
	f->length = i;
	returnValue = true;
EXIT:
	if(returnValue == false)
	{
		ResetList(f);
		LOG_ERROR("FileList_DeSerialize: failed to deserialize file '%s'", filename);
	}
	close(fd);
	return returnValue;
}
bool FileList_Serialize(FileList f, const char *filename)
{
	int fd, i;
	uint32_t temp;
	unsigned char buffer[MAX_BUFFER_SIZE];
	if(0 == f->length)
	{
		return false;
	}
	unlink(filename);
	fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IROTH | S_IROTH);
	if(fd < 0)
	{
		LOG_ERROR("FileList_Serialize: open('%s') failed(%d)", filename, errno);
		return false;
	}

	/*Write the identifier*/
	temp = htonl(OBJECT_FILE_LIST);
	write(fd, &temp, INT_SIZE);

	/*Write the number of files&folders*/
	temp = htonl(f->length);
	write(fd, &temp, INT_SIZE);

	for(i = 0 ; i < f->length ; i++)
	{
		int length;
		length = File_Serialize(f->list[i], buffer, MAX_BUFFER_SIZE);
		temp = htonl(length);
		write(fd, &temp, INT_SIZE);
		write(fd, buffer, length);
	}
	close(fd);
	return true;
}


void FileList_PrintList(const FileList f, const bool recursive)
{
	int i = 0;
	for(i = 0; i < f->length; i++)
	{
		LOG_INFO("%s%c", s_getstr(f->list[i]->filename), S_ISDIR(f->list[i]->mode) ? '/' : ' ');
		if(S_ISDIR(f->list[i]->mode) && !recursive)
		{
			int pos = i++;
			while(NULL != strstr(s_getstr(f->list[i]->filename), s_getstr(f->list[pos]->filename)))
			{
				i++;
			}
			i--;
		}
	}
	return ;
}

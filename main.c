#include <string.h>
#include <sys/stat.h>
#include "cmds.h"
#include "sha.h"
#include "filelist.h"

typedef int (*command_fn)(int argc, char *argv[]);
struct commands
{
	char commandName[16];
	command_fn function;
};


int main(int argc, char *argv[])
{
	struct commands cmds[] = {  {"sha", 			cmd_sha},
								{"version",			cmd_version},
								{"init",			cmd_init},
								{"branch",			cmd_branch},
								{"status",			cmd_status},
								{"add",			    cmd_add},
							 };
	const int commands_len = sizeof(cmds)/sizeof(*cmds);

	int i, returnValue = 1;

	if(1 == argc)
	{
		return 0;
	}
	for(i = 0; i < commands_len; i++)
	{
		if(0 == strcmp(cmds[i].commandName, argv[1]))
		{
			returnValue = cmds[i].function(argc, argv);
			break;
		}
	}
	if(i == commands_len)
	{
		LOG_ERROR("command '%s' not found", argv[1]);
	}
	return 0;
}

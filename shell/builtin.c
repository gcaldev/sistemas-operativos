#include "builtin.h"
#include <string.h>
#include <unistd.h>
#include "runcmd.h"


// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return strcmp("exit", cmd) == 0;
}


static void
_change_dir(char *new_dir)
{
	char updated_path[BUFLEN];

	if (chdir(new_dir)) {
		perror("Changing to directory failed [cd]");
		return NULL;
	}

	if (!getcwd(updated_path, BUFLEN)) {
		perror("Reading updated path failed [pwd]");
		return NULL;
	}

	if (snprintf(prompt, sizeof prompt, "(%s)", updated_path) < 0)
		perror("Changing directory in terminal failed [cd]");
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	char *path;
	char *home_path;

	bool change_to_home = !strcmp("cd", cmd);
	bool change_to_dir = strlen(cmd) > 2 && cmd[0] == 'c' &&
	                     cmd[1] == 'd' && cmd[2] == ' ';

	if (change_to_home) {
		(home_path = getenv("HOME"))
		        ? _change_dir(home_path)
		        : perror("Reading HOME env var failed [cmd]");
		return true;
	}
	if (change_to_dir) {
		path = cmd + 3;
		_change_dir(path);
		return true;
	}

	return false;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp("pwd", cmd))
		return false;

	char current_path[BUFLEN];
	if (!getcwd(current_path, BUFLEN))
		perror("Reading current path failed [pwd]");

	printf("%s\n", current_path);
	return true;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here

	return 0;
}

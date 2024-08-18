#include "exec.h"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	char key[BUFLEN];
	char value[BUFLEN];

	int assign_token_idx;

	for (int i = 0; i < eargc; i++) {
		char *arg = eargv[i];

		assign_token_idx = block_contains(arg, ENV_VAR_ASSIGN_TOKEN);

		if (assign_token_idx == -1) {
			continue;
		}

		get_environ_key(arg, key);
		get_environ_value(arg, value, assign_token_idx);

		setenv(key, value, SET_ENV_OVERWRITE_YES);
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int fd = open(file, flags, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		perror("Error opening file");
		exit(-1);
	}
	return fd;
}

static void
_cast_and_exec_cmd(struct cmd *cmd)
{
	struct execcmd *e = (struct execcmd *) cmd;

	set_environ_vars(e->eargv, e->eargc);
	execvp(e->argv[0], e->argv);

	perror("Error while trying to exec command");
	exit(-1);
}

// Closes fd in case of error and do an exit(-1)
static void
_catch_dup_error(int dup2, int fd)
{
	if (dup2 < 0) {
		close(fd);
		perror("Error in dup2");
		exit(EXIT_FAILURE);
	}
}

static void
_handle_redirection(struct execcmd *r)
{
	int fd_out = -1, fd_err = -1;
	char *cursor = r->scmd;

	while (*cursor) {  // the order is important for 2>&1 case
		if (strncmp(cursor, "2>", 2) == 0) {  // stderr redirection
			cursor += 2;

			if (strlen(r->err_file) > 0) {
				if (strcmp(r->err_file, "&1") == 0) {  // 2>&1 case
					if (fd_out ==
					    -1) {  // the stdout is not redirected yet
						continue;
					} else {  // use same fd than stdout
						fd_err = fd_out;
					}
				} else {
					fd_err = open_redir_fd(r->err_file,
					                       O_CREAT | O_RDWR |
					                               O_CLOEXEC);
				}
				_catch_dup_error(dup2(fd_err, STDERR_FILENO),
				                 fd_err);
			}
		} else if (strncmp(cursor, ">", 1) == 0) {  // stdout redirection
			cursor += 1;

			if (strlen(r->out_file) > 0) {
				fd_out = open_redir_fd(r->out_file,
				                       O_CREAT | O_TRUNC | O_RDWR |
				                               O_CLOEXEC);
				_catch_dup_error(dup2(fd_out, STDOUT_FILENO),
				                 fd_out);
			}
		} else {
			cursor++;
		}
	}

	if (fd_out != -1 && fd_err != -1 && fd_out != fd_err) {
		close(fd_out);
		close(fd_err);
	} else if (fd_out != -1) {
		close(fd_out);
	} else if (fd_err != -1) {
		close(fd_err);
	}

	if (strlen(r->in_file) > 0) {  // stdin redirection
		int fd = open_redir_fd(r->in_file, O_CLOEXEC | O_RDONLY);
		_catch_dup_error(dup2(fd, STDIN_FILENO), fd);
		close(fd);
	}
}

static void
_pipe_cmd(struct pipecmd *p)
{
	int pipefd[2];
	pid_t pid1, pid2;
	int status;

	// Create the pipe
	if (pipe(pipefd) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	// First fork for the command to the left of the pipe
	pid1 = fork();
	if (pid1 == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid1 == 0) {
		close(pipefd[0]);
		// Redirect stdout to the write end of the pipe
		_catch_dup_error(dup2(pipefd[1], STDOUT_FILENO), pipefd[1]);
		close(pipefd[1]);
		_cast_and_exec_cmd(p->leftcmd);
		exit(EXIT_SUCCESS);
	}

	// Second fork for the command to the right of the pipe
	pid2 = fork();
	if (pid2 == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid2 == 0) {
		// Close the write end of the pipe in the child process
		close(pipefd[1]);
		// Redirect stdin to the read end of the pipe
		_catch_dup_error(dup2(pipefd[0], STDIN_FILENO), pipefd[0]);
		close(pipefd[0]);

		if (p->rightcmd->type == PIPE) {
			// If the command on the right is also a pipe, call _pipe_cmd again
			_pipe_cmd((struct pipecmd *) p->rightcmd);
		} else {
			_cast_and_exec_cmd(p->rightcmd);
		}

		exit(EXIT_SUCCESS);
	}

	// Close both ends of the pipe in the parent process
	close(pipefd[0]);
	close(pipefd[1]);

	// Wait for both child processes to finish
	waitpid(pid1, &status, 0);
	waitpid(pid2, &status, 0);
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		_cast_and_exec_cmd(cmd);
		break;
	case BACK: {
		b = (struct backcmd *) cmd;
		_cast_and_exec_cmd(b->c);
		break;
	}

	case REDIR: {
		r = (struct execcmd *) cmd;
		_handle_redirection(r);
		_cast_and_exec_cmd((struct cmd *) r);
		break;
	}

	case PIPE: {
		p = (struct pipecmd *) cmd;

		_pipe_cmd(p);
		break;
	}
	}
}

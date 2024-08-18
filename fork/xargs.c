#ifndef NARGS
#define NARGS 4
#endif
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

void
reset_lines(char *lines[], int from, int to)
{
	for (size_t i = from; i <= to; i++) {
		free(lines[i]);
		lines[i] = NULL;
	}
}


void
execute_command(char *command, char *lines[], int num_lines)
{
	lines[0] = command;
	lines[num_lines + 1] = NULL;

	pid_t child_pid = fork();
	if (child_pid < 0) {
		printf("Error al intentar hacer un fork\n");
		exit(-1);
	}

	if (child_pid == 0) {
		int exec_res = execvp(command, lines);
		if (exec_res < 0) {
			printf("Error al intentar ejecutar el "
			       "comando\n");
			exit(-1);
		}

		exit(0);
	} else {
		waitpid(child_pid, NULL, 0);
	}
}


int
main(int argc, char **argv)
{
	char *command = argv[1];
	char *lines[NARGS + 2] = { NULL };

	size_t len = 0;
	ssize_t nread = 0;
	size_t i = 1;

	while ((nread = getline(&lines[i], &len, stdin)) != -1) {
		lines[i][nread - 1] = '\0';

		if (i == NARGS) {
			execute_command(command, lines, NARGS);
			reset_lines(lines, 1, NARGS);

			i = 1;
			continue;
		}
		i++;
	}
	bool commands_remaining = i != 1;

	if (commands_remaining) {
		execute_command(command, lines, i - 1);
		reset_lines(lines, 1, i);
	}

	return 0;
}
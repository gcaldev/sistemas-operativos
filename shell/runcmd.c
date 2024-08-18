#include "runcmd.h"

int status = 0;
struct cmd *parsed_pipe;

// runs the command in 'cmd'
int
run_cmd(char *cmd)
{
	pid_t p;
	struct cmd *parsed;

	// if the "enter" key is pressed
	// just print the prompt again
	if (cmd[0] == END_STRING)
		return 0;

	// "history" built-in call
	if (history(cmd))
		return 0;

	// "cd" built-in call
	if (cd(cmd))
		return 0;

	// "exit" built-in call
	if (exit_shell(cmd))
		return EXIT_SHELL;

	// "pwd" built-in call
	if (pwd(cmd))
		return 0;

	// parses the command line
	parsed = parse_line(cmd);

	// forks and run the command
	if ((p = fork()) == 0) {
		// keep a reference
		// to the parsed pipe cmd
		// so it can be freed later
		if (parsed->type == PIPE)
			parsed_pipe = parsed;

		if (parsed->type != BACK)
			setpgid(0, 0);

		exec_cmd(parsed);
	}

	// stores the pid of the process
	parsed->pid = p;

	// background process special treatment
	// Hint:
	// - check if the process is
	//		going to be run in the 'back'
	// - print info about it with
	// 	'print_back_info()'
	//
	if (parsed->type == BACK) {
		print_back_info(parsed);
		free_command(parsed);
		return 0;
	}

	// ask if the background child process has ended
	// waits for the process to finish:
	//    esta linea tambien va en la linea 15 ya que
	//    en la shell original espera a mostrar el output
	//    cuando se envia un enter aunque haya terminado
	// waitpid(-1, &status, WNOHANG);

	waitpid(p, &status, 0);

	print_status_info(parsed);

	free_command(parsed);

	return 0;
}

#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"

char prompt[PRMTLEN] = { 0 };

static void
background_handler(int signo)
{
	pid_t res;
	switch (signo) {
	case SIGCHLD:
		res = waitpid(0, &status, WNOHANG);
		if (res > 0) {
			printf_debug("DEBUG: Child %ld finished\n", res);
			fflush(stdout);
		}
		break;

	default:
		break;
	}
}

// runs a shell command
static void
run_shell()
{
	char *cmd;

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}

	// init stack and handler
	static char stack[SIGSTKSZ];
	stack_t ss = {
		.ss_size = SIGSTKSZ,
		.ss_sp = stack,
	};
	struct sigaction psa;
	psa.sa_handler = background_handler;
	psa.sa_flags = SA_ONSTACK | SA_RESTART;
	sigaltstack(&ss, 0);
	sigaction(SIGCHLD, &psa, NULL);
}

int
main(void)
{
	init_shell();

	run_shell();

	return 0;
}

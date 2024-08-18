#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define READ 0   // file descriptor de lectura
#define WRITE 1  // file descriptor de escritura


void
handle_error(char *message)
{
	printf("%s\n", message);
	exit(-1);
}

void
filter(int left[])
{
	long int prime;

	close(left[WRITE]);

	int res = read(left[READ], &prime, sizeof prime);

	if (res < 0)
		handle_error("Error de lectura en pipe");

	if (res == 0) {
		// Si se cerro el extremo de escritura se envia un EOF ya que nunca mas se usa para escribir
		close(left[READ]);
		exit(0);
	}

	printf("primo %ld\n", prime);

	int right[2];

	if (pipe(right) < 0)
		handle_error("Error al crear pipe");

	pid_t child_pid = fork();

	if (child_pid < 0)
		handle_error("Error al intentar hacer un fork");

	if (child_pid == 0) {
		close(left[READ]);
		close(right[WRITE]);

		filter(right);
		exit(0);
	} else {
		long int read_val;
		int read_res;
		close(right[READ]);

		while ((read_res = read(left[READ], &read_val, sizeof read_val)) >
		       0) {
			if (read_val % prime != 0) {
				if (write(right[WRITE], &read_val, sizeof read_val) <
				    0) {
					handle_error(
					        "Error de escritura en pipe");
				}
			}
		}
		if (read_res < 0)
			handle_error("Error de lectura en pipe");

		close(left[READ]);
		close(right[WRITE]);

		wait(NULL);
		exit(0);
	}
}

int
main(int argc, char *argv[])
{
	int initial_pipe[2];
	long int n = atoi(argv[1]);

	if (n < 2) {
		printf("El numero recibido debe ser mayor o igual a 2");
		return 0;
	}

	if (pipe(initial_pipe) < 0)
		handle_error("Error al crear el pipe");

	pid_t child_pid = fork();

	if (child_pid < 0)
		handle_error("Error al intentar hacer un fork");

	if (child_pid == 0) {
		filter(initial_pipe);
	} else {
		close(initial_pipe[READ]);

		printf("primo 2\n");
		for (long int i = 2; i < n; i++) {
			if (i % 2 != 0) {
				if (write(initial_pipe[WRITE], &i, sizeof i) < 0) {
					handle_error(
					        "Error de escritura en pipe");
				}
			}
		}
		close(initial_pipe[WRITE]);
		wait(NULL);
	}
}
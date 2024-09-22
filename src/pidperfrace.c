#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <bsd/string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

void print_help()
{
	fprintf(stderr, "Usage: pidperfrace [-h] MEASURE_PROG PROG\n");
}

void sanitize_input(char *name, char *dest, size_t destlen, char *opt, size_t optlen)
{
	size_t bytes_copied;

	bytes_copied = strlcpy(dest, opt, destlen);
	if (destlen <= optlen) {
		fprintf(stderr, "%d\n", bytes_copied);
		fprintf(stderr, "%s is too long. expected %d characters got %d\n", name, destlen, optlen);
		exit(EXIT_FAILURE);
	}
}

int strtoargv(char **out, size_t outlen, char *input)
{
	char *saveptr;
	char *tok;
	char *delim = " ";
	int i = 0;

	tok = strtok_r(input, delim, &saveptr);
	while (tok != NULL) {
		*out++ = strdup(tok);
		tok = strtok_r(NULL, delim, &saveptr);
		// TODO: Parse out things that are within quotes as their
		// own argument.
		if (i++ > outlen)
			break;
	}

	return 0;
}


int launch(char *measure_prog, size_t measure_proglen, char *prog, size_t proglen)
{
	int err;
	pid_t measure_pid;
	pid_t prog_pid;
	pid_t ppid = getpid();
	char *measure_prog_args[60] = {0};
	char *prog_prog_args[60] = {0};

	prog_pid = fork();
	if (prog_pid < 0) {
		return errno;
	}

	if (prog_pid == 0) {
		kill(getpid(), SIGSTOP);
		strtoargv(prog_prog_args, sizeof(prog_prog_args) / sizeof(char*), prog);

		for (int i = 0; i < sizeof(prog_prog_args) / sizeof(char *); i++) {
			if (!prog_prog_args[i]) break;
			printf("yolo arg %s\n", prog_prog_args[i]);
		}

		printf("in prog child %jd\n", getpid());

		err = execl(prog_prog_args[0], prog_prog_args[1], NULL);
		if (err) {
			perror("execl()");
		}

		printf("actually exited...?\n");

		for (int i = 0; i < sizeof(prog_prog_args) / sizeof(char *); i++) {
			if (!prog_prog_args[i]) {
				break;
			}
			printf("arg %s\n", prog_prog_args[i]);
			free(prog_prog_args[i]);
		}

		// return 0;
		exit(EXIT_SUCCESS);
	}


	measure_pid = fork();
	if (measure_pid < 0) {
		return errno;
	}

	if (measure_pid == 0) {
		kill(getpid(), SIGSTOP);
		printf("in measure child %jd\n", getpid());
		return 0;
	}

	pid_t w;
	int status;
	do {
		w = waitpid(prog_pid, &status, WUNTRACED | WCONTINUED);
		if (w == -1) {
			perror("waitpid");
			exit(EXIT_FAILURE);
		}

		if (WIFEXITED(status)) {
			printf("exited, status=%d\n", WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)) {
			printf("killed by signal %d\n", WTERMSIG(status));
		} else if (WIFSTOPPED(status)) {
			printf("stopped by signal %d\n", WSTOPSIG(status));
			kill(prog_pid, SIGCONT);
		} else if (WIFCONTINUED(status)) {
			printf("continued\n");
		}
	} while (!WIFEXITED(status) && !WIFSIGNALED(status));

	// waitpid()

	return 0;
}

int main(int argc, char *argv[])
{
	int opt;
	char measure_prog[4096] = {0};
	char prog[4096] = {0};
	size_t bytes_copied, optlen = 0;
	int err;

	while ((opt = getopt(argc, argv, "h")) != -1) {
		switch (opt) {
		case 'h':
			print_help();
			return EXIT_SUCCESS;
		default:
			print_help();
			return EXIT_FAILURE;
		}
	}

	if (optind >= argc) {
		print_help();
		return EXIT_FAILURE;
	}

	sanitize_input("MEASURE_PROG", measure_prog, sizeof(measure_prog), argv[optind], strlen(argv[optind]));
	optind++;

	if (optind >= argc) {
		print_help();
		return EXIT_FAILURE;
	}

	sanitize_input("PROG", prog, sizeof(prog), argv[optind], strlen(argv[optind]));

	printf("measure prog: %s\n", measure_prog);
	printf("prog: %s\n\n", prog);

	err = launch(measure_prog, sizeof(measure_prog), prog, sizeof(prog));
	return err;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef NARGS
#define NARGS 4
#endif

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		perror("ERROR. usage: ./xargs COMMANDS\n");
		exit(EXIT_FAILURE);
	}
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;

	char **args =
	        malloc(sizeof(char *) *
	               (argc - 1 + NARGS + 1));  // (dejo el lugar parar el NULL)

	for (int i = 1; i < argc; i++) {
		args[i - 1] = argv[i];
	}

	int size = argc - 1;

	nread = getline(&line, &len, stdin);
	while (nread != -1) {
		while ((size < argc - 1 + NARGS) && nread != -1) {
			if (line[nread - 1] == '\n') {
				line[nread - 1] = '\0';
			}

			args[size] = strdup(line);
			size++;

			nread = getline(&line, &len, stdin);
		}

		pid_t pid = fork();
		if (pid < 0) {
			for (int i = argc - 1; i < size; i++) {
				free(args[i]);
			}
			free(args);
			free(line);
			perror("fork");
			exit(EXIT_FAILURE);
		}

		if (pid == 0) {
			args[size] = NULL;
			if (execvp(args[0], args) < 0) {
				for (int i = argc - 1; i < size; i++) {
					free(args[i]);
					args[i] = NULL;
				}
				free(line);
				free(args);
				perror("exec");
				exit(EXIT_FAILURE);
			}
		}


		int status;
		wait(&status);

		for (int i = argc - 1; i < size; i++) {
			free(args[i]);
			args[i] = NULL;
		}
		size = argc - 1;
	}

	free(line);
	free(args);
}

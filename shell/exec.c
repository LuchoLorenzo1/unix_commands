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
	for (int i = 0; i < eargc; i++) {
		int index = block_contains(eargv[i], '=');
		if (index < 0)
			continue;

		char key[BUFLEN];
		get_environ_key(eargv[i], key);

		char val[BUFLEN];
		get_environ_value(eargv[i], val, index);

		setenv(key, val, 0);
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
// static int
// open_redir_fd(char *file, int flags)
// {
// 	// Your code here

// 	return -1;
// }

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
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		// spawns a command
		//
		// Your code here
		e = (struct execcmd *) cmd;

		set_environ_vars(e->eargv, e->eargc);

		if (execvp(e->argv[0], e->argv) < 0) {
			printf("Error executing command here\n");  // Convert into STDERR output
			exit(-1);
		}
		exit(-1);
		break;

	case BACK: {
		// runs a command in background
		//
		// Your code here
		pid_t fork_id = fork();
		if (fork_id != 0) {
			// Padre
			printf(" [PID= %d]\n", fork_id);
			waitpid(fork_id, NULL, WNOHANG);
		} else {
			// Child process
			b = (struct backcmd *) cmd;
			e = (struct cmd *) b->c;
			exec_cmd(e);
		}
		_exit(-1);
		break;
	}

	case REDIR: {
		r = (struct execcmd *) cmd;

		if (strlen(r->out_file) > 0 && strcmp("&1", r->err_file) == 0) {
			int fdout = fileno(fopen(r->out_file, "w+"));

			if (dup2(fdout, STDOUT_FILENO) < 0) {
				printf("Error dup \n");
			}
			if (dup2(fdout, STDERR_FILENO) < 0) {
				printf("Error dup \n");
			}

			close(fdout);

		} else {
			if (strlen(r->out_file) > 0) {
				int fdout = fileno(fopen(r->out_file, "w+"));
				if (dup2(fdout, STDOUT_FILENO) < 0) {
					printf("Error dup \n");
				}

				close(fdout);
			}
			if (strlen(r->in_file) > 0) {
				int fdin = fileno(fopen(r->in_file, "r"));
				if (dup2(fdin, STDIN_FILENO) < 0) {
					printf("Error dup \n");
				}
				close(fdin);
			}
			if (strlen(r->err_file) > 0) {
				FILE *file = fopen(r->err_file, "w+");
				if (file == NULL) {
					printf("file es NULL \n");
				}
				int fderr = fileno(file);
				if (dup2(fderr, STDERR_FILENO) < 0) {
					printf("Error dup \n");
				}
				close(fderr);
			}
		}

		if (execvp(r->argv[0], r->argv) < 0) {
			printf("Error executing command\n");  // Convert into STDERR output
			exit(-1);
		}

		exit(0);
		break;
	}

	case PIPE: {
		p = (struct pipecmd *) cmd;

		int fd[2];
		if (pipe(fd) < 0)
			exit(-1);

		pid_t proces = fork();

		if (proces == 0) {
			dup2(fd[0], STDIN_FILENO);
			close(fd[1]);
			close(fd[0]);

			if (p->rightcmd->type == PIPE) {
				exec_cmd(p->rightcmd);
			}

			else {
				struct execcmd *right =
				        (struct execcmd *) p->rightcmd;

				if (execvp(right->argv[0], right->argv) < 0) {
					printf("Error executing command\n");  // Convert into STDERR output
					exit(-1);
				}
			}

		} else {
			close(fd[0]);
			pid_t proces2 = fork();


			if (proces2 == 0) {
				struct execcmd *left =
				        (struct execcmd *) p->leftcmd;
				dup2(fd[1], STDOUT_FILENO);
				close(fd[1]);

				if (execvp(left->argv[0], left->argv) < 0) {
					printf("Error executing command\n");  // Convert into STDERR output
					exit(-1);
				}
			} else {
				close(fd[1]);
				waitpid(proces2, NULL, 0);
				waitpid(proces, NULL, 0);
				// free the memory allocated
				// for the pipe tree structure
				free_command(parsed_pipe);
				exit(0);
			}
		}


		break;
	}
	}
}

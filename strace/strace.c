#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <syscall.h>
#include "syscallent.h"

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		perror("Usage: ./strace command\n");
		return -1;
	}

	int pid = fork();
	if (pid == 0) {
		close(1);
		close(2);
		ptrace(PTRACE_TRACEME, getpid());
		execvp(argv[1], argv + 1);
	}

	int status;
	waitpid(pid, &status, 0);

	ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
	waitpid(pid, &status, 0);
	struct user_regs_struct regs;

	printf("%23s \t=\t RET_CODE\n", "SYSCALL");
	while (WIFSTOPPED(status)) {
		ptrace(PTRACE_GETREGS, pid, NULL, &regs);

		switch (regs.orig_rax) {
		case 0:  // READ
			printf("%18s(%llu, %llu) ",
			       SYSCALLS[regs.orig_rax],
			       regs.rdi,
			       regs.rdx);
			break;
		case 1:  // WRITE
			printf("%18s(%llu, %llu) ",
			       SYSCALLS[regs.orig_rax],
			       regs.rdi,
			       regs.rdx);
			break;
		default:
			printf("%23s ", SYSCALLS[regs.orig_rax]);
			break;
		}


		ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
		waitpid(pid, &status, 0);

		ptrace(PTRACE_GETREGS, pid, NULL, &regs);

		if ((long long) regs.rax < 0) {
			printf("\t=\t\x1b[31m%lld \x1b[0m\n", regs.rax);
		} else {
			printf("\t=\t\x1b[32m%lld \x1b[0m\n", regs.rax);
		}

		ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
		waitpid(pid, &status, 0);
	}

	wait(&status);
	return 0;
}

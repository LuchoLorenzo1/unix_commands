#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

int
main(int argc, char *argv[])
{
	if (argc > 2) {
		perror("Usage: ./ps\n");
		exit(EXIT_FAILURE);
	}

	DIR *proc = opendir("/proc");
	if (proc == NULL) {
		perror("couldn't open /proc/");
		exit(EXIT_FAILURE);
	}

	struct dirent *entry;

	char filepath[2048];
	char cmd[2048];

	printf("PID | CMD\n");
	while ((entry = readdir(proc)) != NULL) {
		strcpy(filepath, "/proc/");
		strcat(filepath, entry->d_name);
		strcat(filepath, "/comm");

		FILE *file;
		if ((file = fopen(filepath, "r")) == NULL) {
			continue;
		}
		fscanf(file, "%[^\n]", cmd);
		printf("%s  %s\n", entry->d_name, cmd);
	}

	closedir(proc);

	return 0;
}

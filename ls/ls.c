#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

int
main(int argc, char *argv[])
{
	if (argc > 2) {
		perror("Usage: ./ls DIR\n");
		exit(EXIT_FAILURE);
	}

	char path[2048];
	if (argc == 2) {
		strcpy(path, argv[1]);
	} else {
		strcpy(path, ".");
	}

	DIR *dir = opendir(path);
	if (dir == NULL) {
		perror("couldn't open this directory");
		exit(EXIT_FAILURE);
	}

	char link_path[2048];
	struct dirent *entry;
	struct stat s;

	printf("type\tuid\tmode\tfilename\n");
	while ((entry = readdir(dir)) != NULL) {
		stat(entry->d_name, &s);
		switch (entry->d_type) {
		case DT_DIR:
			printf("\x1b[34md\t\x1b[0m");
			break;
		case DT_LNK:
			readlinkat(dirfd(dir), entry->d_name, link_path, 2048);
			printf("\x1b[33ml\t\x1b[0m");
			printf("%d\t%d\t%s -> %s\n",
			       s.st_uid,
			       s.st_mode,
			       entry->d_name,
			       link_path);
			continue;
		case DT_REG:
			printf("\x1b[32m.\t\x1b[0m");
			break;
		default:
			printf("\x1b[31m?\t\x1b[0m");
			break;
		}
		printf("%d\t%d\t%s\n", s.st_uid, s.st_mode, entry->d_name);
	}

	closedir(dir);
	return 0;
}

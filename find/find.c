#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

struct file_matcher {
	const char *target;
	const char *curr_dirname;
	char *(*matcher)(const char *, const char *);
	DIR *dir;
};

void find_dir(struct file_matcher *fm);

void
find_dir(struct file_matcher *fm)
{
	if (fm->dir == NULL) {
		return;
	}

	struct dirent *entry;
	while ((entry = readdir(fm->dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0)
			continue;

		if (entry->d_type == 4) {
			char name[PATH_MAX];
			strcpy(name, fm->curr_dirname);
			strcat(name, entry->d_name);
			strcat(name, "/");

			struct file_matcher fm2;
			fm2.target = fm->target;
			fm2.curr_dirname = name;
			fm2.matcher = fm->matcher;

			fm2.dir = fdopendir(openat(
			        dirfd(fm->dir), entry->d_name, O_DIRECTORY));
			find_dir(&fm2);

			closedir(fm2.dir);
		}

		if (fm->matcher(entry->d_name, fm->target))
			printf("%s%s\n", fm->curr_dirname, entry->d_name);
	}
}

int
main(int argc, char *argv[])
{
	if (argc > 3) {
		perror("Usage: ./find [-i] [target]");
		exit(EXIT_FAILURE);
	}

	DIR *dir = opendir(".");
	if (dir == NULL) {
		perror("opendir");
		exit(EXIT_FAILURE);
	}

	struct file_matcher fm;
	fm.curr_dirname = "";
	fm.dir = dir;
	fm.matcher = strstr;

	if (argv[1] != NULL && strcmp(argv[1], "-i") == 0) {
		fm.matcher = strcasestr;
		fm.target = argv[2];
	} else {
		fm.target = argv[1];
	}

	if (fm.target == NULL)
		fm.target = "";

	find_dir(&fm);

	closedir(dir);
	exit(EXIT_SUCCESS);
}

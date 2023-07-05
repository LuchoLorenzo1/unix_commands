#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	if (argc != 3) {
		perror("Usage: ./cp SRC CPY\n");
		return -1;
	}

	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("error reading the source file\n");
		return -1;
	}

	int fd2 = open(argv[2],
	               O_RDWR | O_CREAT | O_TRUNC | O_EXCL,
	               S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	if (fd2 < 0) {
		perror("error creating the dest file or the dest file already "
		       "exists\n");
		close(fd);
		return -1;
	}

	struct stat statbuf;
	int err = fstat(fd, &statbuf);
	if (err < 0) {
		close(fd);
		close(fd2);
		perror("error reading the file");
		return -1;
	}

	ftruncate(fd2, statbuf.st_size);

	void *src = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (src == MAP_FAILED) {
		close(fd);
		close(fd2);
		perror("map src failed\n");
		return -1;
	}

	void *dest = mmap(
	        NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);
	if (dest == MAP_FAILED) {
		close(fd);
		close(fd2);
		munmap(src, statbuf.st_size);
		perror("map dest failed\n");
		return -1;
	}

	memcpy(dest, src, statbuf.st_size);

	if (munmap(src, statbuf.st_size) < 0) {
		perror("error deallocating memory\n");
		return -1;
	}
	if (munmap(dest, statbuf.st_size) < 0) {
		perror("error deallocating memory\n");
		return -1;
	}
	return 0;
}

#include <stdio.h>
#include <sys/stat.h>
int
main(int argc, char *argv[])
{
	if (argc < 2) {
		perror("argumentos insuficientes");
		return -1;
	}
	int a = mkdir(argv[1], 0666);
	printf("%d\n", a);
	return 0;
}

#include "builtin.h"
#include "runcmd.h"

char **load_command_history();

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	size_t n = strlen(cmd);
	return n == 4 && strncmp(cmd, "exit", 4) == 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	size_t n = strlen(cmd);
	if (n < 2 || strncmp(cmd, "cd", 2) != 0)
		return false;

	char buf[BUFLEN];
	if (n == 2) {
		strcpy(buf, getenv("HOME"));
	} else {
		if (cmd[2] != ' ')
			return false;
		strcpy(buf, cmd + 3);
	}

	if (chdir(buf) < 0) {
		perror("cd: No such file or directory");
		status = 1;
		return false;
	}
	strcpy(prompt, buf);
	return true;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	size_t n = strlen(cmd);
	if (n == 3 && strncmp(cmd, "pwd", 3) == 0) {
		char buf[BUFLEN] = { 0 };
		if (getcwd(buf, BUFLEN) == NULL)
			exit(-1);
		printf("%s\n", buf);
		return true;
	}
	return false;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here
	if (strncmp(cmd, "history", 7) == 0 && strlen(cmd) >= 9) {
		// char path [BUFLEN];
		// if(getenv("HISTFILE") != NULL){
		// 	strcpy(path,getenv("HISTFILE"));
		// }
		// else{
		// 	strcpy(path,getenv("HOME"));
		// 	strcat(path,"/.fisop_history");
		// }
		// FILE* history = fopen(path,"r");
		// if(history == NULL){
		// 	printf("ERROR \n");
		// 	return 0;
		// }

		// char *end;
		// int cantLineasLeer = strtol(cmd+7, &end, 10);

		// if (cantLineasLeer <= 0)
		// 	return 0;

		// //int cantLineasLeer = cmd[8]-'0';
		// char** buf = malloc(sizeof (char*) * cantLineasLeer);
		// for (int i = 0; i < cantLineasLeer; i++){
		// 	buf[i] = malloc(BUFLEN);
		// }
		// int i = 0;
		// char lineaLeida[BUFLEN];
		// int capacidad = cantLineasLeer;
		// while (fgets(lineaLeida, sizeof(lineaLeida), history) != NULL){
		// 	//Cargar todo en memoria
		// 	if (i == capacidad){
		// 		buf = realloc(buf,sizeof(char*)*(i*2));
		// 		//printf("%d \n",cantLineasLeer);
		// 		for (int k = i; k < i*2; k++){
		// 			buf[k] = malloc(BUFLEN);
		// 		}
		// 		capacidad = i*2;
		// 	}
		// 	strcpy(buf[i],lineaLeida);
		// 	i++;
		// }
		char *end;
		int cantLineasLeer = strtol(cmd + 7, &end, 10);
		int buf_length = 0;
		char **buf = load_command_history(&buf_length);
		if (cantLineasLeer > buf_length)
			cantLineasLeer = buf_length;
		for (int j = buf_length - 1; j >= (buf_length - cantLineasLeer);
		     j--) {
			printf("%s", buf[j]);
		}

		return true;
	}
	return 0;
}


char **
load_command_history(int *buf_length)
{
	char path[BUFLEN];
	if (getenv("HISTFILE") != NULL) {
		strcpy(path, getenv("HISTFILE"));
	} else {
		strcpy(path, getenv("HOME"));
		strcat(path, "/.fisop_history");
	}
	FILE *history = fopen(path, "r");
	if (history == NULL) {
		// printf("ERROR \n");
		return 0;
	}

	int cantLineasLeer = 10;
	char **buf = malloc(sizeof(char *) * cantLineasLeer);
	for (int i = 0; i < cantLineasLeer; i++) {
		buf[i] = malloc(BUFLEN);
	}
	int i = 0;
	char lineaLeida[BUFLEN];
	int capacidad = cantLineasLeer;
	while (fgets(lineaLeida, sizeof(lineaLeida), history) != NULL) {
		// Cargar todo en memoria
		if (i == capacidad) {
			buf = realloc(buf, sizeof(char *) * (i * 2));
			for (int k = i; k < i * 2; k++) {
				buf[k] = malloc(BUFLEN);
			}
			capacidad = i * 2;
		}
		strcpy(buf[i], lineaLeida);
		i++;
	}
	*buf_length = i;
	fclose(history);
	return buf;
}
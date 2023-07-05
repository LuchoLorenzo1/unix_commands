#include "defs.h"
#include "readline.h"
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <assert.h>

#include <sys/ioctl.h>
#include <unistd.h>

#include "builtin.h"

#define CHAR_NL '\n'
#define CHAR_EOF '\004'
#define CHAR_BACK '\b'
#define CHAR_DEL 127
#define CHAR_ESC '\033'

struct termios saved_attributes;

static char buffer[BUFLEN];
void delete_char(void);
void reset_input_mode(void);
void set_input_mode(void);
char *read_noncanonical_mode(void);
char *read_canonical_mode(void);
char *read_line(const char *prompt);

char *
read_line(const char *prompt)
{
#ifndef SHELL_NO_INTERACTIVE
	fprintf(stdout, "%s %s %s\n", COLOR_RED, prompt, COLOR_RESET);
	fprintf(stdout, "%s", "$ ");
	fflush(stdout);
#endif

	if (!isatty(STDIN_FILENO)) {
		return read_canonical_mode();
	}
	set_input_mode();
	return read_noncanonical_mode();
}


void
delete_char()
{
	// genera una secuencia de bytes
	// que indican que se debe borrar un byte
	assert(write(STDOUT_FILENO, "\b \b", 3) > 0);
}
void
reset_input_mode(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}
void
set_input_mode(void)
{
	struct termios tattr;
	/* Make sure stdin is a terminal. */
	if (!isatty(STDIN_FILENO)) {
		fprintf(stderr, "Not a terminal.\n");
		exit(EXIT_FAILURE);
	}
	/* Save the terminal attributes so we can restore them later. */
	tcgetattr(STDIN_FILENO, &saved_attributes);

	/* Set the funny terminal modes. */
	tcgetattr(STDIN_FILENO, &tattr);
	/* Clear ICANON and ECHO. We'll do a manual echo! */
	tattr.c_lflag &= ~(ICANON | ECHO);
	/* Read one char at a time */
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}
// reads a line from the standard input
// and prints the prompt
char *
read_noncanonical_mode()
{
	char c;
	int line_pos = 0;
	memset(buffer, 0, BUFLEN);

	int variableDecremntar = 0;

	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	int numeroColumnas = w.ws_col;


	while (true) {
		assert(read(STDIN_FILENO, &c, 1) > 0);

		if (c == CHAR_NL) {
			// tecla "enter"
			buffer[line_pos] = END_STRING;
			assert(write(STDOUT_FILENO, &c, 1) > 0);
			reset_input_mode();
			return buffer;
		}

		if (c == CHAR_EOF) {
			// teclas "Ctrl-D"
			reset_input_mode();
			return NULL;
		}

		if (c == CHAR_DEL) {
			// tecla "Backspace"
			if (line_pos == 0) {
				// estamos al comienzo de la pantalla
				continue;
			}
			if ((line_pos + 2) % numeroColumnas == 0) {
				char seq[] = "\033[1A";
				assert(write(STDOUT_FILENO, seq, strlen(seq)) > 0);
				char derecha[] = "\033[1C";
				for (int i = 1; i < numeroColumnas; i++) {
					assert(write(STDOUT_FILENO,
					             derecha,
					             strlen(derecha)) > 0);
				}
				assert(write(STDOUT_FILENO,
				             derecha,
				             strlen(derecha)) > 0);
				char del[] = { 127, ' ', 127, '\0' };
				assert(write(STDOUT_FILENO, del, strlen(del)) > 0);
			} else {
				delete_char();
			}
			buffer[line_pos--] = '\0';
		}

		if (c == CHAR_ESC) {
			// comienzo de una sequencia
			// de escape
			char esc_seq;
			assert(read(STDIN_FILENO, &esc_seq, 1) > 0);

			if (esc_seq != '[')
				continue;

			assert(read(STDIN_FILENO, &esc_seq, 1) > 0);
			if (esc_seq == 'A') {
				int buf_length = 0;
				char **buf = load_command_history(&buf_length);
				if (variableDecremntar >= buf_length) {
					continue;
				}
				for (size_t i = 0; i < strlen(buffer); i++) {
					delete_char();
				}
				variableDecremntar++;
				buf[buf_length - variableDecremntar]
				   [strlen(buf[buf_length - variableDecremntar]) - 1] =
				           '\0';

				memset(buffer, 0, BUFLEN);
				strcpy(buffer,
				       buf[buf_length - variableDecremntar]);
				if (write(STDOUT_FILENO, &buffer, strlen(buffer)) <
				    0) {
					printf("Error when writing\n");
				}
				line_pos = strlen(buffer);
			}
			if (esc_seq == 'B') {
				int buf_length = 0;
				char **buf = load_command_history(&buf_length);
				if (variableDecremntar <= 1) {
					for (size_t i = 0; i < strlen(buffer);
					     i++) {
						delete_char();
					}
					strcpy(buffer, "\0");
					continue;
				}
				for (size_t i = 0; i < strlen(buffer); i++) {
					delete_char();
				}
				variableDecremntar--;
				buf[buf_length - variableDecremntar]
				   [strlen(buf[buf_length - variableDecremntar]) - 1] =
				           '\0';

				memset(buffer, 0, BUFLEN);
				strcpy(buffer,
				       buf[buf_length - variableDecremntar]);
				if (write(STDOUT_FILENO, &buffer, strlen(buffer)) <
				    0) {
					printf("Error when writing\n");
				}
				line_pos = strlen(buffer);
			}
			if (esc_seq == 'C') {
				// flecha "derecha"
				// return "flecha derecha\n";
			}
			if (esc_seq == 'D') {
				// flecha "izquierda"
				// return "flecha izquierda\n";
			}
		}

		if (isprint(c)) {  // si es visible
			if (write(STDOUT_FILENO, &c, 1) < 0) {
				printf("Error when writing\n");
			}
			buffer[line_pos++] = c;
		}
	}
}

char *
read_canonical_mode()
{
	int i = 0, c = 0;

	memset(buffer, 0, BUFLEN);

	c = getchar();

	while (c != END_LINE && c != EOF) {
		buffer[i++] = c;
		c = getchar();
	}

	// if the user press ctrl+D
	// just exit normally
	if (c == EOF)
		return NULL;

	buffer[i] = END_STRING;

	return buffer;
}

#ifndef READLINE_H
#define READLINE_H

char *read_line(const char *prompt);

char *read_canonical_mode();

char *read_noncanonical_mode();
#endif  // READLINE_H

#ifndef IO_H
#define IO_H

#include <stddef.h>

struct line {
    char *data;
    size_t len;
    size_t capacity;
};

/* 1 = line, 0 = end of input, -1 = error. Newline is removed. */
int read_line(struct line *line, int accept_crlf);
int write_all(int fd, const char *data, size_t size);
int write_line(int fd, const struct line *line);
/* Include the newline in the same write as the diagnostic text. */
#define fail(text) report_error(text "\n", sizeof(text))
int report_error(const char *message, size_t length);

#endif

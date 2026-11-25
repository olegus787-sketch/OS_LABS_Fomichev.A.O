#include "io.h"
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

int write_all(int fd, const char *data, size_t size)
{
    while (size > 0) {
        ssize_t n = write(fd, data, size);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return -1;
        data += n;
        size -= (size_t)n;
    }
    return 0;
}

int write_line(int fd, const struct line *line)
{
    if (write_all(fd, line->data, line->len) < 0)
        return -1;
    return write_all(fd, "\n", 1);
}

int report_error(const char *message, size_t length)
{
    write_all(STDERR_FILENO, message, length);
    return -1;
}

static int grow(struct line *line)
{
    if (line->capacity > SIZE_MAX / 2)
        return fail("line is too large");
    size_t size = line->capacity ? 2 * line->capacity : 4096;
    void *p;
    if (line->capacity == 0)
        p = mmap(NULL, size, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    else
        p = mremap(line->data, line->capacity, size, MREMAP_MAYMOVE);
    if (p == MAP_FAILED)
        return fail("cannot allocate line buffer");
    line->data = p;
    line->capacity = size;
    return 0;
}

int read_line(struct line *line, int accept_crlf)
{
    static char block[4096];
    static size_t pos, end;
    line->len = 0;
    for (;;) {
        if (pos == end) {
            ssize_t n = read(STDIN_FILENO, block, sizeof(block));
            if (n < 0 && errno == EINTR)
                continue;
            if (n < 0)
                return fail("cannot read stdin");
            if (n == 0 && line->len == 0)
                return 0;
            pos = 0;
            end = (size_t)n;
        }
        if (line->len == line->capacity && grow(line) < 0)
            return -1;
        if (pos == end || block[pos] == '\n') {
            if (pos < end) {
                ++pos;
                if (accept_crlf && line->len > 0 && line->data[line->len - 1] == '\r')
                    --line->len;
            }
            line->data[line->len] = '\0';
            return 1;
        }
        line->data[line->len++] = block[pos++];
    }
}

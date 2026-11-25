#include "io.h"
#include <unistd.h>

/* Reverse the bytes in the half-open interval [begin, end). */
static void reverse(char *data, size_t begin, size_t end)
{
    while (end - begin > 1) {
        --end;
        char tmp = data[begin];
        data[begin++] = data[end];
        data[end] = tmp;
    }
}

static void reverse_utf8(struct line *line)
{
    size_t start = 0;
    while (start < line->len) {
        size_t next = start + 1;
        while (next < line->len &&
               ((unsigned char)line->data[next] & 0xC0) == 0x80)
            ++next;
        reverse(line->data, start, next);
        start = next;
    }
    reverse(line->data, 0, line->len);
}

int main(void)
{
    struct line line = {0};
    int state;
    while ((state = read_line(&line, 0)) == 1) {
        reverse_utf8(&line);
        if (write_line(STDOUT_FILENO, &line) < 0) {
            fail("child: cannot write output");
            return 1;
        }
    }
    if (close(STDOUT_FILENO) < 0) {
        fail("child: cannot close output");
        return 1;
    }
    return state < 0;
}

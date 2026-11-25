#include "io.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

static int open_outputs(int files[2], struct line *line)
{
    struct stat info[2];
    for (int i = 0; i < 2; ++i) {
        if (read_line(line, 1) != 1 || line->len == 0)
            return fail("expected two nonempty output paths");
        files[i] = open(line->data, O_WRONLY | O_CREAT | O_CLOEXEC, 0666);
        if (files[i] < 0 || fstat(files[i], &info[i]) < 0)
            return fail("cannot open or inspect output file");
    }
    if (info[0].st_dev == info[1].st_dev && info[0].st_ino == info[1].st_ino)
        return fail("output files must be different");
    for (int i = 0; i < 2; ++i)
        if (S_ISREG(info[i].st_mode) && ftruncate(files[i], 0) < 0)
            return fail("cannot clear output file");
    return 0;
}

/* Each worker inherits its own standard input and standard output. */
static pid_t start_child(int output, int *writer)
{
    int channel[2];
    if (pipe2(channel, O_CLOEXEC) < 0)
        return fail("cannot create pipe");
    pid_t pid = fork();
    if (pid == 0) {
        if (dup2(channel[0], STDIN_FILENO) < 0 ||
            dup2(output, STDOUT_FILENO) < 0) {
            fail("child: cannot redirect input or output");
            _exit(1);
        }
        char *args[] = {"./child", NULL};
        execve(args[0], args, environ);
        fail("child: cannot execute ./child");
        _exit(1);
    }
    close(channel[0]);
    if (pid < 0) {
        close(channel[1]);
        return fail("cannot fork");
    }
    *writer = channel[1];
    return pid;
}

static int choose_pipe(void)
{
    unsigned char byte;
    for (;;) {
        ssize_t n = getrandom(&byte, 1, 0);
        if (n < 0 && errno == EINTR)
            continue;
        if (n != 1)
            return fail("cannot get a random byte");
        if (byte < 255)
            return byte < 204 ? 0 : 1;
    }
}

static int distribute(int writers[2], struct line *line)
{
    int state;
    while ((state = read_line(line, 1)) == 1) {
        int i = choose_pipe();
        if (i < 0)
            return -1;
        if (write_line(writers[i], line) < 0)
            return fail("parent: cannot write to pipe");
    }
    return state;
}

static int wait_child(pid_t pid)
{
    int status;
    while (waitpid(pid, &status, 0) < 0)
        if (errno != EINTR)
            return fail("cannot wait for child");
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return fail("child finished with an error");
    return 0;
}

int main(void)
{
    struct line line = {0};
    int files[2], writers[2];
    pid_t children[2];
    struct sigaction action = {.sa_handler = SIG_IGN};
    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        fail("cannot configure SIGPIPE");
        return 1;
    }
    if (open_outputs(files, &line) < 0)
        return 1;

    int count = 0;
    while (count < 2) {
        children[count] = start_child(files[count], &writers[count]);
        if (children[count] < 0)
            break;
        ++count;
    }
    close(files[0]);
    close(files[1]);
    int failed = count != 2 || distribute(writers, &line) < 0;
    for (int i = 0; i < count; ++i)
        close(writers[i]);
    for (int i = 0; i < count; ++i)
        if (wait_child(children[i]) < 0)
            failed = 1;
    return failed;
}

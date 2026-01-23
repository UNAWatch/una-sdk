#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {

int _write(int file, char *ptr, int len) {
    (void)file;
    (void)ptr;
    (void)len;
    return -1;
}

int _read(int file, char *ptr, int len) {
    (void)file;
    (void)ptr;
    (void)len;
    return -1;
}

int _close(int file) {
    (void)file;
    return -1;
}

int _lseek(int file, int ptr, int dir) {
    (void)file;
    (void)ptr;
    (void)dir;
    return -1;
}

int _isatty(int file) {
    (void)file;
    return 1;
}

int _fstat(int file, struct stat *st) {
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

}
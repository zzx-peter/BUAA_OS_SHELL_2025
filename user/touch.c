#include <lib.h>

void do_touch(const char *filename) {
    int r = open(filename, O_RDONLY);
    if (r >= 0) {
        close(r);
        return;
    }
    // debugf("touch %s/n", filename);
    int create_result = open(filename, O_CREAT);
    if (create_result == -10) {
        debugf("touch: cannot touch '%s': No such file or directory\n", filename);
    } else if (create_result < 0) {
        debugf("touch: unknown error for %s, code %d\n", filename, create_result);
    } else {
        close(create_result);
    }
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        debugf("touch: missing operand\n");
        return 0;
    }
    for (int i = 1; i < argc; i++) {
        do_touch(argv[i]);
    }
    return 0;
}
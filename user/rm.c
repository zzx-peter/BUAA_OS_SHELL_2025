#include <lib.h>

// 检查目标是否为目录
static int is_dir(const char *path) {
    struct Stat st;
    stat(path, &st);
    // debugf("rm : st_isdir : %d\n", st.st_isdir);
    return st.st_isdir;
}

int main(int argc, char **argv) {
    int opt_r = 0, opt_f = 0;
    int has_target = 0;
    // 解析参数，支持 -r 和 -rf
    for (int i = 1; i < argc; ++i) {
        if (argv[i] && strcmp(argv[i], "-r") == 0) {
            opt_r = 1;
            argv[i] = 0;
        } else if (argv[i] && strcmp(argv[i], "-rf") == 0) {
            opt_r = 1;
            opt_f = 1;
            argv[i] = 0;
        }
    }
    // 处理每个目标
    for (int i = 1; i < argc; ++i) {
        if (!argv[i]) continue;
        has_target = 1;
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            // 文件不存在，-f不报错，否则报错
            if (!opt_f) {
                debugf("rm: cannot remove '%s': No such file or directory\n", argv[i]);
                return 1;
            }
            continue;
        }
        close(fd);
        // 目录处理
        if (is_dir(argv[i])) {
            if (!opt_r) {
                debugf("rm: cannot remove '%s': Is a directory\n", argv[i]);
                return 1;
            }
        }
        // 删除文件或目录
        char full_path[256];
        char cwd[256];
        syscall_getcwd(cwd);
        normalize_path_withcwd((const char*) argv[i], cwd, full_path);
        remove(full_path);
        // remove失败时不输出（实验环境下通常不会失败）
    }
    if (!has_target) {
        debugf("rm: missing operand\n");
        return 1;
    }
    return 0;
}

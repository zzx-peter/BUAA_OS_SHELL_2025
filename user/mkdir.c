#include <lib.h>

// 递归创建目录（mkdir -p）
void mkdir_p(const char *path) {
    char buf[1024];
    int len = 0;
    int mkfd = 0;
    // 逐级创建目录
    for (int i = 0; path[i] != '\0'; i++) {
        buf[len++] = path[i];
        if (path[i] == '/' || path[i] == '\0') {
            buf[len] = '\0';
            if (len > 1) { // 跳过根目录
                int fd = open(buf, O_RDONLY);
                if (fd >= 0) {
                    close(fd);
                } else {
                    mkfd = open(buf, O_MKDIR);
                    if (mkfd < 0) {
                        debugf("mkdir: cannot create directory '%s': No such file or directory\n", buf);
                        return;
                    } else {
                        close(mkfd);
                    }
                }
            }
        }
    }
    mkfd = open(buf, O_MKDIR);
    if (mkfd < 0) {
        debugf("mkdir: cannot create directory '%s': No such file or directory\n", buf);
        return;
    } else {
        close(mkfd);
    }
}

int main(int argc, char **argv) {
    int recursive = 0;
    int has_dir = 0;
    // 检查是否有-p参数
    for (int i = 1; i < argc; ++i) {
        if (argv[i] && strcmp(argv[i], "-p") == 0) {
            recursive = 1;
            argv[i] = 0;
            break;
        }
    }
    // 处理每个参数,其实最多只有两个参数
    for (int i = 1; i < argc; ++i) {
        if (!argv[i]) continue;
        has_dir = 1;
        int fd = open(argv[i], O_RDONLY);
        if (recursive) {
            // -p选项，目录存在直接返回，不存在递归创建
            if (fd >= 0) {
                close(fd);
                continue;
            }
            char full_path[256];
            char cwd[256];
            syscall_getcwd(cwd);
            normalize_path_withcwd((const char*) argv[i], cwd, full_path);
            mkdir_p(full_path);
        } else {
            // 非-p，目录存在报错
            if (fd >= 0) {
                close(fd);
                debugf("mkdir: cannot create directory '%s': File exists\n", argv[i]);
                return 1;
            }
            int mkfd = open(argv[i], O_MKDIR);
            if (mkfd == -10) {
                debugf("mkdir: cannot create directory '%s': No such file or directory\n", argv[i]);
                return 1;
            } else if (mkfd < 0) {
                debugf("mkdir: cannot create directory '%s': error %d\n", argv[i], mkfd);
                return 1;
            } else {
                // debugf("mkdir: create directory successful '%s'.\n", argv[i]);
                close(mkfd);
            }
        }
    }
    if (!has_dir) {
        debugf("mkdir: missing operand\n");
        return 1;
    }
    return 0;
}

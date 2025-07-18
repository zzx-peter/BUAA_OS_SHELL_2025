#include <args.h>
#include <lib.h>
#include <env.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

int hist_count; // 表示指令总数量，上限20
int current_count; // 表示光标当前所在指令索引
char hist_lines[22][1024];
char now_cmd_buf[1025];
char cmd[1024];
int condition_flag;
char buf[1024];
char buf_replaced[1024];
/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes ('\0'), so that the
 *   returned token is a null-terminated string.
 */
void runcmd(char *s);
int _gettoken(char *s, char **p1, char **p2) {
	*p1 = 0;
	*p2 = 0;
	if (s == 0) {
		return 0;
	}

	while (strchr(WHITESPACE, *s)) {
		*s++ = 0;
	}
	if (*s == 0) {
		return 0;
	}
	if (*s == '`') {
		*p1 = s; // 从反引号开始begin
		do {
			s++;
		} while(*s && *s != '`');
		*s = 0;
		s++;
		*p2 = s;
		return 'w';
	}

	if (*s == '>' && *(s + 1) == '>') {
		*p1 = s;
		*s++ = 0;
		*s++ = 0;
		*p2 = s;
		return 'a';
	}

	if (*s == '|' && *(s + 1) == '|') {
		*p1 = s;
		*s++ = 0;
		*s++ = 0;
		*p2 = s;
		return 'O';
	}

	if (*s == '&' && *(s + 1) == '&') {
		*p1 = s;
		*s++ = 0;
		*s++ = 0;
		*p2 = s;
		return 'A';
	}

	if (strchr(SYMBOLS, *s)) {
		int t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		return t;
	}

	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s)) {
		s++;
	}
	*p2 = s;
	return 'w';
}

int gettoken(char *s, char **p1) {
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 128

int parsecmd(char **argv, int *rightpipe) {
	int argc = 0;
	while (1) {
		condition_flag = 0;
		int env_id;
		char *t;
		int fd, r;
		int c = gettoken(0, &t);
		switch (c) {
		case 0:
			return argc;
		case 'w':
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				exit();
			}
			// 处理反引号
			if (t[0] == '`') {
				t[0] = ' ';
				int p[2];
				if(pipe(p) < 0) {
					debugf("failed to create pipe\n");
					exit();
				}
				int env_id = fork();
				if (env_id < 0) {
					debugf("failed to fork in sh.c\n");
					exit();
				} else if (env_id == 0) { // 子进程执行反引号部分
					close(p[0]);
					dup(p[1], 1);
					close(p[1]);
					char tmp[1024] = {0};	
					strcpy(tmp, t);
					runcmd(tmp);
					exit();		
				} else { // 父进程处理argv
					close(p[1]);
					memset(cmd, 0, sizeof(cmd));
					int offset = 0;
					int read_num = 0;
					while((read_num = read(p[0], cmd + offset, sizeof(cmd))) > 0) {
						offset += read_num;
					}
					if (read_num < 0) {
						debugf("error in `\n");
						exit();
					}
					close(p[0]);
					int len = strlen(cmd);
					int j = 0;
					for (j = len - 1; j > 0; j--) {
						if (strchr(WHITESPACE, cmd[j]) && !strchr(WHITESPACE, cmd[j-1])) {
							cmd[j] = '\0';
							len = j;
							break;
						}
					}
					// debugf("the new argv is %s\n", cmd);
					argv[argc++] = cmd;
					break;			
				}
			}
			argv[argc++] = t;
			break;
		case '<':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: < not followed by word\n");
				exit();
			}
			// Open 't' for reading, dup it onto fd 0, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (1/3) */
			fd = open(t, O_RDONLY);
			if (fd < 0) {
				debugf("failed to open '%s'\n", t);
				exit();
			}
			dup(fd, 0);
			close(fd);
			// user_panic("< redirection not implemented");

			break;
		case '>':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit();
			}
			// Open 't' for writing, create it if not exist and trunc it if exist, dup
			// it onto fd 1, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (2/3) */
			fd = open(t, O_WRONLY | O_CREAT | O_TRUNC);
			if (fd < 0) {
				debugf("failed to open '%s'\n", t);
				exit();
			}
			dup(fd, 1);
			close(fd);
			// user_panic("> redirection not implemented");

			break;
		case 'a':
			gettoken(0, &t);
			// 建文件
			if((fd = open(t, O_RDONLY)) < 0) {
			    if ((fd = open(t, O_RDONLY)) < 0) {
        			debugf("the file: %s is not exist and can be built.\n", t);
					exit();
				}
			}
			close(fd);
			// >>
			if ((fd = open(t, O_WRONLY | O_APPEND)) < 0) {
        		debugf("the file: %s can not be written.\n", t);
				exit();				
			}

			if((r = dup(fd, 1)) < 0) {
				debugf("error in dup.\n");
				exit();
			}
			close(fd);

			break;
		case '|':
			/*
			 * First, allocate a pipe.
			 * Then fork, set '*rightpipe' to the returned child envid or zero.
			 * The child runs the right side of the pipe:
			 * - dup the read end of the pipe onto 0
			 * - close the read end of the pipe
			 * - close the write end of the pipe
			 * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of the
			 *   command line.
			 * The parent runs the left side of the pipe:
			 * - dup the write end of the pipe onto 1
			 * - close the write end of the pipe
			 * - close the read end of the pipe
			 * - and 'return argc', to execute the left of the pipeline.
			 */
			int p[2];
			/* Exercise 6.5: Your code here. (3/3) */
			r = pipe(p);
			if (r != 0) {
				debugf("pipe: %d\n", r);
				exit();
			}
			r = fork();
			if (r < 0) {
				debugf("fork: %d\n", r);
				exit();
			}
			*rightpipe = r;
			if (r == 0) {
				dup(p[0], 0);
				close(p[0]);
				close(p[1]);
				return parsecmd(argv, rightpipe);
			} else {
				dup(p[1], 1);
				close(p[1]);
				close(p[0]);
				return argc;
			}
			// user_panic("| not implemented");
			break;
		case 'O':
			env_id = fork();
			if (env_id == 0) { // 子进程
				condition_flag = 1;
				return argc;
			} else {
				u_int tmp;
				int res = ipc_recv(&tmp, 0, 0);
				if (res != 0) {
					return parsecmd(argv, rightpipe);
				} else {
					return 0;
				}
			}
			break;
		case 'A':
			env_id = fork();
			if (env_id == 0) {
				condition_flag = 1;
				return argc;
			} else {
				u_int tmp;
				int res = ipc_recv(&tmp, 0, 0);
				if (res == 0) {
					return parsecmd(argv, rightpipe);
				} else {
					return 0;
				}
			}
			break;
		}
	}

	return argc;
}
void checkcwd() {
	char cwd[256];
	syscall_getcwd(cwd);
	debugf("cwd now is %s\n", cwd);
}

int check_cmdin(char* cmd) {
	if (strcmp(cmd, "cd") == 0 || strcmp(cmd, "pwd") == 0 || strcmp(cmd, "exit") == 0 || strcmp(cmd, "declare") == 0 ||strcmp(cmd, "unset") == 0 || strcmp(cmd, "history") == 0){
		return 1;
	} else {
		return 0;
	}
}
// cd命令实现，返回int，严格按照表格要求
int do_cd(int argc, char **argv) {
	char target[256];
	struct Stat st;

	if (argc > 2) {
		debugf("Too many args for cd command\n");
		return 1;
	}

	if (argc == 1) {
		// cd 无参数，切换到根目录
		syscall_writecwd("/");
		// debugf("cd / success! cwd is /");
		// checkcwd();
		return 0;
	}

	// 规范化路径
	strcpy(target, (const char*) argv[1]);
	char cwd[256];
	syscall_getcwd(cwd);
	char full_path[256];
	normalize_path_withcwd((const char*) target, cwd, full_path);
	// debugf("full_path is %s\n", full_path);
	if (stat(full_path, &st) < 0) {
		debugf("cd: The directory '%s' does not exist\n", target);
		return 1;
	}
	if (!st.st_isdir) {
		debugf("cd: '%s' is not a directory\n", target);
		return 1;
	}
	syscall_writecwd((const char*)full_path);
	// debugf("cd success! cwd is %s\n", full_path);
	// checkcwd();
	return 0;
}


// pwd命令实现，返回int，严格按照表格要求
int do_pwd(int argc, char **argv) {
	if (argc != 1) {
		printf("pwd: expected 0 arguments; got %d\n", argc-1);
		return 2;
	}
	char cwd[256];
	syscall_getcwd(cwd);
	printf("%s\n", cwd);
	return 0;
}

int do_declare(int argc, char **argv) {
	if (argc == 1) {
		// 输出所有环境变量
		char vars[10][34];
		// 临时创建一个指针数组
		char *ptrs[10];
		for (int i = 0; i < 10; i++) {
			ptrs[i] = vars[i];  // 让每个指针指向对应的行
		}
		syscall_showvars(ptrs);
		for (int i = 0; i < 10; i++) {
			if (vars[i][0] != '\0') {
				debugf("%s\n", vars[i]);
			} else {
				continue;
			}
		}
		return 0;
	}
	int opt_x = 0;
	int opt_r = 0;
	for (int i = 1; i < argc; ++i) {
        if (argv[i] && strcmp(argv[i], "-x") == 0) {
            opt_x = 1;
            argv[i] = 0;
        } else if (argv[i] && strcmp(argv[i], "-xr") == 0) {
            opt_x = 1;
            opt_r = 1;
            argv[i] = 0;
        }
    }
	for (int i = 1; i < argc; i++) {
		if (argv[i] == 0) {
			continue;
		}
		//处理NAME=VALUE
		char name[17];
		int name_len = 0;
		char value[17];
		int value_len = 0;
		int len = strlen(argv[i]);
		for (int j = 0; j < len && argv[i][j] != '='; j++) {	
			name[name_len++] = argv[i][j];
		}
		name[name_len] = '\0';
		for (int j = name_len + 1; j < len; j++) {
			value[value_len++] = argv[i][j];
		}
		value[value_len] = '\0';
		syscall_setvar(name, value, opt_x, opt_r);
	}
	return 0;
}

int do_unset(int argc, char **argv) {
	if (argc == 1) {
		debugf("unset: expected 1 argument; got %d\n", argc-1);
		return 1;
	}
	for (int i = 1; i < argc; i++) {
		syscall_unsetvar(argv[i]);
	}
	return 0;
}

void do_history(int argc, char **argv) {
	int fd, n;
	char hist_buf[1025];
	if ((fd = open("/.mosh_history", O_RDONLY)) < 0) {
		debugf("error in history\n");
	}
	while((n = read(fd, hist_buf, 1024)) > 0) {
		hist_buf[n] = '\0';
		printf("%s", hist_buf); 
	}		
	close(fd);
	return;
}
void runcmd(char *s) {
	// debugf("the cmd being runned now is %s\n", s);
	gettoken(s, 0);

	char *argv[MAXARGS];
	int rightpipe = 0;
	int argc = parsecmd(argv, &rightpipe);
	if (argc == 0) {
		return;
	}
	argv[argc] = 0;


	if (argc > 0 && strcmp(argv[0], "pwd") == 0) {
		do_pwd(argc, argv);
		return;
	}
	if (argc > 0 && strcmp(argv[0], "exit") == 0) {
		exit();
	}
	// debugf("here is spwan\n");
	int child = spawn(argv[0], argv);
	close_all();
	if (child >= 0) {
		u_int tmp;
		int res = ipc_recv(&tmp, 0, 0);
		if (condition_flag) {
			ipc_send(syscall_parent_id(0), res, 0, 0);
		}
		wait(child);
	} else {
		debugf("spawn %s: %d\n", argv[0], child);
	}
	if (rightpipe) {
		wait(rightpipe);
	}
	exit();
}

// 刷新行并将光标移动到正确位置
void refresh_line(const char *buf, int len) {
    printf("\r$ ");
	for (int i = 0; i < len; i++) {
        printf("%c", buf[i]);
    }
    printf("\033[K"); // 清除行尾
}

void readline(char *buf, u_int n) {
    int r;
    int len = 0;           // 当前输入长度
    int cursor = 0;        // 光标在buf中的位置
    buf[0] = '\0';
    // 用于保存原始输入行，便于上下键切换时恢复
    char original[1024] = {0};
    current_count = -1; // 表示光标当前所在指令索引
    while (1) {
        char ch = 0;
        if ((r = read(0, &ch, 1)) != 1) {
            if (r < 0) {
                debugf("read error: %d\n", r);
            }
            exit();
        }
        // 处理退格
        if (ch == '\b' || ch == 0x7f) {
            if (cursor > 0) {
                for (int i = cursor - 1; i < len - 1; ++i) {
                    buf[i] = buf[i + 1];
                }
                len--;
                cursor--;
                buf[len] = '\0';
                refresh_line(buf, len);
            }
            continue;
        }
        // 处理回车
        if (ch == '\r' || ch == '\n') {
            buf[len] = '\0';
            return;
        }
        // 处理ESC序列（上下左右键）
        if (ch == 0x1b) { // ESC
            char seq[2];
            if (read(0, &seq[0], 1) == 1 && read(0, &seq[1], 1) == 1) {
                if (seq[0] == '[') {
                    if (seq[1] == 'A') { // 上键
                        if (hist_count > 0) {
							if (current_count == -1) {
                            	strcpy(original, buf);
                            	current_count = hist_count - 1;
                        	} else if (current_count > 0) {
								current_count--;
							}
                            strcpy(buf, hist_lines[current_count]);
                            len = strlen(buf);
                            cursor = len;
                            refresh_line(buf, len);
                        }
                        continue;
                    } else if (seq[1] == 'B') {
						// 下键
						if (current_count != -1) {
							if (current_count < hist_count - 1) {
								current_count++;
								strcpy(buf, hist_lines[current_count]);
							} else {
								current_count = -1;
								strcpy(buf, original);
							}
							len = strlen(buf);
							cursor = len;
							refresh_line(buf, len);
						}
                        continue;
                    } else if (seq[1] == 'C') { // 右键
                        if (cursor < len) {
                            printf("\033[C");
                            cursor++;
                        }
                        continue;
                    } else if (seq[1] == 'D') { // 左键
                        if (cursor > 0) {
                            printf("\033[D");
                            cursor--;
                        }
                        continue;
                    }
                }
            }
            continue;
        }
        // 普通字符插入
        if (len < n - 1) {
            for (int i = len; i > cursor; --i) {
                buf[i] = buf[i - 1];
            }
            buf[cursor] = ch;
            len++;
            cursor++;
            buf[len] = '\0';
            refresh_line(buf, len);
        }
    }
}

int isName(char c) {
    // 检查小写字母a-z
    if (c >= 'a' && c <= 'z') return 1;
	// 检查下划线
    if (c == '_') return 1;
    // 检查数字0-9
    if (c >= '0' && c <= '9') return 1;
    // 检查大写字母A-Z
    if (c >= 'A' && c <= 'Z') return 1;
    // 其他字符都不合法
    return 0;
}

void usage(void) {
	printf("usage: sh [-ix] [script-file]\n");
	exit();
}

void init_history() {
	
	int fd;
	if ((fd = open("/.mosh_history", O_RDONLY | O_CREAT)) < 0) {
		debugf("There is some wrong with mosh_history: %d\n", fd);	
		return;	
	}	
	hist_count = 0;
	// debugf("init\n");
	// 把文件中的内容读到hist_lines中
	while(hist_count < 20) {
		int offset = 0;
		int r = 0;
		// debugf("prepare to read one line\n");
		while((r = readn(fd, hist_lines[hist_count] + offset, 1)) == 1) {
			if (hist_lines[hist_count][offset] == '\n') {
				offset++;
				hist_lines[hist_count][offset] = '\0';
				hist_count++;
				break;
			}
		}
		// debugf("read one line: %s\n", hist_lines[hist_count-1]);
		if (r < 1) {
			break;
		}
	}
	current_count = hist_count - 1;
	close(fd);
}

// 首先更新hist_lines,如果hist_count超过20，则删除最早的指令；之后将数组写到文件中
void store_line(char *buf) {
	int fd;
	
	if ((fd = open("/.mosh_history", O_WRONLY | O_TRUNC | O_CREAT)) < 0) {
		debugf("Failed to open .mosh_history\n");	
		return;	
	}	

	if (hist_count >= 20)  {
		for (int i = 0; i < 19; i++) {
			strcpy(hist_lines[i], hist_lines[i + 1]);
		}
		strcpy(hist_lines[hist_count - 1], buf);
	} else {
		strcpy(hist_lines[hist_count++], buf);
	}
	current_count = hist_count - 1;
	int len = strlen(hist_lines[current_count]);
	hist_lines[current_count][len++] = '\n';
	hist_lines[current_count][len] = '\0';

	for (int i = 0; i < hist_count; ++i) {
		len = strlen(hist_lines[i]);
		write(fd, hist_lines[i], len);
	}

	close(fd);
}


int main(int argc, char **argv) {
	int r;
	int interactive = iscons(0);
	int echocmds = 0;
	
	// Initialize working directory
	// in kernel/env.c/env_alloc()
	
	printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	printf("::                                                         ::\n");
	printf("::                     MOS Shell 2024                      ::\n");
	printf("::                                                         ::\n");
	printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	ARGBEGIN {
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}
	ARGEND

	if (argc > 1) {
		usage();
	}
	if (argc == 1) {
		close(0);
		if ((r = open(argv[0], O_RDONLY)) < 0) {
			user_panic("open %s: %d", argv[0], r);
		}
		user_assert(r == 0);
	}
	// 初始化历史记录
	init_history();
	for (;;) {
		if (interactive) {
			printf("\n$ ");
		}
		readline(buf, sizeof buf);
		// 保存历史记录
		int len = strlen(buf);
		if (len > 0) {
			store_line(buf);
		}
		if (buf[0] == '#') {
			continue;
		}

		for (int i = 0; i < len; ++i) {
			if (buf[i] == '#') {
				buf[i] = '\0';
			}
		}
		// 将buf_raw中<$name>环境变量替换为环境变量值<var>. 
		// syscall_getvar(name，var)通过系统调用得到var
		int i = 0;
		int j = 0;
		for (i = 0; i < len; i++) {
			if (buf[i] == '$') {
				i++;
				char name[17];
				int name_len = 0;
				while (isName(buf[i])) {
					name[name_len++] = buf[i++];
				}
				// 此时buf[i]是一个非变量名字符
				name[name_len] = '\0';
				char value[17];
				syscall_getvar(name, value);
				// debugf("name is %s, value is %s\n");
				for (int k = 0; k < strlen(value); k++) {
					buf_replaced[j++] = value[k];
				}
				if (i >= len) {
					break;
				}
			}
			buf_replaced[j++] = buf[i];
		}
		buf_replaced[j] = '\0';
		
		if (echocmds) {
			printf("# %s\n", buf_replaced);
		}

		// 解析命令，判断是否为cd
		char first[10];
		for (int i = 0; i <= strlen(buf_replaced); i++) {
			if (strchr(WHITESPACE, buf_replaced[i]) || buf_replaced[i] == '\0') {
				first[i] = '\0';
				break;
			}
			first[i] = buf_replaced[i];
		}
		if (check_cmdin(first) == 1) {
			// debugf("%s is not dangerous\n",buf);
			char buf_tmp[1024];
			strcpy(buf_tmp, buf_replaced);
			gettoken(buf_tmp, 0);
			char *argv_local[MAXARGS];
			int rightpipe = 0;
			int argc_local = parsecmd(argv_local, &rightpipe);
			argv_local[argc_local] = 0;
			if (argc_local > 0 && strcmp(argv_local[0], "history") == 0) {
				do_history(argc_local, argv_local);
				continue;
			}
			if (argc_local > 0 && strcmp(argv_local[0], "cd") == 0) {
				do_cd(argc_local, argv_local);
				continue;
			}
			if (argc_local > 0 && strcmp(argv_local[0], "pwd") == 0) {
				do_pwd(argc_local, argv_local);
				continue;
			}
			if (argc_local > 0 && strcmp(argv_local[0], "exit") == 0) {
				int parent_id = syscall_parent_id((u_int) 0);
				ipc_send(parent_id, 0, 0, 0);
				exit();
			}
			if (argc_local > 0 && strcmp(argv_local[0], "declare") == 0) {
				do_declare(argc_local, argv_local);
				continue;
			}
			if (argc_local > 0 && strcmp(argv_local[0], "unset") == 0) {
				do_unset(argc_local, argv_local);
				continue;
			}
		}
		
		if ((r = fork()) < 0) {
			user_panic("fork: %d", r);
		}
		if (r == 0) {
			runcmd(buf_replaced);
			exit();
		} else {
			wait(r);
		}
	}
	return 0;
}

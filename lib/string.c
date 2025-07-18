#include <types.h>

void *memcpy(void *dst, const void *src, size_t n) {
	void *dstaddr = dst;
	void *max = dst + n;

	if (((u_long)src & 3) != ((u_long)dst & 3)) {
		while (dst < max) {
			*(char *)dst++ = *(char *)src++;
		}
		return dstaddr;
	}

	while (((u_long)dst & 3) && dst < max) {
		*(char *)dst++ = *(char *)src++;
	}

	// copy machine words while possible
	while (dst + 4 <= max) {
		*(uint32_t *)dst = *(uint32_t *)src;
		dst += 4;
		src += 4;
	}

	// finish the remaining 0-3 bytes
	while (dst < max) {
		*(char *)dst++ = *(char *)src++;
	}
	return dstaddr;
}

void *memset(void *dst, int c, size_t n) {
	void *dstaddr = dst;
	void *max = dst + n;
	u_char byte = c & 0xff;
	uint32_t word = byte | byte << 8 | byte << 16 | byte << 24;

	while (((u_long)dst & 3) && dst < max) {
		*(u_char *)dst++ = byte;
	}

	// fill machine words while possible
	while (dst + 4 <= max) {
		*(uint32_t *)dst = word;
		dst += 4;
	}

	// finish the remaining 0-3 bytes
	while (dst < max) {
		*(u_char *)dst++ = byte;
	}
	return dstaddr;
}

size_t strlen(const char *s) {
	int n;

	for (n = 0; *s; s++) {
		n++;
	}

	return n;
}

char *strcpy(char *dst, const char *src) {
	char *ret = dst;

	while ((*dst++ = *src++) != 0) {
	}

	return ret;
}

const char *strchr(const char *s, int c) {
	for (; *s; s++) {
		if (*s == c) {
			return s;
		}
	}
	return 0;
}

int strcmp(const char *p, const char *q) {
	while (*p && *p == *q) {
		p++, q++;
	}

	if ((u_int)*p < (u_int)*q) {
		return -1;
	}

	if ((u_int)*p > (u_int)*q) {
		return 1;
	}

	return 0;
}

void normalize_path_withcwd(const char* path, char* cwd, char* full_path){
	//checkcwd();
	char tmp[256];
	char *p, *q;
	// debugf("path is %s\n", path);
	// Handle absolute path
	if (path[0] == '/') {
		strcpy(tmp, path);
	} else {
		// Handle relative path
		// syscall_getcwd(tmp);
		strcpy(tmp, (const char*)cwd);
		int len_tmp = strlen(tmp);
		if (tmp[len_tmp-1] != '/') {
			tmp[len_tmp] = '/';
			tmp[len_tmp+1] = '\0';
			len_tmp = len_tmp + 1;
		}
		for (int i = 0; i < strlen(path); i++) {
			tmp[len_tmp] = path[i];
			len_tmp++;
		}
		tmp[len_tmp] = '\0';
	}
	// tmp是cwd (+/) + path
	// debugf("tmp_full_path is %s\n", tmp);
	// Remove redundant slashes and handle . and ..
	p = tmp;
	q = full_path;
	while (*p) {
		if (*p == '/') {
			//只有第一个/会在这里被识别
			*q++ = '/';
			while (*p == '/') p++;
		} else if (*p == '.' && (*(p+1) == '/' || *(p+1) == '\0')) {
			//处理 . /dir1/./dir2
			if (*(p+1) == '/') { 
				p += 2;
			} else if (*(p+1) == '\0') { 
				p ++;
			} else {
				// user_panic("error char after .");
			}
		} else if (*p == '.' && *(p+1) == '.') {
			//处理 ..
			if (*(p+2) == '/') {
				p += 3;
			} else if (*(p+2) == '\0'){
				p += 2;
			} else {
				// user_panic("error char after ..");
			}
			//q回退
			q --;
			//q回退到了根目录/..
			if (q == full_path) {
				*q++ = '/';
			} else {
				// /dir1/..,此时*q=/
				q--;
				while(*q != '/'){
					q--;
				}
				// / 此时q在/后面写东西
				q++;
			}
		} else {
			// 处理目录名
			while (*p && *p != '/') *q++ = *p++;
		}
	}
	if((q-1) > full_path && *(q-1) == '/') {q--;}
	*q = '\0';
}
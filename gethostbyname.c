#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

struct hostent *gethostbyname(const char *name)
{
    static int first = 1;
    static struct hostent *(*orig_gethostbyname)(const char *);
    static char zephyr_replace[1024] = {0};
    static int zephyr_local_length;
    static char *zephyr_local;
    
    struct hostent *t;

    if (first) {
	void *handle;
	char *error;
	char *buf;

	handle = dlopen("/lib/libc.so.6", RTLD_LAZY);
	if (!handle) {
	    /* We're screwed. */
	    fprintf(stderr, "hacked gethostbyname: couldn't load /lib/libc.so.6\n");
	    fputs(dlerror(), stderr);
	    exit(1);
	}

	dlerror();
	orig_gethostbyname = dlsym(handle, "gethostbyname");
	if ((error = dlerror())) {
	    /* We're screwed. */
	    fprintf(stderr, "hacked gethostbyname: couldn't find original gethostbyname\n");
	    fputs(error, stderr);
	    exit(1);
	}

	buf = getenv("ZEPHYR_SERVER");
	if (buf) {
	    strncpy(zephyr_replace, buf, 1024);
	    zephyr_replace[1023] = '\0';
	}

	buf = getenv("ZEPHYR_LOCAL");
	if (buf) {
	    t = (*orig_gethostbyname)(buf);
	    if (t->h_addr_list[0]) {
		zephyr_local_length = t->h_length;
		zephyr_local = malloc(zephyr_local_length);
		memcpy(zephyr_local, t->h_addr_list[0], zephyr_local_length);
	    }
	}
	first = 0;
    }

    t = (*orig_gethostbyname)(name);
    if (!strcmp(name, zephyr_replace)) {
	if (t->h_length == zephyr_local_length && t->h_addr_list[0])
	    memcpy(t->h_addr_list[0], zephyr_local, zephyr_local_length);
    }
    return t;
}


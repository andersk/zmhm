#include <nss.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>

enum nss_status _nss_zmhm_getservbyname_r(const char *name, const char *proto,
					  struct servent *result,
					  char *buffer, size_t buflen,
					  int *errnop)
{
    char *p = buffer;
    int bufsize;
    int fd;
    struct sockaddr_in sa, ssa;
    char buf[1024];
    int port;

    /* Only allow udp requests */
    if (proto == NULL || strcmp(proto, "udp")) {
	*errnop = ENOENT;
	return NSS_STATUS_NOTFOUND;
    }
    if (strcmp(name, "zephyr-hm") && strcmp(name, "zephyr-clt")) {
	*errnop = ENOENT;
	return NSS_STATUS_NOTFOUND;
    }
    if (getenv("ZMHM_DISABLE")) {
	*errnop = ENOENT;
	return NSS_STATUS_NOTFOUND;
    }
    
    /* Hack: Don't affect zephrd unless ZMHM_FORCE set */
    if (!getenv("ZMHM_FORCE")) {
	int nread;
	char link[64];
	nread = readlink("/proc/self/exe", link, sizeof(link));
	if (nread >= 0 && nread <= 63) {
	    link[nread] = 0;
	    if (!strcmp(link, "/usr/sbin/zephyrd")) {
		*errnop = ENOENT;
		return NSS_STATUS_NOTFOUND;
	    }
	}
    }
    
    bufsize = strlen(name) + 1	/* s_name */
	+ 4 			/* s_aliases */
	+ 4			/* s_proto */
	;
    if (buflen < bufsize) {
	*errnop = ERANGE;
	return NSS_STATUS_TRYAGAIN;
    }
    
    memcpy(p, name, strlen(name)+1);
    result->s_name = p;
    p += strlen(name)+1;
    memset(p, 0, 4);
    result->s_aliases = (char **)p;
    p += 4;
    memcpy(p, proto, strlen(proto)+1);
    result->s_proto = p;
    p += 4;

    fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
	*errnop = errno;
	return NSS_STATUS_TRYAGAIN;
    }
    sa.sin_family = AF_INET;
    sa.sin_port = 0;
    inet_aton("127.0.0.1", &sa.sin_addr);
    if (bind(fd, (const struct sockaddr *)&sa, sizeof(sa)) == -1) {
	*errnop = errno;
	return NSS_STATUS_UNAVAIL;
    }
    sprintf(buf, "ZMHM0.1 query %s %s\n", name,
	    getenv("ZEPHYR_SERVER") ?: ".default.");
    ssa.sin_family = AF_INET;
    ssa.sin_port = htons(2104);
    inet_aton("127.0.0.1", &ssa.sin_addr);
    if (sendto(fd, buf, strlen(buf), MSG_DONTWAIT,
	       (const struct sockaddr *)&ssa, sizeof(ssa)) == -1) {
	*errnop = errno;
	return NSS_STATUS_UNAVAIL;
    }
    shutdown(fd, SHUT_WR);
    if (recv(fd, buf, 1024, 0) == -1) {
	*errnop = errno;
	return NSS_STATUS_UNAVAIL;
    }
    sscanf(buf, "ZMHM0.1 %d", &port);
    result->s_port = htons(port);
    close(fd);

    return NSS_STATUS_SUCCESS;
}

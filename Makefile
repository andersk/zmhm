all: libzmhm-gethostbyname.so.1 libnss_zmhm.so.2

libzmhm-%.so.1: %.o
	gcc -shared -Wl,-soname,libzmhm-$*.so.1 -o $@ $< -lc -ldl
	strip -R .comment $@

libnss_zmhm.so.2: nss_zmhm.o
	gcc -shared -Wl,-soname,$@ -o $@ $< -lc
	strip -R .comment $@

%.o: %.c
	gcc -fPIC -rdynamic -g -c -Wall $<

clean:
	rm libzmhm-gethostbyname.so.1 libnss_zmhm.so.2 *.o

# @Author: lizhiheng <lzh>
# @Date:   2018-09-03T00:03:42+08:00
# @Email:  phoenix_lzh@sina.com
# @Filename: Makefile
# @Last modified by:   lzh
# @Last modified time: 2018-09-03T00:07:28+08:00
# @License: GPLV3
CFLAGS+=-pthread
LDFLAGS+=-pthread

main:
	gcc $(CFLAGS) -o server server.c $(LDFLAGS)
	gcc $(CFLAGS) -o client client.c $(LDFLAGS)

clean:
	rm -rf *.o *.gch server client

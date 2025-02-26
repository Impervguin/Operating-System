CC := gcc
CFLAGS := 

pair.out : socketpair/socketpair.c
	$(CC) -o $@ $(CFLAGS) $^

serverunix.out : socketunix/socketserver.c socketunix/socketcs.h
	$(CC) -o $@ $(CFLAGS) $^

clientunix.out : socketunix/socketclient.c socketunix/socketcs.h
	$(CC) -o $@ $(CFLAGS) $^

serverforkinet.out : socketinet/serverfork.c socketinet/rw.h
	$(CC) -o $@ $(CFLAGS) $^

serverthreadinet.out : socketinet/serverthread.c socketinet/rw.h
	$(CC) -o $@ $(CFLAGS) $^

clientinet.out : socketinet/client.c socketinet/rw.h
	$(CC) -o $@ $(CFLAGS) $^

onewayserver.out : socketunix/onewayserver.c socketunix/oneway.h
	$(CC) -o $@ $(CFLAGS) $^

onewayclient.out : socketunix/onewayclient.c socketunix/oneway.h
	$(CC) -o $@ $(CFLAGS) $^

clean :
	rm *.socket *out
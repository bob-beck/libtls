CFLAGS += -Wall -Werror
LDLIBS += -ltls -lssl -lcrypto

all: client server

client: client.o report_tls.o
	$(CC) $(CFLAGS) -o client client.o report_tls.o $(LDLIBS)

server: server.o report_tls.o
	$(CC) $(CFLAGS) -o server server.o report_tls.o $(LDLIBS)

clean:
	/bin/rm -f client server *.o

CC := g++
CFLAGS := -Wall -Werror -Wextra -std=c++11
DBG := -g

all: server client

server:
	$(CC) $(CFLAGS) server.cpp -o server $(DBG)

client:
	$(CC) $(CFLAGS) client.cpp -o client $(DBG)

run_server:
	@echo "Enter port number:"
	@read port; \
	echo "To quit, press 'q'"; \
	./server $$port

run_client:
	@echo "Enter client name:"
	@read client_name; \
	echo "Enter server port:"; \
	read server_port; \
	echo "Enter connection period:"; \
	read connection_period; \
	echo "To quit, press 'q'"; \
	./client $$client_name $$server_port $$connection_period

clean:
	rm -rf server client log.txt
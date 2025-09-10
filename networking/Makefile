# Makefile for S.H.A.M. Protocol Implementation
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=199309L
LIBS = -lcrypto

# Detect OS for OpenSSL paths
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    # macOS with Homebrew OpenSSL
    OPENSSL_PREFIX := $(shell brew --prefix openssl 2>/dev/null)
    ifneq ($(OPENSSL_PREFIX),)
        CFLAGS += -I$(OPENSSL_PREFIX)/include
        LDFLAGS += -L$(OPENSSL_PREFIX)/lib
    endif
endif

TARGETS = server client
SOURCES = server.c client.c
HEADERS = sham.h

.PHONY: all clean test

all: $(TARGETS)

server: server.c $(HEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o server server.c $(LIBS)

client: client.c $(HEADERS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o client client.c $(LIBS)

clean:
	rm -f $(TARGETS) *.o
	rm -f server_log.txt client_log.txt
	rm -f received_file test_file

# Test targets
test-file: all
	echo "This is a test file for S.H.A.M. protocol!" > test_file
	@echo "Created test_file. Now run:"
	@echo "Terminal 1: ./server 8080"
	@echo "Terminal 2: ./client 127.0.0.1 8080 test_file output_file"

test-chat: all
	@echo "Chat mode test. Run:"
	@echo "Terminal 1: ./server 8080 --chat"
	@echo "Terminal 2: ./client 127.0.0.1 8080 --chat"

test-logging: all
	echo "Test file with logging enabled" > test_file
	@echo "Testing with logging enabled. Run:"
	@echo "Terminal 1: RUDP_LOG=1 ./server 8080"
	@echo "Terminal 2: RUDP_LOG=1 ./client 127.0.0.1 8080 test_file output_file"

# Install OpenSSL dependencies (Linux)
install-deps-linux:
	sudo apt update && sudo apt install -y libssl-dev

# Install OpenSSL dependencies (macOS)
install-deps-macos:
	brew install openssl
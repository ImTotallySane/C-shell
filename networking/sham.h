// sham.h
#ifndef SHAM_H
#define SHAM_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <openssl/evp.h>

#define PACKET_DATA_SIZE 1024
#define RTO_MS 500
#define SENDER_WINDOW_SIZE 10
#define RECEIVER_BUFFER_SIZE 16384

#define FLAG_SYN 0x1
#define FLAG_ACK 0x2
#define FLAG_FIN 0x4

// ############## LLM Generated Code Begins ##############

#pragma pack(push, 1)

struct sham_header {
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t flags;
    uint16_t window_size;
};

#pragma pack(pop)

// ############## LLM Generated Code Ends ##############

#define PACKET_HEADER_SIZE sizeof(struct sham_header)
#define PACKET_SIZE (PACKET_HEADER_SIZE + PACKET_DATA_SIZE)

extern FILE* log_file;
void init_logging(const char* filename);
void sham_log(const char* format, ...);

void calculate_md5(const char* filepath);

#endif
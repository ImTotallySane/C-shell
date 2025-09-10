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
#include <openssl/evp.h> // For modern MD5 API

// --- S.H.A.M. Protocol Constants ---
#define PACKET_DATA_SIZE 1024
#define RTO_MS 500              // Retransmission Timeout in milliseconds
#define SENDER_WINDOW_SIZE 10   // Congestion window size in packets
#define RECEIVER_BUFFER_SIZE 16384 // 16 KB receiver buffer

// Control Flags for sham_header.flags
#define FLAG_SYN 0x1
#define FLAG_ACK 0x2
#define FLAG_FIN 0x4

// --- S.H.A.M. Packet Structure ---
#pragma pack(push, 1) // Ensure struct is packed without padding
struct sham_header {
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t flags;
    uint16_t window_size; // Flow control window
};
#pragma pack(pop)

#define PACKET_HEADER_SIZE sizeof(struct sham_header)
#define PACKET_SIZE (PACKET_HEADER_SIZE + PACKET_DATA_SIZE)

// --- Logging Utility ---
extern FILE* log_file;
void init_logging(const char* filename);
void sham_log(const char* format, ...);

// --- File Utility ---
void calculate_md5(const char* filepath);

#endif // SHAM_H
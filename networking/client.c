// client.c
#define _POSIX_C_SOURCE 199309L
#include "sham.h"
#include <time.h>

// --- Global Variables & Logging ---
FILE* log_file = NULL;

void sham_log(const char* format, ...) {
    if (!log_file) return;
    
    char time_buffer[30];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t curtime = tv.tv_sec;
    strftime(time_buffer, 30, "%Y-%m-%d %H:%M:%S", localtime(&curtime));
    
    fprintf(log_file, "[%s.%06ld] [LOG] ", time_buffer, tv.tv_usec);
    
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    fprintf(log_file, "\n");
    fflush(log_file);
}

void init_logging(const char* filename) {
    if (getenv("RUDP_LOG")) {
        log_file = fopen(filename, "w");
        if (!log_file) perror("fopen log file");
    }
}

// --- Sliding Window Packet Structure ---
typedef struct {
    char packet[PACKET_SIZE];
    size_t len;
    uint32_t seq_num;
    struct timeval time_sent;
    bool acked;
} PacketWindowSlot;

// --- File Transfer Logic ---
void handle_file_transfer(int sockfd, struct sockaddr_in* server_addr, const char* input_file) {
    FILE* file = fopen(input_file, "rb");
    if (!file) {
        perror("fopen input_file");
        return;
    }

    PacketWindowSlot window[SENDER_WINDOW_SIZE];
    memset(window, 0, sizeof(window));

    uint32_t base_seq = 101;
    uint32_t next_seq_num = 101;
    uint16_t receiver_window = UINT16_MAX; // Assume max window until we get an ACK
    bool file_fully_read = false;

    fcntl(sockfd, F_SETFL, O_NONBLOCK); // Set socket to non-blocking

    while (base_seq < next_seq_num || !file_fully_read) {
        
        // --- 1. Send new packets if window allows ---
        while ((next_seq_num - base_seq) < (SENDER_WINDOW_SIZE * PACKET_DATA_SIZE) && 
               (next_seq_num - base_seq) < receiver_window && !file_fully_read) {
            
            int window_index = (next_seq_num - 101) / PACKET_DATA_SIZE % SENDER_WINDOW_SIZE;
            
            char* data_ptr = window[window_index].packet + PACKET_HEADER_SIZE;
            size_t bytes_read = fread(data_ptr, 1, PACKET_DATA_SIZE, file);

            if (bytes_read > 0) {
                struct sham_header* header = (struct sham_header*)window[window_index].packet;
                *header = (struct sham_header){ .seq_num = htonl(next_seq_num), .flags = 0 };
                
                window[window_index].len = PACKET_HEADER_SIZE + bytes_read;
                window[window_index].seq_num = next_seq_num;
                window[window_index].acked = false;
                gettimeofday(&window[window_index].time_sent, NULL);

                sendto(sockfd, window[window_index].packet, window[window_index].len, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
                sham_log("SND DATA SEQ=%u LEN=%zu", next_seq_num, bytes_read);
                next_seq_num += bytes_read;
            } else {
                file_fully_read = true;
                break;
            }
        }
        
        // --- 2. Check for incoming ACKs ---
        char ack_packet[PACKET_SIZE];
        struct sham_header* ack_header = (struct sham_header*)ack_packet;
        while(recvfrom(sockfd, ack_packet, PACKET_SIZE, 0, NULL, NULL) > 0) {
            if (ntohs(ack_header->flags) & FLAG_ACK) {
                uint32_t acked_seq = ntohl(ack_header->ack_num);
                sham_log("RCV ACK=%u", acked_seq);
                if (acked_seq > base_seq) {
                    base_seq = acked_seq;
                }
                receiver_window = ntohs(ack_header->window_size);
                sham_log("FLOW WIN UPDATE=%u", receiver_window);
            }
        }

        // --- 3. Check for timeouts and retransmit ---
        if (base_seq < next_seq_num) {
            struct timeval now;
            gettimeofday(&now, NULL);
            int window_index = (base_seq - 101) / PACKET_DATA_SIZE % SENDER_WINDOW_SIZE;
            
            long time_diff_ms = (now.tv_sec - window[window_index].time_sent.tv_sec) * 1000 + 
                                (now.tv_usec - window[window_index].time_sent.tv_usec) / 1000;
            
            if (time_diff_ms >= RTO_MS) {
                sham_log("TIMEOUT SEQ=%u", window[window_index].seq_num);
                sendto(sockfd, window[window_index].packet, window[window_index].len, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
                sham_log("RETX DATA SEQ=%u LEN=%zu", window[window_index].seq_num, window[window_index].len - PACKET_HEADER_SIZE);
                gettimeofday(&window[window_index].time_sent, NULL); // Reset timer
            }
        }
        struct timespec sleep_time;
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = 1000 * 1000; // 1000 microseconds = 1 millisecond
        nanosleep(&sleep_time, NULL);
    }
    
    // --- 4-Way Handshake (Client side) ---
    struct sham_header fin_header = { .seq_num = htonl(next_seq_num), .flags = htons(FLAG_FIN) };
    sendto(sockfd, &fin_header, PACKET_HEADER_SIZE, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
    sham_log("SND FIN SEQ=%u", next_seq_num);
    
    char packet[PACKET_SIZE];
    struct sham_header* header = (struct sham_header*)packet;
    
    // Set socket back to blocking for handshake
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK);
    
    recvfrom(sockfd, packet, PACKET_SIZE, 0, NULL, NULL);
    sham_log("RCV ACK FOR FIN");
    recvfrom(sockfd, packet, PACKET_SIZE, 0, NULL, NULL);
    sham_log("RCV FIN SEQ=%u", ntohl(header->seq_num));
    
    struct sham_header final_ack = { .ack_num = htonl(ntohl(header->seq_num) + 1), .flags = htons(FLAG_ACK) };
    sendto(sockfd, &final_ack, PACKET_HEADER_SIZE, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
    sham_log("SND ACK=%u", ntohl(header->seq_num) + 1);

    fclose(file);
}

// --- Chat Logic ---
void handle_chat(int sockfd, struct sockaddr_in* server_addr) {
    printf("Chat mode activated. Type '/quit' to exit.\n");
    fd_set read_fds;
    char buffer[PACKET_DATA_SIZE];
    
    while(1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sockfd, &read_fds);
        
        if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }
        
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            // Clear the buffer first
            memset(buffer, 0, sizeof(buffer));
            
            if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                break; // EOF or error
            }
            
            // Remove newline character
            buffer[strcspn(buffer, "\n")] = '\0';
            
            struct sham_header header = { .flags = htons(0) };
            if (strcmp(buffer, "/quit") == 0) {
                header.flags = htons(FLAG_FIN);
            }
            
            // Clear the packet buffer completely
            char packet[PACKET_SIZE];
            memset(packet, 0, sizeof(packet));
            
            // Copy header and message
            memcpy(packet, &header, PACKET_HEADER_SIZE);
            strcpy(packet + PACKET_HEADER_SIZE, buffer);
            
            // Send only the necessary bytes (header + string length + null terminator)
            size_t message_len = strlen(buffer) + 1; // +1 for null terminator
            sendto(sockfd, packet, PACKET_HEADER_SIZE + message_len, 0, 
                   (struct sockaddr*)server_addr, sizeof(*server_addr));

            if(ntohs(header.flags) & FLAG_FIN) break;
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            char packet[PACKET_SIZE];
            memset(packet, 0, sizeof(packet)); // Clear receive buffer
            
            int bytes = recvfrom(sockfd, packet, PACKET_SIZE, 0, NULL, NULL);
            if (bytes > (int)PACKET_HEADER_SIZE) {
                struct sham_header* header = (struct sham_header*)packet;
                if(ntohs(header->flags) & FLAG_FIN) break;
                
                // Ensure null termination
                int max_msg_len = bytes - PACKET_HEADER_SIZE;
                char* message = packet + PACKET_HEADER_SIZE;
                if (max_msg_len > 0) {
                    message[max_msg_len - 1] = '\0'; // Force null termination
                    printf("Friend: %s\n", message);
                }
            }
        }
    }
    
    // Simple FIN handshake for chat mode
    char packet[PACKET_SIZE];
    struct sham_header* header = (struct sham_header*)packet;
    
    // Send ACK for FIN
    struct sham_header fin_ack = { .flags = htons(FLAG_ACK) };
    sendto(sockfd, &fin_ack, PACKET_HEADER_SIZE, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
    
    // Send our FIN
    struct sham_header our_fin = { .flags = htons(FLAG_FIN) };
    sendto(sockfd, &our_fin, PACKET_HEADER_SIZE, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
    
    // Wait for final ACK
    recvfrom(sockfd, packet, PACKET_SIZE, 0, NULL, NULL);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage:\n  %s <ip> <port> <in_file> <out_name> [loss]\n", argv[0]);
        fprintf(stderr, "  %s <ip> <port> --chat [loss]\n", argv[0]);
        exit(1);
    }
    
    init_logging("client_log.txt");
    
    const char* server_ip = argv[1];
    int port = atoi(argv[2]);
    bool is_chat_mode = (argc > 3 && strcmp(argv[3], "--chat") == 0);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    // --- 3-Way Handshake ---
    struct sham_header syn_header = { .seq_num = htonl(100), .flags = htons(FLAG_SYN) };
    sendto(sockfd, &syn_header, PACKET_HEADER_SIZE, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    sham_log("SND SYN SEQ=100");
    
    char packet[PACKET_SIZE];
    struct sham_header* header = (struct sham_header*)packet;
    recvfrom(sockfd, packet, PACKET_SIZE, 0, NULL, NULL);
    if ((ntohs(header->flags) & (FLAG_SYN | FLAG_ACK))) {
        sham_log("RCV SYN-ACK SEQ=%u ACK=%u", ntohl(header->seq_num), ntohl(header->ack_num));
        
        struct sham_header ack_header = {
            .ack_num = htonl(ntohl(header->seq_num) + 1),
            .flags = htons(FLAG_ACK)
        };
        sendto(sockfd, &ack_header, PACKET_HEADER_SIZE, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        sham_log("SND ACK FOR SYN");
        printf("Connection established.\n");
    }

    if (is_chat_mode) {
        handle_chat(sockfd, &server_addr);
    } else {
        if (argc < 5) {
            fprintf(stderr, "File transfer mode requires <input_file> and <output_file_name>\n");
            exit(1);
        }
        handle_file_transfer(sockfd, &server_addr, argv[3]);
    }

    if(log_file) fclose(log_file);
    close(sockfd);
    printf("Connection closed.\n");
    return 0;
}
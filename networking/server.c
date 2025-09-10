// server.c
#include "sham.h"

// --- Global Variables ---
FILE* log_file = NULL;

// --- Logging Function ---
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

// --- MD5 Calculation (Modern EVP API) ---
void calculate_md5(const char* filepath) {
    unsigned char c[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    FILE *inFile = fopen(filepath, "rb");
    if (inFile == NULL) {
        printf("%s can't be opened.\n", filepath);
        return;
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_md5();
    EVP_DigestInit_ex(mdctx, md, NULL);

    int bytes;
    unsigned char data[1024];
    while ((bytes = fread(data, 1, 1024, inFile)) != 0) {
        EVP_DigestUpdate(mdctx, data, bytes);
    }
    
    EVP_DigestFinal_ex(mdctx, c, &md_len);
    EVP_MD_CTX_free(mdctx);
    
    printf("MD5: ");
    for(unsigned int i = 0; i < md_len; i++) {
        printf("%02x", c[i]);
    }
    printf("\n");
    fclose(inFile);
}

// --- File Transfer Logic ---
void handle_file_transfer(int sockfd, struct sockaddr_in* client_addr, socklen_t* addr_len, float loss_rate, const char* output_filename) {
    FILE* output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("fopen output_file");
        return;
    }

    uint32_t expected_seq = 101; // Following the handshake
    char packet[PACKET_SIZE];
    struct sham_header* header = (struct sham_header*)packet;

    // Buffer for out-of-order packets
    char ooo_buffer[SENDER_WINDOW_SIZE][PACKET_DATA_SIZE];
    uint32_t ooo_seqs[SENDER_WINDOW_SIZE] = {0};
    size_t ooo_lens[SENDER_WINDOW_SIZE] = {0};
    
    while (1) {
        int bytes_recvd = recvfrom(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr*)client_addr, addr_len);
        if (bytes_recvd < (int)PACKET_HEADER_SIZE) continue;

        if ((float)rand() / RAND_MAX < loss_rate) {
            sham_log("DROP DATA SEQ=%u", ntohl(header->seq_num));
            continue;
        }
        
        uint32_t received_seq = ntohl(header->seq_num);
        uint16_t received_flags = ntohs(header->flags);
        size_t data_len = bytes_recvd - PACKET_HEADER_SIZE;

        if (received_flags & FLAG_FIN) {
            sham_log("RCV FIN SEQ=%u", received_seq);
            break; // Exit loop to start 4-way handshake
        }
        
        sham_log("RCV DATA SEQ=%u LEN=%zu", received_seq, data_len);
        
        if (received_seq == expected_seq) {
            fwrite(packet + PACKET_HEADER_SIZE, 1, data_len, output_file);
            expected_seq += data_len;

            // Check buffer for contiguous packets
            bool advanced;
            do {
                advanced = false;
                for (int i = 0; i < SENDER_WINDOW_SIZE; i++) {
                    if (ooo_seqs[i] == expected_seq) {
                        fwrite(ooo_buffer[i], 1, ooo_lens[i], output_file);
                        expected_seq += ooo_lens[i];
                        ooo_seqs[i] = 0; // Invalidate buffer slot
                        advanced = true;
                        break; 
                    }
                }
            } while (advanced);

        } else if (received_seq > expected_seq) {
            // Buffer out-of-order packet
            for (int i = 0; i < SENDER_WINDOW_SIZE; i++) {
                if (ooo_seqs[i] == 0) {
                    ooo_seqs[i] = received_seq;
                    ooo_lens[i] = data_len;
                    memcpy(ooo_buffer[i], packet + PACKET_HEADER_SIZE, data_len);
                    break;
                }
            }
        }
        
        // Send cumulative ACK
        struct sham_header ack_header = {
            .ack_num = htonl(expected_seq),
            .flags = htons(FLAG_ACK),
            .window_size = htons(RECEIVER_BUFFER_SIZE)
        };
        sendto(sockfd, &ack_header, PACKET_HEADER_SIZE, 0, (struct sockaddr*)client_addr, *addr_len);
        sham_log("SND ACK=%u WIN=%u", expected_seq, RECEIVER_BUFFER_SIZE);
    }
    
    fclose(output_file);
    calculate_md5(output_filename);

    // --- 4-Way Handshake (Server side) ---
    struct sham_header fin_ack = { .ack_num = htonl(ntohl(header->seq_num) + 1), .flags = htons(FLAG_ACK) };
    sendto(sockfd, &fin_ack, PACKET_HEADER_SIZE, 0, (struct sockaddr*)client_addr, *addr_len);
    sham_log("SND ACK FOR FIN");

    struct sham_header server_fin = { .seq_num = htonl(8500), .flags = htons(FLAG_FIN) };
    sendto(sockfd, &server_fin, PACKET_HEADER_SIZE, 0, (struct sockaddr*)client_addr, *addr_len);
    sham_log("SND FIN SEQ=8500");

    recvfrom(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr*)client_addr, addr_len);
    sham_log("RCV ACK=%u", ntohl(header->ack_num));
}

// --- Chat Logic ---
void handle_chat(int sockfd, struct sockaddr_in* client_addr, socklen_t* addr_len) {
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
        
        // Check for keyboard input
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
                   (struct sockaddr*)client_addr, *addr_len);

            if(ntohs(header.flags) & FLAG_FIN) break;
        }

        // Check for network input
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
    sendto(sockfd, &fin_ack, PACKET_HEADER_SIZE, 0, (struct sockaddr*)client_addr, *addr_len);
    
    // Send our FIN
    struct sham_header our_fin = { .flags = htons(FLAG_FIN) };
    sendto(sockfd, &our_fin, PACKET_HEADER_SIZE, 0, (struct sockaddr*)client_addr, *addr_len);
    
    // Wait for final ACK
    recvfrom(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr*)client_addr, addr_len);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port> [--chat] [loss_rate]\n", argv[0]);
        exit(1);
    }

    init_logging("server_log.txt");
    srand(time(NULL));

    int port = atoi(argv[1]);
    bool is_chat_mode = (argc > 2 && strcmp(argv[2], "--chat") == 0);
    float loss_rate = 0.0;
    const char* output_filename = "received_file"; // Default, could be made an arg

    if (!is_chat_mode && argc > 2) loss_rate = atof(argv[2]);
    if (is_chat_mode && argc > 3) loss_rate = atof(argv[3]);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    printf("Server listening on port %d...\n", port);

    char packet[PACKET_SIZE];
    struct sham_header* header = (struct sham_header*)packet;

    // --- 3-Way Handshake ---
    recvfrom(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);
    if (ntohs(header->flags) & FLAG_SYN) {
        sham_log("RCV SYN SEQ=%u", ntohl(header->seq_num));
        
        struct sham_header syn_ack = {
            .seq_num = htonl(5000),
            .ack_num = htonl(ntohl(header->seq_num) + 1),
            .flags = htons(FLAG_SYN | FLAG_ACK),
            .window_size = htons(RECEIVER_BUFFER_SIZE)
        };
        sendto(sockfd, &syn_ack, PACKET_HEADER_SIZE, 0, (struct sockaddr*)&client_addr, addr_len);
        sham_log("SND SYN-ACK SEQ=5000 ACK=%u", ntohl(header->seq_num) + 1);
    }
    
    recvfrom(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);
    if(ntohs(header->flags) & FLAG_ACK) {
        sham_log("RCV ACK FOR SYN");
        printf("Connection established with %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }

    if (is_chat_mode) {
        handle_chat(sockfd, &client_addr, &addr_len);
    } else {
        handle_file_transfer(sockfd, &client_addr, &addr_len, loss_rate, output_filename);
    }

    if(log_file) fclose(log_file);
    close(sockfd);
    printf("Connection closed.\n");
    return 0;
}
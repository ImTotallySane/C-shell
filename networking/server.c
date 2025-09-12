#include "sham.h"

FILE* log_file = NULL;

// ############## LLM Generated Code Begins ##############

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

// ############## LLM Generated Code Ends ##############

void init_logging(const char* filename) {
    if (getenv("RUDP_LOG")) {
        log_file = fopen(filename, "w");
        if (!log_file) perror("fopen log file");
    }
}

void calculate_md5(const char* filepath) {
    unsigned char c[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    FILE *inFile = fopen(filepath, "rb");
    if (inFile == NULL) {
        printf("MD5 Error: %s can't be opened.\n", filepath);
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

    // ############## LLM Generated Code Begins ##############

    uint32_t expected_seq = 101;
    char packet[PACKET_SIZE];
    struct sham_header* header = (struct sham_header*)packet;

    char ooo_buffer[SENDER_WINDOW_SIZE][PACKET_DATA_SIZE];
    uint32_t ooo_seqs[SENDER_WINDOW_SIZE] = {0};
    size_t ooo_lens[SENDER_WINDOW_SIZE] = {0};
    
    while (1) {
        int bytes_recvd = recvfrom(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr*)client_addr, addr_len);
        if (bytes_recvd < (int)PACKET_HEADER_SIZE) continue;

        uint16_t received_flags_preview = ntohs(((struct sham_header*)packet)->flags);
        
        if (!(received_flags_preview & FLAG_FIN) && (float)rand() / RAND_MAX < loss_rate) {
            sham_log("DROP DATA SEQ=%u", ntohl(header->seq_num));
            continue;
        }
        
        uint32_t received_seq = ntohl(header->seq_num);
        uint16_t received_flags = ntohs(header->flags);
        size_t data_len = bytes_recvd - PACKET_HEADER_SIZE;

        if (received_flags & FLAG_FIN) {
            sham_log("RCV FIN SEQ=%u", received_seq);
            break;
        }
        
        sham_log("RCV DATA SEQ=%u LEN=%zu", received_seq, data_len);
        
        if (received_seq == expected_seq) {
            fwrite(packet + PACKET_HEADER_SIZE, 1, data_len, output_file);
            expected_seq += data_len;

            bool advanced;
            do {
                advanced = false;
                for (int i = 0; i < SENDER_WINDOW_SIZE; i++) {
                    if (ooo_seqs[i] == expected_seq) {
                        fwrite(ooo_buffer[i], 1, ooo_lens[i], output_file);
                        expected_seq += ooo_lens[i];
                        ooo_seqs[i] = 0;
                        advanced = true;
                        break; 
                    }
                }
            } while (advanced);

        } else if (received_seq > expected_seq) {
            bool already_buffered = false;
            for(int i = 0; i < SENDER_WINDOW_SIZE; i++) if(ooo_seqs[i] == received_seq) already_buffered = true;
            
            if(!already_buffered) {
                for (int i = 0; i < SENDER_WINDOW_SIZE; i++) {
                    if (ooo_seqs[i] == 0) {
                        ooo_seqs[i] = received_seq;
                        ooo_lens[i] = data_len;
                        memcpy(ooo_buffer[i], packet + PACKET_HEADER_SIZE, data_len);
                        break;
                    }
                }
            }
        }
        
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

    struct sham_header fin_ack = { .ack_num = htonl(ntohl(header->seq_num) + 1), .flags = htons(FLAG_ACK) };
    sendto(sockfd, &fin_ack, PACKET_HEADER_SIZE, 0, (struct sockaddr*)client_addr, *addr_len);
    sham_log("SND ACK FOR FIN");

    struct sham_header server_fin = { .seq_num = htonl(8500), .flags = htons(FLAG_FIN) };
    sendto(sockfd, &server_fin, PACKET_HEADER_SIZE, 0, (struct sockaddr*)client_addr, *addr_len);
    sham_log("SND FIN SEQ=8500");

    recvfrom(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr*)client_addr, addr_len);
    sham_log("RCV ACK=%u", ntohl(header->ack_num));

    // ############## LLM Generated Code Ends ##############
}

typedef struct {
    char packet[PACKET_SIZE];
    size_t len;
    uint32_t seq_num;
    struct timeval time_sent;
    bool active;
    bool is_fin;
} OutgoingPacket;

void handle_chat(int sockfd, struct sockaddr_in* client_addr, socklen_t* addr_len, float loss_rate) {
    printf("Chat mode activated. Type '/quit' to exit.\n");
    fd_set read_fds;
    char packet[PACKET_SIZE];
    struct sham_header* header = (struct sham_header*)packet;

    uint32_t server_seq = 8500;
    uint32_t client_seq_latest = 0;
    bool connection_active = true;

    // ############## LLM Generated Code Begins ##############

    OutgoingPacket outbox = { .active = false, .is_fin = false };

    while(connection_active) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sockfd, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = RTO_MS * 1000;

        int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
        if (activity < 0) { perror("select"); break; }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char buffer[PACKET_DATA_SIZE];
            memset(buffer, 0, sizeof(buffer));
            
            if (fgets(buffer, sizeof(buffer), stdin) == NULL || outbox.is_fin) continue;
            buffer[strcspn(buffer, "\n")] = '\0';
            
            size_t message_len = strlen(buffer);
            struct sham_header send_header = { .seq_num = htonl(server_seq), .flags = 0 };

            if (strcmp(buffer, "/quit") == 0) {
                send_header.flags = htons(FLAG_FIN);
                outbox.is_fin = true;
            }
            
            outbox.len = PACKET_HEADER_SIZE + message_len;
            memcpy(outbox.packet, &send_header, PACKET_HEADER_SIZE);
            memcpy(outbox.packet + PACKET_HEADER_SIZE, buffer, message_len);
            outbox.seq_num = server_seq;
            outbox.active = true;
            gettimeofday(&outbox.time_sent, NULL);

            sendto(sockfd, outbox.packet, outbox.len, 0, (struct sockaddr*)client_addr, *addr_len);

            if (outbox.is_fin) {
                sham_log("SND FIN SEQ=%u", server_seq);
            } else {
                sham_log("SND DATA SEQ=%u LEN=%zu", server_seq, message_len);
                server_seq += message_len;
            }
        }
    
    // ############## LLM Generated Code Ends ##############

        if (FD_ISSET(sockfd, &read_fds)) {
            int bytes = recvfrom(sockfd, packet, PACKET_SIZE, 0, NULL, NULL);
            if (bytes <= 0) continue;

            if (ntohs(header->flags) & FLAG_ACK) {
                uint32_t ack_num = ntohl(header->ack_num);
                sham_log("RCV ACK=%u", ack_num);
                if (outbox.active && ack_num > outbox.seq_num) {
                    if (outbox.is_fin) {
                        sham_log("RCV ACK FOR FIN");
                        connection_active = false; // Our FIN is acked, we can exit the loop
                    }
                    outbox.active = false;
                }
                continue;
            }

            if ((float)rand() / RAND_MAX < loss_rate) {
                sham_log("DROP DATA SEQ=%u", ntohl(header->seq_num));
                continue;
            }

            client_seq_latest = ntohl(header->seq_num);
            size_t message_len = bytes - PACKET_HEADER_SIZE;

            if (ntohs(header->flags) & FLAG_FIN) {
                sham_log("RCV FIN SEQ=%u", client_seq_latest);
                connection_active = false;
                continue;
            }
            
            sham_log("RCV DATA SEQ=%u LEN=%zu", client_seq_latest, message_len);
            
            char* message = packet + PACKET_HEADER_SIZE;
            message[message_len] = '\0';
            printf("Friend: %s\n", message);

            struct sham_header ack_header = { 
                .ack_num = htonl(client_seq_latest + message_len),
                .flags = htons(FLAG_ACK),
                .window_size = htons(RECEIVER_BUFFER_SIZE)
            };
            sendto(sockfd, &ack_header, PACKET_HEADER_SIZE, 0, (struct sockaddr*)client_addr, *addr_len);
            sham_log("SND ACK=%u WIN=%u", client_seq_latest + message_len, RECEIVER_BUFFER_SIZE);
        }

        if (outbox.active) {
            struct timeval now;
            gettimeofday(&now, NULL);
            long time_diff_ms = (now.tv_sec - outbox.time_sent.tv_sec) * 1000 + (now.tv_usec - outbox.time_sent.tv_usec) / 1000;

            if (time_diff_ms >= RTO_MS) {
                sham_log("TIMEOUT SEQ=%u", outbox.seq_num);
                const char* type = outbox.is_fin ? "FIN" : "DATA";
                sendto(sockfd, outbox.packet, outbox.len, 0, (struct sockaddr*)client_addr, *addr_len);
                sham_log("RETX %s SEQ=%u LEN=%zu", type, outbox.seq_num, outbox.len - PACKET_HEADER_SIZE);
                gettimeofday(&outbox.time_sent, NULL);
            }
        }
    }
    // ############## LLM Generated Code Begins ##############
    // Second half of the 4-way handshake
    if (outbox.is_fin) { // We initiated
        recvfrom(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr*)client_addr, addr_len);
        client_seq_latest = ntohl(header->seq_num);
        sham_log("RCV FIN SEQ=%u", client_seq_latest);
        struct sham_header final_ack = { .ack_num = htonl(client_seq_latest + 1), .flags = htons(FLAG_ACK) };
        sendto(sockfd, &final_ack, PACKET_HEADER_SIZE, 0, (struct sockaddr*)client_addr, *addr_len);
        sham_log("SND ACK=%u", client_seq_latest + 1);
    } else { // They initiated
        struct sham_header fin_ack = { .ack_num = htonl(client_seq_latest + 1), .flags = htons(FLAG_ACK) };
        sendto(sockfd, &fin_ack, PACKET_HEADER_SIZE, 0, (struct sockaddr*)client_addr, *addr_len);
        sham_log("SND ACK FOR FIN");
        struct sham_header our_fin = { .seq_num = htonl(server_seq), .flags = htons(FLAG_FIN) };
        sendto(sockfd, &our_fin, PACKET_HEADER_SIZE, 0, (struct sockaddr*)client_addr, *addr_len);
        sham_log("SND FIN SEQ=%u", server_seq);
        recvfrom(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr*)client_addr, addr_len);
        sham_log("RCV ACK=%u", ntohl(header->ack_num));
    }
}

// ############## LLM Generated Code ends ##############

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
    const char* output_filename = "received_file";

    if (is_chat_mode) {
        if (argc > 3) loss_rate = atof(argv[3]);
    } else {
        if (argc > 2 && strcmp(argv[2], "--chat") != 0) {
            loss_rate = atof(argv[2]);
        }
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

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
        handle_chat(sockfd, &client_addr, &addr_len, loss_rate);
    } else {
        handle_file_transfer(sockfd, &client_addr, &addr_len, loss_rate, output_filename);
    }

    if(log_file) fclose(log_file);
    close(sockfd);
    printf("Connection closed.\n");
    return 0;
}
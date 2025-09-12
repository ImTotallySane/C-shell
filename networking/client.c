#define _POSIX_C_SOURCE 200809L
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

typedef struct {
    char packet[PACKET_SIZE];
    size_t len;
    uint32_t seq_num;
    struct timeval time_sent;
    bool acked;
} PacketWindowSlot;

void handle_file_transfer(int sockfd, struct sockaddr_in* server_addr, const char* input_file, const char* output_name_on_server) {
    FILE* file = fopen(input_file, "rb");
    if (!file) { perror("fopen input_file"); return; }

    // ############## LLM Generated Code Begins ##############

    PacketWindowSlot window[SENDER_WINDOW_SIZE];
    memset(window, 0, sizeof(window));

    uint32_t base_seq = 101;
    uint32_t next_seq_num = 101;
    uint16_t receiver_window = UINT16_MAX;
    bool file_fully_read = false;

    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    while (base_seq < next_seq_num || !file_fully_read) {
        while ((next_seq_num - base_seq) < (SENDER_WINDOW_SIZE * PACKET_DATA_SIZE) && 
               (next_seq_num - base_seq) < receiver_window && !file_fully_read) {
            
            int window_index = ((next_seq_num - 101) / PACKET_DATA_SIZE) % SENDER_WINDOW_SIZE;
            
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

        if (base_seq < next_seq_num) {
            struct timeval now;
            gettimeofday(&now, NULL);
            int window_index = ((base_seq - 101) / PACKET_DATA_SIZE) % SENDER_WINDOW_SIZE;
            
            long time_diff_ms = (now.tv_sec - window[window_index].time_sent.tv_sec) * 1000 + 
                                (now.tv_usec - window[window_index].time_sent.tv_usec) / 1000;
            
            if (time_diff_ms >= RTO_MS) {
                sham_log("TIMEOUT SEQ=%u", window[window_index].seq_num);
                sendto(sockfd, window[window_index].packet, window[window_index].len, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
                sham_log("RETX DATA SEQ=%u LEN=%zu", window[window_index].seq_num, window[window_index].len - PACKET_HEADER_SIZE);
                gettimeofday(&window[window_index].time_sent, NULL);
            }
        }
        struct timespec sleep_time = { .tv_sec = 0, .tv_nsec = 1000000 };
        nanosleep(&sleep_time, NULL);
    }
    
    struct sham_header fin_header = { .seq_num = htonl(next_seq_num), .flags = htons(FLAG_FIN) };
    sendto(sockfd, &fin_header, PACKET_HEADER_SIZE, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
    sham_log("SND FIN SEQ=%u", next_seq_num);
    
    char packet[PACKET_SIZE];
    struct sham_header* header = (struct sham_header*)packet;
    
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags & ~O_NONBLOCK);
    
    recvfrom(sockfd, packet, PACKET_SIZE, 0, NULL, NULL);
    sham_log("RCV ACK FOR FIN");
    recvfrom(sockfd, packet, PACKET_SIZE, 0, NULL, NULL);
    sham_log("RCV FIN SEQ=%u", ntohl(header->seq_num));
    
    struct sham_header final_ack = { .ack_num = htonl(ntohl(header->seq_num) + 1), .flags = htons(FLAG_ACK) };
    sendto(sockfd, &final_ack, PACKET_HEADER_SIZE, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
    sham_log("SND ACK=%u", ntohl(header->seq_num) + 1);

    // ############## LLM Generated Code Begins ##############

    fclose(file);
}

typedef struct {
    char packet[PACKET_SIZE];
    size_t len;
    uint32_t seq_num;
    struct timeval time_sent;
    bool active;
    bool is_fin;
} OutgoingPacket;

void handle_chat(int sockfd, struct sockaddr_in* server_addr, float loss_rate) {
    printf("Chat mode activated. Type '/quit' to exit.\n");
    fd_set read_fds;
    char packet[PACKET_SIZE];
    struct sham_header* header = (struct sham_header*)packet;

    uint32_t client_seq = 101;
    uint32_t server_seq_latest = 0;
    bool connection_active = true;

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
            struct sham_header send_header = { .seq_num = htonl(client_seq), .flags = 0 };

            if (strcmp(buffer, "/quit") == 0) {
                send_header.flags = htons(FLAG_FIN);
                outbox.is_fin = true;
            }
            
            outbox.len = PACKET_HEADER_SIZE + message_len;
            memcpy(outbox.packet, &send_header, PACKET_HEADER_SIZE);
            memcpy(outbox.packet + PACKET_HEADER_SIZE, buffer, message_len);
            outbox.seq_num = client_seq;
            outbox.active = true;
            gettimeofday(&outbox.time_sent, NULL);
            
            sendto(sockfd, outbox.packet, outbox.len, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
            
            if (outbox.is_fin) {
                sham_log("SND FIN SEQ=%u", client_seq);
            } else {
                sham_log("SND DATA SEQ=%u LEN=%zu", client_seq, message_len);
                client_seq += message_len;
            }
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            int bytes = recvfrom(sockfd, packet, PACKET_SIZE, 0, NULL, NULL);
            if (bytes <= 0) continue;

            if (ntohs(header->flags) & FLAG_ACK) {
                uint32_t ack_num = ntohl(header->ack_num);
                sham_log("RCV ACK=%u", ack_num);
                if (outbox.active && ack_num > outbox.seq_num) {
                    if(outbox.is_fin) {
                        sham_log("RCV ACK FOR FIN");
                        connection_active = false;
                    }
                    outbox.active = false;
                }
                continue;
            }

            if ((float)rand() / RAND_MAX < loss_rate) {
                sham_log("DROP DATA SEQ=%u", ntohl(header->seq_num));
                continue;
            }

            server_seq_latest = ntohl(header->seq_num);
            size_t message_len = bytes - PACKET_HEADER_SIZE;

            if (ntohs(header->flags) & FLAG_FIN) {
                sham_log("RCV FIN SEQ=%u", server_seq_latest);
                connection_active = false;
                continue;
            }
            
            sham_log("RCV DATA SEQ=%u LEN=%zu", server_seq_latest, message_len);
            
            char* message = packet + PACKET_HEADER_SIZE;
            message[message_len] = '\0';
            printf("Friend: %s\n", message);
            
            struct sham_header ack_header = { 
                .ack_num = htonl(server_seq_latest + message_len),
                .flags = htons(FLAG_ACK),
                .window_size = htons(RECEIVER_BUFFER_SIZE)
            };
            sendto(sockfd, &ack_header, PACKET_HEADER_SIZE, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
            sham_log("SND ACK=%u WIN=%u", server_seq_latest + message_len, RECEIVER_BUFFER_SIZE);
        }

        if (outbox.active) {
            struct timeval now;
            gettimeofday(&now, NULL);
            long time_diff_ms = (now.tv_sec - outbox.time_sent.tv_sec) * 1000 + (now.tv_usec - outbox.time_sent.tv_usec) / 1000;

            if (time_diff_ms >= RTO_MS) {
                sham_log("TIMEOUT SEQ=%u", outbox.seq_num);
                const char* type = outbox.is_fin ? "FIN" : "DATA";
                sendto(sockfd, outbox.packet, outbox.len, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
                sham_log("RETX %s SEQ=%u LEN=%zu", type, outbox.seq_num, outbox.len - PACKET_HEADER_SIZE);
                gettimeofday(&outbox.time_sent, NULL);
            }
        }
    }
    
    if (outbox.is_fin) { // We initiated
        recvfrom(sockfd, packet, PACKET_SIZE, 0, NULL, NULL);
        server_seq_latest = ntohl(header->seq_num);
        sham_log("RCV FIN SEQ=%u", server_seq_latest);
        struct sham_header final_ack = { .ack_num = htonl(server_seq_latest + 1), .flags = htons(FLAG_ACK) };
        sendto(sockfd, &final_ack, PACKET_HEADER_SIZE, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
        sham_log("SND ACK=%u", server_seq_latest + 1);
    } else { // They initiated
        struct sham_header fin_ack = { .ack_num = htonl(server_seq_latest + 1), .flags = htons(FLAG_ACK) };
        sendto(sockfd, &fin_ack, PACKET_HEADER_SIZE, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
        sham_log("SND ACK FOR FIN");
        struct sham_header our_fin = { .seq_num = htonl(client_seq), .flags = htons(FLAG_FIN) };
        sendto(sockfd, &our_fin, PACKET_HEADER_SIZE, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
        sham_log("SND FIN SEQ=%u", client_seq);
        recvfrom(sockfd, packet, PACKET_SIZE, 0, NULL, NULL);
        sham_log("RCV ACK=%u", ntohl(header->ack_num));
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage:\n  %s <ip> <port> <in_file> <out_name> [loss_rate]\n", argv[0]);
        fprintf(stderr, "  %s <ip> <port> --chat [loss_rate]\n", argv[0]);
        exit(1);
    }
    
    init_logging("client_log.txt");
    srand(time(NULL));

    const char* server_ip = argv[1];
    int port = atoi(argv[2]);
    bool is_chat_mode = (strcmp(argv[3], "--chat") == 0);
    float loss_rate = 0.0;
    
    if (is_chat_mode) {
        if (argc > 4) loss_rate = atof(argv[4]);
    } else {
        if (argc < 5) {
            fprintf(stderr, "File transfer mode requires <input_file> and <output_file_name>\n");
            exit(1);
        }
        if (argc > 5) loss_rate = atof(argv[5]);
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

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
        handle_chat(sockfd, &server_addr, loss_rate);
    } else {
        handle_file_transfer(sockfd, &server_addr, argv[3], argv[4]);
    }

    if(log_file) fclose(log_file);
    close(sockfd);
    printf("Connection closed.\n");
    return 0;
}
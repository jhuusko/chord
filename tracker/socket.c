#include "socket.h"

int create_socket(int local, int type) {
    struct sockaddr_in in_addr;

    if (type < 1 || type > 2) {
        fprintf(stderr, "Invalid parameter: type, in create_socket.\n");
        return -1;
    }
    int insocket = socket(AF_INET, type, 0);
    if(insocket < 0) {
        perror("failed to create incoming socket");
        return -1;
    } else {
        in_addr.sin_family = AF_INET;
        in_addr.sin_addr.s_addr = INADDR_ANY;
        in_addr.sin_port = htons(local);
        in_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if(bind(insocket, (struct sockaddr*)&in_addr, sizeof(in_addr))) {
        perror("Failed to bind socket"); 
        return -1;
    }
    
    return insocket; 
}

void send_all(int socket, struct sockaddr_in* to, uint8_t* msg, uint32_t msg_len) {
    uint32_t sent = 0;
    do {
        sent += sendto(socket, msg + sent, msg_len - sent, 0, (struct sockaddr*)to, sizeof(*to));
    } while(sent != msg_len);
}

struct message* read_message(int socket) {
    struct message* msg = calloc(sizeof(struct message), 1);
    msg->data = calloc(1, 256);
    msg->remote_length = sizeof(msg->remote);
    msg->read_bytes = recvfrom(socket, msg->data, 256, MSG_WAITALL,
                              (struct sockaddr*)&(msg->remote), &(msg->remote_length));
    msg->type = *(uint8_t*)msg->data; 
    
    return msg;
}

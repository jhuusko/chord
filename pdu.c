#include "pdu.h"

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

    int option = 1;
    setsockopt(insocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    if(bind(insocket, (struct sockaddr*)&in_addr, sizeof(in_addr))) {
        perror("Failed to bind socket");
        return -1;
    }

    return insocket;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <math.h>
#include <fcntl.h>
#include <poll.h>

struct message {
    uint8_t type;
    uint8_t* data;
    ssize_t read_bytes;
    struct sockaddr_in remote;
    socklen_t remote_length;
};

/*
 * Starts the node
 * @param local the port to listen for incoming traffic on
 * @param type 1 = TCP, 2 = UDP
 * @param remote the port to send traffic to
 * @param host the hostname to connect to
 * @param type the type of the node, can be either SOCK_DGRAM or SOCK_STREAM
 * @return returns created file descriptor, -1 on failure
 */
int create_socket(int local, int type);

void send_all(int socket, struct sockaddr_in* to, uint8_t* msg, uint32_t msg_len);

struct message* read_message(int socket);

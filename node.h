#include <stdio.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include "pdu.h"
#include "hashtable/hash.h"
#include "hashtable/hash_table.h"

#define TRACKER_PORT 0
#define PDU_PORT 1
#define SUCC_PORT 2
#define PRED_PORT 3
#define NEW_CONN_PORT 4

#define BUFF_SIZE 1024
#define SEND_ALIVE_FREQUENCY 5

struct hash_table* table = NULL;
uint16_t ports[5];

int alive = 1;
uint8_t range_start = 0;
uint8_t range_end = 0;

char my_address[ADDRESS_LENGTH] = {'\0'};
char succ_address[ADDRESS_LENGTH] = {'\0'};
char trac_address[ADDRESS_LENGTH] = {'\0'};
// NOT stored in network-byte-order
uint16_t succ_port = 0;
uint16_t trac_port = 0;

/**
 * Main function, handles initiation-processes and incoming messages.
 * @argc - number of arguments.
 * @argv - [TRACKER_ADDRESS] [TRACKER_PORT]
 * @return EXIT_SUCCESS if everything went smooth, else EXIT_FAILURE
 */
int main(int argc, char** argv);

/**
 * Initiates socket.
 * @param fd - file-descriptor for binding socket to.
 * @param port - port that socket should be opened at.
 * @param type - socket-type: [ SOCK_DGRAM | SOCK_STREAM ] - PDU/TCP
 */
void init_socket(struct pollfd* fd, int port, int type);

/**
 * Initiates the node to the network by asking tracker if there are any other nodes in the network,
 * if there are any - node asks to get invited and does'nt leave until it is.
 * @param fds - array of file-descriptors.
 * @return 0 if node is alone in the network, otherwise the port of successor-node
 */
uint16_t init_node(struct pollfd* fds);

/**
 * Sends alive-messages to tracker to let it know node is still active.
 * @param fds - file-descriptor of the tracker.
 */
void send_alive(struct pollfd* fds);

/**
 * Interprets what message was received and distributes tasks accordingly.
 * @param fds - array of file-descriptors.
 * @param message_size - size of message in bytes.
 * @param message - array of bytes to be processed.
 * @return number of bytes NOT processed.
 */
uint16_t handle_request(struct pollfd* fds, uint16_t message_size, char* message);

/**
 * Handles all NET_JOIN_PDU.
 * @param fds - array of file-descriptors.
 * @param message_size - size of message in bytes.
 * @param message - array of bytes to be processed.
 * @return number of bytes processed.
 */
uint16_t join_network(struct pollfd *fds, uint16_t message_size, char *message);

/**
 * Handles all NET_JOIN_RESPONSE_PDU.
 * @param fds - array of file-descriptors.
 * @param message_size - size of message in bytes.
 * @param message - array of bytes to be processed.
 * @return number of bytes processed.
 */
uint16_t join_response(struct pollfd* fds, uint16_t message_size, char* message);

/**
 * Handles all NET_LEAVING_PDU.
 * @param fds - array of file-descriptors.
 * @param message_size - size of message in bytes.
 * @param message - array of bytes to be processed.
 * @return number of bytes processed.
 */
uint16_t leaving_network(struct pollfd* fds, uint16_t message_size, char* message);

/**
 * Sends a NET_LEAVING_PDU.
 * @param fds - array of file-descriptors.
 * @param message_size - size of message in bytes.
 * @param message - array of bytes to be processed.
 */
void send_leaving_network(struct pollfd* fds);

/**
 * Handles all VAL_INSERT_PDU.
 * @param fds - array of file-descriptors.
 * @param message_size - size of message in bytes.
 * @param message - array of bytes to be processed.
 * @return number of bytes processed.
 */
uint16_t val_insert(struct pollfd* fds, uint16_t message_size, char* message);

/**
 * Handles all VAL_LOOKUP_PDU.
 * @param fds - array of file-descriptors.
 * @param message_size - size of message in bytes.
 * @param message - array of bytes to be processed.
 * @return number of bytes processed.
 */
uint16_t val_lookup(struct pollfd* fds, uint16_t message_size, char* message);

/**
 * Handles all VAL_REMOVE_PDU.
 * @param fds - array of file-descriptors.
 * @param message_size - size of message in bytes.
 * @param message - array of bytes to be processed.
 * @return number of bytes processed.
 */
uint16_t val_remove(struct pollfd* fds, uint16_t message_size, char* message);

/**
 * Determines the new hash-range of this and prospect node.
 * @param successor_range_start - pointer to variable where low-end of range should be stored.
 * @param successor_range_end - pointer to variable where upper-end of range should be stored.
 */
void new_range(uint8_t* successor_range_start, uint8_t* successor_range_end);

/**
 * Handles interrupt-signal so the node closes correctly.
 * @param signal - the signal received.
 */
static void sigHandler(int signal);

/**
 * Used for generating port-numbers. Generates a random number between 3k-53k and
 * makes sure it's not taken by another port (by this node).
 * @return a random number.
 */
int random_port();

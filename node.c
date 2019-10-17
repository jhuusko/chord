#include "node.h"

int main(int argc, char** argv) {

    if (argc < 3) {
        fprintf(stderr, "Expected 2 arguments. [tracker_address] [tracker_port]\n");
        exit(EXIT_FAILURE);
    }
    if ((memcpy(trac_address, argv[1], ADDRESS_LENGTH)) == NULL) {
        fprintf(stderr, "Failed to parse tracker address.\n");
        exit(EXIT_FAILURE);
    }
    if ((trac_port = (uint16_t)strtol(argv[2], NULL, 10)) == 0) {
        fprintf(stderr, "Failed to convert tracker_port.\n");
        exit(EXIT_FAILURE);
    }

    // Init signal handler
    struct sigaction sig = {0};
    sig.sa_handler = sigHandler;
    sigaction(SIGINT, &sig, NULL);
    srand(time(NULL));

    struct pollfd fds[5] = {'\0'};

    // Init node to network
    if (init_node(fds) == 0) {

        if ((table = table_create(hash_ssn, 256)) == NULL) {
            fprintf(stderr, "Failed to create table.\nNow exiting.\n");
            for (int i = 0; i < 5; i++) {
                close(fds[i].fd);
            }
            exit(EXIT_FAILURE);
        }
        range_start = 0;
        range_end = 255;
    }
    printf("Im listening to PDUs on port: %d\n", ports[TRACKER_PORT]);

    // Init timer for keeping track of alive messages
    __time_t last_update = 0;
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    srandom((unsigned int)time.tv_sec);

    while (alive) {
        if (poll(fds, 5, 5000) < 0) {
            perror("Poll failed.\n");
            break;
        }

        // sending alive message to tracker
        clock_gettime(CLOCK_REALTIME, &time);
        if (time.tv_sec - last_update >= SEND_ALIVE_FREQUENCY) {
            send_alive(fds);
            last_update = time.tv_sec;
        }

        for (int i = 0; alive && i < 5 ; i++) {
             if (i == 4 && fds[i].revents & POLLIN) {
                // Waiting on predecessor to send connection-request,
                // but not if you are the predecessor
                printf("Waiting for accept\n");
                if ((range_end - range_start) != 255) {
                    fds[PRED_PORT].fd = accept(fds[NEW_CONN_PORT].fd, NULL, NULL);
                    fds[PRED_PORT].events = POLLIN;
                    printf("Connected with predecessor.\n");
                }
             } else if (fds[i].revents & POLLIN) {

                char buffer[BUFF_SIZE];
                char reserve_buffer[BUFF_SIZE];

                size_t leftover = 0;
                do {
                    memcpy(buffer, reserve_buffer, leftover);
                    memset(reserve_buffer, '\0', BUFF_SIZE);
                    uint16_t message_size = (uint16_t)read(fds[i].fd, buffer+leftover, ((BUFF_SIZE-leftover)));
                    size_t old_leftover = leftover;
                    leftover = handle_request(fds, (uint16_t)(message_size+leftover), buffer);
                    memcpy(reserve_buffer, (buffer+message_size+old_leftover)-leftover, leftover);
                    memset(buffer, '\0', BUFF_SIZE);
                } while (leftover);

            }
        }
    }

    printf("Leaving network!\n");
    if (range_end - range_start != 255) {
      send_leaving_network(fds);
    }

    printf("freeing table.\n");
    if (table != NULL) {
        table_free(table);
    }

    printf("closing sockets.\n");
    for (int i = 0; i < 5; i++) {
        shutdown(fds[i].fd, SHUT_RDWR);
        close(fds[i].fd);
    }

    printf("Bye!!");
    return EXIT_SUCCESS;
}

/**********************************************************************************************************************/

uint16_t handle_request(struct pollfd* fds, uint16_t message_size, char message[message_size]) {

    int processed = 0;

    do {
        int8_t type = *message;

        switch (type) {
            case NET_JOIN:

                printf("I received a NET_JOIN_PDU!\n");
                processed = join_network(fds, message_size, message);
                break;

            case NET_JOIN_RESPONSE:

                printf("I received a NET_JOIN_RESPONSE_PDU!\n");
                processed = join_response(fds, message_size, message);
                break;

            case NET_CLOSE_CONNECTION:

                printf("I received a NET_CLOSE_CONNECTION_PDU!\n");

                shutdown(fds[PRED_PORT].fd, SHUT_RDWR);
                close(fds[PRED_PORT].fd);

                processed = sizeof(struct NET_CLOSE_CONNECTION_PDU);
                break;

            case NET_NEW_RANGE:

                printf("I received a NET_NEW_RANGE_PDU!\n");
                struct NET_NEW_RANGE_PDU new_range_pdu;
                if (message_size < sizeof(new_range_pdu)) {
                    printf("Message only contains part of pdu (%d bytes).\n", message_size);
                    return message_size;
                }

                memcpy(&new_range_pdu, message, sizeof(new_range_pdu));
                range_end = new_range_pdu.new_range_end;
                printf("My new range is: %d-%d\n", range_start, range_end);

                processed = sizeof(new_range_pdu);
                break;

            case NET_LEAVING:

                printf("I received a NET_LEAVING_PDU!\n");
                processed = leaving_network(fds, message_size, message);
                break;

            case VAL_INSERT:

                printf("I received a VAL_INSERT_PDU!\n");
                processed = val_insert(fds, message_size, message);
                break;

            case VAL_LOOKUP:

                printf("I received a VAL_LOOKUP_PDU!\n");
                processed = val_lookup(fds, message_size, message);
                break;

            case VAL_REMOVE:

                printf("I received a VAL_REMOVE_PDU!\n");
                processed = val_remove(fds, message_size, message);
                break;

            default:
                fprintf(stderr, "I received a non-valid PDU.\n");
                return 0;
        }

        message+=processed;
        message_size-=processed;

    } while (processed && message_size);

    return message_size;
}

/**********************************************************************************************************************/

uint16_t join_network(struct pollfd *fds, uint16_t message_size, char message[message_size]) {

    struct NET_JOIN_PDU net_join;

    if (message_size < sizeof(net_join)) {
        printf("Message only contains part of pdu (%d bytes).\n", message_size);
        return 0;
    }

    memcpy(&net_join, message, sizeof(net_join));
    uint8_t alone_in_network = (uint8_t)((range_end - range_start) == 255);

    // If I'm not alone in network
    if (!alone_in_network) {
        printf("I am not alone in the network.\n");
        // If my span is grater than max_span; update the join_pdu and forward message
        // TODO what to do if someone has the same span as me?
        if (net_join.max_span < (range_end - range_start)) {
            printf("I have the greatest span (%d, former: %d). Inserting my info in pdu.\n", (range_end-range_start), net_join.max_span);
            net_join.max_span = range_end - range_start;
            memcpy(net_join.max_address, my_address, ADDRESS_LENGTH);
            net_join.max_port = htons(ports[NEW_CONN_PORT]);
            if (send(fds[SUCC_PORT].fd, &net_join, sizeof(net_join), 0) < 0) {
                perror("Failed sending NET_JOIN to successor.\n");
            }
            return sizeof(net_join);
        }

        // If my span is smaller than max_span; then just forward to successor
        if (net_join.max_span > (range_end - range_start)) {
            printf("My span is smaller (%d, max: %d), sending to successor.\n", (range_end-range_start), net_join.max_span);
            if (send(fds[SUCC_PORT].fd, &net_join, sizeof(net_join), 0) < 0) {
                perror("Failed sending NET_JOIN to successor.\n");
            }
            return sizeof(net_join);
        }
        // If someone else has the same span as me; then also forward to successor
        if (!(strncmp(net_join.max_address, my_address, ADDRESS_LENGTH) == 0 && htons(net_join.max_port) == ports[NEW_CONN_PORT])) {
            printf("Someone else has the same span as me, just sending it to successor.\n");
            if (send(fds[SUCC_PORT].fd, &net_join, sizeof(net_join), 0) < 0) {
                perror("Failed sending NET_JOIN to successor.\n");
            }
            return sizeof(net_join);
        }

        printf("I received my own information.\nSending NET_CLOSE_CONNECTION_PDU to successor.\n");
        // If i have the max_span, send close connection to old successor
        struct NET_CLOSE_CONNECTION_PDU net_close_pdu;
        net_close_pdu.type = NET_CLOSE_CONNECTION;
        if (send(fds[SUCC_PORT].fd, &net_close_pdu, sizeof(net_close_pdu), 0) < 0) {
            perror("Failed to send NET_CLOSE_CONNECTION_PDU.\n");
            exit(EXIT_FAILURE);
        }

        // closing my end to successor
        close(fds[SUCC_PORT].fd);
        init_socket(&fds[SUCC_PORT], (ports[SUCC_PORT] = random_port()), SOCK_STREAM);
    }


    // Request prospect node to be my new successor
    struct sockaddr_in join_addr;
    join_addr.sin_family = AF_INET;
    inet_pton(AF_INET, net_join.src_address, &(join_addr.sin_addr));
    join_addr.sin_port = net_join.src_port;

    if (connect(fds[SUCC_PORT].fd, (struct sockaddr*)&join_addr, sizeof(join_addr)) < 0) {
        perror("Failed connecting to successor.");
        exit(EXIT_FAILURE);
    }
    printf("Connected with successor.\n");

    // Sending join-response to prospect-node
    struct NET_JOIN_RESPONSE_PDU net_join_resp;
    net_join_resp.type = NET_JOIN_RESPONSE;
    memcpy(net_join_resp.next_address, alone_in_network ? my_address : succ_address, ADDRESS_LENGTH);
    net_join_resp.next_port = alone_in_network ? htons(ports[NEW_CONN_PORT]) : htons(succ_port);
    new_range(&net_join_resp.range_start, &net_join_resp.range_end);
    printf("My range is: %d-%d\n", range_start, range_end);

    if (send(fds[SUCC_PORT].fd, &net_join_resp, sizeof(net_join_resp), 0) < 0) {
        perror("Failed to send NET_JOIN_RESPONSE_PDU.\n");
        exit(EXIT_FAILURE);
    }

    // Store my successors port and address
    memcpy(succ_address, net_join.src_address, ADDRESS_LENGTH);
    succ_port = htons(net_join.src_port);

    uint32_t inserts_sent = 0;
    //Transfer upper half of hash range to successor
    for (int i = net_join_resp.range_start; i <= net_join_resp.range_end; i++) {
        struct table_entry* entry = table->entries[i];

        while (entry) {
            uint8_t response[24+510] = {'\0'};

            uint8_t name_len = strlen(entry->name)+1;
            uint8_t email_len = strlen(entry->email)+1;

            response[0] = VAL_INSERT;
            memcpy(response+1, entry->ssn, 13);
            response[14] = name_len;
            memcpy(response+16, entry->name, name_len);
            response[16+name_len] = email_len;
            memcpy(response+24+name_len, entry->email, email_len);

            if (send(fds[SUCC_PORT].fd, response, 24 + name_len + email_len , 0) < 0) {
                perror("Failed to send VAL_INSERT_PDU.\n");
                exit(EXIT_FAILURE);
            }
            table_remove(table, entry->ssn);
            inserts_sent++;
            entry = entry->next;
        }
    }
    printf("I sent %d entries to port: %d Address: %s \n", inserts_sent, succ_port, succ_address);
    return sizeof(net_join);
}

/**********************************************************************************************************************/

uint16_t join_response(struct pollfd* fds, uint16_t message_size, char message[message_size]) {

    struct NET_JOIN_RESPONSE_PDU pdu;

    if (message_size < sizeof(pdu)) {
        printf("Message only contains part of pdu (%d bytes).\n", message_size);
        return 0;
    }

    memcpy(&pdu, message, sizeof(pdu));

    if ((table = table_create(hash_ssn, 256)) < 0) {
        perror("Failed to create table.\nNow exiting.\n");
        exit(EXIT_FAILURE);
    }
    range_start = pdu.range_start;
    range_end = pdu.range_end;
    printf("My range is: %d-%d\n", range_start, range_end);

    // Get my successors port and address
    memcpy(succ_address, pdu.next_address, ADDRESS_LENGTH);
    succ_port = htons(pdu.next_port);

    // connect to predecessors old successor
    struct sockaddr_in pdu_addr;
    pdu_addr.sin_family = AF_INET;
    inet_pton(AF_INET, succ_address, &(pdu_addr.sin_addr));
    pdu_addr.sin_port = htons(succ_port);

    // Waiting on successor to accept my connection
    if (connect(fds[SUCC_PORT].fd, (struct sockaddr*)&pdu_addr, sizeof(pdu_addr)) < 0) {
        perror("Failed connecting to successor.\n");
        exit(EXIT_FAILURE);
    }
    printf("Connected with successor.\n");

    return sizeof(pdu);
}

/**********************************************************************************************************************/

uint16_t leaving_network(struct pollfd* fds, uint16_t message_size, char message[message_size]) {

    struct NET_LEAVING_PDU leave_pdu;

    if (message_size < sizeof(leave_pdu)) {
        printf("Message only contains part of pdu (%d bytes).\n", message_size);
        return 0;
    }

    memcpy(&leave_pdu, message, sizeof(leave_pdu));

    shutdown(fds[SUCC_PORT].fd, SHUT_RDWR);
    close(fds[SUCC_PORT].fd);
    ports[SUCC_PORT] = random_port();
    init_socket(&fds[SUCC_PORT], ports[SUCC_PORT], SOCK_STREAM);

    if (strncmp(leave_pdu.next_address, my_address, ADDRESS_LENGTH) == 0 && htons(leave_pdu.next_port) == ports[NEW_CONN_PORT]) {
        printf("I'm now alone in the network.\n");
        return sizeof(leave_pdu);
    }

    // Get my successors port and address
    memcpy(succ_address, leave_pdu.next_address, ADDRESS_LENGTH);
    succ_port = htons(leave_pdu.next_port);

    struct sockaddr_in succ_addr;
    succ_addr.sin_family = AF_INET;
    inet_pton(AF_INET, leave_pdu.next_address, &(succ_addr.sin_addr));
    succ_addr.sin_port = leave_pdu.next_port;

    // Waiting on successor to accept my connection
    if (connect(fds[SUCC_PORT].fd, (struct sockaddr*)&succ_addr, sizeof(succ_addr)) < 0) {
        perror("Failed connecting to successor.");
        exit(EXIT_FAILURE);
    }
    printf("Connected with successor.\n");

    return sizeof(leave_pdu);
}

/**********************************************************************************************************************/

void send_leaving_network(struct pollfd* fds) {

    printf("Sending NET_CLOSE_CONNECTION_PDU to successor.\n");
    // Sending close_connection_pdu to successor
    struct NET_CLOSE_CONNECTION_PDU close_connection_pdu;
    close_connection_pdu.type = NET_CLOSE_CONNECTION;
    if (send(fds[SUCC_PORT].fd, &close_connection_pdu, sizeof(close_connection_pdu), 0) < 0) {
        printf("Failed sending NET_CLOSE_CONNECTION_PDU.\n");
        return;
    }

    shutdown(fds[SUCC_PORT].fd, SHUT_RDWR);
    close(fds[SUCC_PORT].fd);

    printf("Sending NET_NEW_RANGE_PDU to predecessor.\n");
    // Sending new_range_pdu to predecessor
    struct NET_NEW_RANGE_PDU new_range_pdu;
    new_range_pdu.type = NET_NEW_RANGE;
    new_range_pdu.new_range_end = range_end;

    if (send(fds[PRED_PORT].fd, &new_range_pdu, sizeof(new_range_pdu), 0) < 0) {
        perror("Failed sending NET_NEW_RANGE to predecessor.\n");
        return;
    }

    uint32_t inserts_sent = 0;
    //Transfer my table to successor
    for (int i = range_start; i <= range_end; i++) {
        struct table_entry* entry = table->entries[i];

        while (entry) {
            uint8_t response[24+510] = {'\0'};

            uint8_t name_len = strlen(entry->name)+1;
            uint8_t email_len = strlen(entry->email)+1;

            response[0] = VAL_INSERT;
            memcpy(response+1, entry->ssn, 13);
            response[14] = name_len;
            memcpy(response+16, entry->name, name_len);
            response[16+name_len] = email_len;
            memcpy(response+24+name_len, entry->email, email_len);

            if (send(fds[PRED_PORT].fd, response, 24 + name_len + email_len , 0) < 0) {
                perror("Failed to send VAL_INSERT_PDU.\n");
                return;
            }

            inserts_sent++;
            entry = entry->next;
        }
    }
    printf("I sent %d entries to my predecessor\n", inserts_sent);

    printf("Sending NET_LEAVING_PDU to predecessor.\n");
    // Sending leaving_pdu to predecessor
    struct NET_LEAVING_PDU leaving_pdu;
    leaving_pdu.type = NET_LEAVING;
    memcpy(leaving_pdu.next_address, succ_address, ADDRESS_LENGTH);
    leaving_pdu.next_port = htons(succ_port);
    if (send(fds[PRED_PORT].fd, &leaving_pdu, sizeof(leaving_pdu), 0) < 0) {
        perror("Failed sending NET_LEAVING_PDU to predecessor.\n");
        return;
    }

    shutdown(fds[PRED_PORT].fd, SHUT_RDWR);
    close(fds[PRED_PORT].fd);

}

/**********************************************************************************************************************/

uint16_t val_insert(struct pollfd* fds, uint16_t message_size, char message[message_size]) {

    // pdu not long enough to read name_length
    if (message_size < 15) {
        printf("Message only contains part of pdu (%d bytes).\n", message_size);
        return 0;
    }
    uint8_t name_length = (uint8_t)message[14];

    // pdu not long enough to read email_length
    if (message_size < 17 + name_length) {
        printf("Message only contains part of pdu (%d bytes).\n", message_size);
        return 0;
    }

    uint8_t email_length = (uint8_t)message[16 + name_length];

    // pdu does not countaion the entire struct
    if (message_size < 24 + name_length + email_length) {
        printf("Message only contains part of pdu (%d bytes).\n", message_size);
        return 0;
    }

    struct VAL_INSERT_PDU pdu;
    uint16_t pdu_size = (uint16_t)(24 + name_length + email_length);
    memcpy(&pdu, message, pdu_size);

    char ssn[SSN_LENGTH];
    char name[name_length];
    char email[email_length];

    memcpy(ssn, &pdu.ssn, SSN_LENGTH);
    memcpy(name, &pdu.name, name_length);
    memcpy(email, &message[24 + name_length], email_length);

    ssn[SSN_LENGTH-1] = '\0';
    name[name_length-1] = '\0';
    email[email_length-1] = '\0';

    hash_t h_ssn = hash_ssn(ssn);

    // if hash is whitin our range instert into table
    if (range_start <= h_ssn && h_ssn <= range_end) {
        printf("Received ssn: %s \n", ssn);
        printf("Received name: %s \n", name);
        printf("Received email: %s \n", email);
        printf("Received hashed ssn: %d \n\n", h_ssn);

        table_insert(table, ssn, name, email);

    }
    // send to successor
    else {
        printf("Hash not within my range, sending to successor.\n");
        if (send(fds[SUCC_PORT].fd, message, pdu_size, 0) < 0) {
            perror("Failed sending VAL_INSERT_PDU to successor.\n");
        }
    }
    return pdu_size;
}

/**********************************************************************************************************************/

uint16_t val_lookup(struct pollfd* fds, uint16_t message_size, char message[message_size]) {
    struct VAL_LOOKUP_PDU pdu;

    if (message_size < sizeof(pdu)) {
        printf("Message only contains part of pdu (%d bytes).\n", message_size);
        return 0;
    }

    memcpy(&pdu, message, sizeof(pdu));
    uint8_t name_len = 0;
    uint8_t email_len = 0;
    hash_t h_ssn = hash_ssn((char*)pdu.ssn);
    uint8_t response[24+510] = {'\0'};

    // If hash of ssn is within my range, look it up and send to agent
    if (range_start <= h_ssn && h_ssn <= range_end) {

        printf("hashed ssn: %d is within my range, fetching the information...\n", h_ssn);
        struct table_entry* entry = table_lookup(table, pdu.ssn);

        if(entry != NULL){
            response[0] = VAL_LOOKUP_RESPONSE;

            name_len = strlen(entry->name)+1;
            email_len = strlen(entry->email)+1;

            memcpy(response+1, entry->ssn, 13);
            response[14] = name_len;
            memcpy(response+16, entry->name, name_len);
            response[16+name_len] = email_len;
            memcpy(response+24+name_len, entry->email, email_len);
            printf("Sent entry with ssn: %s\n", entry->ssn);
        }
        else{
            printf("Entry with ssn: %s does not exist.\n", pdu.ssn);
        }

        struct sockaddr_in response_addr;
        response_addr.sin_family = AF_INET;
        inet_pton(AF_INET, pdu.sender_address, &(response_addr.sin_addr));
        response_addr.sin_port = pdu.sender_port;

        size_t sent;
        if ((sent = sendto(fds[PDU_PORT].fd, response, (24 + name_len + email_len), 0,
                   (struct sockaddr*)&response_addr, sizeof(response_addr))) < 0) {
            perror("Failed to send VAL_LOOKUP_RESPONSE_PDU.\n");
            exit(EXIT_FAILURE);
        }

    }

    // If request is not within my range; send it to successor
    else {
        printf("hashed ssn %d, not within my range: %d-%d, sending to successor.\n", h_ssn, range_start, range_end);
        if (send(fds[SUCC_PORT].fd, &pdu, sizeof(pdu), 0) < 0) {
            perror("Failed to send VAL_LOOKUP_PDU.\n");
            exit(EXIT_FAILURE);
        }
    }

    return sizeof(pdu);
}

/**********************************************************************************************************************/

uint16_t val_remove(struct pollfd* fds, uint16_t message_size, char message[message_size]) {
    struct VAL_REMOVE_PDU pdu;

    if (message_size < sizeof(pdu)) {
        printf("Message only contains part of pdu (%d bytes).\n", message_size);
        return 0;
    }

    memcpy(&pdu, message, sizeof(pdu));

    // If hash of ssn is within my range, remove the entry
    hash_t h_ssn = hash_ssn((char*)pdu.ssn);
    if (range_start <= h_ssn && h_ssn <= range_end) {
        printf("hashed ssn: %d is within my range, removing entry...\n", h_ssn);
        table_remove(table, (char*)pdu.ssn);
        printf("Removed entry with ssn: %s.\n", (char*)pdu.ssn);
    }
    // If request is not within my range; send it to successor
    else {
        printf("hashed ssn %d, not within my range: %d-%d, sending to successor.\n", h_ssn, range_start, range_end);
        if (send(fds[SUCC_PORT].fd, &pdu, sizeof(pdu), 0) < 0) {
            perror("Failed to send VAL_LOOKUP_PDU.\n");
            exit(EXIT_FAILURE);
        }
    }

    return sizeof(pdu);
}

/**********************************************************************************************************************/

uint16_t init_node(struct pollfd* fds) {

    // Init sockets
    for (size_t i = 0; i < 5; i++) {
        if(i != 3){
            init_socket(&fds[i], (ports[i] = random_port()), i < 2 ? SOCK_DGRAM : SOCK_STREAM);
        }
    }

    // Set predecessor- and new_connection-port in listening-mode
    listen(fds[PRED_PORT].fd, 1);
    listen(fds[NEW_CONN_PORT].fd, 1);

    struct sockaddr_in trackeraddr;
    trackeraddr.sin_family = AF_INET;
    inet_pton(AF_INET, trac_address, &(trackeraddr.sin_addr));
    trackeraddr.sin_port = htons(trac_port);

    // Sending stun
    struct STUN_LOOKUP_PDU pdu;
    pdu.type = STUN_LOOKUP;
    pdu.port = htons(ports[TRACKER_PORT]);
    if (sendto(fds[TRACKER_PORT].fd, &pdu, sizeof(pdu), 0, (struct sockaddr*)&trackeraddr, sizeof(trackeraddr)) < 0) {
        perror("Failed to receive STUN_LOOKUP_PDU from tracker.\n");
        exit(EXIT_FAILURE);
    }

    // Receiving stun response
    struct STUN_RESPONSE_PDU resp_pdu;
    if (recvfrom(fds[TRACKER_PORT].fd, &resp_pdu, sizeof(resp_pdu), MSG_WAITALL, NULL, NULL) < 0) {
        perror("Failed to receive STUN_RESPONSE_PDU from tracker.\n");
        exit(EXIT_FAILURE);
    }
    memcpy(my_address, resp_pdu.address, ADDRESS_LENGTH);

    // Sending get-node
    struct NET_GET_NODE_PDU net_get_pdu;
    net_get_pdu.type = NET_GET_NODE;
    net_get_pdu.port = htons(ports[TRACKER_PORT]);
    if (sendto(fds[TRACKER_PORT].fd, &net_get_pdu, sizeof(net_get_pdu), 0,
               (struct sockaddr*)&trackeraddr, sizeof(trackeraddr)) < 0) {
        perror("Failed to send NET_GET_NODE_PDU to tracker.\n");
        exit(EXIT_FAILURE);
    }

    // Receiving get-node response
    struct NET_GET_NODE_RESPONSE_PDU net_get_resp_pdu;
    if (recvfrom(fds[TRACKER_PORT].fd, &net_get_resp_pdu,
                 sizeof(net_get_resp_pdu), MSG_WAITALL, NULL, NULL) < 0) {
        perror("Failed to receive NET_GET_NODE_RESPONSE_PDU from tracker.\n");
        exit(EXIT_FAILURE);
    }

    // If port is 0 the tracker does not track any other node - I'm lone
    if (htons(net_get_resp_pdu.port) == 0) {
        printf("I'm alone in the network.\n");
        return 0;
    }

    // Get my successors port and address
    succ_port = htons(net_get_resp_pdu.port);
    memcpy(succ_address, net_get_resp_pdu.address, ADDRESS_LENGTH);
    printf("Sending JOIN_PDU to address: %s port: %d.\n", succ_address, succ_port);

    struct sockaddr_in join_addr;
    join_addr.sin_family = AF_INET;
    inet_pton(AF_INET, succ_address, &(join_addr.sin_addr));
    join_addr.sin_port = htons(succ_port);

    // Sending join-request
    struct NET_JOIN_PDU join_pdu;
    join_pdu.type = NET_JOIN;
    memcpy(join_pdu.src_address, my_address, ADDRESS_LENGTH);
    join_pdu.src_port = htons(ports[NEW_CONN_PORT]);
    join_pdu.max_span = 0;
    if (sendto(fds[PDU_PORT].fd, &join_pdu, sizeof(join_pdu), 0,
               (struct sockaddr*)&join_addr, sizeof(join_addr))< 0) {
        perror("Failed to send NET_JOIN_PDU to node.\n");
        exit(EXIT_FAILURE);
    }

    printf("My new range is: %d-%d\n", range_start, range_end);

    return succ_port;
}

/**********************************************************************************************************************/

void init_socket(struct pollfd* fds, int port, int type) {
    int socket;
    if ((socket = create_socket(port, type)) < 0) {
        exit(EXIT_FAILURE);
    }
    fds->fd = socket;
    fds->events = POLLIN;
}

/**********************************************************************************************************************/

void send_alive(struct pollfd* fds) {

    struct NET_ALIVE_PDU alive_pdu;
    alive_pdu.type = NET_ALIVE;
    alive_pdu.port = htons(ports[TRACKER_PORT]);

    struct sockaddr_in trackeraddr;
    trackeraddr.sin_family = AF_INET;
    inet_pton(AF_INET, trac_address, &(trackeraddr.sin_addr));
    trackeraddr.sin_port = htons(trac_port);

    if (sendto(fds[TRACKER_PORT].fd, &alive_pdu, sizeof(alive_pdu), 0,
               (struct sockaddr*)&trackeraddr, sizeof(trackeraddr)) < 0) {
        perror("Failed to send NET_ALIVE.\n");
    }
}

/**********************************************************************************************************************/

void new_range(uint8_t *suc_start, uint8_t *suc_end) {

    *suc_start = range_start;
    *suc_end = range_end;
    range_end = (uint8_t)((range_end - range_start)/2) + range_start;
    *suc_start = range_end+(uint8_t)1;
}

/**********************************************************************************************************************/

static void sigHandler(int signal) {
    printf("signal was detected.\n");
    if (signal == SIGINT) {
        alive = 0;
    } else {
        printf("unknown signal.\n");
    }
}

/**********************************************************************************************************************/

int random_port() {

    int i = 0;
    int r = (rand() % 50000)+3000;
    for (int i = 0; i < 5; i++) {
        if (r == ports[i]) {
            return random_port();
        }
    }
    return r;
}

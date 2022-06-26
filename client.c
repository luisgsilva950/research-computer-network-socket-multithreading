#include <stdio.h>
#include <sys/socket.h>
#include <strings.h>
#include <unistd.h>
#include "common.c"


int MY_ID;

char *send_message(int _socket, char message[]) {
    int message_size = strlen(message) + 1;
    printf("Sending message: %d bytes: %s", message_size, message);
    const int count = send(_socket, message, (ssize_t) message_size, 0);
    if (count != message_size) error("Error sending message");
    char *r = malloc(BUFFER_SIZE_IN_BYTES);
    memset(r, 0, BUFFER_SIZE_IN_BYTES);
    unsigned total_bytes_received = 0;
    while (1) {
        int bytes_received_count = recv(_socket, r, BUFFER_SIZE_IN_BYTES - total_bytes_received, 0);
        if (bytes_received_count == 0) break;
        printf("Response message: %d bytes: %s", bytes_received_count, r);
        total_bytes_received = total_bytes_received + bytes_received_count;
    }
    return r;
}

int main(int argc, char *argv[]) {
    if (argc < 3) print_client_usage_pattern();
    char message[BUFFER_SIZE_IN_BYTES];
    struct socket_context context = initialize_client_socket(argv[1], argv[2]);
    char *ip = inet_ntoa(((struct sockaddr_in *) &context.socket_address)->sin_addr);
    int _socket = context.socket;
    char r_copy[BUFFER_SIZE_IN_BYTES] = {};
    char *inserted_id_response = send_message(_socket, "01\n");
    strcpy(r_copy, inserted_id_response);
    MY_ID = get_inserted_id(r_copy);
    while (1) {
        context = initialize_client_socket(argv[1], argv[2]);
        _socket = context.socket;
        memset(message, 0, BUFFER_SIZE_IN_BYTES);
        fgets(message, BUFFER_SIZE_IN_BYTES - 1, stdin);
        char mapped_message[BUFFER_SIZE_IN_BYTES] = {};
        memset(mapped_message, 0, BUFFER_SIZE_IN_BYTES);
        if (strlen(message) > 0) {
            if (is_equal(message, "close connection\n")) {
                printf("Will close client connection with: %s\n", ip);
                sprintf(mapped_message, "02 %s\n", get_id_as_string(MY_ID));
                send_message(_socket, mapped_message);
//                struct socket_context broadcast_context = initialize_broadcast_socket(argv[2]);
//                send_message(broadcast_context.socket, );

            } else if (is_equal(message, "list equipment\n")) {
                printf("Will list equipments connected on server: %s\n", ip);
                sprintf(mapped_message, "04\n");
                send_message(_socket, mapped_message);
//                struct socket_context broadcast_context = initialize_broadcast_socket(argv[2]);
//                send_message(broadcast_context.socket, );

            } else if (is_equal(get_first_word(message), "request")) {
                int id = get_id_from_list_information_client_message(message);
                printf("Will list information from: %s\n", get_id_as_string(id));
                sprintf(mapped_message, "05 %s %s\n", get_id_as_string(MY_ID), get_id_as_string(id));
                send_message(_socket, mapped_message);
            } else {
                send_message(_socket, message);
            }
        }
        close(_socket);
    }
}

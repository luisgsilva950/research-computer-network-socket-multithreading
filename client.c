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

char *get_error_message_from_code(char *code) {
    if (get_string_as_integer(code) == LIMIT_EXCEEDED) {
        return "Equipment limit exceeded\n";
    } else if (get_string_as_integer(code) == SOURCE_EQUIPMENT_NOT_FOUND) {
        return "Source equipment not found\n";
    } else if (get_string_as_integer(code) == TARGET_EQUIPMENT_NOT_FOUND) {
        return "Target equipment not found\n";
    }
    return "Equipment not found\n";
}

int main(int argc, char *argv[]) {
    if (argc < 3) print_client_usage_pattern();
    char message[BUFFER_SIZE_IN_BYTES];
    struct socket_context context = initialize_client_socket(argv[1], argv[2]);
    char *ip = inet_ntoa(((struct sockaddr_in *) &context.socket_address)->sin_addr);
    int _socket = context.socket;
    char add_response_copy[BUFFER_SIZE_IN_BYTES] = {};
    char *inserted_id_response = send_message(_socket, "01\n");
    strcpy(add_response_copy, inserted_id_response);
    char *error_code = get_sequence_word_in_buffer(add_response_copy, 1);
    if (get_string_as_integer(error_code) != ERROR_RESPONSE) {
        MY_ID = get_string_as_integer(get_sequence_word_in_buffer(add_response_copy, 2));
        printf("New ID: %s\n", get_number_as_string(MY_ID));
    } else {
        printf("%s", get_sequence_word_in_buffer(add_response_copy, 3));
    }
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
                sprintf(mapped_message, "%s %s\n", get_number_as_string(REMOVE_EQUIPMENT_REQUEST),
                        get_number_as_string(MY_ID));
                send_message(_socket, mapped_message);
                printf("Successful removal");
                exit(0);
//                struct socket_context broadcast_context = initialize_broadcast_socket(argv[2]);
//                send_message(broadcast_context.socket, );

            } else if (is_equal(message, "list equipment\n")) {
                printf("Will list equipments connected on server: %s\n", ip);
                sprintf(mapped_message, "%s\n", get_number_as_string(LIST_EQUIPMENTS_REQUEST));
                send_message(_socket, mapped_message);
//                struct socket_context broadcast_context = initialize_broadcast_socket(argv[2]);
//                send_message(broadcast_context.socket, );

            } else if (is_equal(get_first_word(message), "request")) {
                int id = get_string_as_integer(get_sequence_word_in_buffer(message, 4));
                printf("Will list information from: %s\n", get_number_as_string(id));
                sprintf(mapped_message, "%s %s %s\n", get_number_as_string(GET_EQUIPMENT_INFORMATION_REQUEST),
                        get_number_as_string(MY_ID), get_number_as_string(id));
                char *read_response = send_message(_socket, mapped_message);
                int response_code = get_string_as_integer(get_sequence_word_in_buffer(read_response, 1));
                if (response_code != ERROR_RESPONSE) {
                    int target_id = get_string_as_integer(get_sequence_word_in_buffer(read_response, 3));
                    float read_value = get_string_as_float(get_sequence_word_in_buffer(read_response, 4));
                    printf("Value from %s: %.2f\n", get_number_as_string(target_id), read_value);
                } else {
                    char *code = get_sequence_word_in_buffer(read_response, 3);
                    printf("%s", get_error_message_from_code(code));
                }
            } else {
                send_message(_socket, message);
            }
        }
        close(_socket);
    }
}

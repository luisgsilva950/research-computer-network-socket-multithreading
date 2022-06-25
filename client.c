#include <stdio.h>
#include <sys/socket.h>
#include <strings.h>
#include <unistd.h>
#include "common.c"

int main(int argc, char *argv[]) {
    if (argc < 3) print_client_usage_pattern();
    char message[BUFFER_SIZE_IN_BYTES];
    while (1) {
        struct socket_context context = initialize_client_socket(argv[1], argv[2]);
        int _socket = context.socket;
        memset(message, 0, BUFFER_SIZE_IN_BYTES);
        fgets(message, BUFFER_SIZE_IN_BYTES - 1, stdin);
        if (strlen(message) > 0) {
            int message_size = strlen(message) + 1;
            printf("Sending message: %d bytes: %s", message_size, message);
            const int count = send(_socket, message, (ssize_t) message_size, 0);
            if (count != message_size) error("Error sending message");
            char buffer[BUFFER_SIZE_IN_BYTES];
            memset(buffer, 0, BUFFER_SIZE_IN_BYTES);
            unsigned total_bytes_received = 0;
            while (1) {
                int bytes_received_count = recv(_socket, buffer, BUFFER_SIZE_IN_BYTES - total_bytes_received, 0);
                if (bytes_received_count == 0) break;
                printf("Response message: %d bytes: %s", bytes_received_count, buffer);
                total_bytes_received = total_bytes_received + bytes_received_count;
            }
            close(_socket);
        }
    }
}

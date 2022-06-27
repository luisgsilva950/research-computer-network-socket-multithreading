#include <sys/socket.h>
#include <strings.h>
#include <pthread.h>
#include "common.c"


struct thread_data {
    int client_socket;
    struct sockaddr client_socket_address;
};

void *start_client_thread(void *data) {
    const struct thread_data *th_data = ((struct thread_data *) data);
    char *client_ip = inet_ntoa(((struct sockaddr_in *) &th_data->client_socket_address)->sin_addr);
    int client_port = ntohs(((struct sockaddr_in *) &th_data->client_socket_address)->sin_port);
    printf("Connection from: %s:%d established! Socket: %d\n", client_ip, client_port, th_data->client_socket);
    char buffer[BUFFER_SIZE_IN_BYTES] = {};
    memset(buffer, 0, BUFFER_SIZE_IN_BYTES);
    ssize_t count = recv(th_data->client_socket, buffer, BUFFER_SIZE_IN_BYTES - 1, 0);
    if ((int) count == 0) { close(th_data->client_socket); }
    else {
        printf("Message received from %s: %d bytes: %s", client_ip, (int) count, buffer);
        char buffer_copy[BUFFER_SIZE_IN_BYTES] = {};
        strcpy(buffer_copy, buffer);
        int command = get_command_type(buffer_copy);
        if (command == ADD_EQUIPMENT_REQUEST) handle_add_new_equipment(client_ip, th_data->client_socket);
        if (command == REMOVE_EQUIPMENT_REQUEST) {
            int equipment_id = get_string_as_integer(get_sequence_word_in_buffer(buffer_copy, 2));
            handle_remove_equipment(equipment_id, th_data->client_socket);
        }
        if (command == LIST_EQUIPMENTS_REQUEST) {
            int id = get_id_from_socket(th_data->client_socket - 1);
            printf("Id from equipment request: %s\n", get_number_as_string(id));
            handle_list_equipments(id, th_data->client_socket);
        }
        if (command == GET_EQUIPMENT_INFORMATION_REQUEST) {
            int id = get_id_from_socket(th_data->client_socket - 1);
            int target_id = get_string_as_integer(get_sequence_word_in_buffer(buffer_copy, 3));
            handle_request_information(id, target_id, th_data->client_socket);
        } else {
            char response[BUFFER_SIZE_IN_BYTES] = {};
            sprintf(response, "Command is %d\n", command);
            send(th_data->client_socket, response, strlen(response) + 1, 0);
            close(th_data->client_socket);
        }
    }
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) print_server_usage_pattern();
    struct socket_context context = initialize_server_socket("v4", argv[1]);
    int _socket = context.socket;
    while (1) {
        struct sockaddr_storage client_socket_storage;
        struct sockaddr *client_socket_address = (struct sockaddr *) (&client_socket_storage);
        pthread_t thread_id = NULL;
        struct thread_data *th_data = malloc(sizeof(*th_data));
        if (!th_data) error("Error instantiating thread data.");
        int client_socket_address_len = sizeof(*client_socket_address);
        int client_socket = accept(_socket, client_socket_address, (socklen_t *) &client_socket_address_len);
        if (did_communication_fail(client_socket)) error("Error with client socket communication...");
        th_data->client_socket = client_socket;
        memcpy(&(th_data->client_socket_address), &client_socket_address, sizeof(*client_socket_address));
        pthread_create(&thread_id, NULL, start_client_thread, th_data);
    }
}
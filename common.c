#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

enum Constants {
    BUFFER_SIZE_IN_BYTES = 100,
    SERVER_LISTENING_SIZE = 10,
    MAX_EQUIPMENTS_SIZE = 15
};

enum Boolean {
    FALSE = 0,
    TRUE = 1
};

enum MessageType {
    ADD_EQUIPMENT_REQUEST = 1,
    REMOVE_EQUIPMENT_REQUEST = 2,
    ADD_EQUIPMENT_RESPONSE = 3,
    LIST_EQUIPMENTS_REQUEST = 4,
    GET_EQUIPMENT_INFORMATION_REQUEST = 5,
    GET_EQUIPMENT_INFORMATION_RESPONSE = 6,
    REMOVE_EQUIPMENT_RESPONSE = 8,
    ERROR_RESPONSE = 7
};

enum ErrorCode {
    LIMIT_EXCEEDED = 4,
    SOURCE_EQUIPMENT_NOT_FOUND = 2,
    TARGET_EQUIPMENT_NOT_FOUND = 3,
    EQUIPMENT_NOT_FOUND = 1
};

static int NEXT_ID = 1;

struct storage_client {
    int socket;
    char *ip;
    int id;
};

static struct storage_client clients[MAX_EQUIPMENTS_SIZE] = {};


struct socket_context {
    struct sockaddr socket_address;
    int socket;
};

int is_equal(const char *string1, const char *string2) {
    return !strcmp(string1, string2);
}

void error(char *msg) {
    perror(msg);
    exit(0);
}

void print_server_usage_pattern() {
    printf("usage: <v4|v6> <server port>");
    printf("example: 127.0.0.1 5501");
    exit(EXIT_FAILURE);
}

void print_client_usage_pattern() {
    printf("usage: <server IP> <server port>");
    printf("example: 127.0.0.1 5501");
    exit(EXIT_FAILURE);
}

char *get_number_as_string(int id) {
    char *aux = malloc(8 * (sizeof(char)));
    if (id >= 10) {
        sprintf(aux, "%d", id);
    } else {
        sprintf(aux, "0%d", id);
    }
    return aux;
}

int get_string_as_integer(const char *raw_integer) {
    return atoi(raw_integer);
}

float get_string_as_float(const char *raw_integer) {
    return atof(raw_integer);
}

int parse_client_address(const char *raw_address, const char *raw_port, struct sockaddr_storage *storage) {
    if (raw_address == NULL) return FALSE;
    int PORT_NUMBER = get_string_as_integer(raw_port);
    if (PORT_NUMBER == 0) return FALSE;
    PORT_NUMBER = htons(PORT_NUMBER);
    struct in_addr ipv4_address;
    if (inet_pton(AF_INET, raw_address, &ipv4_address)) {
        struct sockaddr_in *ipv4_socket = (struct sockaddr_in *) storage;
        ipv4_socket->sin_port = PORT_NUMBER;
        ipv4_socket->sin_family = AF_INET;
        ipv4_socket->sin_addr = ipv4_address;
        return sizeof(struct sockaddr_in);
    }
    struct in6_addr ipv6_address;
    if (inet_pton(AF_INET6, raw_address, &ipv6_address)) {
        struct sockaddr_in6 *ipv6_socket = (struct sockaddr_in6 *) storage;
        ipv6_socket->sin6_port = PORT_NUMBER;
        ipv6_socket->sin6_family = AF_INET6;
        memcpy(&(ipv6_socket->sin6_addr), &ipv6_address, sizeof(ipv6_address));
        return sizeof(struct sockaddr_in6);
    }
    return FALSE;
}

int parse_broadcast_address(const char *raw_port, struct sockaddr_storage *storage) {
    int PORT_NUMBER = get_string_as_integer(raw_port);
    if (PORT_NUMBER == 0) return FALSE;
    PORT_NUMBER = htons(PORT_NUMBER);
    struct sockaddr_in *ipv4_socket = (struct sockaddr_in *) storage;
    bzero(&(ipv4_socket->sin_zero), 8);
    ipv4_socket->sin_port = PORT_NUMBER;
    ipv4_socket->sin_family = AF_INET;
    ipv4_socket->sin_addr.s_addr = htonl(INADDR_BROADCAST);        /* send to all */
    return sizeof(struct sockaddr_in);
}


int parse_server_address(const char *protocol, const char *raw_port, struct sockaddr_storage *storage) {
    uint16_t port = (uint16_t) get_string_as_integer(raw_port);
    if (port == 0) return FALSE;
    port = htons(port);
    memset(storage, 0, sizeof(*storage));
    if (is_equal(protocol, "v4")) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return sizeof(struct sockaddr_in);
    } else if (is_equal(protocol, "v6")) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return sizeof(struct sockaddr_in6);
    }
    return FALSE;
}

struct socket_context initialize_client_socket(char *address, char *port) {
    struct sockaddr_storage storage;
    int protocol_struct_length = parse_client_address(address, port, &storage);
    if (protocol_struct_length == FALSE) print_client_usage_pattern();
    int _socket = socket(storage.ss_family, SOCK_STREAM, 0);
    if (_socket < FALSE) error("Error opening socket...");
    struct sockaddr *socket_address = (struct sockaddr *) &storage;
    int connection_status_code = connect(_socket, socket_address, protocol_struct_length);
    if (connection_status_code < FALSE) error("Error connecting...");
    struct socket_context context;
    context.socket_address = *socket_address;
    context.socket = _socket;
    return context;
}

struct socket_context initialize_broadcast_socket(char *port) {
    struct sockaddr_storage storage;
    int protocol_struct_length = parse_broadcast_address(port, &storage);
    if (protocol_struct_length == FALSE) print_client_usage_pattern();
    int _socket = socket(storage.ss_family, SOCK_DGRAM, 0);
    if (_socket < FALSE) error("Error opening socket...");
    struct sockaddr *socket_address = (struct sockaddr *) &storage;
    int connection_status_code = connect(_socket, socket_address, protocol_struct_length);
    if (connection_status_code < FALSE) error("Error connecting...");
    struct socket_context context;
    context.socket_address = *socket_address;
    context.socket = _socket;
    return context;
}

struct socket_context initialize_server_socket(char *protocol, char *port) {
    struct sockaddr_storage storage;
    int protocol_struct_length = parse_server_address(protocol, port, &storage);
    if (protocol_struct_length == FALSE) print_server_usage_pattern();
    int _socket = socket(storage.ss_family, SOCK_STREAM, 0);
    int enabled = 1;
    int set_socket_opt_code = setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(int));
    if (set_socket_opt_code != FALSE) error("Error setting socket opt...");
    struct sockaddr *socket_address = (struct sockaddr *) (&storage);
    int bind_server_code = bind(_socket, socket_address, protocol_struct_length);
    if (bind_server_code != FALSE) error("Error binding server...");
    int listen_server_code = listen(_socket, SERVER_LISTENING_SIZE);
    if (listen_server_code != FALSE) error("Error configuring server listening...");
    struct socket_context context;
    context.socket_address = *socket_address;
    context.socket = _socket;
    return context;
}


int get_next_id() {
    return NEXT_ID;
}

float generate_random_number() {
    return ((float) rand() / RAND_MAX) * 10;
}

enum Boolean did_communication_fail(int client_socket) {
    return client_socket < FALSE;
}


enum Boolean insert_new_equipment(int client_socket, char *ip, int next_id) {
    int counter = 0;
    for (counter = 0; counter < MAX_EQUIPMENTS_SIZE; counter++) {
        if (clients[counter].id == 0) {
            clients[counter].id = next_id;
            clients[counter].ip = ip;
            clients[counter].socket = client_socket;
            return TRUE;
        }
    }
    return FALSE;
}

void remove_equipment(const int id) {
    int counter = 0;
    for (counter = 0; counter < MAX_EQUIPMENTS_SIZE; counter++) {
        if (clients[counter].id == id) {
            NEXT_ID = clients[counter].id;
            clients[counter].socket = -1;
            clients[counter].id = 0;
            clients[counter].ip = "";
        }
    }
}

int *get_equipments_excluding_current(int id) {
    int *equipments = NULL;
    int count = 0, i = 0;
    for (i = 0; i < MAX_EQUIPMENTS_SIZE; i++) {
        if (clients[i].id != 0 && clients[i].id != id) {
            equipments = realloc(equipments, (count + 1) * sizeof(int));
            equipments[count] = clients[i].id;
            count++;
        }
    }
    return equipments;
}

void print_int_values(int *values) {
    int counter;
    for (counter = 0; counter < MAX_EQUIPMENTS_SIZE; counter++) {
        if (values[counter]) {
            printf("Value: %d\n", values[counter]);
        }
    }
}

char *get_sequence_word_in_buffer(char buffer[], int sequence_number) {
    int i;
    char *buffer_copy = malloc(BUFFER_SIZE_IN_BYTES);
    strcpy(buffer_copy, buffer);
    char *token = strtok(buffer_copy, " ");
    for (i = 0; i < (sequence_number - 1); i++) {
        token = strtok(NULL, " ");
    }
    return token;
}

char *get_first_word(char buffer[]) {
    return get_sequence_word_in_buffer(buffer, 1);
}

int get_command_type(char buffer[]) {
    char *first_word = get_first_word(buffer);
    return get_string_as_integer(first_word);
}

struct storage_client get_client_from_id(int id) {
    int counter = 0;
    struct storage_client response;
    for (counter = 0; counter < MAX_EQUIPMENTS_SIZE; counter++) {
        if (clients[counter].id == id) {
            response = clients[id];
        }
    }
    return response;
}


int get_id_from_socket(int socket) {
    int counter = 0;
    int response = -1;
    for (counter = 0; counter < MAX_EQUIPMENTS_SIZE; counter++) {
        if (clients[counter].socket == socket) {
            response = clients[counter].id;
        }
    }
    return response;
}

enum Boolean is_equipment_present(int id) {
    enum Boolean is_present = FALSE;
    int counter;
    for (counter = 0; counter < MAX_EQUIPMENTS_SIZE; counter++) {
        if (clients[counter].id == id) {
            is_present = TRUE;
            break;
        }
    }
    return is_present;
}

void send_message_to_broadcast(char *message, enum Boolean use_broadcast_ip) {
    int i;
    // Necessary approach because using broadcast ip I got a permission error
    for (i = 0; i < MAX_EQUIPMENTS_SIZE; i++) {
        if (clients[i].id != 0) {
            printf("Sending message to broadcast...\n");
            const int count = send(clients[i].socket, message, strlen(message) + 1, 0);
            if (count != strlen(message) + 1) error("Error sending message");
        }
    }
}

void handle_request_information(int id, int target_id, int client_socket) {
    char message[BUFFER_SIZE_IN_BYTES] = "";
    if (!is_equipment_present(id)) {
        printf("Equipment %s not found\n", get_number_as_string(id));
        sprintf(message, "%s %s %s\n", get_number_as_string(ERROR_RESPONSE),
                get_number_as_string(id), get_number_as_string(SOURCE_EQUIPMENT_NOT_FOUND));
    } else if (!is_equipment_present(target_id)) {
        printf("Equipment %s not found\n", get_number_as_string(target_id));
        sprintf(message, "%s %s %s\n", get_number_as_string(ERROR_RESPONSE),
                get_number_as_string(id), get_number_as_string(TARGET_EQUIPMENT_NOT_FOUND));
    } else {
        float random_value = generate_random_number();
        printf("Equipment %s will list information of equipment %s\n", get_number_as_string(id),
               get_number_as_string(target_id));
        sprintf(message, "%s %s %s %.2f\n", get_number_as_string(GET_EQUIPMENT_INFORMATION_RESPONSE),
                get_number_as_string(id), get_number_as_string(target_id), random_value);
    }
    send(client_socket, message, strlen(message) + 1, 0);
    close(client_socket);
}

void handle_list_equipments(int id, int client_socket) {
    int counter = 0;
    char message[BUFFER_SIZE_IN_BYTES] = {};
    int *equipment_ids = get_equipments_excluding_current(id);
    print_int_values(equipment_ids);
    for (counter = 0; counter < MAX_EQUIPMENTS_SIZE; counter++) {
        if (equipment_ids[counter] == 0 || equipment_ids[counter] > MAX_EQUIPMENTS_SIZE) break;
        if (equipment_ids[counter] != id) {
            char aux[12] = {};
            sprintf(aux, "%s ", get_number_as_string(equipment_ids[counter]));
            strcat(message, aux);
        }
    }
    strcat(message, "\n");
    printf("Listing equipments: %s", message);
    send(client_socket, message, strlen(message) + 1, 0);
    close(client_socket);
}

void handle_add_new_equipment(char *ip, int client_socket) {
    int next_id = get_next_id();
    enum Boolean is_success_inserted = insert_new_equipment(client_socket, ip, next_id);
    char response[BUFFER_SIZE_IN_BYTES] = {};
    if (is_success_inserted) {
        NEXT_ID++;
        char *aux = malloc(BUFFER_SIZE_IN_BYTES);
        sprintf(aux, "Equipment %s added\n", get_number_as_string(next_id));
        printf("%s", aux);
        send_message_to_broadcast(aux, FALSE);
        sprintf(response, "%s %s\n", get_number_as_string(ADD_EQUIPMENT_RESPONSE), get_number_as_string(next_id));
    } else {
        sprintf(response, "Equipment limit exceeded\n");
    }
    send(client_socket, response, strlen(response) + 1, 0);
    close(client_socket);
}

void handle_remove_equipment(int id, int client_socket) {
    remove_equipment(id);
    char response[BUFFER_SIZE_IN_BYTES] = {};
    char *aux = malloc(BUFFER_SIZE_IN_BYTES);
    sprintf(aux, "Equipment %s removed\n", get_number_as_string(id));
    printf("%s", aux);
    send_message_to_broadcast(aux, FALSE);
    sprintf(response, "%s %s 01\n", get_number_as_string(REMOVE_EQUIPMENT_RESPONSE), get_number_as_string(id));
    send(client_socket, response, strlen(response) + 1, 0);
    close(client_socket);
}

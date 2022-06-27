#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

static const char *ADD = "add";
static const char *LIST = "list";
static const char *READ = "read";
static const char *REMOVE = "remove";

enum Constants {
    BUFFER_SIZE_IN_BYTES = 100,
    SERVER_LISTENING_SIZE = 10,
    MAX_EQUIPMENTS_SIZE = 15
};

enum Boolean {
    FALSE = 0,
    TRUE = 1
};

enum Equipments {
    NOT_FOUND = -1
};

enum MessageType {
    ADD_EQUIPMENT_REQUEST = 1,
    REMOVE_EQUIPMENT_REQUEST = 2,
    ADD_EQUIPMENT_RESPONSE = 3,
    LIST_EQUIPMENTS_REQUEST = 4,
    GET_EQUIPMENT_INFORMATION_REQUEST = 5,
    GET_EQUIPMENT_INFORMATION_RESPONSE = 6,
    OK_RESPONSE = 7,
    ERROR_RESPONSE = 8
};

static int SERVER_EQUIPMENTS_COUNT = 0;
static int NEXT_ID = 1;

struct order_context {
    int equipment_id;
    int sensor_id;
};

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

int parse_client_address(const char *raw_address, const char *raw_port, struct sockaddr_storage *storage) {
    if (raw_address == NULL) return FALSE;
    int PORT_NUMBER = atoi(raw_port);
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
    int PORT_NUMBER = atoi(raw_port);
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
    uint16_t port = (uint16_t) atoi(raw_port);
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

int *get_sensor_ids_from_message(char *message) {
    char *string_aux = malloc(BUFFER_SIZE_IN_BYTES);
    memset(string_aux, 0, BUFFER_SIZE_IN_BYTES);
    static int sensor_ids[3] = {NOT_FOUND, NOT_FOUND, NOT_FOUND};
    memset(sensor_ids, 0, 3 * sizeof(int));
    strcpy(string_aux, message);
    char *token = strtok(string_aux, " ");
    int is_equipment_id_now = FALSE;
    int count_sensors = 0;
    while (token) {
        if (!isalpha(token[0]) && !is_equipment_id_now) {
            sensor_ids[count_sensors] = atoi(token);
            count_sensors = count_sensors + 1;
        }
        if (count_sensors == 3) break;
        if (is_equal(token, "in")) is_equipment_id_now = TRUE;
        token = strtok(NULL, " ");
    }
    return sensor_ids;
}

int get_equipment_id_from_message(char *message) {
    char *string_aux = malloc(BUFFER_SIZE_IN_BYTES);
    strcpy(string_aux, message);
    char *token = strtok(string_aux, " ");
    int equipment_id = NOT_FOUND;
    int is_equipment_id_now = FALSE;
    while (token) {
        if (is_equipment_id_now) equipment_id = atoi(token);
        if (is_equal(token, "in")) is_equipment_id_now = TRUE;
        token = strtok(NULL, " ");
    }
    return equipment_id;
}

int get_equipments_count(struct order_context *equipments) {
    int count = 0;
    struct order_context *context;
    for (context = equipments; context->sensor_id != NOT_FOUND; context++) {
        count = count + 1;
    }
    return count;
}

int get_next_id() {
    return NEXT_ID;
}

int *get_sensors_by_equipment(struct order_context *equipments, int equipment_id) {
    int *sensors = NULL;
    int count = 0, i = 0;
    for (i = 0; i < MAX_EQUIPMENTS_SIZE; i++) {
        const struct order_context context = equipments[i];
        if (context.equipment_id == equipment_id && sensors == NULL) {
            sensors = malloc((count + 1) * sizeof(int));
            sensors[count] = context.sensor_id;
            count = count + 1;
        } else if (context.equipment_id == equipment_id) {
            sensors = realloc(sensors, (count + 1) * sizeof(int));
            sensors[count] = context.sensor_id;
            count = count + 1;
        }
    }
    return sensors;
}

char *get_add_success_response(int *sensors_added, int *sensors_already_present, int equipment_id) {
    static char response[BUFFER_SIZE_IN_BYTES] = "";
    static char added_sensor_message[BUFFER_SIZE_IN_BYTES] = "";
    static char already_present_sensor_message[BUFFER_SIZE_IN_BYTES] = "";
    memset(response, 0, BUFFER_SIZE_IN_BYTES);
    memset(added_sensor_message, 0, BUFFER_SIZE_IN_BYTES);
    memset(already_present_sensor_message, 0, BUFFER_SIZE_IN_BYTES);
    int *sensor;
    enum Boolean has_sensors_added = FALSE;
    enum Boolean has_already_present_sensors = FALSE;
    char equipment_id_as_string[32];
    sprintf(equipment_id_as_string, "0%d", equipment_id);
    for (sensor = sensors_added; (int) *sensor != '\0'; sensor++) {
        has_sensors_added = TRUE;
        char sensor_as_string[17] = "";
        sprintf(sensor_as_string, "0%d ", (int) *sensor);
        strcat(added_sensor_message, sensor_as_string);
    }
    for (sensor = sensors_already_present; (int) *sensor != '\0'; sensor++) {
        has_already_present_sensors = TRUE;
        char sensor_as_string[32] = "";
        sprintf(sensor_as_string, "0%d ", (int) *sensor);
        strcat(already_present_sensor_message, sensor_as_string);
    }
    if (has_sensors_added && has_already_present_sensors) {
        sprintf(response, "sensor %s", added_sensor_message);
        strcat(response, "added ");
        strcat(response, already_present_sensor_message);
        strcat(response, "already exists in ");
        strcat(response, equipment_id_as_string);
    }
    if (has_sensors_added && !has_already_present_sensors) {
        sprintf(response, "sensor %s", added_sensor_message);
        strcat(response, "added");
    }
    if (!has_sensors_added && has_already_present_sensors) {
        sprintf(response, "sensor %s", already_present_sensor_message);
        strcat(response, "already exists in ");
        strcat(response, equipment_id_as_string);
    }
    strcat(response, "\n");
    return response;
}

char *get_remove_success_response(int *sensors_removed, int *sensors_not_present, int equipment_id) {
    static char response[BUFFER_SIZE_IN_BYTES] = "";
    static char removed_sensor_message[BUFFER_SIZE_IN_BYTES] = "";
    static char not_present_sensor_message[BUFFER_SIZE_IN_BYTES] = "";
    memset(response, 0, BUFFER_SIZE_IN_BYTES);
    memset(removed_sensor_message, 0, BUFFER_SIZE_IN_BYTES);
    memset(not_present_sensor_message, 0, BUFFER_SIZE_IN_BYTES);
    int *sensor;
    enum Boolean has_sensors_removed = FALSE;
    enum Boolean has_not_present_sensors = FALSE;
    char equipment_id_as_string[32];
    sprintf(equipment_id_as_string, "0%d", equipment_id);
    for (sensor = sensors_removed; (int) *sensor != '\0'; sensor++) {
        has_sensors_removed = TRUE;
        char sensor_as_string[17] = "";
        sprintf(sensor_as_string, "0%d ", (int) *sensor);
        strcat(removed_sensor_message, sensor_as_string);
    }
    for (sensor = sensors_not_present; (int) *sensor != '\0'; sensor++) {
        has_not_present_sensors = TRUE;
        char sensor_as_string[32] = "";
        sprintf(sensor_as_string, "0%d ", (int) *sensor);
        strcat(not_present_sensor_message, sensor_as_string);
    }
    if (has_sensors_removed && has_not_present_sensors) {
        sprintf(response, "sensor %s", removed_sensor_message);
        strcat(response, "removed ");
        strcat(response, not_present_sensor_message);
        strcat(response, "does not exist in ");
        strcat(response, equipment_id_as_string);
    }
    if (has_sensors_removed && !has_not_present_sensors) {
        sprintf(response, "sensor %s", removed_sensor_message);
        strcat(response, "removed");
    }
    if (!has_sensors_removed && has_not_present_sensors) {
        sprintf(response, "sensor %s", not_present_sensor_message);
        strcat(response, "does not exist in ");
        strcat(response, equipment_id_as_string);
    }
    strcat(response, "\n");
    return response;
}

float generate_random_number() {
    return ((float) rand() / RAND_MAX) * 10;
}

char *get_read_response(int *installed_sensors, int *not_installed_sensors) {
    static char response[BUFFER_SIZE_IN_BYTES] = "";
    static char installed_sensor_message[BUFFER_SIZE_IN_BYTES] = "";
    static char not_installed_sensors_message[BUFFER_SIZE_IN_BYTES] = "";
    srand(time(NULL));
    memset(response, 0, BUFFER_SIZE_IN_BYTES);
    memset(installed_sensor_message, 0, BUFFER_SIZE_IN_BYTES);
    memset(not_installed_sensors_message, 0, BUFFER_SIZE_IN_BYTES);
    int *sensor;
    enum Boolean has_sensors_installed = FALSE;
    enum Boolean has_not_sensors_installed = FALSE;
    if (installed_sensors != NULL) {
        for (sensor = installed_sensors; (int) *sensor != '\0'; sensor++) {
            has_sensors_installed = TRUE;
            char sensor_as_string[7] = "";
            sprintf(sensor_as_string, "%.2f ", generate_random_number());
            strcat(installed_sensor_message, sensor_as_string);
        }
    }
    if (not_installed_sensors != NULL) {
        for (sensor = not_installed_sensors; (int) *sensor != '\0'; sensor++) {
            has_not_sensors_installed = TRUE;
            char sensor_as_string[7] = "  ";
            sprintf(sensor_as_string, "0%d ", (int) *sensor);
            strcat(not_installed_sensors_message, sensor_as_string);
        }
    }
    if (has_sensors_installed && has_not_sensors_installed) {
        strcat(response, installed_sensor_message);
        strcat(response, "and ");
        strcat(response, not_installed_sensors_message);
        strcat(response, "not installed");
    }
    if (has_sensors_installed && !has_not_sensors_installed) {
        strcat(response, installed_sensor_message);
    }
    if (!has_sensors_installed && has_not_sensors_installed) {
        sprintf(response, "sensor(s) ");
        strcat(response, not_installed_sensors_message);
        strcat(response, "not installed");
    }
    strcat(response, "\n");
    return response;
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

void handle_add_new_equipment(char *ip, int client_socket) {
    int next_id = get_next_id();
    enum Boolean is_success_inserted = insert_new_equipment(client_socket, ip, next_id);
    char response[BUFFER_SIZE_IN_BYTES] = {};
    if (is_success_inserted) {
        NEXT_ID++;
        sprintf(response, "New ID: %s\n", get_number_as_string(next_id));
    } else {
        sprintf(response, "Equipments size exceeded!\n");
    }
    send(client_socket, response, strlen(response) + 1, 0);
    close(client_socket);
}

void handle_remove_equipment(int id, int client_socket) {
    remove_equipment(id);
    char response[BUFFER_SIZE_IN_BYTES] = {};
    sprintf(response, "Successful removal\n");
    send(client_socket, response, strlen(response) + 1, 0);
    close(client_socket);
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

void handle_list_equipments(int id, int client_socket) {
    int counter = 0;
    char message[BUFFER_SIZE_IN_BYTES] = {};
    int *equipment_ids = get_equipments_excluding_current(id);
    print_int_values(equipment_ids);
    for (counter = 0; counter < MAX_EQUIPMENTS_SIZE; counter++) {
        if (equipment_ids[counter] == 0) break;
        char aux[12] = {};
        sprintf(aux, "%s ", get_number_as_string(equipment_ids[counter]));
        strcat(message, aux);
    }
    strcat(message, "\n");
    printf("Listing equipments: %s", message);
    send(client_socket, message, strlen(message) + 1, 0);
    close(client_socket);
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
    return atoi(first_word);
}

int get_inserted_id(char buffer[]) {
    char *value;
    char buffer_copy[BUFFER_SIZE_IN_BYTES] = {};
    strcpy(buffer_copy, buffer);
    printf("Buffer: %s", buffer_copy);
    value = strtok(buffer_copy, " ");
    value = strtok(NULL, " ");
    value = strtok(NULL, " ");
    printf("Generated Id: %s\n", value);
    return atoi(value);
}

int get_remove_id_from_remove_request(char buffer[]) {
    char *value;
    char buffer_copy[BUFFER_SIZE_IN_BYTES] = {};
    strcpy(buffer_copy, buffer);
    printf("Buffer: %s", buffer_copy);
    value = strtok(buffer_copy, " ");
    value = strtok(NULL, " ");
    printf("Will remove Id: %s\n", value);
    return atoi(value);
}

int get_information_id_from_list_information_server_message(char buffer[]) {
    char *value;
    char buffer_copy[BUFFER_SIZE_IN_BYTES] = {};
    strcpy(buffer_copy, buffer);
    printf("Buffer: %s", buffer_copy);
    value = strtok(buffer_copy, " ");
    value = strtok(NULL, " ");
    value = strtok(NULL, " ");
    printf("Will remove Id: %s\n", value);
    return atoi(value);
}


int get_id_from_list_information_client_message(char buffer[]) {
    char *value;
    char buffer_copy[BUFFER_SIZE_IN_BYTES] = {};
    strcpy(buffer_copy, buffer);
    printf("Buffer: %s", buffer_copy);
    value = strtok(buffer_copy, " ");
    value = strtok(NULL, " ");
    value = strtok(NULL, " ");
    value = strtok(NULL, " ");
    printf("Will list information from Id: %s\n", value);
    return atoi(value);
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

void handle_request_information(int id, int target_id, int client_socket) {
    char message[BUFFER_SIZE_IN_BYTES] = "";
    float random_value = generate_random_number();
    printf("Equipment %s will list information of equipment %s\n", get_number_as_string(id),
           get_number_as_string(target_id));
    sprintf(message, "%s %s %s %.2f\n", get_number_as_string(GET_EQUIPMENT_INFORMATION_RESPONSE),
            get_number_as_string(id), get_number_as_string(target_id), random_value);
    send(client_socket, message, strlen(message) + 1, 0);
    close(client_socket);
}

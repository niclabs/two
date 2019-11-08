#include "net.h"

#include "sock.h"
#include "logging.h"


/*
 * Per-client struct.
 */
typedef struct net_client {
    uint8_t *buf_in_data;               // In buffer's memory
    uint8_t *buf_out_data;              // Out buffer's memory
    cbuf_t buf_in[1];                   // Circular buffer in
    cbuf_t buf_out[1];                  // Circular buffer out
    void *state;                        // The client's state
    sock_t socket;                      // The client's socket
    callback_t cb;                      // Next callback to execute
    struct net_client *next;
} net_client_t;

/*
 * Resets a client to it's default values.
 */
void reset_client(net_client_t *p_client, size_t data_buffer_size, size_t client_state_size)
{
    memset(p_client->buf_in_data, 0, data_buffer_size);
    cbuf_init(p_client->buf_in, p_client->buf_in_data, data_buffer_size);
    memset(p_client->buf_out_data, 0, data_buffer_size);
    cbuf_init(p_client->buf_out, p_client->buf_out_data, data_buffer_size);

    memset(p_client->state, 0, client_state_size);
    memset(&p_client->socket, 0, sizeof(sock_t));
    p_client->cb.func = NULL;
    p_client->cb.debug_info = NULL;

    p_client->next = NULL;

    return;
}

/*
 * Reads from a client's socket into it's in circular buffer,
 * if data is available.
 */
net_return_code_t read_from_socket(net_client_t *p_client, unsigned int available_data)
{
    unsigned int available_in_space = cbuf_maxlen(p_client->buf_in) - cbuf_len(p_client->buf_in);
    unsigned int readable_data = (available_data <= available_in_space) ? available_data : available_in_space;

    if (readable_data > 0) {
        uint8_t buf_aux[readable_data];

        // Data is read
        int read_rc = sock_read(&p_client->socket, buf_aux, readable_data);
        if (read_rc < 0) {
            ERROR("Error in reading from socket");
            return NET_READ_ERROR;
        }
        DEBUG("Read %i bytes from socket", read_rc);

        cbuf_push(p_client->buf_in, buf_aux, read_rc);
    }
    else {
        WARN("Buffer in is full for client");
    }

    return NET_OK;
}

/*
 * Writes into a client's socket from it's out circular buffer,
 * if data is available.
 */
net_return_code_t write_to_socket(net_client_t *p_client)
{
    unsigned int writable_data = cbuf_len(p_client->buf_out);

    // Data is written if available
    if (writable_data > 0) {
        uint8_t buf_aux[writable_data];

        cbuf_peek(p_client->buf_out, buf_aux, writable_data);

        int write_rc = sock_write(&p_client->socket, buf_aux, writable_data);
        if (write_rc < 0) {
            ERROR("Error in writing to socket");
            return NET_WRITE_ERROR;
        }

        if ((unsigned int)write_rc < writable_data) {
            WARN("Could only write %i out of %i bytes into socket", write_rc, writable_data);
        }
        else {
            DEBUG("Wrote %i bytes into socket", write_rc);
        }

        cbuf_pop(p_client->buf_out, buf_aux, write_rc);
    }

    return NET_OK;
}

/*
 * Calls a client's default callback on a new connection.
 */
void on_new_client(net_client_t *p_client, callback_t default_callback)
{
    callback_t cb = default_callback.func(p_client->buf_in, p_client->buf_out, p_client->state);

    // Replaces callback
    p_client->cb = cb;
}

/*
 * Reads data from a client's socket and executes it's callback.
 */
net_return_code_t on_new_data(net_client_t *p_client, unsigned int data_size)
{
    net_return_code_t rc = NET_OK;

    // Reads from the socket into the client's buffers
    DEBUG("Received data from client");
    rc = read_from_socket(p_client, data_size);

    // The callback does stuff
    callback_t cb = p_client->cb.func(p_client->buf_in, p_client->buf_out, p_client->state);

    // Callback is replaced
    p_client->cb = cb;

    return rc;
}

/*
 * Checks if a client needs to be disconnected.
 *
 * If it is, the socket disconnects and the memory resets.
 */
net_return_code_t disconnect_client(net_client_t *p_client, size_t data_buffer_size, size_t client_state_size)
{
    net_return_code_t rc = NET_OK;

    // If client should be disconnected

    if (sock_destroy(&p_client->socket) < 0) {
        rc = NET_SOCKET_ERROR;
    }
    DEBUG("Client disconnected");

    reset_client(p_client, data_buffer_size, client_state_size);

    return rc;
}

net_return_code_t net_server_loop(unsigned int port, callback_t default_callback, int *stop_flag, size_t data_buffer_size, size_t client_state_size)
{
    // Socket error return codes go here
    int sock_rc = 0;

    INFO("Initializing server");

    // Allocation ofr memory for the clients
    net_client_t clients[NET_MAX_CLIENTS];
    uint8_t buffers_in[NET_MAX_CLIENTS][data_buffer_size];
    uint8_t buffers_out[NET_MAX_CLIENTS][data_buffer_size];
    uint8_t states[NET_MAX_CLIENTS][client_state_size];

    // Client initialization
    for (unsigned int i = 0; i < NET_MAX_CLIENTS; i++) {
        net_client_t *p_client = clients + i;

        p_client->buf_in_data = buffers_in[i];
        p_client->buf_out_data = buffers_out[i];
        p_client->state = states[i];

        reset_client(p_client, data_buffer_size, client_state_size);

        if (i + 1 != NET_MAX_CLIENTS) {
            p_client->next = &clients[i + 1];
        }
    }

    // Server socket setup
    sock_t sock_mem;
    sock_t *server_socket = &sock_mem;
    sock_rc = sock_create(server_socket);
    if (sock_rc != 0) {
        ERROR("Server socket couldn't be created");
        return NET_SOCKET_ERROR;
    }
    sock_rc = sock_listen(server_socket, port);
    if (sock_rc != 0) {
        ERROR("Server socket couldn't be set to listen");
        return NET_SOCKET_ERROR;
    }

    // Function return code
    net_return_code_t rc = NET_OK;

    net_client_t *first_available = clients;
    net_client_t *first_connected = NULL;

    // Main loop
    while (!*stop_flag) {

        net_client_t *prev_client = first_connected;
        net_client_t *curr_client = prev_client;

        while (curr_client != NULL) {

            unsigned int available_data = 0;

            if ((available_data = sock_poll(&curr_client->socket)) != 0) {
                rc = on_new_data(curr_client, available_data);

                if (rc != NET_OK) {
                    break;
                }
            }

            // Writes to the socket from the client's buffers
            //DEBUG("Writing data for client");
            rc = write_to_socket(curr_client);
            if (rc != NET_OK) {
                break;
            }

            // If client needs to be disconnected
            if (curr_client->cb.func == NULL) {

                net_client_t *next = curr_client->next;

                prev_client->next = next;

                rc = disconnect_client(curr_client, data_buffer_size, client_state_size);
                if (rc != NET_OK) {
                    break;
                }

                curr_client->next = first_available;
                first_available = curr_client;

                curr_client = next;
                continue;
            }

            // Next iteration
            prev_client = curr_client;
            curr_client = curr_client->next;
        }

        if (first_available != NULL) {

            // Accept any clients left waiting by the OS
            sock_rc = sock_accept(server_socket, &first_available->socket);
            if (sock_rc < 0) {
                ERROR("Error in accepting a client");
                rc = NET_SOCKET_ERROR;
                break;
            }

            // If one was accepted, set it's callback
            if (sock_rc > 0) {
                DEBUG("New client connection");

                on_new_client(first_available, default_callback);

                // If client needs to be disconnected
                if (first_available->cb.func == NULL) {
                    rc = disconnect_client(first_available, data_buffer_size, client_state_size);
                    if (rc != NET_OK) {
                        break;
                    }
                    continue;
                }

                net_client_t *next = first_available->next;

                first_available->next = first_connected;

                first_connected = first_available;
                first_available = next;
            }
        }

        if (rc == NET_SOCKET_ERROR || rc == NET_READ_ERROR || rc == NET_WRITE_ERROR) {
            break;
        }
    }

    // For every client
    for (unsigned int i = 0; i < NET_MAX_CLIENTS; i++) {
        net_client_t *p_client = clients + i;

        if (p_client->cb.func != NULL) {
            INFO("Disconnecting client from slot %i", i);
            sock_rc = sock_destroy(&p_client->socket);
            if (sock_rc < 0) {
                ERROR("Error in destroying socket");
                rc = NET_SOCKET_ERROR;
            }
            INFO("Client disconnected");

            reset_client(p_client, data_buffer_size, client_state_size);
        }
    }

    return rc;

}

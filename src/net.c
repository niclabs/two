#include "net.h"

#include "sock_non_blocking.h"
#include "logging.h"


/*
* Per-client struct.
*/
typedef struct {
    uint8_t* buf_in_data;             // In buffer's memory
    uint8_t* buf_out_data;            // Out buffer's memory
    cbuf_t buf_in[1];                       // Circular buffer in
    cbuf_t buf_out[1];                      // Circular buffer out
    void* state;                            // The client's state
    sock_t* socket;                         // The client's socket
    callback_t cb;                        // Next callback to execute
} Client;

/*
* Resets a client to it's default values.
*/
void reset_client(Client* p_client, size_t data_buffer_size, size_t client_state_size)
{
    memset(p_client->buf_in_data, 0, data_buffer_size);
    cbuf_init(p_client->buf_in, p_client->buf_in_data, data_buffer_size);
    memset(p_client->buf_out_data, 0, data_buffer_size);
    cbuf_init(p_client->buf_out, p_client->buf_out_data, data_buffer_size);

    memset(p_client->state, 0, client_state_size);

    p_client->socket = NULL;
    p_client->cb.func = NULL;
    p_client->cb.debug_info = NULL;

    return;
}

/*
 * Reads from a client's socket into it's in circular buffer,
 * if data is available.
 */
NetReturnCode read_from_socket(Client* p_client, unsigned int available_data)
{
    unsigned int available_in_space = cbuf_maxlen(p_client->buf_in)-cbuf_len(p_client->buf_in);
    unsigned int readable_data = (available_data <= available_in_space) ? available_data : available_in_space;

    if (readable_data > 0)
    {
        uint8_t buf_aux[readable_data];

        // Data is read
        int read_rc = sock_read(p_client->socket, buf_aux, readable_data);
        if (read_rc < 0)
        {
            ERROR("Error in reading from socket");
            return ReadError;
        }
        DEBUG("Read %i bytes from socket", read_rc);

        cbuf_push(p_client->buf_in, buf_aux, read_rc);
    }
    else
    {
        WARN("Buffer in is full for client");
    }

    return Ok;
}

/*
 * Writes into a client's socket from it's out circular buffer,
 * if data is available.
 */
NetReturnCode write_to_socket(Client* p_client)
{
    unsigned int writable_data = cbuf_len(p_client->buf_out);

    // Data is written if available
    if (writable_data > 0)
    {
        uint8_t buf_aux[writable_data];

        cbuf_peek(p_client->buf_out, buf_aux, writable_data);

        int write_rc = sock_write(p_client->socket, buf_aux, writable_data);
        if (write_rc < 0)
        {
            ERROR("Error in writing to socket");
            return WriteError;
        }

        if ((unsigned int)write_rc < writable_data)
        {
            WARN("Could only write %i out of %i bytes into socket", write_rc, writable_data);
        }
        else
        {
            DEBUG("Wrote %i bytes into socket", write_rc);
        }

        cbuf_pop(p_client->buf_out, buf_aux, write_rc);
    }

    return Ok;
}

NetReturnCode net_server_loop(unsigned int port, callback_t default_callback, int* stop_flag, size_t data_buffer_size, size_t client_state_size)
{
    // Socket error return codes go here
    int sock_rc = 0;
    INFO("Initializing server");

    // Allocation ofr memory for the clients
    Client clients[NET_MAX_CLIENTS];
    uint8_t buffers_in[NET_MAX_CLIENTS][data_buffer_size];
    uint8_t buffers_out[NET_MAX_CLIENTS][data_buffer_size];
    uint8_t states[NET_MAX_CLIENTS][client_state_size];

    // Client initialization
    for (unsigned int i = 0; i < NET_MAX_CLIENTS; i++)
    {
        Client* p_client = clients+i;

        p_client->buf_in_data = buffers_in[i];
        p_client->buf_out_data = buffers_out[i];
        p_client->state = states[i];

        reset_client(p_client, data_buffer_size, client_state_size);
    }

    // Server socket setup
    sock_t* server_socket = NULL;
    sock_rc = sock_create(server_socket);
    if (sock_rc != 0)
    {
        ERROR("Server socket couldn't be created");
        return SocketError;
    }
    sock_rc = sock_listen(server_socket, port);
    if (sock_rc != 0)
    {
        ERROR("Server socket couldn't be set to listen");
        return SocketError;
    }

    // Function return code
    NetReturnCode rc = Ok;

    // Main loop
    while (!*stop_flag)
    {
        // For every client
        for (unsigned int i = 0; i < NET_MAX_CLIENTS; i++)
        {
            Client* p_client = clients+i;

            DEBUG("Writing data for client %i", i);
            rc = write_to_socket(p_client);

            if (rc != Ok)
                break;

            unsigned int available_data = 0;

            // If the client is connected and has data
            if (p_client->cb.func != NULL && (available_data=sock_poll(p_client->socket)) != 0)
            {
                // Reads from the socket into the client's buffers
                DEBUG("Received data from client %i", i);
                rc = read_from_socket(p_client, available_data);
                if (rc != Ok)
                    break;

                // The callback does stuff
                DEBUG("Executing callback");
                callback_t cb = p_client->cb.func(p_client->buf_in, p_client->buf_out, p_client->state);

                // Writes to the socket from the client's buffers
                DEBUG("Writing data for client %i", i);
                rc = write_to_socket(p_client);
                if (rc != Ok)
                    break;

                // If client should be disconnected
                if (cb.func == NULL)
                {
                    INFO("Disconnecting client from slot %i", i);
                    sock_rc = sock_destroy(p_client->socket);
                    if (sock_rc < 0)
                    {
                        ERROR("Error in destroying socket");
                        rc = SocketError;
                        break;
                    }
                    INFO("Client disconnected");

                    reset_client(p_client, data_buffer_size, client_state_size);
                }

                // Callback is replaced
                p_client->cb = cb;
            }
            // If the client slot is available
            else
            {
                // Accept any clients left waiting by the OS
                sock_rc = sock_accept(server_socket, p_client->socket);
                if (sock_rc < 0)
                {
                    ERROR("Error in accepting a client");
                    rc = SocketError;
                    break;
                }

                // If one was accepted, set it's callback
                if (sock_rc > 0)
                {
                    INFO("Client connected on slot %i", i);

                    // The default callback does stuff
                    DEBUG("Executing callback");
                    callback_t cb = default_callback.func(p_client->buf_in, p_client->buf_out, p_client->state);

                    // Writes to the socket from the client's buffers
                    DEBUG("Writing data for client %i", i);
                    rc = write_to_socket(p_client);
                    if (rc != Ok)
                        break;

                    // If client should be disconnected
                    if (cb.func == NULL)
                    {
                        INFO("Disconnecting client from slot %i", i);
                        sock_rc = sock_destroy(p_client->socket);
                        if (sock_rc < 0)
                        {
                            ERROR("Error in destroying socket");
                            rc = SocketError;
                            break;
                        }
                        INFO("Client disconnected");

                        reset_client(p_client, data_buffer_size, client_state_size);
                    }

                    // Callback is replaced
                    p_client->cb = cb;
                }
            }

            if (*stop_flag)
                break;
        }
    }

    // For every client
    for (unsigned int i = 0; i < NET_MAX_CLIENTS; i++)
    {
        Client* p_client = clients+i;

        if (p_client->cb.func != NULL)
        {
            INFO("Disconnecting client from slot %i", i);
            sock_rc = sock_destroy(p_client->socket);
            if (sock_rc < 0)
            {
                ERROR("Error in destroying socket");
                rc = SocketError;
            }
            INFO("Client disconnected");

            reset_client(p_client, data_buffer_size, client_state_size);
        }
    }

    return rc;

}

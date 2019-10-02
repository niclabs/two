#include "net.h"

#include "sock_non_blocking.h"
#include "logging.h"


/*
* Per-client struct.
*/
typedef struct {
    unsigned char* buf_in_data;             // In buffer's memory
    unsigned char* buf_out_data;            // Out buffer's memory
    cbuf_t buf_in[1];                       // Circular buffer in
    cbuf_t buf_out[1];                      // Circular buffer out
    void* state;                            // The client's state
    sock_t* socket;                         // The client's socket
    net_Callback cb;                        // Next callback to execute
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
    p_client->cb = NULL;
}

/*
 * Reads from a client's socket into it's in circular buffer, 
 * if data is available.
 */
NetReturnCode read_from_socket(Client* p_client, int available_data)
{
    unsigned int available_in_space = cbuf_maxlen(p_client->buf_in)-cbuf_len(p_client->buf_in);
    unsigned int readable_data = (available_data <= available_in_space) ? available_data : available_in_space;

    if (readable_data > 0)
    {
        unsigned char buf_aux[readable_data];

        // Data is read
        DEBUG("Received data from client %i", i);
        int read_rc = sock_read(p_client->socket, buf_aux, readable_data);
        if (read_rc < 0)
        {
            ERROR("Error in reading from socket\n");
            return ReadError;
        }
        DEBUG("Read %i bytes from socket", read_rc);

        cbuf_push(p_client->buf_in, buf_aux, read_rc);
    }
    else
    {
        WARN("Buffer in is full for client %i", i);
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
        unsigned char buf_aux[writable_data];

        cbuf_peek(p_client->buf_out, buf_aux, writable_data);

        DEBUG("Writing data for client %i", i);
        int write_rc = sock_write(p_client->socket, buf_aux, writable_data);
        if (write_rc < 0)
        {
            ERROR("Error in writing to socket\n");
            return WriteError;
        }

        if (write_rc < writable_data)
            WARN("Could only write %i out of %i bytes into socket", write_rc, writable_data);
        else
            DEBUG("Wrote %i bytes into socket", write_rc);

        cbuf_pop(p_client->buf_out, buf_aux, write_rc);
    }

    return Ok;
}

NetReturnCode net_server_loop(unsigned int port, net_Callback default_callback, int* stop_flag, size_t data_buffer_size, size_t client_state_size)
{
    // Socket error return codes go here
    int sock_rc = 0;
    INFO("Initializing server");

    // Allocation ofr memory for the clients
    Client clients[NET_MAX_CLIENTS];
    unsigned char buffers_in[NET_MAX_CLIENTS][data_buffer_size];
    unsigned char buffers_out[NET_MAX_CLIENTS][data_buffer_size];
    unsigned char states[NET_MAX_CLIENTS][client_state_size];

    // Client initialization
    for (unsigned int i = 0; i < NET_MAX_CLIENTS; i++)
    {
        Client client = clients[i];
        
        client.buf_in_data = buffers_in[i];
        client.buf_out_data = buffers_out[i];
        client.state = states[i];

        reset_client(&client, data_buffer_size, client_state_size);
    }

    // Server socket setup
    sock_t* server_socket;
    sock_rc = sock_create(server_socket);
    if (sock_rc != 0)
    {
        ERROR("Server socket couldn't be created\n");
        return SocketError;
    }
    sock_rc = sock_listen(server_socket, port);
    if (sock_rc != 0)
    {
        ERROR("Server socket couldn't be set to listen\n");
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
            Client client = clients[i];

            rc = write_to_socket(&client);
            if (rc != Ok)
                break;

            int available_data = 0;

            // If the client is connected and has data
            if (client.cb != NULL && (available_data=sock_poll(client.socket)) != 0)
            {
                // Reads from the socket into the client's buffers
                rc = read_from_socket(&client, available_data);
                if (rc != Ok)
                    break;

                // The callback does stuff
                DEBUG("Executing callback");
                net_Callback cb = (net_Callback) client.cb(client.buf_in, client.buf_out, client.state);

                // Writes to the socket from the client's buffers
                rc = write_to_socket(&client);
                if (rc != Ok)
                    break;

                // If client should be disconnected
                if (cb == NULL)
                {
                    INFO("Disconnecting client from slot %i\n", i);
                    sock_rc = sock_destroy(client.socket);
                    if (sock_rc < 0)
                    {
                        ERROR("Error in destroying socket\n");
                        rc = SocketError;
                        break;
                    }
                    INFO("Client disconnected");

                    reset_client(&client, data_buffer_size, client_state_size);
                }

                // Callback is replaced
                client.cb = cb;
            }
            // If the client slot is available
            else
            {
                // Accept any clients left waiting by the OS
                sock_rc = sock_accept(server_socket, client.socket);
                if (sock_rc < 0)
                {
                    ERROR("Error in accepting a client\n");
                    rc = SocketError;
                    break;
                }

                // If one was accepted, set it's callback
                if (sock_rc > 0)
                {
                    INFO("Client connected on slot %i\n", i);

                    // The default callback does stuff
                    DEBUG("Executing callback");
                    net_Callback cb = (net_Callback) default_callback(client.buf_in, client.buf_out, client.state);

                    // Writes to the socket from the client's buffers
                    rc = write_to_socket(&client);
                    if (rc != Ok)
                        break;

                    // If client should be disconnected
                    if (cb == NULL)
                    {
                        INFO("Disconnecting client from slot %i\n", i);
                        sock_rc = sock_destroy(client.socket);
                        if (sock_rc < 0)
                        {
                            ERROR("Error in destroying socket\n");
                            rc = SocketError;
                            break;
                        }
                        INFO("Client disconnected");

                        reset_client(&client, data_buffer_size, client_state_size);
                    }

                    // Callback is replaced
                    client.cb = cb;
                }
            }
        }
    }

    // For every client
    for (unsigned int i = 0; i < NET_MAX_CLIENTS; i++)
    {
        Client client = clients[i];

        if (client.cb != NULL)
        {
            INFO("Disconnecting client from slot %i\n", i);
            sock_rc = sock_destroy(client.socket);
            if (sock_rc < 0)
            {
                ERROR("Error in destroying socket\n");
                rc = SocketError;
            }
            INFO("Client disconnected");

            clean_client(&client);
        }
    }

    return rc;

}
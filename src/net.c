#include "net.h"
#include "logging.h"

/*
* Union for abstracting the client's state.
*/
union client_state {
    void* ptr;                                  // Pointer to give the callback
    unsigned char data[NET_CLIENT_STATE_SIZE]   // Data allocation
};

/*
* Per-client struct.
*/
typedef struct {
    char buf_in[NET_CLIENT_BUFFER_SIZE];    // Input buffer
    char buf_out[NET_CLIENT_BUFFER_SIZE];   // Output buffer
    unsigned int write_out[1];              // Amount to write from buf_out
    union client_state state;               // The client's state
    sock_t* socket;                         // The client's socket
    net_Callback cb;                        // Next callback to execute
} Client;

/*
* Resets a client to it's default values.
*/
void clean_client(Client* p_client)
{
    Client client = *p_client;
    memset(p_client->buf_in, 0, NET_CLIENT_BUFFER_SIZE);
    memset(p_client->buf_out, 0, NET_CLIENT_BUFFER_SIZE);
    memset(p_client->state.ptr, 0, NET_CLIENT_STATE_SIZE);
    p_client->socket = NULL;
    p_client->cb = NULL;
}

/*
* Resets an array of clients to it's default values.
*/
void init_clients(Client* clients)
{
    for (unsigned int i = 0; i < NET_MAX_CLIENTS; i++)
    {
        clean_client(&clients[i]);
    }
}

NetReturnCode net_server_loop(uint16_t port, net_Callback default_callback, int* stop_flag)
{
    // Socket error return codes go here
    int sock_rc = 0;
    INFO("Initializing server");

    // Client array initialization
    Client clients[NET_MAX_CLIENTS];
    init_clients(clients);

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

            // If the client is connected and has data
            if (client.cb != NULL && (sock_poll(client.socket) || *client.write_out != 0))
            {
                // TODO: How much data should we read at a time? It alse needs to be appended to the existing data
                // Data is read
                DEBUG("Received data from client %i", i);
                int read_rc = sock_read(client.socket, client.buf_in, NET_CLIENT_BUFFER_SIZE);
                if (read_rc < 0)
                {
                    ERROR("Error in reading from socket\n");
                    rc = ReadError;
                    break;
                }
                DEBUG("Read %i bytes from socket", read_rc);

                // The callback does stuff
                DEBUG("Executing callback");
                net_Callback cb = client.cb(client.buf_in, client.buf_out, client.write_out, client.state.ptr);

                // Data is written if available
                if (*client.write_out > 0)
                {
                    DEBUG("Writing data for client %i", i);
                    int write_rc = sock_write(client.socket, client.buf_out, *client.write_out);
                    if (write_rc < 0)
                    {
                        ERROR("Error in writing to socket\n");
                        rc = WriteError;
                        break;
                    }
                    *client.write_out = *client.write_out - write_rc;
                    DEBUG("Wrote %i bytes into socket", write_rc);
                }

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

                    clean_client(&client);
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
                    client.cb = default_callback;
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
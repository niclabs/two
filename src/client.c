#include "client.h"
#include "logging.h"

void pseudoclient(hstates_t *hs, uint16_t port, char *ip)
{
    int rc = http_client_connect(hs, port, ip);

    if (rc < 0) {
        ERROR("in client connect");
    }
    else {
        response_received_type_t rr;
        rc = http_get(hs, "index", "example.org", "text", &rr);
        if (rc < 0) {
            ERROR("in http_get");
        }
        /*else {
            INFO("data received: %u ", rr.size_data);
            for (uint32_t i = 1; i <= rr.size_data; i++) {
                printf("%c", (char) rr.data[i]);

            }
            printf("\n");
        }
        */
        http_client_disconnect(hs);
    }
}

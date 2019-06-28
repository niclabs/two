#include "client.h"
#include "logging.h"

void pseudoclient(hstates_t *hs, uint16_t port, char *ip)
{
    int rc = http_client_connect(hs, port, ip);

    if (rc < 0) {
        ERROR("in client connect");
    }
    else {
        int index;
        rc = http_get(hs, "index", "example.org", "text", &index);
        if (rc < 0) {
            ERROR("in http_get");
        }
    }
}

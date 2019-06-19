#include "client.h"

void pseudoclient(hstates_t *hs, uint16_t port, char *ip){
  http_client_connect(hs,port, ip);
  http_get(hs, "index", "text");
}

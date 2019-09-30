/*
   This API contains the methods in HTTP layer
*/
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "headers.h"


/***********************************************
 * Data buffer struct
 ***********************************************/

//Revisar
 #ifdef HTTP_CONF_MAX_RESPONSE_SIZE
 #define HTTP_MAX_RESPONSE_SIZE (HTTP_CONF_MAX_RESPONSE_SIZE)
 #else
 #define HTTP_MAX_RESPONSE_SIZE (128)
 #endif

 #ifdef HTTP_CONF_MAX_DATA_SIZE
 #define HTTP_MAX_DATA_SIZE (HTTP_CONF_MAX_DATA_SIZE)
 #else
 #define HTTP_MAX_DATA_SIZE (16384)
 #endif

typedef struct {
    uint32_t size;
    uint8_t buf[HTTP_MAX_DATA_SIZE];
} data_t;

/***********************************************
 * Aplication resources structs
 ***********************************************/

 #ifdef HTTP_CONF_MAX_RESOURCES
 #define HTTP_MAX_RESOURCES (HTTP_CONF_MAX_RESOURCES)
 #else
 #define HTTP_MAX_RESOURCES (16)
 #endif

 #ifdef HTTP_CONF_MAX_PATH_SIZE
 #define HTTP_MAX_PATH_SIZE (HTTP_CONF_MAX_PATH_SIZE)
 #else
 #define HTTP_MAX_PATH_SIZE (32)
 #endif

typedef int (*http_resource_handler_t) (char *method, char *uri, uint8_t *response, int maxlen);

typedef struct {
    char path[HTTP_MAX_PATH_SIZE];
    char method[8];
    http_resource_handler_t handler;
} http_resource_t;

typedef struct {
    http_resource_t resource_list[HTTP_MAX_RESOURCES];
    uint8_t resource_list_size;
} resource_list_t;


/***********************************************
 * Server API methods
 ***********************************************/

 /**
  * Generate a respnse from server to each client request
  *
  * @param data_buff data structure
  * @param headers_buff headers data structure
  * @return 0 if ok -1 if an error ocurred
  */
int http_server_response(data_t *data_buff, headers_t *headers_buff);


#endif /* HTTP_H */

/*
   This API contains the methods in Resource Manager layer
 */
#ifndef HTTP_H
#define HTTP_H

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
//

/***********************************************
* Server API methods
***********************************************/

/**
 * Generate a response from server to each client request
 *
 * @param data_buff data buffer
 * @param data_size data buffer size
 * @param headers_buff headers data structure
 * @return 0 if ok -1 if an error ocurred
 */
int http_server_response(uint8_t *data_buff, uint32_t *data_size, header_list_t *headers_buff);

/**
 * Utility function to check for method support
 *
 * @returns 1 if the method is supported by the implementation, 0 if not
 */
int http_has_method_support(char *method);


#endif /* HTTP_H */

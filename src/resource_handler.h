/*
   This API contains the methods in Resource Manager layer
 */
#ifndef RESOURCE_HANDLER_H
#define RESOURCE_HANDLER_H


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

/***********************************************
* API methods
***********************************************/

/**
 * Get a resource handler for the given path
 *
 * @return resource handler if ok, NULL if other case
 */
http_resource_handler_t resource_handler_get(char *method, char *path);

/**
 * Set callback to handle an http resource
 *
 * A resource is composed by a method and a path
 *
 * A path consists of a sequence of path segments separated by a slash
 * ("/") character.  A path is always defined for a URI, though the
 * defined path may be empty (zero length)
 * (More info in https://tools.ietf.org/html/rfc3986#section-3.3)
 *
 * For this function, the path must start with a '/'.
 *
 * Attempting to define a malformed path, or a path for an unsupported method
 * will result in an error return
 *
 * @return 0 if ok, -1 if error
 */
int resource_handler_set(char *method, char *path, http_resource_handler_t handler);


#endif /* RESOURCE_MANAGER_H */

/*
   This API contains the Resource Manager methods to be used by
   HTTP/2
 */

#include <errno.h>
#include <strings.h>
#include <stdio.h>

//#define LOG_LEVEL (LOG_LEVEL_DEBUG)

#include "resource_handler.h"
#include "http.h"
#include "logging.h"

#ifndef MIN
#define MIN(n, m)   (((n) < (m)) ? (n) : (m))
#endif

/***********************************************
* Aplication resources structs
***********************************************/

typedef struct {
    http_resource_t resource_list[HTTP_MAX_RESOURCES];
    uint8_t resource_list_size;
} resource_list_t;

static resource_list_t app_resources;

/***********************************************/


/*
 * Check for valid HTTP path according to
 * RFC 2396 (see https://tools.ietf.org/html/rfc2396#section-3.3)
 *
 * TODO: for now this function only checks that the path starts
 * by a '/'. Validity of the path should be implemented according to
 * the RFC
 *
 * @return 1 if the path is valid or 0 if not
 * */
int is_valid_path(char *path)
{
    if (path[0] != '/') {
        return 0;
    }
    return 1;
}


/**
 * Get a resource handler for the given path
 */
http_resource_handler_t resource_handler_get(char *method, char *path)
{
    http_resource_t res;

       for (int i = 0; i < app_resources.resource_list_size; i++) {
        res = app_resources.resource_list[i];
        if (strncmp(res.path, path, HTTP_MAX_PATH_SIZE) == 0 && strcmp(res.method, method) == 0) {
            return res.handler;
        }
       }

    return NULL;
}


int resource_handler_set(char *method, char *path, http_resource_handler_t handler)
{
    if (method == NULL || path == NULL || handler == NULL) {
        errno = EINVAL;
        ERROR("ERROR found %d", errno );
        return -1;
    }

    if (!http_has_method_support(method)) {
        errno = EINVAL;
        ERROR("Method %s not implemented yet", method);
        return -1;
    }

    if (!is_valid_path(path)) {
        errno = EINVAL;
        ERROR("Path %s does not have a valid format", path);
        return -1;
    }

    if (strlen(path) >= HTTP_MAX_PATH_SIZE) {
        errno = EINVAL;
        ERROR("Path length is larger than max supported size (%d). Try updating HTTP_CONF_MAX_PATH_SIZE", HTTP_MAX_PATH_SIZE);
        return -1;
    }

    // Checks if the app_resources variable is initialized
    static uint8_t inited_app_resources = 0;
    if(!inited_app_resources) {
      memset(&app_resources, 0, sizeof(resource_list_t));
      inited_app_resources = 1;
    }

    // Checks if the path and method already exist
    http_resource_t *res;
    for (int i = 0; i < app_resources.resource_list_size; i++) {
        res = &app_resources.resource_list[i];
        //If it does, replaces the resource
        if (strncmp(res->path, path, HTTP_MAX_PATH_SIZE) == 0 && strcmp(res->method, method) == 0) {
            res->handler = handler;
            return 0;
        }
    }

    // Checks if the list is full
    if (app_resources.resource_list_size >= HTTP_MAX_RESOURCES) {
        ERROR("HTTP resource limit (%d) reached. Try changing value for HTTP_CONF_MAX_RESOURCES", HTTP_MAX_RESOURCES);
        return -1;
    }

    // Adds the resource to the list
    res = &app_resources.resource_list[app_resources.resource_list_size++];

    // Sets values
    strncpy(res->method, method, 8);
    strncpy(res->path, path, HTTP_MAX_PATH_SIZE);
    res->handler = handler;

    return 0;
}

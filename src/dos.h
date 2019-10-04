
#ifndef DOS_H
#define DOS_H

#include <stdint.h>

#include "resource_manager.h"



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
int two_register_resource(char *method, char *path, http_resource_handler_t handler);


#endif /* DOS_H */

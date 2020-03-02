#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/**
 * Disable UDP
 */
#define UIP_CONF_UDP (0)

/**
 * Enable TCP
 */
#define UIP_CONF_TCP (1)

/**
 * Not enough memory in contiki for multiple clients
 */
#define CONFIG_HTTP2_MAX_CLIENTS (3)

/**
 * Disable hpack dynamic table by default
 */
#define CONFIG_HTTP2_HEADER_TABLE_SIZE (0)

#endif

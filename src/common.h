/*
 * Copyright (C) 2016 Martine Lenders <mlenders@inf.fu-berlin.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief   Definitions for tests/lwip/
 *
 * @author  Martine Lenders <mlenders@inf.fu-berlin.de>
 */
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief   Application configuration
 * @{
 */
#define SOCK_BUF_SIZE (256)
#define SOCK_QUEUE_LEN (10)
#define SERVER_BUFFER_SIZE (64)
    /**
 * @}
 */

#ifdef MODULE_SOCK_TCP
    /**
 * @brief   Echo server shell command
 *
 * @param[in] argc  number of arguments
 * @param[in] argv  array of arguments
 *
 * @return  0 on success
 * @return  other on error
 */
    int echo_server(int argc, char **argv);

    /**
 * @brief   Echo client shell command
 *
 * @param[in] argc  number of arguments
 * @param[in] argv  array of arguments
 *
 * @return  0 on success
 * @return  other on error
 */
    int echo_client(int argc, char **argv);
#endif

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */
/** @} */

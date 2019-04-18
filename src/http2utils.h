#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "http2api.h"


int verify_setting(uint16_t id, uint32_t value);
int read_n_bytes(uint8_t *buff_read, int n);
int tcp_write(uint8_t *bytes, uint8_t length);//here just for the code to compile
int tcp_read(uint8_t *bytes, uint8_t length);//here just for the code to compile

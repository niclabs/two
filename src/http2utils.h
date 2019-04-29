#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "http2.h"


int verify_setting(uint16_t id, uint32_t value);
int read_n_bytes(uint8_t *buff_read, int n);
int http_write(uint8_t *bytes, int length);//here just for the code to compile
int http_read(uint8_t *bytes, int length);//here just for the code to compile

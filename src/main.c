
//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
//#include <stdint.h>
//#include <stdbool.h> 
#include "frametests.h"

#include <errno.h>
#include <stdio.h>
#include "shell.h"



static const shell_command_t shell_commands[] = {
    {"frameheadertests", "Frame Header Tests", frame_header_encode_decode_test},
    {"settingstests", "Settings Tests", setting_encode_decode_test},
    {"frametests", "Frame Tests", frame_encode_decode_test},
    {NULL, NULL, NULL}};
static char line_buf[SHELL_DEFAULT_BUFSIZE];
int main(void) {
    
   /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */

    puts("RIOT lwip test application");
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}

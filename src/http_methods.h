/*
This API contains the HTTP methods to be used by
HTTP/2
*/

/*Server*/
int init_server(uint16_t port);

int set_function_to_path(char * fun_name, char * path);

int recieve(char * headers);

int http_write(char * buf, int len);

int http_read(char * buf, int len);

/*Client*/
int client_connect(uint16_t port, char * ip);

int client_disconnect(void);

int get(char * path, char * headers);

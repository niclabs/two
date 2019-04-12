/*
This API contains the HTTP methods to be used by
HTTP/2
*/

//TODO: establecer variables globales

/*******************************Server*******************************/

/*
Given a port number this function initialize a server
Input: port number
Output: 0 if the action was successful, -1 if not
*/
int init_server(uint16_t port){
  /*iniciar con sock_create, luego
  abrir listen infinito con sock_listen*/
  return -1;
}

/*
Set an internal server function to a specific path
Input: function name
       specific path
Output: 0 if the action was successful, -1 if not
*/
int set_function_to_path(char * fun_name, char * path){
  /*manejar errores*/
  return -1
}

/*
Given the content of the request made by the client, this function calls
the functions necessary to respond to the request
Input: Encoded request
Output: 0 if the action was successful, -1 if not
*/
int recieve(char * headers){
  return -1;
}

/*

Input: path where the function is

Output: 0 if the action was successful, -1 if not
*/
int get_receive(char * path, char * headers){
  // buscar respuesta correspondiente a path
  // codificar respuesta
  // enviar respuesta a socket
  return -1;
}

/*
This function write in the socket with the client
Input: buffer with the data to writte
Output: 0 if the action was successful, -1 if not
*/
int http_write(char * buf, int len){
  /*sock_write*/
  return -1;
}

/*
This function read the data from the socket with the client
Input: buffer where the data will be write
Output: 0 if the action was successful, -1 if not
*/
int http_read(char * buf, int len){
  /*sock_read*/
  return -1;
}


/*******************************Client*******************************/

/*
This function start a connection from client to server
Intput: port number
        ip adress
Output: 0 if the connection was successful,-1 if not
*/
int client_connect(uint16_t port, char * ip){
  return -1;
}

/*
This function stop the connection between client and server
Output: 0 if the action was successful, -1 if not
*/
int client_disconnect(void){
  return -1;
}

/*

Input:
Output:
*/
int get(char * path, char * headers){
  return -1/*funcion de capa inferior*/;
}

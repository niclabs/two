# HTTP/2 basic server on linux

This folder contains a basic server example for running on linux targets. 

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 

## Dependencies

See [main README](https://github.com/niclabs/two#dependencies)


## Running the example

Clone the repo:
```{bash}
$ git clone https://github.com/niclabs/two.git
```

Compile the example
```{bash}
$ cd two
$ make
```

Run the server in the port 8888
```{bash}
$ bin/basic 8888
[INFO] Starting HTTP/2 server in port 888
```

Test the connection using curl
```{bash}
$ curl --http2-prior-knowledge http://localhost:8888
Hello, World!!!
```

# two: HTTP/2 for constrained IoT devices

Experimental HTTP/2 server and library, aimed at use in [constrained devices](https://tools.ietf.org/html/rfc7228). 
The implementation uses a single thread for handling clients, and each client requires around 1.5K of static RAM 
in the base configuration (without [HPACK dynamic table](https://httpwg.org/specs/rfc7541.html#dynamic.table)). 
This can be further reduced through [configuration macros](#configuration-macros).

## Project Status

| branch | build status | coverage |
|--------|--------------|----------|
| master | [![Build Status](https://travis-ci.com/niclabs/two.svg?branch=master)](https://travis-ci.com/niclabs/two) | [![codecov](https://codecov.io/gh/niclabs/two/branch/master/graph/badge.svg)](https://codecov.io/gh/niclabs/two) |
| develop | [![Build Status](https://travis-ci.com/niclabs/two.svg?branch=develop)](https://travis-ci.com/niclabs/two) | [![codecov](https://codecov.io/gh/niclabs/two/branch/develop/graph/badge.svg)](https://codecov.io/gh/niclabs/two) |

## Features

* HTTP/2 spec conformant. It passes most [h2spec tests](https://github.com/summerwind/h2spec) within the limitations below (see [h2spec.conf](h2spec.conf) for more information).
* 1.5K of RAM needed per client, including input and output buffers (it can be reduced).
* Fully [configurable](#configuration-macros) using C language macros.
* Supports [HPACK compression](https://httpwg.org/specs/rfc7541.html).
* Compatible with the [Contiki NG](http://contiki-ng.org/) operating system.

## Limitations

* Only HTTP GET method is supported.
* [Prior HTTP/2 knowledge](https://httpwg.org/specs/rfc7540.html#known-http) in assumed by the server. Connection upgrade is not implemented for now.
* Single [HTTP/2 stream](https://httpwg.org/specs/rfc7540.html#StreamsLayer) support only. This also means no [stream priority](https://httpwg.org/specs/rfc7540.html#StreamPriority), and no [server push](https://httpwg.org/specs/rfc7540.html#PushResources).
* Maximum effective frame size is limited to 512 bytes by default (configurable). In practice, this only affects handling of [HEADERS](https://httpwg.org/specs/rfc7540.html#HEADERS) frames, since [DATA](https://httpwg.org/specs/rfc7540.html#DATA) frame size can be limited through the [flow control](https://httpwg.org/specs/rfc7540.html#FlowControl) window. Reception of a HEADERS frame larger than 512 bytes results in a [FLOW_CONTROL_ERROR](https://httpwg.org/specs/rfc7540.html#ErrorCodes) response.
* No HTTPS support (for now).

## Why

Standardization efforts with regards to the topic of the Internet of Things are very active accross the different layers of the network stack. 

On the application layer, MQTT and CoAP are IoT-specific protocols which are gaining in popularity on the constrained device space. 
However, being specific, interoperability with the rest of the Web requires the use of translation proxies and middle boxes. 
This not only presents an interoperability issue, but a security issue also, since end-to-end encryption becomes harder when intermediaries are
required.

Unlike HTTP/1.X, [HTTP/2 is suitable fo many IoT applications](https://www.ietf.org/archive/id/draft-montenegro-httpbis-h2ot-profile-00.txt).

* HTTP/2 is compact, configurable and flexible.
* It uses header compression and binary encoding of protocol messages
* It reduces connection establishement overhead, using a single connection for multiple messages
* Although the use of TLS is not mandatory with HTTP/2, the [industry is leaning towards requiring towards the use of HTTPS everywhere](https://daniel.haxx.se/blog/2015/03/06/tls-in-http2/), and this should become the norm in IoT as well.
* Even though HTTP/2 relies on TCP, which may be too demanding for battery powered devices, HTTP/3 will rely on QUIC (UDP), which may be a better fit.

We believe usage of HTTP/2 in constrained devices for Internet of Things applications should be further studied, however to date, no implementations are available for embedded devices.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 

### Dependencies

The project is designed to have no external dependencies other than gcc and libc, you will also need `make` to build, and [Contiki NG](https://github.com/contiki-ng/contiki-ng/wiki) if building for embedded targets. 
Internally and for testing in Travis, we use a [Docker image with embedded toolchain support](https://hub.docker.com/r/niclabs/embeddable) that you can install as well.

### Installing

Clone the repo:
```{bash}
$ git clone https://github.com/niclabs/two.git
```

If all your dependencies are working as intended, you can compile the library by running .
```{bash}
$ cd two
$ make
```

This will also compile a basic server executable (under `bin/`), implemented under `examples/basic/`. 
The following command will run the server in the port 8888
```{bash}
$ bin/basic 8888
[INFO] Starting HTTP/2 server in port 888
```

and it can be tested using curl
```{bash}
$ curl --http2-prior-knowledge http://localhost:8888
Hello, World!!!
```

### Compiling inside Docker

If you decided to use our docker image linked in the [dependencies](#dependencies) section, you will have to clone the repository first
```{bash}
$ git clone https://github.com/niclabs/two.git
```

Then go to the project's directory in the terminal and execute the `embeddable` command.
```{bash}
$ cd two
$ embeddable
```
You should see a `user@<docker hash>:work` prompt.


Then you can compile the project normally
```{bash}
user@hash:work $ make
```

### Building for embedded targets

See [contiki](examples/contiki/).

## Testing

To run the automated unit tests for the project you can do:
```{bash}
make distclean && make test
```

The output should end with an `OK`.

If you have the [h2spec](https://github.com/summerwind/h2spec) tool installed (or are running inside docker) you can run conformance tests using
```{bash}
make h2spec
```

## Configuration macros

Configuration macros for the server are defined in [two-conf.h](src/two-conf.h). To override, you can define a new header file "my-config.h"
and addding `-DPROJECT_CONF_H=\"my-config.h\"` to compilation CFLAGS
```{bash}
$ cd two
$ CFLAGS=-DPROJECT_CONF_H=\"my-config.h\" make
```

The following configuration macros are defined
* `CONFIG_HTTP2_HEADER_TABLE_SIZE`, maximum value for the dynamic hpack header table. Setting this to zero disables use of the dynamic table for HPACK. This setting affects the size of static memory used by client.
* `CONFIG_HTTP2_MAX_CONCURRENT_STREAMS`, maximum number of concurrent streams alloed by HTTP/2. It cannot be larger than 0.
* `CONFIG_HTTP2_INITIAL_WINDOW_SIZE`, initial value for HTTP/2 window size. This value cannot be larger than the read buffer size.
* `CONFIG_HTTP2_MAX_FRAME_SIZE`, initial value for SETTINGS_MAX_FRAME_SIZE. It has no effect on the size of the allocation buffers, the effective max frame size is given by the setting `CONFIG_HTTP2_SOCK_READ_SIZE`.
* `CONFIG_HTTP2_MAX_HEADER_LIST_SIZE`, initial value for SETTINGS_MAX_HEADER_LIST_SIZE. It effectively sets the maximum number of decompressed bytes for the header list (see [header_list](src/header_list.h)). This setting has no impact on the static memory used by the implementation, however the value must be chosen carefully, since it have an effect on the stack size.
* `CONFIG_HTTP2_SETTINGS_WAIT`, maximum time in milliseconds tom wait for the remote endpoint to reply to a settings frame (300 by default).
* `CONFIG_HTTP2_SOCK_READ_SIZE`, size for the socket read buffer (512 bytes by default). This effectively limits the maximum frame size that can be received. Modifications to this value alter the total static memory used by the implementation.
* `CONFIG_HTTP2_SOCK_WRITE_SIZE`, size for the socker write buffer (512 bytes by default). Modifications to this value alter the total static memory used by the implementation. 
* `CONFIG_HTTP2_STREAM_BUF_SIZE`, set the maximum total data that can be received by a stream. This the total header block size that can be sent in HEADERS and CONTINUATION frames, and also the total data size that can be send by a HTTP response. This settings affects the static memory used by client.
* `CONFIG_HTTP2_MAX_CLIENTS`, maximum number of concurrent clients allowed by the server.
* `CONFIG_TWO_MAX_RESOURCES`, sets the maximum number of [resource paths](src/two.h#L57) supported by the server. The default is 4.

The approximate size of the memory used per client can be calculated as
```
CONFIG_HTTP2_HEADER_TABLE_SIZE + CONFIG_HTTP2_SOCK_READ_SIZE + CONFIG_HTTP2_STREAM_BUF_SIZE + CONFIG_HTTP2_SOCK_WRITE_SIZE
```

## Server API

A simple server API is defined in [two.h](src/two.h). All it is needed to run a server is to register one or more resources
and then call [two_server_start()](src/two.h#L31), as shown by the code below.

```{c}
#include "two.h"

int hello(char * method, char * uri, char * response, unsigned int maxlen) 
{
    // copy response data to the provided buffer, the response size
    // cannot be larger than maxlen
    strcpy(response, "Hello, World!!!");

    // return content length or -1 for an error
    return 15;
}

int main() {
    // GET /hello requests will call the hello callback
    two_register_resource("GET", "/hello", "text/plain", hello);

    // start the server on port 8888
    two_server_start(8888);
}
```

A resource binds an action to a server [path](https://tools.ietf.org/html/rfc3986#section-3.3).
The action is defined through a callback, and can be anything (returning a static message, returning a reading from a sensor, etc.). However,
the callback must not block. Since the server is single-threaded, blocking the callback will prevent the server from
interacting with other clients. 

The content type of a resource response is defined when registering the resource, and supported
content types are defined in [content_type.h](src/content_type.h).


## More examples

See [examples](examples/).

## Authors

* **Sandra Céspedes** - *Mastermind behind the project* - [webpage](https://www.cec.uchile.cl/~scespedes/)
* **Maite González** - *Initial work* - [maitegm](https://github.com/people/maitegm)
* **Javiera Bermudez** - [javi801](https://github.com/people/javi801)
* **Sebastian  Cifuentes** - [sebastiancif](https://github.com/people/sebastiancif)
* **Valeria Guidotti** - [valegui](https://github.com/people/valegui)
* **Gabriel Norambuena** - [gnorambuena](https://github.com/people/gnorambuena)
* **Pablo Aliaga** - [maitegm](https://github.com/people/pabloaliaga)
* **Diego Ortego** - [gedoix](https://github.com/people/gedoix)
* **Felipe Lalanne** - [pipex](https://github.com/people/pipex)

See the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE.md](LICENSE.md) file for details

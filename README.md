[![Development Build Status](https://travis-ci.com/niclabs/two.svg?branch=develop)](https://travis-ci.com/niclabs/two)
[![codecov](https://codecov.io/gh/niclabs/two/branch/develop/graph/badge.svg)](https://codecov.io/gh/niclabs/two)

# two: HTTP/2 for constrained IoT devices

Experimental HTTP/2 server and library, aimed at use in [constrained devices](https://tools.ietf.org/html/rfc7228). 
The implementation uses a single thread for handling clients, and each client requires around 1.5K of static RAM 
in the base configuration (without [HPACK dynamic table](https://httpwg.org/specs/rfc7541.html#dynamic.table)). 
This can be further reduced through [configuration macros](#configuration-macros).

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

### Building for Contiki targets

The easiest way to build for Contiki targets is by using the [Contiki NG docker image](https://github.com/contiki-ng/contiki-ng/wiki/Docker), or our [embeddable docker image](https://hub.docker.com/r/niclabs/embeddable) if working with ARM targets.
Otherwise you can find other installation methods in the [Contiki NG wiki](https://github.com/contiki-ng/contiki-ng/wiki). A basic server example for this platform can be found in `examples/contiki`.
To build for the Contiki native platform, do
```{bash}
$ cd examples/contiki
$ make
```

This should generate a server.native executable file. To run, do
```{bash}
$ sudo ./server.native
opened tun device ``/dev/tun0''
tun0: flags=4305<UP,POINTOPOINT,RUNNING,NOARP,MULTICAST>  mtu 1500
        inet 172.17.0.2  netmask 255.255.255.255  destination 172.17.0.2
        inet6 fe80::190e:7a78:e345:7872  prefixlen 64  scopeid 0x20<link>
        inet6 fd00::1  prefixlen 64  scopeid 0x0<global>
        unspec 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  txqueuelen 500  (UNSPEC)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

[INFO: Main      ] Starting Contiki-NG-release/v4.4-dirty
[INFO: Main      ] - Routing: RPL Lite
[INFO: Main      ] - Net: tun6
[INFO: Main      ] - MAC: nullmac
[INFO: Main      ] - 802.15.4 PANID: 0xabcd
[INFO: Main      ] - 802.15.4 Default channel: 26
[INFO: Main      ] Node ID: 1800
[INFO: Main      ] Link-layer address: 0102.0304.0506.0708
[INFO: Main      ] Tentative link-local IPv6 address: fe80::302:304:506:708
[INFO: Native    ] Added global IPv6 address fd00::302:304:506:708
[INFO] Starting HTTP/2 server in port 8888
```
Notice that you need the `sudo` command to create the tun device required to access the Contiki network stack, accessible through the given global IPv6 address. To perform a request, on another terminal do
```{bash}
$ curl --http2-prior-knowledge http://[fd00::302:304:506:708]:8888
Hello, World!!!
```

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
* _CONFIG_HTTP2_HEADER_TABLE_SIZE_, maximum value for the dynamic hpack header table. Setting this to zero disables use of the dynamic table for HPACK. This setting affects the size of static memory used by client.
* _CONFIG_HTTP2_MAX_CONCURRENT_STREAMS_, maximum number of concurrent streams alloed by HTTP/2. It cannot be larger than 0.
* _CONFIG_HTTP2_INITIAL_WINDOW_SIZE_, initial value for HTTP/2 window size. This value cannot be larger than the read buffer size.
* _CONFIG_HTTP2_MAX_FRAME_SIZE_, initial value for SETTINGS_MAX_FRAME_SIZE. It has no effect on the size of the allocation buffers, the effective max frame size is given by the setting _CONFIG_HTTP2_SOCK_READ_SIZE_.
* _CONFIG_HTTP2_MAX_HEADER_LIST_SIZE_, initial value for SETTINGS_MAX_HEADER_LIST_SIZE. It effectively sets the maximum number of decompressed bytes for the header list (see [header_list](src/header_list.h)). This setting has no impact on the static memory used by the implementation, however the value must be chosen carefully, since it have an effect on the stack size.
* _CONFIG_HTTP2_SETTINGS_WAIT_, maximum time in milliseconds tom wait for the remote endpoint to reply to a settings frame (300 by default).
* _CONFIG_HTTP2_SOCK_READ_SIZE_, size for the socket read buffer (512 bytes by default). This effectively limits the maximum frame size that can be received. Modifications to this value alter the total static memory used by the implementation.
* _CONFIG_HTTP2_SOCK_WRITE_SIZE_, size for the socker write buffer (512 bytes by default). Modifications to this value alter the total static memory used by the implementation. 
* _CONFIG_HTTP2_STREAM_BUF_SIZE_, set the maximum total data that can be received by a stream. This the total header block size that can be sent in HEADERS and CONTINUATION frames, and also the total data size that can be send by a HTTP response. This settings affects the static memory used by client.
* _CONFIG_HTTP2_MAX_CLIENTS_, maximum number of concurrent clients allowed by the server.
* _CONFIG_TWO_MAX_RESOURCES_, sets the maximum number of [resource paths](src/two.h#L_57) supported by the server. The default is 4.

The approximate size of the memory used per client can be calculated as
```
CONFIG_HTTP2_HEADER_TABLE_SIZE + CONFIG_HTTP2_SOCK_READ_SIZE + CONFIG_HTTP2_STREAM_BUF_SIZE + CONFIG_HTTP2_SOCK_WRITE_SIZE
```

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

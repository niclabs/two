[![Development Build Status](https://travis-ci.com/niclabs/two.svg?branch=develop)](https://travis-ci.com/niclabs/two)
[![codecov](https://codecov.io/gh/niclabs/two/branch/develop/graph/badge.svg)](https://codecov.io/gh/niclabs/two)

# two: HTTP/2 for constrained devices

Experimental HTTP/2 server/client library, using static memory allocation only, aimed at use in [constrained devices](https://tools.ietf.org/html/rfc7228).

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

## Implementation details

1. Prior version knowledge is assumed. Server replies with HTTP/2 connection preface. Connection upgrade and HTTP/1.1 are not supported at the moment.
2. SETTINGS, HEADERS, CONTINUATION, DATA, WINDOW_UPDATE and GO_AWAY frame support is implemented
3. Header compression with HPACK is optionally supported and configurable.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 

### Dependencies

The project is designed to have no external dependencies other than gcc and libc, you willa also need `make` to build, and we are working on [Contiki OS](http://www.contiki-os.org) support. 
Internally and for testing in Travis, we use a [Docker image with basic support for embedded programming](https://github.com/niclabs/docker/tree/master/embeddable) that you can download as well.

### Installing

Clone the repo with:

```{bash}
git clone https://github.com/niclabs/two.git
```

If all your dependencies are working as intended, you can compile the project with:

```{bash}
make
```

### Compiling inside Docker

If you decided to use our docker image linked in the [dependencies](#dependencies) section, go to the project's directory in the terminal and execute the `embeddable` command.

Then you can compile the project normally:

```{bash}
make
```

## Running the tests

To run the automated unit tests for the project you can do:

```{bash}
make distclean && make test
```

The output should end with an `OK`.

## Built With

* [Travis](https://travis-ci.com/) - Continuous integration
* [ZenHub](https://www.zenhub.com/) - Issue and TODO tracking and metrics

<!-- ## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us. -->

## Versioning

We use [GitHub](https://github.com/) for versioning. For the versions available, see the [tags on this repository](https://github.com/niclabs/two/tags).

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

## Acknowledgments

* Hat tip to [the source of this README template](https://gist.github.com/PurpleBooth/109311bb0361f32d87a2#file-readme-template-md)

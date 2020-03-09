# HTTP/2 server on Contiki-NG

This example defines a basic HTTP/2 server to run on embedded target using [Contiki-NG](https://github.com/contiki-ng/contiki-ng/).

The easiest way to build for Contiki targets is by using the [Contiki NG docker image](https://github.com/contiki-ng/contiki-ng/wiki/Docker), or our [embeddable docker image](https://hub.docker.com/r/niclabs/embeddable) if working with ARM targets.

For building for a specific 
Otherwise you can find other installation methods in the [Contiki NG wiki](https://github.com/contiki-ng/contiki-ng/wiki). A basic server example for this platform can be found in `examples/contiki`.

```{bash}
$ cd examples/contiki
examples/contiki $ make
```

This should generate a `server.native executable` file. To run, do
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

For building for a specific [Contiki-NG platform](https://github.com/contiki-ng/contiki-ng/wiki#the-contiki-ng-platforms), you need to set the TARGET environment variable. For instance, for the zoul platform.
```{bash}
examples/contiki $ TARGET=zoul make
```
To flash the firmware, set PORT with the serial device for the board.
```{bash}
examples/contiki $ TARGET=zoul PORT=/dev/ttyUSB0 make server.upload
Flashing /dev/ttyUSB0
Opening port /dev/ttyUSB0, baud 460800
Reading data from build/zoul/remote-revb/server.bin
Cannot auto-detect firmware filetype: Assuming .bin
Connecting to target...
...
```
By default, Contiki-NG builds are compiled with shell support. To connect to the device
```{bash}
examples/contiki $ TARGET=zoul PORT=/dev/ttyUSB0 make login
connecting to /dev/ttyUSB0 [OK]

#0012.4b00.194a.5233> help
Available commands:
'> help': Shows this help
'> reboot': Reboot the board by watchdog_reboot()
'> log module level': Sets log level (0--4) for a given module (or "all"). For module "mac", level 4 also enables per-slot logging.
'> mac-addr': Shows the node's MAC address
'> ip-addr': Shows all IPv6 addresses
'> ip-nbr': Shows all IPv6 neighbors
'> ping addr': Pings the IPv6 address 'addr'
'> routes': Shows the route entries
'> rpl-set-root 0/1 [prefix]': Sets node as root (1) or not (0). A /64 prefix can be optionally specified.
'> rpl-local-repair': Triggers a RPL local repair
'> rpl-refresh-routes': Refreshes all routes through a DTSN increment
'> rpl-status': Shows a summary of the current RPL state
'> rpl-nbr': Shows the RPL neighbor table
'> rpl-global-repair': Triggers a RPL global repair
#0012.4b00.194a.5233>
```

If the device is configured as part of a [RPL network](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-IPv6-ping), you can connect to the server using its global IPv6 address (given by the `ip-addr` command).

To connect to the server through a [SLIP connection](https://en.wikipedia.org/wiki/Serial_Line_Internet_Protocol), see instructions for testing with h2spec below.

## Testing

It is possible to run the suite of [h2spec](https://github.com/summerwind/h2spec) tests against Contiki NG targets. 
For running against a native target, it suffices to run `make h2spec` inside the `examples/contiki` folder.

For running against an embedded target, you will need to configure the device to act as a [RPL border router](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-RPL-border-router)
(or use two devices connected through wireless link). This will let the device obtain an IPv6 address to test against.

For this, you will need to set the local environment variable BORDER_ROUTER.
```{bash}
examples/contiki $ TARGET=zoul BORDER_ROUTER=1 make clean all
examples/contiki $ TARGET=zoul PORT=/dev/ttyUSB0 BORDER_ROUTER=1 make server.upload
Flashing /dev/ttyUSB0
Opening port /dev/ttyUSB0, baud 460800
Reading data from build/zoul/remote-revb/server.bin
Cannot auto-detect firmware filetype: Assuming .bin
Connecting to target...
...
```

To connect to the device using a SLIP connection.

```{bash}
examples/contiki $ TARGET=zoul PORT=/dev/ttyUSB0 make connect-router
sudo ../../../contiki-ng/tools/serial-io/tunslip6 -s /dev/ttyUSB0 fd00::1/64
********SLIP started on ``/dev/ttyUSB0''
opened tun device ``/dev/tun0''
ifconfig tun0 inet `hostname` mtu 1500 up
ifconfig tun0 add fd00::1/64
ifconfig tun0 add fe80::0:0:0:1/64
ifconfig tun0

tun0: flags=4305<UP,POINTOPOINT,RUNNING,NOARP,MULTICAST>  mtu 1500
        inet 172.17.0.2  netmask 255.255.255.255  destination 172.17.0.2
        inet6 fe80::1  prefixlen 64  scopeid 0x20<link>
        inet6 fe80::7f67:4b6b:28f0:a831  prefixlen 64  scopeid 0x20<link>
        inet6 fd00::1  prefixlen 64  scopeid 0x0<global>
        unspec 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  txqueuelen 500  (UNSPEC)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

[INFO: BR        ] Waiting for prefix
*** Address:fd00::1 => fd00:0000:0000:0000
[INFO: BR        ] Waiting for prefix
[INFO: BR        ] Server IPv6 addresses:
[INFO: BR        ]   fd00::212:4b00:194a:5233
[INFO: BR        ]   fe80::212:4b00:194a:5233
```

If everything worked correctly, you should see a pair of IPv6 addresses after 'Waiting for prefix' (if not, try rebooting the device),
this means the device has an IP address and it can receive connections. To test, try to ping the global IPv6 address
(starting with `fd00::`).
```{bash}
examples/contiki $ ping6 fd00::212:4b00:194a:5233
PING fd00::212:4b00:194a:5233(fd00::212:4b00:194a:5233) 56 data bytes
64 bytes from fd00::212:4b00:194a:5233: icmp_seq=1 ttl=64 time=41.0 ms
64 bytes from fd00::212:4b00:194a:5233: icmp_seq=2 ttl=64 time=30.0 ms
64 bytes from fd00::212:4b00:194a:5233: icmp_seq=3 ttl=64 time=39.6 ms
```

If you are using docker, you must connect to the running instance on another terminal to test. First list the running
instances.
```{bash}
CONTAINER ID        IMAGE                COMMAND                  CREATED             STATUS              PORTS               NAMES
host $ docker ps
4ad3d887fec3        niclabs/embeddable   "/sbin/tini -- entry…"   3 days ago          Up 3 days                               nervous_swirles
```
Run a new shell inside the instance
```{bash}
host $ docker exec -ti nervous_swirles bash
user@id:work $
```
Ping the device
```{bash}
user@id:work $ ping6 fd00::212:4b00:194a:5233
PING fd00::212:4b00:194a:5233(fd00::212:4b00:194a:5233) 56 data bytes
64 bytes from fd00::212:4b00:194a:5233: icmp_seq=1 ttl=64 time=41.0 ms
64 bytes from fd00::212:4b00:194a:5233: icmp_seq=2 ttl=64 time=30.0 ms
64 bytes from fd00::212:4b00:194a:5233: icmp_seq=3 ttl=64 time=39.6 ms
```

Check that the HTTP/2 server is receiving connections
```{bash}
examples/contiki $ curl --http2-prior-knowledge http://[fd00::212:4b00:194a:5233]:8888
Hello, World!!!
```

If ping and curl work, you can now run h2spec tests against the embedded device using
```{bash}
examples/contiki $ TARGET=zoul H2SPEC_ADDR=fd00::212:4b00:194a:5233 make h2spec
------------------------------
Integration tests with h2spec
------------------------------
generic/1: PASS
generic/2: PASS
generic/3.1: PASS
...
```

Note that the set of passing tests for Contiki-NG is smaller than the set for linux targets. This is due to
a more limited configuration for the embedded targets, mainly disabling the HPACK dynamic table and limiting the 
number of clients to at most 1 for memory considerations. See [h2spec.conf](h2spec.conf) for more details on the tests.

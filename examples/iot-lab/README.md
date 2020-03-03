# HTTP/2 server on IoT-Lab targets

The [IoT-Lab](https://www.iot-lab.info) is an open large scale testbed for testing applications in wireless sensor networks. It allows testing with multiple types of boards, architectures and communication technologies. For more information on how to use the testbed, please see the [IoT-Lab tutorials](https://www.iot-lab.info/tutorials/).

This example allows to build and run a HTTP/2 server in an IoT-Lab m3 board.

## Requirements

* A valid account for the FIT IoT-LAB ([registration](https://www.iot-lab.info/testbed/signup) is open for everyone)
* Either:
    - [Contiki NG dependencies](https://github.com/contiki-ng/contiki-ng/wiki)
    - [IoT-Lab board support files](https://github.com/iot-lab/iot-lab-contiki-ng) for Contiki NG. See [IoT-Lab Contiki NG tutorial](https://www.iot-lab.info/tutorials/contiki-ng-compilation/).
    - [IoT-Lab cli-tools](https://github.com/iot-lab/cli-tools).
* Or 
    - [NIC Labs embeddable docker image](https://hub.docker.com/r/niclabs/embeddable), already containing the above dependencies.


## Building the firmware

Build the code with [RPL border router](https://github.com/contiki-ng/contiki-ng/wiki/Tutorial:-RPL-border-router) support
```
examples/iot-lab $ BORDER_ROUTER=1 make
```

If everything went well, you will see a `server.iotlab` under the current folder.

## Submitting an experiment and creating a network

In the following we will launch an experiment in IoT-Lab and upload firmware to the reserved devices. 
To know more about this process see [Submit an experiment with M3 nodes](https://www.iot-lab.info/tutorials/submit-experiment-m3-clitools/) and [Public IPv6 (6LoWPAN/RPL) network with M3 nodes](https://www.iot-lab.info/tutorials/contiki-public-ipv6-m3/).

Authenticate with your username and input your password when prompted
```{bash}
examples/iot-lab $ iotlab-auth -u <your username>
Password:
"Written"
```

Submit a new experiment for a m3 node on one of the [IoT-Lab sites](https://www.iot-lab.info/deployment/) (Grenoble in this case).
```{bash}
examples/iot-lab $ iotlab-experiment submit -d 60 -l 1,archi=m3:at86rf231+site=grenoble,server.iotlab
{
    "id": 194934
}
```
Wait for the experiment to become ready
```{bash}
$ iotlab-experiment get -i 194934 -s
{
    "state": "Running"
}
```
List experiment resources, notice we were assigned the m3-100 node.
```{bash}
$ iotlab-experiment get -i 194934 -r
{
    "items": [
        {
            "archi": "m3:at86rf231",
            "mobile": "0",
            "mobility_type": " ",
            "network_address": "m3-100.grenoble.iot-lab.info",
            "site": "grenoble",
            "state": "Alive",
            "uid": "b080",
            "x": "1.00",
            "y": "25.23",
            "z": "-0.04"
        }
    ]
}
```

Log via ssh to the IoT-Lab site and setup a tun interface to the node using one of the available [IPv6 prefixes](https://www.iot-lab.info/tutorials/understand-ipv6-subnetting-on-the-fit-iot-lab-testbed/).
```{bash}
$ ssh <username>@grenoble.iot-lab.info
grenoble $ sudo tunslip6.py -v2 -L -a m3-100 -p 20000 2001:660:5307:3116::1/64
Switch from 'root' to 'username'
Calling tunslip:
        'tunslip6 [u'-v2', '-a', 'm3-100', '-L', '-p', '20000', '2001:660:5307:3116::1/64']'
slip connected to ``172.16.10.100:20000''

19:16:21 opened tun device ``/dev/tun0''
0000.000 ifconfig tun0 inet `hostname` mtu 1500 up
0000.008 ifconfig tun0 add 2001:660:5307:3116::1/64
0000.013 ifconfig tun0 add fe80::660:5307:3116:1/64
0000.019 ifconfig tun0

tun0: flags=4305<UP,POINTOPOINT,RUNNING,NOARP,MULTICAST>  mtu 1500
        inet 192.168.1.5  netmask 255.255.255.255  destination 192.168.1.5
        inet6 fe80::9046:af15:d211:9106  prefixlen 64  scopeid 0x20<link>
        inet6 2001:660:5307:3116::1  prefixlen 64  scopeid 0x0<global>
        inet6 fe80::660:5307:3116:1  prefixlen 64  scopeid 0x20<link>
        unspec 00-00-00-00-00-00-00-00-00-00-00-00-00-00-00-00  txqueuelen 500  (UNSPEC)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

0000.841 [INFO: BR        ] Waiting for prefix
0000.841 *** Address:2001:660:5307:3116::1 => 2001:0660:5307:3116
0001.841 [INFO: BR        ] Waiting for prefix
0001.842 [INFO: BR        ] Server IPv6 addresses:
0001.842 [INFO: BR        ]   2001:660:5307:3116::b080
0001.842 [INFO: BR        ]   fe80::b080
```

In this case the device was assigned the `2001:660:5307:3116::b080` global IPv6 address. From another ssh session now try pinging
the device.

```{bash}
grenoble $ ping6 2001:660:5307:3116::b080
PING 2001:660:5307:3116::b080(2001:660:5307:3116::b080) 56 data bytes
64 bytes from 2001:660:5307:3116::b080: icmp_seq=1 ttl=64 time=6.80 ms
64 bytes from 2001:660:5307:3116::b080: icmp_seq=2 ttl=64 time=6.77 ms
```

## Test the HTTP/2 endpoints

```{bash}
grenoble $ curl --http2-prior-knowledge http://[2001:660:5307:3116::b080]:8888
Hello, World!!!
grenoble $ curl --http2-prior-knowledge http://[2001:660:5307:3116::b080]:8888/light
Light sensor reading: 3000
```

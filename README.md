# two

Experimental HTTP2 implementation for running on constrained devices

## Building and testing

1. Clone this repo

```
git clone -b echo-example --recursive https://github.com/niclabs/two.git
```

2. Make sure you have [RIOT dependencies](https://github.com/RIOT-OS/RIOT/wiki/Introduction#compiling-riot) installed for your target platform, or [Vagrant](https://www.vagrantup.com) configured if you want to avoid the trouble (we include a Vagrantfile for a quickstart).
3. If building on [native](https://github.com/RIOT-OS/RIOT/wiki/Family%3A-native), make sure to initialize tap interfaces using

```
$ RIOT/dist/tools/tapsetup/tapsetup -c 2
```

4. Build the code for your platform, for example on native, we use

```
make all
```

5. Run the example, on two different terminals run `make term PORT=tap0` and `make term PORT=tap1`

6. Using the command `help` on the RIOT shell, you can see the available commands. For example for getting the IPv6 address of the device run

```
ifconfig
```

and for running the echo echo server on port 12345, use
```

echo-server 12345
```

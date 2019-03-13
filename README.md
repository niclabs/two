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

6. Using the command `help` on the RIOT shell, you can see the available commands.

## Building and testing with IoT-Lab

See [IOT-LAB.md](IOT-LAB.md) for information.

### tl;dr

* Create an account on the [IoT-Lab](https://www.iot-lab.info/testbed/signup)
* Include the following code at the end of the RIOT Makefile
```
# For iot-lab
ifeq ($(ENV),iotlab)
  include $(CURDIR)/../Makefile.iotlab
endif
```
* Run
```
ENV=iotlab BOARD=iotlab-m3 make flash term
```
to flash and start a serial connection with the nodes on the IoT-Lab testbed. 

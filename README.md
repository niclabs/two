# two

Experimental HTTP2 implementation for running on constrained devices

## Building and testing with IoT-Lab

See [IOT-LAB.md] for information.

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

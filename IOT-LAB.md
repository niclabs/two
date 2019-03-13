# Testing and debugging with IoT-Lab

The [IoT-Lab](https://www.iot-lab.info) is an open large scale testbed for testing applications in wireless sensor networks. It allows testing with multiple types of boards, architectures and communication technologies. For more information on how to use the testbed, please see the [IoT-Lab tutorials](https://www.iot-lab.info/tutorials/).

## Requirements

* A valid account for the FIT IoT-LAB (registration there is open for everyone)
* [IoT-Lab cli-tools](https://github.com/iot-lab/cli-tools).
* The tool [jq](https://stedolan.github.io/jq/) for JSON processing

## Setup

The file [Makefile.iotlab](Makefile.iotlab) in the root of this repository contains definitions of targets to submit and control IoT-Lab experiments using the GNU Make build system. In particular, it redefines the targets flash, term and debug in order to perform operations in the IoT-Lab platform. 

To enable, it is necessary to include the following lines at the end of the RIOT application Makefile

```
# For iot-lab
ifeq ($(ENV),iotlab)
  include $(CURDIR)/../Makefile.iotlab
endif
```

Make sure that the route of `Makefile.iotlab` file is correctly defined in the include line. The `ifeq ($(ENV), iotlab)` condition is not strictly necessary, but it gives better control of the environment in which the flash,term,debug operations are performed (REMEMBER that `Makefile.iotlab` overrides the flash,term,debug targets).

## Variables

* IOTLAB_SITE (grenoble). The [IoT-Lab site](https://www.iot-lab.info/deployment/) where the code will be tested.
* IOTLAB_NODES (2). Number of nodes to reserve, it is ignored if IOTLAB_RESOURCES is defined.
* IOTLAB_DURATION (30). Experiment duration in minutes.
* IOTLAB_ARCHI (m3:at86rf231). [IoT-Lab hardware](https://www.iot-lab.info/hardware/) to use. 
* IOTLAB_NAME. The name of the experiment in the IoT-Lab platform. By default is the same name as the RIOT application.
* IOTLAB_WAIT (60). The time to wait for the resources to become available when submitting the experiment (60 seconds by default).
* IOTLAB_RESOURCES (optional). The physical resources to reserve using the string format specified in `iotlab-experiment submit --help`. And example would be 1-3+7+15 to reserve nodes 1,2,3,7 and 15.
* IOTLAB_ID (optional). Id of the experiment to use when running flash, term and debug operations. If not defined the first experiment of the user in "Running" state with the application name will be used.

## Targets

The makefile defines the following targets besides redefining the targets flash, term and debug. 

* experiment-submit
* experiment-check
* experiment-stop

### experiment-submit

This target schedules a new experiment on the IoT-Lab and waits until it enters a "Running" state. It will request `IOTLAB_NODES` or `IOTLAB_RESOURCES` for `IOTLAB_DURATION` minutes.

If an experiment with the defined ID is already running, the target will terminate without error.

### flash

This target redefines the RIOT flash target. It uploads the locally built binary to the `IOTLAB_SITE` and return when ready. It depends on the experiment-submit target so it will launch a new experiment if one is not available.

Usage example

```
ENV=iotlab BOARD=iotlab-m3 IOTLAB_RESOURCES=10-12,15,16 make flash
```

Will reserve (if not reserved) the IoT Lab nodes 10,11,12,15,16 in the Grenoble site and flash with the local .elf or .hex file.

### term

This target redefines the RIOT term target. It will use ssh to login on the IoT-Lab serrver specified by `IOTLAB_SITE` and start the [serial_aggregator](https://github.com/iot-lab/aggregation-tools) to communicate with all targets. Use Ctrl+D to finish the server-side process.

Usage example

```
ENV=iotlab BOARD=iotlab-m3 make term
```

Will open a connection to all reserved nodes in the experiment.

### debug

This target redefines the RIOT debug target. It selects the first node in the resources list to act as debug node. If PORT is defined with a node id, it will use that node instead. It will then open GDB with a connection to the remote debug node to perform debugging operations. For more information on debugging with IoT-Lab check the [Debugging with M3 nodes](https://www.iot-lab.info/tutorials/debugger-on-m3-nodes/) tutorial entry.

Usage example

```
ENV=iotlab BOARD=iotlab-m3 PORT=15 make debug
```
will start a debugging sesion on the m3-15 node of the site grenoble.

### experiment-stop

Stops the running experiment

### experiment-check

Checks if an experiment is running
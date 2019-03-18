# name of your application
APPLICATION = two

# If no BOARD is found in the environment, use this default:
ifeq ($(ENV),iotlab)
  BOARD ?= iotlab-m3
endif

BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../RIOT

# lwIP's memory management doesn't seem to work on non 32-bit platforms at the
# moment.
BOARD_BLACKLIST := arduino-uno arduino-duemilanove arduino-mega2560 chronos \
                   esp8266-esp-12x esp8266-olimex-mod esp8266-sparkfun-thing \
                   msb-430 msb-430h telosb waspmote-pro wsn430-v1_3b \
                   wsn430-v1_4 z1 jiminy-mega256rfr2 mega-xplained
BOARD_INSUFFICIENT_MEMORY = blackpill bluepill nucleo-f031k6 nucleo-f042k6 \
                            nucleo-l031k6 nucleo-f030r8 nucleo-f302r8 \
                            nucleo-f303k8 nucleo-f334r8 nucleo-l053r8 \
                            saml10-xpro saml11-xpro stm32f0discovery

# Uncomment this to enable scheduler statistics for ps:
#CFLAGS += -DSCHEDSTATISTICS

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Include network modules and some dependencies
USEMODULE += ipv6_addr
USEMODULE += netdev_default

# Include lwip and dependencies
USEMODULE += lwip_netdev
USEMODULE += lwip_ipv6_autoconfig
USEMODULE += lwip_tcp
USEMODULE += lwip_sock_tcp

# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

ifeq ($(BOARD),native)
  USEMODULE += lwip_ethernet
endif

include $(RIOTBASE)/Makefile.include

# For iot-lab
ifeq ($(ENV),iotlab)
  include $(CURDIR)/../Makefile.iotlab
endif

#make file for running tests on frames
#CC=gcc
#CFLAGS=-I.

#frametests:  frames.o utils.o frametests.o
#	$(CC) -o frametests frames.o utils.o frametests.o

#cleanframe:
#	rm -f ./*.o ./frametests

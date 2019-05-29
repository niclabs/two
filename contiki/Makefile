CONTIKI_PROJECT = two
all: $(CONTIKI_PROJECT)


SRCDIR ?= ../src
CONTIKI ?= ../build/contiki

PROJECTDIRS += $(SRCDIR)
PROJECT_SOURCEFILES=http_methods.c sock.c http_methods_bridge.c http2.c http2utils.c frames.c utils.c

#UIP_CONF_IPV6=1
include $(CONTIKI)/Makefile.include

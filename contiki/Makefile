CONTIKI_PROJECT = two
all: $(CONTIKI_PROJECT)

SRCDIR ?= ../src
CONTIKI ?= ../build/contiki

PROJECTDIRS += $(SRCDIR)
PROJECT_SOURCEFILES=http.c sock.c http_bridge.c http2.c http2utils.c frames.c utils.c

DEFINES += WITH_CONTIKI

#UIP_CONF_IPV6=1
include $(CONTIKI)/Makefile.include

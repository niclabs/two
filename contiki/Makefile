CONTIKI_PROJECT = test_server
all: $(CONTIKI_PROJECT)

SRCDIR ?= ../src
CONTIKI ?= ../build/contiki

PROJECTDIRS += $(SRCDIR)
PROJECT_SOURCEFILES=sock.c cbuf.c

DEFINES += WITH_CONTIKI

#linker optimizations
SMALL=1

# Contiki configurations
CONTIKI_WITH_IPV6=1
CONTIKI_WITH_RPL=0

include $(CONTIKI)/Makefile.include

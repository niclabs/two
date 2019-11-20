CONTIKI_PROJECT = tiny
all: $(CONTIKI_PROJECT)

SRC ?= ../../src
CONTIKI ?= ../../../contiki-ng

PROJECTDIRS += $(SRC)
PROJECT_SOURCEFILES=cbuf.c \
          event.c

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

SERVER_ADDR ?= 2001:dead:beef::1
SERVER_PORT ?= 8888

DEFINES += WITH_CONTIKI

include $(CONTIKI)/Makefile.include

CONTIKI_PROJECT = server client
all: $(CONTIKI_PROJECT)

SRCDIR ?= ../../src
CONTIKI ?= ../../../contiki

PROJECTDIRS += $(SRCDIR)
PROJECT_SOURCEFILES=http.c \
          http_bridge.c \
          http2.c \
          http2utils.c \
          frames.c \
          hpack.c \
          hpack_utils.c \
          sock.c \
          cbuf.c \
          utils.c

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

#linker optimizations
SMALL=1

# Server configuration
SERVER_ADDR ?= 2001:dead:beef::1
SERVER_PORT ?= 8888

DEFINES += WITH_CONTIKI
DEFINES += SERVER_ADDR=\"$(SERVER_ADDR)\"
DEFINES += SERVER_PORT=$(SERVER_PORT)

# From https://github.com/contiki-os/contiki/wiki/Reducing-Contiki-OS-firmware-size
ifeq (TARGET,zoul)
CFLAGS += -ffunction-sections
LDFLAGS += -Wl,--gc-sections,--undefined=_reset_vector__,--undefined=InterruptVectors,--undefined=_copy_data_init__,--undefined=_clear_bss_init__,--undefined=_end_of_init__
endif

# Contiki configurations
CONTIKI_WITH_IPV6=1

include $(CONTIKI)/Makefile.include

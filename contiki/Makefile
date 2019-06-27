CONTIKI_PROJECT = server client
all: $(CONTIKI_PROJECT)

SRCDIR ?= ../src
CONTIKI ?= ../../contiki

PROJECTDIRS += $(SRCDIR)
PROJECT_SOURCEFILES=sock.c cbuf.c

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

#linker optimizations
SMALL=1

# From https://github.com/contiki-os/contiki/wiki/Reducing-Contiki-OS-firmware-size
ifeq (TARGET,zoul)
CFLAGS += -ffunction-sections
LDFLAGS += -Wl,--gc-sections,--undefined=_reset_vector__,--undefined=InterruptVectors,--undefined=_copy_data_init__,--undefined=_clear_bss_init__,--undefined=_end_of_init__
endif

# Contiki configurations
CONTIKI_WITH_IPV6=1

include $(CONTIKI)/Makefile.include

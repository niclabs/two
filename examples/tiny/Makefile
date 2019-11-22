CONTIKI_PROJECT = tiny
all: $(CONTIKI_PROJECT)

TWO ?= ../../src
CONTIKI ?= ../../../contiki-ng

PROJECTDIRS += $(TWO)
PROJECT_SOURCEFILES=cbuf.c event.c

# Disable UDP
DEFINES += UIP_CONF_TCP=1
DEFINES += UIP_CONF_UDP=0

CFLAGS += -ffunction-sections -I$(TWO)
ifeq ($(word 1, $(shell uname -m -s)),Darwin)
LDFLAGS += -Wl,-dead_strip
else
LDFLAGS += -Wl,--gc-sections,--undefined=_reset_vector__,--undefined=InterruptVectors,--undefined=_copy_data_init__,--undefined=_clear_bss_init__,--undefined=_end_of_init__
endif

include $(CONTIKI)/Makefile.include

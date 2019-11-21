CONTIKI_PROJECT = echo
all: $(CONTIKI_PROJECT)

TWO = ../../src
CONTIKI ?= ../../../contiki-ng

PROJECTDIRS += $(TWO)
PROJECT_SOURCEFILES += cbuf.c event.c

DEFINES += WITH_CONTIKI
DEFINES += UIP_CONF_TCP=1
DEFINES += UIP_CONF_UDP=0

CFLAGS += -ffunction-sections
 LDFLAGS += -Wl,--gc-sections,--undefined=_reset_vector__,--undefined=InterruptVectors,--undefined=_copy_data_init__,--undefined=_clear_bss_init__,--undefined=_end_of_init__

include $(CONTIKI)/Makefile.include

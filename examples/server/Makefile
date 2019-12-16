CONTIKI_PROJECT = server
all: $(CONTIKI_PROJECT)

TWO ?= ../../src
CONTIKI ?= ../../../contiki-ng

PROJECTDIRS += $(TWO) $(TWO)/frames $(TWO)/hpack $(TWO)/http2
PROJECT_SOURCEFILES=two.c \
					cbuf.c \
					event.c \
					headers.c \
					http.c \
				 	resource_handler.c \
					utils.c \
					frames.c \
					settings_frame.c \
					headers_frame.c \
					data_frame.c \
					continuation_frame.c \
					window_update_frame.c \
					ping_frame.c \
					rst_stream_frame.c \
					goaway_frame.c \
					hpack.c \
					decoder.c \
					encoder.c \
					huffman.c \
					tables.c \
					hpack_utils.c \
					http2.c \
					check.c \
					handle.c \
					send.c \
					flowcontrol.c \
					http2_utils.c


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

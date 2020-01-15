CONTIKI_PROJECT = server
all: $(CONTIKI_PROJECT)

TWO ?= ../..
SRC = $(TWO)/src
CONTIKI ?= ../../../contiki-ng

HTTP2_PORT ?= 8888
ADDR = fd00::302:304:506:708

PROJECTDIRS += $(SRC) $(SRC)/frames $(SRC)/hpack $(SRC)/http2
PROJECT_SOURCEFILES=two.c \
					cbuf.c \
					event.c \
					header_list.c \
					utils.c \
					frames.c \
					hpack.c \
					decoder.c \
					encoder.c \
					huffman.c \
					tables.c \
					hpack_utils.c \
					http2.c


# Disable UDP
DEFINES += UIP_CONF_TCP=1
DEFINES += UIP_CONF_UDP=0
DEFINES += PROJECT_CONF_H=\"project-conf.h\"

MODULES += os/services/shell

CFLAGS += -ffunction-sections -I$(SRC)
ifeq ($(word 1, $(shell uname -m -s)),Darwin)
LDFLAGS += -Wl,-dead_strip
else
LDFLAGS += -Wl,--gc-sections,--undefined=_reset_vector__,--undefined=InterruptVectors,--undefined=_copy_data_init__,--undefined=_clear_bss_init__,--undefined=_end_of_init__
endif

# Get specs from configuration file
ALL_SPECS = $(shell sed -e 's/\#.*$$//' -e "/^\s*$$/d" $(TWO)/h2spec.conf)
SPEC ?= $(ALL_SPECS)

.PHONY: h2spec-pre
h2spec-pre:
	@echo "------------------------------"
	@echo "Integration tests with h2spec"
	@echo "------------------------------"

.PHONY: $(ALL_SPECS)
$(ALL_SPECS): TARGET=native
$(ALL_SPECS): /usr/local/bin/h2spec $(CONTIKI_PROJECT)
	@if ! test -f summary.txt; then echo "0 0" > summary.txt; fi
	@(sudo ./server.native 2>/dev/null > server.log & echo $$! > server.pid) && sleep 0.5 && \
		(h2spec $@ -p $(HTTP2_PORT) -h [$(ADDR)] > h2spec.log && rm h2spec.log); \
		PID=$$(cat server.pid); \
		TOTAL=$$(awk '{print $$1 + 1}' summary.txt); \
		FAILURES=$$(awk '{print $$2}' summary.txt); \
		sed -i 1,20d server.log; \
		echo -n "$@: "; \
		if test -e /proc/$$PID; then \
			sudo pkill server.native && \
			if test -f h2spec.log; then \
				echo "FAIL"; \
				echo "  Client output: "; \
				FAILURES=$$(($$FAILURES + 1)); \
				cat h2spec.log | sed -e/^Failures:/\{ -e:1 -en\;b1 -e\} -ed | grep -a -B 1 -A 4 "×" | sed /^$$/q; \
				if test -s server.log; then echo "  Server output:"; cat server.log | sed "s/^/    /"; fi; \
				rm h2spec.log; \
			else \
				echo "PASS"; \
			fi; \
		else \
			FAILURES=$$(($$FAILURES + 1)); \
			echo "FAIL"; \
			echo "  Server output:"; cat server.log | sed "s/^/    /" ; \
		fi; \
		echo "$$TOTAL $$FAILURES" > summary.txt; \
		rm server.pid server.log

h2spec: h2spec-pre $(SPEC)
	@echo "------------------------------"
	@awk '{printf "total: %d, passed: %d, failed: %d\n", $$1,$$1 - $$2,$$2}' summary.txt; \
		FAILED=$$(awk '{print $$2}' summary.txt); \
		rm summary.txt; \
		test $$FAILED -eq 0


include $(CONTIKI)/Makefile.include

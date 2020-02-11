CONTIKI_PROJECT = server
all: $(CONTIKI_PROJECT)

# Relevant directories

# Source directories
TWO ?= ../..
SRC = $(TWO)/src

# Contiki sources
CONTIKI ?= ../../../contiki-ng


PROJECTDIRS += $(SRC) $(SRC)/hpack
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

# Override contiki configuration
DEFINES += PROJECT_CONF_H=\"project-conf.h\"

#Default contiki target
TARGET ?= native

# Include src directory first
# to avoid clashes with system include
# files
CFLAGS += -I$(SRC)

# Include shell module by default
ifeq ($(DISABLE_SHELL),)
MODULES += os/services/shell
endif

# Use newlib for ARM based targets
# (save memory)
ifneq ($(filter $(TARGET),zoul openmote iotlab cc2538dk nrf52dk),)
LDFLAGS += -specs=nano.specs
endif

# Get specs from configuration file
H2SPEC_ALL = $(filter-out $(H2SPEC_SKIP), $(shell sed -e 's/\#.*$$//' -e "/^\s*$$/d" $(TWO)/h2spec.conf))
H2SPEC_TEST ?= $(H2SPEC_ALL)

# HTTP2 server port and address for h2spec testing
HTTP2_PORT ?= 8888
HTTP2_ADDR ?= fd00::302:304:506:708

# Begin h2spec targets
# `make h2spec` will run all h2spec tests
# against a native target
# 
# `make <spec>` will run one of the specs
# in h2spec.conf against a native target 
#
# `TARGET=<target != native> HTTP2_ADDR=<addr> make h2spec`
# will run h2spec against a remote target with IPv6 address HTTP2_ADDR
.PHONY: h2spec-pre h2spec-post
h2spec-pre:
	@rm -f summary.txt
	@echo "------------------------------"
	@echo "Integration tests with h2spec"
	@echo "------------------------------"

.PHONY: $(H2SPEC_ALL)

# if we are testing a native target
ifeq ($(TARGET),native)
$(H2SPEC_ALL): server.native /usr/local/bin/h2spec
	@if ! test -f summary.txt; then echo "0 0" > summary.txt; fi
	@(sudo ./server.native 2>/dev/null > server.log & echo $$! > server.pid) && sleep 0.5 && \
		(h2spec $@ -p $(HTTP2_PORT) -h [$(HTTP2_ADDR)] > h2spec.log && rm h2spec.log); \
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
else
# we are testing a remote target

# Specs to skip on embedded targets (frame size too large)
H2SPEC_SKIP = http2/4.2/1 http2/4.2/2 http2/4.2/3 http2/6.10/1

$(H2SPEC_ALL): /usr/local/bin/h2spec
	@if ! test -f summary.txt; then echo "0 0" > summary.txt; fi
	@(h2spec $@ -p $(HTTP2_PORT) -h [$(HTTP2_ADDR)] > h2spec.log && rm h2spec.log); \
		TOTAL=$$(awk '{print $$1 + 1}' summary.txt); \
		FAILURES=$$(awk '{print $$2}' summary.txt); \
		/bin/echo -n "$@: "; \
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
		echo "$$TOTAL $$FAILURES" > summary.txt && sleep 1
endif

h2spec-post:
	@echo "------------------------------"
	@awk '{printf "total: %d, passed: %d, failed: %d\n", $$1,$$1 - $$2,$$2}' summary.txt; \
		FAILED=$$(awk '{print $$2}' summary.txt); \
		rm summary.txt; \
		test $$FAILED -eq 0

h2spec: h2spec-pre $(H2SPEC_TEST) h2spec-post

include $(CONTIKI)/Makefile.include

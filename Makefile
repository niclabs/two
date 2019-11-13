TWO = .
CFLAGS = -std=c99 -Wall -Wextra

TARGETDIRS = examples/server/
TESTDIRS = tests/

MODULES	+= hpack http2 frames

all: server

PORT ?= 8888
STOP_ON_FAIL ?= 1

# Get specs from configuration file
ALL_SPECS = $(shell sed -e 's/\#.*$$//' -e "/^\s*$$/d" h2spec.conf)
SPEC ?= $(ALL_SPECS)


.PHONY: $(ALL_SPECS)
$(ALL_SPECS): /usr/local/bin/h2spec ./bin/server
	@echo "--- h2spec $@ -p $(PORT) ---"
	@(./bin/server $(PORT) & echo $$! > server.pid)
	@h2spec $@ -p $(PORT) > h2spec.log || STOP=$(STOP_ON_FAIL) &&\
		kill `cat server.pid` || true &&\
		cat h2spec.log &&\
		echo "-------" &&\
		rm server.pid h2spec.log &&\
		test "$${STOP}" != 1

h2spec: $(SPEC)


include $(TWO)/Makefile.include

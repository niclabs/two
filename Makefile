TWO = .
CFLAGS = -std=c99 -Wall -Wextra

TARGETDIRS = examples/server/ examples/echo/ examples/tiny/
TESTDIRS = tests/

MODULES	+= hpack http2 frames

all: server echo tiny

PORT ?= 8888
CI ?= 0

# Get specs from configuration file
ALL_SPECS = $(shell sed -e 's/\#.*$$//' -e "/^\s*$$/d" h2spec.conf)
SPEC ?= $(ALL_SPECS)


.PHONY: h2spec-pre
h2spec-pre: 
	@echo "------------------------------"
	@echo "Integration tests with h2spec"
	@echo "------------------------------"

.PHONY: $(ALL_SPECS)
$(ALL_SPECS): /usr/local/bin/h2spec ./bin/server
	@if ! test -f summary.txt; then echo "0 0" > summary.txt; fi 
	@(./bin/server $(PORT) 2> server.log & echo $$! > server.pid) && \
		(h2spec $@ -p $(PORT) > h2spec.log && rm h2spec.log) || true && \
		TOTAL=$$(awk '{print $$1 + 1}' summary.txt) && \
		FAILURES=$$(awk '{print $$2}' summary.txt) && \
		echo -n "$@: " && \
		if test -e /proc/`cat server.pid`; then \
			kill `cat server.pid`; \
			if test -f h2spec.log; then \
				echo "FAIL"; \
				echo "Client output: " && \
				FAILURES=$$(($$FAILURES + 1)); \
				cat h2spec.log | sed -e/^Failures:/\{ -e:1 -en\;b1 -e\} -ed | grep -a -B 1 -A 2 "×"; \
				if test -s server.log; then echo "Server output:"; cat server.log | sed "s/^/    /"; fi; \
				rm h2spec.log; \
			else \
				echo "PASS"; \
			fi; \
		else \
			FAILURES=$$(($$FAILURES + 1)); \
			echo "FAIL"; \
			cat server.log; \
		fi && \
		echo "$$TOTAL $$FAILURES" > summary.txt && \
		rm server.pid server.log

h2spec: h2spec-pre $(SPEC)
	@echo "------------------------------"
	@awk '{printf "total: %d, passed: %d, failed: %d\n", $$1,$$1 - $$2,$$2}' summary.txt && \
		FAILED=$$(awk '{print $$2}' summary.txt) && \
		rm summary.txt && \
		test $$FAILED -eq 0

include $(TWO)/Makefile.include

# Base directory
TWO = .

# Where are the source files for implementation
TARGETDIRS = examples/basic/

# Where are test files located
TESTDIRS = tests/

# All targets
all: basic

# Integration tests with h2spec
# `make h2spec` will run all h2spec tests
# against a local server
# 
# `make <spec>` will run one of the specs
# in h2spec.conf against a local target 
#
# NOTE: it does not work in OS X
H2SPEC_ALL = $(filter-out $(H2SPEC_SKIP), $(shell sed -e 's/\#.*$$//' -e "/^\s*$$/d" $(TWO)/h2spec.conf))
H2SPEC_TEST ?= $(H2SPEC_ALL)

# Port to use for h2spec tests
H2SPEC_PORT ?= 8888

.PHONY: h2spec-pre h2spec-post
h2spec-pre: 
	@rm -f summary.txt
	@echo "------------------------------"
	@echo "Integration tests with h2spec"
	@echo "------------------------------"

h2spec-post:
	@echo "------------------------------"
	@awk '{printf "total: %d, passed: %d, failed: %d\n", $$1,$$1 - $$2,$$2}' summary.txt; \
		FAILED=$$(awk '{print $$2}' summary.txt); \
		rm summary.txt; \
		test $$FAILED -eq 0

.PHONY: $(H2SPEC_ALL)
$(H2SPEC_ALL): /usr/local/bin/h2spec ./bin/basic
	@if ! test -f summary.txt; then echo "0 0" > summary.txt; fi 
	@(./bin/basic $(H2SPEC_PORT) 2> server.log & echo $$! > server.pid) && sleep 0.3 && \
		(h2spec $@ -p $(H2SPEC_PORT) > h2spec.log && rm h2spec.log); \
		TOTAL=$$(awk '{print $$1 + 1}' summary.txt); \
		FAILURES=$$(awk '{print $$2}' summary.txt); \
		echo -n "$@: "; \
		if test -e /proc/`cat server.pid`; then \
			kill `cat server.pid`; \
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
		sleep 0.3; \
		echo "$$TOTAL $$FAILURES" > summary.txt; \
		rm server.pid server.log

# Run all target
h2spec: h2spec-pre $(H2SPEC_ALL) h2spec-post
	
include $(TWO)/Makefile.include

TWO = .
CFLAGS = -std=c99 -Wall -Wextra 
DEBUG_FLAGS = -DENABLE_DEBUG

TARGETDIRS = examples/client/ examples/server/
TESTDIRS = tests/

MODULES	+= hpack http2

all: client server

PORT=8888
server.pid: ./bin/server
	@(./bin/server $(PORT) & echo $$! > $@)

h2spec: /usr/local/bin/h2spec server.pid
	h2spec $(SPEC) -p $(PORT) > h2spec.log
	@-kill `cat server.pid` 
	@cat h2spec.log
	@rm server.pid h2spec.log


include $(TWO)/Makefile.include

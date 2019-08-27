TWO = .
CFLAGS = -std=c99 -Wall -Wextra 
DEBUG_FLAGS = -DENABLE_DEBUG

TARGETDIRS = examples/client/ examples/server/
TESTDIRS = tests/

all: client server

include $(TWO)/Makefile.include

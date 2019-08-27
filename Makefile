TWO = .
CFLAGS = -std=c99 -Wall -Wextra 

PROJECTDIRS = examples/client/ examples/server/

all: client server

include $(TWO)/Makefile.include

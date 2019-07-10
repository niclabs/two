QUIET ?= 1

TARGET_DIR = ./bin
SUBDIRS = ./src/ ./tests/ ./examples/client/ ./examples/server/

### General build rules
include $(CURDIR)/Makefile.include

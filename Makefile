QUIET ?= 1

TARGET_DIR = ./bin
SUBDIRS = ./src/ ./tests/ ./examples/client/

### General build rules
include $(CURDIR)/Makefile.include

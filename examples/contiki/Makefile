CONTIKI_PROJECT = server
all: $(CONTIKI_PROJECT)

# Two base directory
TWO ?= ../..

# Contiki sources
CONTIKI = ../../../contiki-ng

# Override contiki configuration
DEFINES += PROJECT_CONF_H=\"project-conf.h\"

# Include contiki makefile
include $(TWO)/Makefile.contiki

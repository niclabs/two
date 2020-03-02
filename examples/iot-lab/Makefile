CONTIKI_PROJECT = server
all: $(CONTIKI_PROJECT)

TARGET = iotlab
BOARD ?= m3

# Two base directory
TWO ?= ../..

# Contiki sources
CONTIKI = ../../../contiki-ng

# Override contiki configuration
DEFINES += PROJECT_CONF_H=\"project-conf.h\"

# Include contiki makefile
include $(TWO)/Makefile.contiki

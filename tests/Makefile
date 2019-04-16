# Shell used in this makefile
# bash is used for 'echo -en'
SHELL = /bin/bash

ifeq ($(OS),Windows_NT)
  ifeq ($(shell uname -s),) # not in a bash-like shell
	MKDIR = mkdir
  else # in a bash-like shell, like msys
	MKDIR = mkdir -p
  endif
	TARGET_EXTENSION=.exe
else
	MKDIR = mkdir -p
	TARGET_EXTENSION=out
endif

.PHONY: test

BASEDIR		?= $(CURDIR)/..
BUILDDIR	?= $(BASEDIR)/build
DEPSDIR		?= $(BASEDIR)/deps
SRCDIR		?= $(BASEDIR)/src
TESTSDIR	?= $(BASEDIR)/tests

PATHU = $(DEPSDIR)/unity/src/
PATHS = $(SRCDIR)
PATHT = $(TESTSDIR)
PATHB = $(BUILDDIR)/tests/
PATHD = $(PATHB)depends/
PATHO = $(PATHB)objs/
PATHR = $(PATHB)results/

# Make all paths absolute.
override PATHU := $(abspath $(PATHU))/
override PATHS := $(abspath $(PATHS))/
override PATHT := $(abspath $(PATHT))/
override PATHB := $(abspath $(PATHB))/
override PATHD := $(abspath $(PATHD))/
override PATHO := $(abspath $(PATHO))/
override PATHR := $(abspath $(PATHR))/

BUILD_PATHS = $(PATHB) $(PATHD) $(PATHO) $(PATHR) $(PATHU)

SRCT = $(wildcard $(PATHT)*.c)

COMPILE=gcc -c
LINK=gcc
DEPEND=gcc -MM -MG -MF
CFLAGS=-I. -I$(PATHU) -I$(PATHS) -DTEST

RESULTS = $(patsubst $(PATHT)test_%.c,$(PATHR)test_%.txt,$(SRCT) )

PASSED = `grep -s PASS $(PATHR)*.txt`
FAIL = `grep -s FAIL $(PATHR)*.txt`
IGNORE = `grep -s IGNORE $(PATHR)*.txt`

test: $(BUILD_PATHS) $(RESULTS)
	@echo -e "-----------------------\nIGNORES:\n-----------------------"
	@echo -e "$(IGNORE)"
	@echo -e "-----------------------\nFAILURES:\n-----------------------"
	@echo -e "$(FAIL)"
	@echo -e "-----------------------\nPASSED:\n-----------------------"
	@echo -e "$(PASSED)"
	@echo -e "\nDONE"

$(PATHR)%.txt: $(PATHB)%.$(TARGET_EXTENSION)
	$(Q)-$< > $@ 2>&1

$(PATHB)test_%.$(TARGET_EXTENSION): $(PATHO)test_%.o $(PATHO)%.o $(PATHO)unity.o #$(PATHD)test_%.d
	$(Q)$(LINK) -o $@ $^

$(PATHO)%.o:: $(PATHT)%.c
	$(Q)$(COMPILE) $(CFLAGS) $< -o $@

$(PATHO)%.o:: $(PATHS)%.c
	$(Q)$(COMPILE) $(CFLAGS) $< -o $@

$(PATHO)%.o:: $(PATHU)%.c $(PATHU)%.h
	$(Q)$(COMPILE) $(CFLAGS) $< -o $@

$(PATHD)%.d:: $(PATHT)%.c
	$(Q)$(DEPEND) $@ $<

$(PATHB):
	$(Q)$(MKDIR) $(PATHB)

$(PATHD):
	$(Q)$(MKDIR) $(PATHD)

$(PATHO):
	$(Q)$(MKDIR) $(PATHO)

$(PATHR):
	$(Q)$(MKDIR) $(PATHR)

$(PATHU):
	$(Q)git clone https://github.com/ThrowTheSwitch/Unity.git unity

.PRECIOUS: $(PATHB)test_%.$(TARGET_EXTENSION)
.PRECIOUS: $(PATHD)%.d
.PRECIOUS: $(PATHO)%.o
.PRECIOUS: $(PATHR)%.txt

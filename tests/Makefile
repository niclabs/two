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

COMPILE=gcc -c -Wall -O0
LINK=gcc 
DEPEND=gcc -MM -MG -MF
CFLAGS=-I. -I$(PATHU) -I$(PATHS) -DTEST 
LDFLAGS += --coverage

ifneq ($(DEBUG),)
	CFLAGS += -DENABLE_DEBUG
endif

RESULTS = $(patsubst $(PATHT)test_%.c,$(PATHR)test_%.txt,$(SRCT) )

null := 
space = $(null) $(null)

PASSED = $(shell grep -s PASS $(PATHR)*.txt)
PASSED_COUNT = $(words $(subst \n,$(space),$(subst $(space),-,$(PASSED))))
FAIL = $(shell grep -s FAIL $(PATHR)*.txt)
FAIL_COUNT = $(words $(subst \n,$(space),$(subst $(space),-,$(FAIL))))
IGNORE = $(shell grep -s IGNORE $(PATHR)*.txt)
IGNORE_COUNT = $(words $(subst \n,$(space),$(subst $(space),-,$(IGNORE))))

test: $(BUILD_PATHS) $(RESULTS)
	@echo -e "-----------------------\nIGNORES: $(IGNORE_COUNT)\n-----------------------"
	@echo -e "$(IGNORE)"
	@echo -e "-----------------------\nFAILURES: $(FAIL_COUNT)\n-----------------------"
	@echo -e "$(FAIL)"
	@echo -e "-----------------------\nPASSED: $(PASSED_COUNT)\n-----------------------"
	@echo -e "$(PASSED)"
	@echo -e "\n$(if $(FAIL),FAILED,OK)"
	$(Q)test -z "$(FAIL)"

$(PATHR)%.txt: $(PATHB)%.$(TARGET_EXTENSION)
	$(Q)-$< > $@
	$(Q)test -s $@ || ($(RM) $@ && false)  # if file size is zero then an error ocurred before the redirection

$(PATHB)test_%.$(TARGET_EXTENSION): $(PATHO)test_%.o $(PATHO)%.o $(PATHO)unity.o #$(PATHD)test_%.d
	$(Q)$(LINK) $(LDFLAGS) -o $@ $^

$(PATHO)%.o:: $(PATHT)%.c
	$(Q)$(COMPILE) $(CFLAGS) -g -fprofile-arcs -ftest-coverage $< -o $@

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
	$(Q)git clone https://github.com/ThrowTheSwitch/Unity.git $(DEPSDIR)/unity

.PRECIOUS: $(PATHB)test_%.$(TARGET_EXTENSION)
.PRECIOUS: $(PATHD)%.d
.PRECIOUS: $(PATHO)%.o
.PRECIOUS: $(PATHR)%.txt

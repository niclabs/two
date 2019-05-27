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

.PHONY: test clean dirs

BASEDIR		?= $(CURDIR)/..
BUILDDIR	?= $(BASEDIR)/build
SRCDIR		?= $(BASEDIR)/src
TESTSDIR	?= $(BASEDIR)/tests

PATHU = $(BUILDDIR)/unity/src/
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

COMPILE=gcc -c -Wall -O0 -std=c99
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
nl := \n

PASSED = $(shell grep -s PASS $(PATHR)*.txt)
PASSED_COUNT = $(words $(PASSED))
FAIL = $(shell grep -s FAIL $(PATHR)*.txt)
FAIL_COUNT = $(words $(FAIL))
IGNORE = $(shell grep -s IGNORE $(PATHR)*.txt)
IGNORE_COUNT = $(words $(IGNORE))

test: $(BUILD_PATHS) $(RESULTS)
	@echo -e "-----------------------\nIGNORES: $(IGNORE_COUNT)\n-----------------------"
	@echo -e "$(subst $(space),$(nl),$(IGNORE))"
	@echo -e "-----------------------\nFAILURES: $(FAIL_COUNT)\n-----------------------"
	@echo -e "$(subst $(space),$(nl),$(FAIL))"
	@echo -e "-----------------------\nPASSED: $(PASSED_COUNT)\n-----------------------"
	@echo -e "$(subst $(space),$(nl),$(PASSED))"
	@echo -e "\n$(if $(FAIL),FAILED,OK)"
	@test -z "$(FAIL)"

ALL_TESTS = $(patsubst $(PATHT)test_%.c,test_%,$(SRCT))
.PHONY: $(ALL_TESTS)
$(ALL_TESTS): test_%: $(BUILD_PATHS) $(PATHB)test_%.$(TARGET_EXTENSION)
	$(Q)$(PATHB)$@.$(TARGET_EXTENSION)

clean:
	@$(RM) -r $(PATHB)

$(PATHR)%.txt: $(PATHB)%.$(TARGET_EXTENSION)
	$(Q)-trap '$(RM) $@' INT; $< > $@
	@test -s $@ || ($(RM) $@ && false)  # if file size is zero then an error ocurred before the redirection

$(PATHB)test_%.$(TARGET_EXTENSION): $(PATHO)test_%.o $(PATHO)%.o $(PATHO)unity.o #$(PATHD)test_%.d
	$(Q)$(LINK) $(LDFLAGS) -o $@ $^

$(PATHO)%.o:: $(PATHT)%.c $(PATHT)unit.h
	$(Q)$(COMPILE) $(CFLAGS) -g $< -o $@

$(PATHO)%.o:: $(PATHS)%.c
	$(Q)$(COMPILE) $(CFLAGS) -fprofile-arcs -ftest-coverage $< -o $@

$(PATHO)unity.o:: $(PATHU)unity.c $(PATHU)unity.h
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
	$(Q)git clone https://github.com/ThrowTheSwitch/Unity.git $(BUILDDIR)/unity

.PRECIOUS: $(PATHB)test_%.$(TARGET_EXTENSION)
.PRECIOUS: $(PATHD)%.d
.PRECIOUS: $(PATHO)%.o
.PRECIOUS: $(PATHR)%.txt

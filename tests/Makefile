ifeq ($(OS),Windows_NT)
	EXT=exe
else
	EXT=out
endif

# Directories
BASEDIR		?= ..
BUILD_DIR	?= $(BASEDIR)/build

# Target configuration
TARGET=test.PHONY

SUBDIRS := unity

# General CFLAG configuration for tests
CFLAGS := -g

# Add sources
TESTS   := $(notdir $(wildcard $(BASEDIR)/tests/*.c))
SOURCES := $(TESTS) $(TESTS:test_%.c=../src/%.c)

# Base object directory for target
OBJDIR := $(BUILD_DIR)/test

RESULTS = $(patsubst test_%.c,$(OBJDIR)/test_%.txt,$(TESTS))


# Test formatting variables
null :=
space = $(null) $(null)
nl := \n

PASSED = $(shell grep -s PASS $(OBJDIR)/*.txt)
PASSED_COUNT = $(words $(PASSED))
FAIL = $(shell grep -s FAIL $(OBJDIR)/*.txt)
FAIL_COUNT = $(words $(FAIL))
IGNORE = $(shell grep -s IGNORE $(OBJDIR)/*.txt)
IGNORE_COUNT = $(words $(IGNORE))


test: $(RESULTS)
	@echo -e "-----------------------\nIGNORES: $(IGNORE_COUNT)\n-----------------------"
	@echo -e "$(subst $(space),$(nl),$(IGNORE))"
	@echo -e "-----------------------\nFAILURES: $(FAIL_COUNT)\n-----------------------"
	@echo -e "$(subst $(space),$(nl),$(FAIL))"
	@echo -e "-----------------------\nPASSED: $(PASSED_COUNT)\n-----------------------"
	@echo -e "$(subst $(space),$(nl),$(PASSED))"
	@echo -e "\n$(if $(FAIL),FAILED,OK)"
	@test -z "$(FAIL)"

TEST_TARGETS = $(patsubst test_%.c,test_%,$(TESTS))
.PHONY: $(TEST_TARGETS)
$(TEST_TARGETS): test_%: $(OBJDIR)/test_%.$(EXT)
	$(Q)$(OBJDIR)/$@.$(EXT)
	@echo ""
	@echo -e "--------------------------\nCode coverage data\n--------------------------"
	$(Q)gcov -b -n $(patsubst test_%,$(OBJDIR)/src/%.gcno,$@)

# Build results
$(OBJDIR)/%.txt: $(OBJDIR)/%.$(EXT)
	$(Q)-trap '$(RM) $@' INT; $< > $@
	@test -s $@ || ($(RM) $@ && false)  # if file size is zero then an error ocurred before the redirection

# Build test binaries
$(OBJDIR)/test_%.$(EXT): LDFLAGS += --coverage
$(OBJDIR)/test_%.$(EXT): $(OBJDIR)/tests/test_%.o $(OBJDIR)/src/%.o $(OBJDIR)/tests/unity/unity.o
	@mkdir -p $(dir $@)
	$(TRACE_LD)
	$(Q)$(strip $(LINK))

# Object specific CFLAGS
$(OBJDIR)/src/%.o: CFLAGS += -DNDEBUG -fprofile-arcs -ftest-coverage
$(OBJDIR)/tests/%.o: CFLAGS += -Wno-unused-parameter

# Add extra build files
BUILD_FILES += $(wildcard $(OBJDIR)/test_*.$(EXT)) \
			   $(RESULTS) \
			   $(wildcard $(OBJDIR)/src/*.gcda) \
			   $(wildcard $(OBJDIR)/src/*.gcno) 


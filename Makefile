# Makefile
# Dylan Armstrong, 2026

# Detect OS
ifdef OS
    # Windows_NT environment variable is set on Windows
    OS_TYPE := Windows_NT
else
    UNAME_S := $(shell uname -s)
    OS_TYPE := $(UNAME_S)
endif

# Default target
.DEFAULT_GOAL := all

# If on Windows, delegate to Makefile.win
ifeq ($(OS_TYPE),Windows_NT)
    # Windows specific - delegate to Makefile.win
    all clean generate build test help:
		$(MAKE) -f Makefile.win $@
else ifeq ($(OS_TYPE),Darwin)
    # macOS specific - delegate to Makefile.mac
    all clean generate build test help:
		$(MAKE) -f Makefile.mac $@
else ifeq ($(OS_TYPE),Linux)
    # Linux specific - delegate to Makefile.linux
    all clean generate build test help:
		$(MAKE) -f Makefile.linux $@
else
    # For other systems - add more specific rules here
    $(error This system ($(OS_TYPE)) is not yet supported. Currently Windows, macOS, and Linux are supported.)
endif

.PHONY: all clean generate build test help

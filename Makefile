# Makefile
# Dylan Armstrong, 2026

# Detect OS
UNAME_S := $(shell uname -s)
OS := $(OS)

# Default target
.DEFAULT_GOAL := all

# If on macOS, delegate to Makefile.mac
ifeq ($(UNAME_S),Darwin)
    # macOS specific - delegate to Makefile.mac
    all clean generate build test help:
		$(MAKE) -f Makefile.mac $@
else ifeq ($(OS),Windows_NT)
    # Windows specific - delegate to Makefile.win
    all clean generate build test help:
		$(MAKE) -f Makefile.win $@
else
    # For other systems (Linux, etc.) - add Linux specific rules here
    $(error This system ($(UNAME_S)) is not yet supported. Currently only macOS and Windows are supported.)
endif

.PHONY: all clean generate build test help

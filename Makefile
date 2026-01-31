# Makefile
# Dylan Armstrong, 2026

# Detect OS
UNAME_S := $(shell uname -s)

# Default target
.DEFAULT_GOAL := all

# If on macOS, delegate to Makefile.mac
ifeq ($(UNAME_S),Darwin)
    # macOS specific - delegate to Makefile.mac
    all clean generate build test help:
		$(MAKE) -f Makefile.mac $@
else
    # For other systems (Linux, Windows, etc.) - add Linux/Windows specific rules here
    $(error This system ($(UNAME_S)) is not yet supported. Currently only macOS is supported.)
endif

.PHONY: all clean generate build test help

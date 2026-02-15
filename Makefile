# Makefile
# Dylan Armstrong, Kodo (newbee1905), 2026
#
# This Makefile delegates the build process to OS-specific files.
# It detects the operating system and calls the appropriate Makefile.

# This file itself has no build targets, only delegation.
.PHONY: help

# Default target
.DEFAULT_GOAL := all

# Detect OS and set the appropriate Makefile
ifeq ($(OS),Windows_NT)
    OS_TYPE := Windows
    MAKEFILE_TO_USE := Makefile.win
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        OS_TYPE := Linux
        MAKEFILE_TO_USE := Makefile.unix
    else ifeq ($(UNAME_S),Darwin)
        OS_TYPE := macOS
        MAKEFILE_TO_USE := Makefile.unix
    else
        $(error This system ($(UNAME_S)) is not supported. Only Windows, Linux, and macOS.)
    endif
endif

# Help message for the root Makefile
help:
	@echo "Top-level Makefile. Using '$(MAKEFILE_TO_USE)' for $(OS_TYPE)."
	@echo "Run 'make <target>' where target is one of the delegated commands."
	@echo "------------------------------------------------------------"
	@$(MAKE) -f $(MAKEFILE_TO_USE) help

# Fallback rule to delegate any other target to the OS-specific Makefile.
# This passes all targets (like 'build', 'clean', 'debug') and variables
# (like 'MODE=debug') to the underlying Makefile.
%:
	@$(MAKE) -f $(MAKEFILE_TO_USE) $@

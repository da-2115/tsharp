#!/bin/bash

# Build script for T# Programming Language
# Supports: macOS, Linux
# Dylan Armstrong, 2026

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Detect OS
OS_TYPE=$(uname -s)

echo -e "Detected OS: ${YELLOW}$OS_TYPE${NC}"
echo ""

# Check for required tools
echo -e "${BLUE}Checking dependencies...${NC}"

if ! command -v cmake &> /dev/null; then
    echo -e "${RED}✗ CMake not found${NC}"
    echo "Install with:"
    if [ "$OS_TYPE" = "Darwin" ]; then
        echo "  brew install cmake"
    else
        echo "  sudo apt-get install cmake"
    fi
    exit 1
fi
echo -e "${GREEN}✓ CMake${NC}"

if ! command -v make &> /dev/null; then
    echo -e "${RED}✗ Make not found${NC}"
    echo "Install with:"
    if [ "$OS_TYPE" = "Darwin" ]; then
        echo "  brew install make"
    else
        echo "  sudo apt-get install build-essential"
    fi
    exit 1
fi
echo -e "${GREEN}✓ Make${NC}"

if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo -e "${RED}✗ C++ compiler not found${NC}"
    echo "Install with:"
    if [ "$OS_TYPE" = "Darwin" ]; then
        echo "  xcode-select --install"
    else
        echo "  sudo apt-get install build-essential"
    fi
    exit 1
fi
echo -e "${GREEN}✓ C++ Compiler${NC}"

if ! command -v java &> /dev/null; then
    echo -e "${RED}✗ Java not found (required for ANTLR)${NC}"
    echo "Install with:"
    if [ "$OS_TYPE" = "Darwin" ]; then
        echo "  brew install openjdk"
    else
        echo "  sudo apt-get install openjdk-11-jdk"
    fi
    exit 1
fi
echo -e "${GREEN}✓ Java${NC}"

echo ""
echo -e "${BLUE}Building T# Compiler...${NC}"
echo ""

# Create build directory if needed
if [ ! -d "build" ]; then
    mkdir build
fi

# Run CMake
echo -e "${YELLOW}Running CMake...${NC}"
cmake . -B build

# Build with Make
echo -e "${YELLOW}Running Make...${NC}"
# Determine parallel jobs (nproc not available on macOS, use sysctl instead)
if command -v nproc &> /dev/null; then
    JOBS=$(nproc)
else
    JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 1)
fi
make -C build -j$JOBS

echo ""

# Check if binary was created and copy to current directory
if [ -f "build/tsharp" ]; then
    cp build/tsharp ./tsharp
fi

if [ -f "./tsharp" ]; then
    echo -e "${GREEN}✓ Build successful!${NC}"
    echo ""
    echo -e "${BLUE}T# compiler location:${NC} $(pwd)/tsharp"
    echo -e "${BLUE}Version:${NC}"
    ./tsharp --version 2>/dev/null || echo "  (version check not available)"
    echo ""
    echo -e "${GREEN}Ready to use! Try:${NC}"
    echo "  ./test.sh              # Run test suite"
    echo "  ./tsharp examples/test.tsharp  # Run example"
    exit 0
else
    echo -e "${RED}✗ Build failed - compiler binary not found${NC}"
    exit 1
fi

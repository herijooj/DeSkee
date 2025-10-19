#!/usr/bin/env bash
# Run script for DS/DSi development
# This ensures you're in the right environment

cd "$(dirname "$0")"

echo "🎮 Building and running DS/DSi homebrew..."
echo ""

# Check if we're already in nix develop
#!/bin/bash

# Check if we're in a nix shell
if command -v nix &> /dev/null && [ -z "$IN_NIX_SHELL" ]; then
    echo "Starting Nix shell..."
    nix develop --command "$0"
    exit 0
fi

if [ -f "Makefile" ]; then
    if command -v make &> /dev/null; then
        echo "Building DeSkee.nds..."
        if make; then
            if command -v desmume &> /dev/null; then
                echo "Launching DeSmuME..."
                nix run nixpkgs#desmume -- DeSkee.nds &
            fi
        fi
    else
        echo "Makefile found but 'make' command not found. Please install GNU Make."
    fi
else
    if [ -f "../Makefile" ]; then
        echo "Building DeSkee.nds..."
        cd ..
        if make; then
            if command -v desmume &> /dev/null; then
                echo "Launching DeSmuME..."
                nix run nixpkgs#desmume -- DeSkee.nds &
            fi
        fi
    else
        echo "Makefile not found. Are you in the project directory?"
    fi
fi

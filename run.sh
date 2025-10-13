#!/usr/bin/env bash
# Run script for DS/DSi development
# This ensures you're in the right environment

cd "$(dirname "$0")"

echo "🎮 Building and running DS/DSi homebrew..."
echo ""

# Check if we're already in nix develop
if [ -z "$IN_NIX_SHELL" ]; then
    echo "Entering nix development environment..."
    nix develop --command bash -c '
        echo "Building dsi-app.nds..."
        if make; then
            echo "Launching DeSmuME..."
            nix run nixpkgs#desmume -- dsi-app.nds &
        else
            exit 1
        fi
    '
else
    echo "Building dsi-app.nds..."
    if make; then
        echo "Launching DeSmuME..."
        nix run nixpkgs#desmume -- dsi-app.nds &
    else
        exit 1
    fi
fi

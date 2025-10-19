#!/usr/bin/env bash
# Run script with melonDS for DS/DSi development

cd "$(dirname "$0")"

echo "🎮 Building and running DS/DSi homebrew (melonDS)..."
echo ""

# Check if we're already in nix develop
if [ -z "$IN_NIX_SHELL" ]; then
    echo "Entering nix development environment..."
    nix develop --command bash -c '
        echo "Building DeSkee.nds..."
        if make; then
            echo "Launching melonDS..."
            nix run nixpkgs#melonDS -- DeSkee.nds &
        else
            exit 1
        fi
    '
else
    echo "Building DeSkee.nds..."
    if make; then
        echo "Launching melonDS..."
        nix run nixpkgs#melonDS -- DeSkee.nds &
    else
        exit 1
    fi
fi

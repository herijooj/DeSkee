{
  description = "devkitNix DSi/NDS project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    devkitNix.url = "github:bandithedoge/devkitNix";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    devkitNix,
    ...
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs {
        inherit system;
        # devkitNix provides an overlay with the toolchains
        overlays = [devkitNix.overlays.default];
      };
    in {
      # Development shell for Nintendo DS/DSi (devkitARM)
      # Includes DeSmuME emulator for testing
      devShells.default = pkgs.mkShell.override {stdenv = pkgs.devkitNix.stdenvARM;} {
        packages = with pkgs; [
          desmume   # Nintendo DS emulator
          melonDS   # More accurate DS emulator (alternative)
        ];
        
        shellHook = ''
          echo "🎮 Nintendo DS/DSi Development Environment"
          echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
          echo "Commands:"
          echo "  make        - Build the .nds file"
          echo "  make clean  - Clean build artifacts"
          echo "  run         - Build and run in DeSmuME"
          echo "  run-melon   - Build and run in melonDS"
          echo ""
          echo "Emulators available: DeSmuME, melonDS"
          
          # Create convenient run commands
          run() {
            echo "Building..."
            if make; then
              echo "Launching DeSmuME..."
              nix run nixpkgs#desmume -- dsi-app.nds &
            fi
          }
          export -f run
          
          run-melon() {
            echo "Building..."
            if make; then
              echo "Launching melonDS..."
              nix run nixpkgs#melonDS -- dsi-app.nds &
            fi
          }
          export -f run-melon
        '';
      };

      # Package build configuration for NDS
      packages.default = pkgs.devkitNix.stdenvARM.mkDerivation {
        name = "dsi-example";
        src = ./.;

        makeFlags = ["TARGET=dsi-app"];
        
        installPhase = ''
          mkdir -p $out
          cp dsi-app.nds $out
        '';
      };
    });
}

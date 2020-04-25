{ sources ? import ./nix/sources.nix
, nixpkgs ? sources.nixpkgs
, pkgs ? import nixpkgs {}
}:

pkgs.callPackage ./nix/build.nix {}

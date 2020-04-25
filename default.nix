{ sources ? import ./nix/sources.nix, nixpkgs ? sources.nixpkgs
, pkgs ? import nixpkgs { } }:

(import ./nix/ci.nix { inherit sources nixpkgs pkgs; }).default

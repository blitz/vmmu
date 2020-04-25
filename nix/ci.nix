{ sources ? import ./sources.nix, nixpkgs ? sources.nixpkgs
, pkgs ? import nixpkgs { } }:

let
  lib = pkgs.lib;

  configs = rec {
    gcc8-release = {
      stdenv = pkgs.gcc8Stdenv;
      buildType = "Release";
    };
    gcc8-debug = {
      stdenv = pkgs.gcc8Stdenv;
      buildType = "Debug";
    };

    gcc9-release = {
      stdenv = pkgs.gcc9Stdenv;
      buildType = "Release";
    };
    gcc9-debug = {
      stdenv = pkgs.gcc9Stdenv;
      buildType = "Debug";
    };

    clang-release = {
      stdenv = pkgs.llvmPackages_latest.stdenv;
      buildType = "Release";
    };
    clang-debug = {
      stdenv = pkgs.llvmPackages_latest.stdenv;
      buildType = "Debug";
    };

    coverage = {
      buildType = "Debug";
      coverage = true;
    };

    default = { buildType = "Release"; };
  };

  buildPackage = overrides: pkgs.callPackage ./build.nix overrides;

in lib.attrsets.mapAttrs (_: buildPackage) configs

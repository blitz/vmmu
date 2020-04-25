{ stdenv, lib, cmake, ninja, catch2, nix-gitignore, gcovr, python3Packages
, buildType ? "Debug", coverage ? false, asan ? false }:

stdenv.mkDerivation {
  pname = "vmmu";
  version = "0.9-dev";

  src = nix-gitignore.gitignoreSource [ ".git" "nix" "*.nix" ] ../.;

  nativeBuildInputs = [ cmake ninja ] ++ lib.optionals (coverage) [ gcovr ];
  checkInputs = [ catch2 ];

  cmakeBuildType = buildType;
  cmakeFlags = [
    "-DBUILD_COVERAGE=${if coverage then "ON" else "OFF"}"
    "-DASAN=${if asan then "ON" else "OFF"}"
  ];

  doCheck = true;
}

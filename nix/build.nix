{ stdenv, cmake, ninja, catch2, nix-gitignore }:

stdenv.mkDerivation {
  pname = "vmmu";
  version = "0.9-dev";

  src = nix-gitignore.gitignoreSource [ ".git" "nix" "*.nix" ] ../.;

  nativeBuildInputs = [ cmake ninja ];
  checkInputs = [ catch2 ];

  doCheck = true;
}

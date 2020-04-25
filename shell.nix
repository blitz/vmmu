{ sources ? import ./nix/sources.nix
, nixpkgs ? sources.nixpkgs
, pkgs ? import nixpkgs {}
}:

pkgs.mkShell {
  #inputsFrom = with pkgs; [ hello gnutar ];

  buildInputs = [ pkgs.niv pkgs.cmake pkgs.ninja pkgs.cmake-format pkgs.catch2 ];
}

{ pkgs }: {
	deps = [
		pkgs.unzip
  pkgs.nodejs
  pkgs.cmake
  pkgs.clang_12
		pkgs.ccls
		pkgs.gdb
		pkgs.gnumake
	];
}
# Compilation on Windows

clang sray.c -Lraylib-5.5_win64_msvc16\lib -lmsvcrt -lraylib -lOpenGL32 -lGdi32 -lWinMM -lkernel32 -lshell32 -lUser32 -Xlinker /NODEFAULTLIB:libcmt

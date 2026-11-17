# Rogue DOS Tribute JEC.091 — Windows README

These instructions are for 64-bit Windows 10/11. Rogue DOS Tribute is a derivative
based on the Rogue 5.4 family and inspired by DOS Rogue 1.48; it is
not a direct port of the original DOS 1.48 source. For game controls and items,
see rogue_en.html or rogue_en.txt. For macOS/Linux builds, see README_mac_linux_en.md.

## Prerequisites

Use the MSYS2 UCRT64 shell. The Windows build always uses the PDCursesMod
VT backend; the former wincon backend is not supported.

    pacman -S --needed make zip mingw-w64-ucrt-x86_64-gcc \
        mingw-w64-ucrt-x86_64-binutils mingw-w64-ucrt-x86_64-pdcurses

## Build and run

    make clean
    make
    ./rogue.exe

Change the executable name with PROGRAM or PROG (PROGRAM takes precedence):

    make PROGRAM=rogue-dos
    make PROG=rogue-dos

The regular build selects its initial language from the Windows UI language,
unless ROGUEOPTS contains language=ja or language=en. ROGUE_LANG is not used.
DOS-style key bindings are the default; ROGUEOPTS=nodoskeys selects Rogue 5.4-based keys.

    make japanese       Japanese initial language
    make english        English initial language
    make original       English initial language, unchanged gameplay/colors
    make wizard         Include wizard functions

These targets rebuild the .win.o objects without deleting save, score or lock
files. PROGRAM also determines their default names. Use FILEDIR=games to
specify a data directory; create that directory before launching the game.
Windows Unicode file operations support Japanese paths.

## Startup and display

A direct Explorer launch requests an 80x24 private console. Launching from
an existing cmd, PowerShell or UCRT64 session does not resize that terminal.
If the default saved game exists on a direct launch, choose:

    R / Enter    Resume saved adventure
    N            Start a new game
    F            Choose another save file
    Q            Quit

With no default save, a new game starts automatically. A Unicode filename can
also be dragged onto the EXE to restore that saved game.

    ./rogue.exe -s              Show scores
    ./rogue.exe -r              Resume default save
    ./rogue.exe 'Adventure.save' Resume named file

Restoring a game normally consumes (deletes) its save file: back it up before
testing. Ordinary Japanese input and standalone emoji are supported; complex
emoji sequences depend on the terminal and font.

## Windows distribution ZIP

After building the Windows executable in MSYS2 UCRT64, create a ZIP with:

    make dist.win

The archive `rogue_dos_tribute_jec091_windows-rogue.zip` contains the EXE,
Japanese/English Windows READMEs, the HTML/text guides, and LICENSE.TXT.
No DLLs or test scripts are bundled. MSYS2's zip utility is required:

    pacman -S --needed zip

If desired, inspect runtime DLL dependencies manually with `ldd rogue.exe`.
When all dependencies resolve to standard Windows System32 DLLs, bundling
extra DLLs is generally unnecessary. Before releasing, also test the EXE on
Windows 10/11 without MSYS2 installed. The Windows icon is embedded in the
EXE; use `make dist.src` for a source archive.

## GitHub Actions build verification

The .github/workflows/build.yml workflow runs on push, pull requests, and
manual dispatch. It builds the normal and initially-English (make original)
variants on Linux, macOS, and Windows UCRT64. It saves executables and
Windows distribution ZIPs as build artifacts.

Workflow artifacts are for build verification, not standalone releases.
Before publishing, test Windows Terminal and ConsoleHost (start/end curtains,
grave, floor transition, Japanese save/restore), and check that no extra
non-system runtime DLLs are required.

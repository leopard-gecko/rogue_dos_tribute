# Rogue DOS Tribute JEC.091 — Windows README

This source tree builds `rogue.exe` for 64-bit Windows 10/11.
For macOS/Linux build and installation instructions, see `README_mac_linux_en.md`.

Rogue DOS Tribute is an independent derivative based on Rogue 5.4-family
source code, including the portable framework of Rogue 5.4.5 JEC. Its
monsters, items, rules, controls, and presentation have been reworked with
reference to DOS Rogue 1.48. It is not a direct port of the original
DOS Rogue 1.48 source. It adds Japanese/English switching, color, and
support for modern macOS, Linux, and Windows.

## Build

Use the **MSYS2 UCRT64** shell on Windows. Install MSYS2, open its UCRT64
shell, and install these development packages:

    pacman -S --needed make mingw-w64-ucrt-x86_64-gcc \
        mingw-w64-ucrt-x86_64-binutils mingw-w64-ucrt-x86_64-pdcurses

Normally, build directly from the source directory with:

    make

The default build creates:

    ./rogue.exe

Run it with:

    ./rogue.exe

The standard build uses these convenience option defaults:

    color
    extchars
    nocursor

You can override local compiler settings on the make command line:

    make CC=gcc
    make CFLAGS="-O2"

To remove built objects and rebuild, use:

    make clean
    make

## Language selection

Select the startup display language with the `language` option in `ROGUEOPTS`.
In the UCRT64 shell, for example:

    ROGUEOPTS=language=ja ./rogue.exe
    ROGUEOPTS=language=en ./rogue.exe
    ROGUEOPTS=language=auto ./rogue.exe

`language=ja` selects Japanese, `language=en` selects English, and
`language=auto` chooses the initial language from the OS settings. `auto`
only determines the initial language and does not appear in the in-game
options. During play, use the `language` option and press `J` for Japanese
or `E` for English. Names and call names are stored as UTF-8 strings.

The `ROGUEOPTS` value `language=ja/en/auto` takes priority over the build-time
default. The ordinary `make` build defaults to `auto`, which consults the
user's Windows UI language.

`ROGUEOPTS` is read once at startup. On new games, `terse/noterse` and
`jump/nojump` specify initial options; on restore, saved `terse` and `jump`
values take precedence. Display language is selected at startup and can
also be changed during play.

To select a default initial language independent of the OS locale, use:

    make japanese
    make english
    make original

`make japanese` selects Japanese initially, and `make english` selects
English. `make original` also selects English initially, without changing
the DOS 1.48 gameplay rules or JEC color/option defaults.

DOS-style key bindings are enabled by default. Set `nodoskeys` in
`ROGUEOPTS` to use bindings based on Rogue 5.4 instead:

    ROGUEOPTS=nodoskeys ./rogue.exe

For controls, see the guide or press `?` during play.

## Wizard mode

To build with wizard mode enabled, use:

    make wizard

## Program and data file names

Change the executable name at build time with `PROGRAM`. `PROG` is also
accepted as an alias; `PROGRAM` takes precedence if both are specified.
By default, the save, score, and lock file names follow the executable name.

    make PROGRAM=rogue-dos
    make PROG=rogue-dos

The executable is then `rogue-dos.exe`, and the data files are named:

    rogue-dos.save
    rogue-dos.scr
    rogue-dos.lck

Use `FILEDIR` to specify a common data-file directory:

    make PROGRAM=rogue-dos FILEDIR=games

When launching an uninstalled executable directly, create the directory
beforehand. Japanese file paths are handled using the Windows Unicode API.

Individual file names can also be specified and take precedence over `FILEDIR`:

    make PROGRAM=rogue-dos SAVEFILE=my.save SCOREFILE=my.scr LOCKFILE=my.lck

## Startup and display

Double-clicking the EXE in Explorer opens a dedicated console and requests
an 80×24-character window. Launching from an existing cmd, PowerShell, or
UCRT64 terminal does not resize that terminal.

If the default saved game exists on direct launch, choose:

    R / Enter    Resume saved adventure
    N            Start a new game
    F            Choose another save file
    Q            Quit

Without a default saved game, a new game starts. You can also drag and drop
a save file onto the EXE to restore it.

    ./rogue.exe -s               Show scores
    ./rogue.exe -r               Resume default save
    ./rogue.exe 'Adventure.save'  Resume named file

Restoring a game normally consumes (deletes) the save file, so keep a copy
when testing. Japanese input and standalone emoji are supported; complex
emoji sequences depend on the terminal and font.

## Files

Default save file:

    $(PROGRAM).save, normally rogue.save

Default score file:

    $(PROGRAM).scr, normally rogue.scr

Default lock file:

    $(PROGRAM).lck, normally rogue.lck

## Documentation

The source tree includes these user-facing documents:

    rogue_ja.txt            Japanese plain-text guide
    rogue_en.txt            English plain-text guide
    rogue_ja.html           Japanese HTML guide
    rogue_en.html           English HTML guide
    README_mac_linux_ja.md  macOS/Linux README (Japanese)
    README_mac_linux_en.md  macOS/Linux README (English)
    rogue.6.en.in           English manual-page template
    rogue.6.ja.in           Japanese manual-page template

See the guides for game controls and item descriptions.

## Main characteristics of this port

Rogue DOS Tribute retains the portable build/display framework of the
Rogue 5.4.5 JEC port while adapting gameplay toward classic DOS Rogue 1.48:

- UTF-8 Japanese/English display with runtime language switching
- Support for MSYS2 UCRT64 builds on Windows 10/11
- Standard builds default to color, one identify-scroll type, and cursor hiding during ordinary play
- Runtime-selectable extended characters via `extchars / noextchars`
- DOS-style corridor and maze-passage display with a dedicated player color on passage cells
- Potion throwing effects for monsters
- DOS-style potion list, including `paralysis` and `thirst quenching`
- A single identify scroll type, `blank paper`, and `vorpalize weapon`
- Ice Monster frost attacks and slime splitting
- The wand/staff of striking, crossbow, and crossbow bolts
- Monster names, stats, flags, and level-selection tables adjusted toward DOS Rogue 1.48
- `PROGRAM`-based executable naming with matching default save, score, and lock file names
- Japanese and English guides/manual pages updated to match this source

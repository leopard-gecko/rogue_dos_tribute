# Rogue DOS Tribute JEC.091 — Windows (English)

This document explains how to build and run Rogue DOS Tribute on **64-bit Windows 10 / 11**, including environment variables and save, score, and lock file locations. For an introduction and platform documentation, see the [repository README](https://github.com/leopard-gecko/rogue_dos_tribute). For gameplay, commands, and items, see [A Guide to the Dungeons of Doom](rogue_en.html).

日本語版：[README_windows_ja.md](README_windows_ja.md)

## 1. Requirements

To build a native Windows executable, use **MSYS2 UCRT64**. Install MSYS2 from <https://www.msys2.org/> and open “MSYS2 UCRT64” from the Start menu.

```sh
pacman -Syu
```

If requested, restart UCRT64 and finish updating it, then install the compiler, development tools, and PDCursesMod:

```sh
pacman -S --needed make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-binutils mingw-w64-ucrt-x86_64-pdcurses
```

Change to the extracted source directory.

## 2. Build

```sh
make clean
make
```

This produces **`rogue.exe`**. For an ordinary build, `make` alone is also sufficient.

| Purpose | Command |
|---|---|
| Regular build | `make` |
| Build as `jrogue.exe` | `make PROGRAM=jrogue` (or `make PROG=jrogue`) |
| Use Japanese as the initial language | `make japanese` |
| Use English as the initial language | `make english` |
| Use English initially, with the normal gameplay defaults | `make original` |
| Include wizard mode | `make wizard` |

Regular `make` selects the initial language from the Windows display language. `make original` changes **only the initial language to English**; it does not change the game rules, color, or other default settings. Run `make clean` before changing build modes.

Standard builds enable color and extended characters and hide the cursor during ordinary play. The main defaults are:

| Setting | Regular `make` | `make original` |
|---|---|---|
| Initial language | Windows display language | English |
| `color` (color display) | on | on |
| `extchars` (extended character display) | on | on |
| `cursor` (cursor display) | off | off |

DOS-style key bindings are enabled by default. You can specify `nodoskeys` in `ROGUEOPTS` at launch to use the alternative key bindings. See the guide for details of the options.

You can override compiler and other build settings as needed:

```sh
make CC=gcc
make CFLAGS="-O2"
```

If `PROGRAM` and `PROG` are both specified, `PROGRAM` takes precedence. Changing the executable name also changes the default data file names.

## 3. Launch and exit

**Double-click `rogue.exe`** in Explorer to start. When a default saved game exists, the startup menu offers:

| Key | Action |
|---|---|
| `R` or `Enter` | Resume the saved adventure |
| `N` | Start a new adventure |
| `F` | Choose another save file |
| `Q` | Quit |

Without a default saved game, a new game starts immediately. You can also **drag and drop a saved-game file onto `rogue.exe`** to resume it. Paths with Japanese characters or spaces are supported.

You can launch the executable from PowerShell, Command Prompt, or MSYS2 UCRT64. In PowerShell:

```powershell
.\rogue.exe
```

The game uses an **80-column × 24-row** display. Double-click launches attempt to resize the terminal window to 80×24. Launching from an existing terminal does not resize its window. Some terminal hosts or settings may prevent automatic resizing.

Command-line options include:

| Command | Purpose |
|---|---|
| `rogue.exe -r` | Resume the default saved game |
| `rogue.exe -s` | Display scores |
| `rogue.exe Adventure.save` | Resume the specified saved game |

In PowerShell, prefix the executable name with `.\` (for example, `.\rogue.exe -s`).

Restoring a saved game normally consumes (deletes) that save file; make a copy first if testing. Japanese input and standalone emoji are supported, while complex emoji sequences depend on the terminal and font.

## 4. Setting environment variables on Windows

Rogue reads startup options from the Windows environment variable **`ROGUEOPTS`**. This section covers how to set it in Windows. For the available options and their meanings, see the “Options” section of the game guide.

**Set it temporarily in the current shell:** `language=ja` is only an example value.

PowerShell:

```powershell
$env:ROGUEOPTS = 'language=ja'
.\rogue.exe
```

Command Prompt:

```cmd
set "ROGUEOPTS=language=ja"
rogue.exe
```

MSYS2 UCRT64:

```sh
export ROGUEOPTS='language=ja'
./rogue.exe
```

These settings apply to that shell and programs started from it. They are not inherited by unrelated terminals or Explorer double-click launches.

**Save it as a persistent Windows user environment variable:**

1. Press `Win + R`, enter `sysdm.cpl`, and press Enter.
2. Open the **Advanced** tab and choose **Environment Variables**.
3. Under **User variables**, select **New**, enter `ROGUEOPTS` as the name and the desired string (for example, `language=ja`) as the value. Select **Edit** if it already exists.
4. Confirm with **OK** and open a new terminal. If the change does not take effect, sign out of Windows and sign back in.

To stop using the persistent setting, remove the `ROGUEOPTS` user variable.

## 5. Save, score, and lock file locations

By default, **`rogue.save`**, **`rogue.scr`**, and **`rogue.lck`** are placed in the game's working directory. A different location can be specified **at build time** using `FILEDIR`.

To use a dedicated folder from MSYS2 UCRT64:

```sh
mkdir -p rogue-data
make clean
make FILEDIR=rogue-data
```

An absolute Windows path with `/` separators is also supported:

```sh
make clean
make FILEDIR='C:/Games/RogueDOS'
```

Normal `FILEDIR` directories are **not created automatically**. Create the folder first and ensure it is writable, or use:

```sh
make prepare-filedir FILEDIR='C:/Games/RogueDOS'
```

**Use the Windows Saved Games known folder (personal builds only):**

```sh
make clean
make FILEDIR=saved-games
```

The build resolves the current user's actual Windows Saved Games location and places data in its `Rogue` subfolder, creating the folder when necessary. The usual path is shown below; Windows folder relocation is supported.

```text
C:/Users/<username>/Saved Games/Rogue/
```

**The builder's absolute save directory is embedded in the EXE.** Do not use this setting for binaries distributed to other PCs; normally leave `FILEDIR` unset for redistribution.

Data file names follow `PROGRAM` by default. For example, `make PROGRAM=rogue-dos` builds `rogue-dos.exe` and uses `rogue-dos.save`, `rogue-dos.scr`, and `rogue-dos.lck`.

```sh
make PROGRAM=rogue-dos
```

Individual file names can be overridden, taking precedence over `FILEDIR`:

```sh
make PROGRAM=rogue-dos SAVEFILE=my.save SCOREFILE=my.scr LOCKFILE=my.lck
```

Relative file paths are resolved from the working directory at launch. Run `make clean` before rebuilding with new `FILEDIR` settings. `make clean` does not remove existing save, score, or lock files.

## 6. Distributed files and guides

Create a full source-distribution archive from MSYS2 UCRT64 with:

```sh
make dist.src
```

This produces `rogue_dos_tribute_jec091-src.tar.gz`.

| File | Purpose |
|---|---|
| `rogue.exe` | Windows executable (after building) |
| `rogue.save` / `rogue.scr` / `rogue.lck` | Save, score, and lock files used during play |
| `Makefile` / `Makefile.win` | Source builds |
| `windows/rogue.ico` / `windows/rogue-icon.rc` | Windows icon resources |
| `windows/windows-saved-games.ps1` | Helper to resolve and create the Windows Saved Games location |
| `rogue_ja.html` / `rogue_en.html` | Japanese and English HTML guides |
| `rogue_ja.txt` / `rogue_en.txt` | Japanese and English plain-text guides |
| `README_mac_linux_ja.md` / `README_mac_linux_en.md` | macOS/Linux documentation |

For the documentation index and an introduction to the game, see the [repository README](https://github.com/leopard-gecko/rogue_dos_tribute).

## 7. Copyright and licenses

See [LICENSE.TXT](LICENSE.TXT) for the original Rogue and third-party copyright and license terms and for notes on changes in this derivative, including localization, added features, and the Windows icon.

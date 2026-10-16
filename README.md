# rvz-convert

Converts Wii games from Dolphin's RVZ format to WBFS, ready for USB Loader GX.

---

## Building

Use the provided Makefile to build the project. Make sure to pass in multithreading parameters, as Dolphin is a large project with many submodules of its own.
e.g.

```
make -j$(nproc)
sudo make install
```

## Usage

Please use after installing to PATH (`sudo make install`)

```
rvz-convert [options] file.rvz [file2.rvz ...]
```

| Option            | Description                                                  |
|-------------------|--------------------------------------------------------------|
| `-j`, `--jobs N`  | Convert up to N games at the same time (default: 1)          |
| `-k`, `--keep-logs` | Keep the conversion log even when it succeeds              |
| `-h`, `--help`    | Display the help message                                     |
| `-v`, `--version` | Display version information                                  |

The output is written to the current directory, in the layout USB Loader GX expects:

```
Game Name [GAMEID]/[GAMEID].wbfs
```

Copy that folder into the `wbfs/` folder of your USB drive.

### Examples

Convert a single game:

```
rvz-convert "Link's Crossbow Training (USA).rvz"
```

Convert a whole folder, two games at a time:

```
rvz-convert -j 2 *.rvz
```

### Converting several games at once

You can pass any number of files. By default they are converted one after
another; use `-j N` to run up to N conversions in parallel. Every line of
output is prefixed with the game name so the progress of each job stays
readable.

Each conversion uses its own hidden temporary directory
(`.rvz-convert-XXXXXX`) in the current directory, so it is also safe to run
several `rvz-convert` processes in the same folder at the same time.

Keep disk space in mind: while a game is being converted, a full-size ISO
(up to ~4.7 GB for a single layer disc, ~8.5 GB for dual layer) is stored
temporarily, so `-j 3` may need ~15 GB or more of free space.

### Errors and logs

Every step (`dolphin-tool convert`, `wit id`, `wit cp`) is checked. If one
fails, rvz-convert:

- prints which step failed and its exit code;
- prints the full output of the tools that ran for that game;
- saves that output as `Game Name.rvz-convert.log` in the current directory;
- removes the temporary ISO and any half-written output folder;
- continues with the remaining games.

At the end, if more than one file was given, a summary lists how many games
were converted and which ones failed. The exit code is `0` when every game
was converted, `1` when at least one failed, and `2` for invalid options.

A game is not overwritten if its `[GAMEID].wbfs` already exists; delete it
first if you want to convert it again. Note the game ID is only known after
the RVZ is unpacked, so this check happens after the first step.

With `--keep-logs`, the log of a successful conversion is saved as
`rvz-convert.log` inside the game's output folder.

If a conversion is interrupted (e.g. Ctrl+C), its `.rvz-convert-XXXXXX`
folder may be left behind and can be deleted safely.

### Dependencies:

Note: Most of these are dolphin dependencies. Still working on cutting out the Qt6 dependencies as it's a total PITA to compile the entirety of dolphin.

- Qt6 Base
- Qt6 SVG
- Qt6 qtbase private libraries
- Working compiler toolchain (build-essential)
- GNU Make
- CMake
- Xi
- LibXRandr
- ncurses (`libtinfo`, needed to link wit)

On Ubuntu 24.04 (also works under WSL on Windows):

```
sudo apt install build-essential cmake qt6-base-dev qt6-base-private-dev libqt6svg6-dev \
    libxi-dev libxrandr-dev libncurses-dev
```

## Changelog

### v1.1.0

- Accept multiple input files in one run.
- New `-j/--jobs` option to convert several games in parallel.
- Each conversion uses a unique temporary directory instead of a shared
  `tmp.iso`/`id.txt`, so parallel runs no longer overwrite each other.
- Every step's exit code is checked; on failure the tool output is shown and
  saved to a log file, temporary files are cleaned up, and the exit code is
  non-zero.
- New `-k/--keep-logs` option.
- External tools are run directly instead of through the shell, so file names
  with quotes or other special characters work.
- `--help` and `--version` now exit right away.
- Documented the ncurses build dependency.

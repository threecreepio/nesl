# Nesl - Nes LUA runner

This project aims to allow running a Lua script with a headless NES emulator.

The Lua library is modelled after FCEUX, but runs on the QuickNES core from Bizhawk for performance reasons.

## Usage

After downloading a copy from https://github.com/threecreepio/nesl/releases you can run the program from a command line:

nesl script.lua [rom.nes]

You can skip the NES rom if your lua script loads one itself.

## Building

Requires CMake (>= 3.10) and a C++ compiler. Lua 5.1 and other
dependencies are vendored under `lib/`, so no system libraries
are needed beyond the toolchain. Ninja is the recommended
generator and is what the CI uses; the `release` CMake preset
(see `CMakePresets.json`) selects it and writes outputs to
`build/Release/`.

    cmake -G Ninja --preset release -S . -B build/Release
    cmake --build build/Release

The result is `build/Release/nesl` (Linux/macOS) or
`build/Release/nesl.exe` (Windows). This is a headless binary
suitable for running scripts from the command line; there is
no GUI.

## Testing

The test suite lives in `testing/` and is bash + Lua. Each test is
a `.lua` file that exits 0 on pass and non-zero on failure; the
runner iterates them with a 10s timeout each.

Run the full suite:

    cmake --build build/Release                 # if not already built
    bash testing/run_tests.sh

If the binary is at a non-default path, set `NESL_BIN`:

    NESL_BIN=/path/to/nesl bash testing/run_tests.sh

The suite currently contains 7 regression tests covering edge
cases in the Lua-facing API: nil/non-string arguments, oversized
buffers, savestate type confusion, bad iNES headers, and similar
inputs that previously caused segfaults. All tests should pass
on a clean build.

### Test ROMs (optional)

The test suite can optionally run the standard public-domain
NES test ROMs from the nesdev community. These are pulled in
as a git submodule under `testing/test_roms/`.

First-time setup (on a fresh clone):

    git submodule update --init --recursive

This downloads ~27 MB of test ROMs into `testing/test_roms/`.
If the submodule isn't initialized, the regression tests still
run normally; only the optional public-domain test ROM
validation is skipped.

See `testing/test_roms.md` for details on which ROMs are used
and how to update the pinned version.

## Todo

Lower priority:
- add luasocket library
- add sound debugging?
- add zapper support?
- add iup+im+cd library?
- support more fceux like command line arguments
- match fceux cpu flags better in hooks

## Notable differences from FCEUX

Most of the Lua support should be very similar to FCEUX. But since it uses QuickNES as it's emulator instead of FCEUX there are differences.

- There is an `emu.nesl` value that you can use to check if you are running in nesl instead of fceux.

- If you check CPU registers while running, these aren't updated in quite the same way as on FCEUX and may act differently.

- When saving images Nesl has support for png and bmp file formats. Bmp files save faster, but take more space. If you intend to post-process the images in some way it can be useful to get them quickly as bmps.

- There is a `gui.screenshotarchive` function, if you call it with the path to a tar file it will store any screenshots uncompressed in that file as a TAR archive. Note that the TAR file is overwritten if one already exists in the location. This reduces the strain on the file system if you are using bmp screenshots and can increase screenshot performance significantly. Pass in nil to close the tarball, otherwise it will close when the program ends.

## History

- 0.1.3
screenshots
memory hooks

- 0.1.2
Bugfixes

- 0.1.1
Initial version

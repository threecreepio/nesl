# Nesl - Nes LUA runner

This project aims to allow running a Lua script with a headless NES emulator.

The Lua library is modelled after FCEUX, but runs on the QuickNES core from Bizhawk for performance reasons. It is not currently complete, and some aspects of the original lua will most likely never be ported - for example running fm2 movies and operating a TAS editor window.

## Usage

After downloading a copy from https://github.com/threecreepio/nesl/releases you can run the program from a command line:

nesl script.lua [rom.nes]

You can skip the NES rom if your lua script loads one itself.

## Todo

Add Lua functions:
- memory watch functions registerwrite and registerexec
- add screenshot support

Lower priority:
- debuggerloop?
- add luasocket library
- add sound debugging?
- add zapper support?
- add ui library?

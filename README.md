# Nesl - Nes LUA runner

This project aims to allow running a Lua script with a headless NES emulator.

The Lua library is modelled after FCEUX, but runs on the QuickNES core from Bizhawk for performance reasons.

## Usage

After downloading a copy from https://github.com/threecreepio/nesl/releases you can run the program from a command line:

nesl script.lua [rom.nes]

You can skip the NES rom if your lua script loads one itself.

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

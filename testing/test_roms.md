# Test ROMs (git submodule)

The `test_roms/` directory in this folder is a **git submodule**
pointing to https://github.com/christopherpow/nes-test-roms, a
collection of public-domain NES test ROMs maintained by the
nesdev community.

## First-time setup

When you clone this repo, the submodule is empty. Initialize it:

    git submodule update --init --recursive

This downloads ~27 MB of test ROMs into `test_roms/`.

## What's in here

We use a small subset of the ROMs in this repo. The relevant ones:

  - `test_roms/other/nestest.nes`
      The standard 6502 instruction-timing test. Runs ~5000
      instructions and prints a 5001-line log. Used by
      `test_nestest.lua` to verify CPU correctness.

  - `test_roms/blargg_ppu_tests_2005.09.15b/`
      blargg's PPU test suite. Each sub-test (palette, sprite hit,
      vbl timing, vram access) prints "Passed" or "Failed #N" to
      a known RAM address. Used by `test_blargg_ppu.lua`.

  - `test_roms/blargg_apu_2005.07.30/`
      blargg's APU test suite. Same pattern. Used by
      `test_blargg_apu.lua`.

  - `test_roms/mmc3_irq_tests/`
      Tests for MMC3 IRQ scanline counter behavior — the exact
      area covered by the P1-4 / P1-5 fixes. Used by
      `test_mmc3_irq.lua`.

  - `test_roms/scanline/scanline.nes`
      PPU scanline timing test. Used by `test_scanline.lua`.

The other ~60 directories in the repo are not currently used by
our tests but are kept available for future expansion.

## Updating

To pull a newer version of the test ROMs:

    cd test_roms
    git pull origin master
    cd ..
    git add test_roms
    git commit -m "Update test ROMs submodule"

Or to update to a specific commit (preferred for reproducibility):

    cd test_roms
    git fetch origin
    git checkout <commit-sha>
    cd ..
    git add test_roms
    git commit -m "Pin test ROMs to <commit-sha>"

The current pinned commit is recorded in the parent repo's index
as a gitlink (see `git ls-files --stage test_roms`).

## Why a submodule?

- The test ROMs are a separate project with their own version
  history. Tracking them as a submodule gives us a specific,
  reproducible version of the test suite per commit of this
  repo.
- Avoids polluting this repo with 27 MB of binary files in every
  clone and every commit.
- The ROMs are widely understood to be redistributable: blargg's
  tests (the bulk of what we use) are released into the public
  domain by the author, Shay Green, as is the convention for
  test ROMs in the nesdev community. The upstream
  `christopherpow/nes-test-roms` repo does not bundle a single
  top-level LICENSE file, so we treat the collection as a
  curated set of community test ROMs rather than asserting a
  single license. If you add a new ROM to this submodule,
  verify its redistribution terms before committing.

## What if the submodule isn't initialized?

The Lua test scripts in `testing/` (Phase 3) check for the
presence of the test ROM files at startup. If the submodule
isn't initialized, they print a clear "skipped" message and exit
0, so the rest of the test suite (`bash testing/run_tests.sh`)
still passes.

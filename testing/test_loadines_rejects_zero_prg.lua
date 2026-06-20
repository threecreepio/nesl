-- Regression test: load_ines must reject iNES files with prg_count == 0.
-- Pre-fix: load_ines accepted a 0-PRG ROM, then the first mapper's
--   default_reset_state() called set_prg_bank() which did
--   `bank_count = prg_size() >> bs; bank %= bank_count` with bank_count==0
--   -> SIGFPE / process death. (With the signal handler installed,
--   this now prints a backtrace and exits 136 instead, but it
--   still crashes.)
-- Post-fix: load_ines must return a clean error string and emu.loadrom
--   must raise that error via luaL_error. No process death, no
--   backtrace.
--
-- We synthesize a 16-byte iNES header (NES\x1A, prg_count=0, chr_count=1,
-- mapper 0) and call emu.loadrom on it. Acceptable outcomes:
--   a) pcall returns false with a non-empty error string mentioning PRG
--      (or similar). The process exits 0.
-- Unacceptable: exit code != 0, including the signal-handler exit path
--   (backtrace followed by a non-zero exit), since that means the bug
--   wasn't fixed at the source.

emu.loadrom("example.nes")
local warmup_ok, warmup_err = pcall(emu.frameadvance)
if not warmup_ok then
    io.stderr:write("warmup frame errored (ignoring): " .. tostring(warmup_err) .. "\n")
end

local ok, err = pcall(emu.loadrom, "zero_prg.nes")

if ok then
    io.stderr:write(
        "loadines_rejects_zero_prg FAIL: emu.loadrom(zero_prg.nes) returned success; " ..
        "expected a clean Lua error\n")
    os.exit(1)
end

if type(err) ~= "string" or err == "" then
    io.stderr:write(
        "loadines_rejects_zero_prg FAIL: emu.loadrom raised non-string/empty error: " ..
        tostring(err) .. "\n")
    os.exit(1)
end

io.stderr:write("loadines_rejects_zero_prg: loadrom rejected (expected): " .. tostring(err) .. "\n")
print("loadines_rejects_zero_prg PASS")

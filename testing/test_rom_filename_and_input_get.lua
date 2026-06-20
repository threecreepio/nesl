-- Regression tests: rom.getfilename returns the basename of the loaded ROM,
-- and input.get returns a table with the same key list as joypad.set's table path.
--
-- Pre-fix:
--   rom.getfilename always returned "" because romFileName was
--         never populated by loadRomFile.
--   input.get always returned an empty table.
-- Post-fix: both return meaningful data.

emu.loadrom("example.nes")
local warmup_ok, warmup_err = pcall(emu.frameadvance)
if not warmup_ok then
    io.stderr:write("warmup frame errored (ignoring): " ..
        tostring(warmup_err) .. "\n")
end

-- rom.getfilename() should return a non-empty string.
local fn = rom.getfilename()
if type(fn) ~= "string" then
    io.stderr:write("rom_filename_and_input_get FAIL: rom.getfilename() did not return a string\n")
    os.exit(1)
end
if fn == "" then
    io.stderr:write("rom_filename_and_input_get FAIL: rom.getfilename() returned empty string\n")
    os.exit(1)
end
io.stderr:write("rom_filename_and_input_get: rom.getfilename() = " .. fn .. "\n")

-- input.get() should return a table with the expected keys.
local input = input.get()
if type(input) ~= "table" then
    io.stderr:write("rom_filename_and_input_get FAIL: input.get() did not return a table\n")
    os.exit(1)
end
local expected_keys = { "A", "B", "select", "start", "up", "down", "left", "right" }
for _, k in ipairs(expected_keys) do
    if input[k] == nil then
        io.stderr:write("rom_filename_and_input_get FAIL: input.get() missing key " .. k .. "\n")
        os.exit(1)
    end
end
io.stderr:write("rom_filename_and_input_get: input.get() returned all expected keys\n")

print("rom_filename_and_input_get PASS")

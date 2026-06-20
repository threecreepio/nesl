-- Regression test: gui.savescreenshotas must safely handle long paths.
-- Pre-fix: strncpy without NUL on truncation left the buffer unterminated;
--   subsequent strrchr/strcmp/fopen/mtar_write_file_header read past the array.
-- Post-fix: it should NUL-terminate and either succeed (truncated path) or error cleanly.

emu.loadrom("example.nes")
local warmup_ok, warmup_err = pcall(emu.frameadvance)
if not warmup_ok then
    io.stderr:write("warmup frame errored (ignoring): " .. tostring(warmup_err) .. "\n")
end

-- Build a path >= 0x2000 chars (the screenshotpath buffer size) with a .bmp extension.
local long_prefix = string.rep("a", 0x2000)
local long_path = long_prefix .. ".bmp"
local ok, err = pcall(gui.savescreenshotas, long_path)
if not ok then
    -- Acceptable: error, as long as we got here and didn't crash.
    io.stderr:write("screenshot_long_path: long path errored (acceptable): " .. tostring(err) .. "\n")
end
-- Drive one frame; if the buffer is unterminated, the file write / strrchr crashes.
local ok2, err2 = pcall(emu.frameadvance)
if not ok2 then
    io.stderr:write("screenshot_long_path: frameadvance errored: " .. tostring(err2) .. "\n")
end
print("screenshot_long_path PASS")

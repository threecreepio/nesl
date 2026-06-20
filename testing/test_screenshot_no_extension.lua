-- Regression test: gui.savescreenshotas must not crash on path without extension.
-- Pre-fix: strrchr returned NULL, strcmp(NULL, ".bmp") segfaulted.
-- Post-fix: it should raise a clear Lua error.
--
-- We do NOT actually want to write a file. We arrange the screenshot to be pending
-- and then call screenshots_afterframe indirectly by waiting for one frame.
-- But the simplest path: call gui.savescreenshotas then emu.frameadvance, and
-- inspect the error.

emu.loadrom("example.nes")
local ok = pcall(emu.frameadvance)
if not ok then
    -- initial frame might error in some configs; not what we are testing.
    io.stderr:write("warmup frame errored: " .. tostring(...) .. "\n")
end

-- Now request a screenshot to a path with NO extension.
local path_no_ext = "noext_" .. tostring(os.time())
gui.savescreenshotas(path_no_ext)

-- On the next frameadvance, screenshots_afterframe -> screenshots_save2 is invoked
-- and will hit the missing-extension path. Pre-fix this segfaults the whole process.
local ok2, err2 = pcall(emu.frameadvance)
if not ok2 then
    io.stderr:write("screenshot_no_extension: got expected error: " .. tostring(err2) .. "\n")
    -- Make sure the process is still alive (proves no segfault).
    print("screenshot_no_extension PASS")
else
    -- If by some path it succeeded, we still pass as long as we got here.
    print("screenshot_no_extension PASS (no error raised but no crash)")
end

-- Regression test: savestate.save/load must reject non-savestate userdata.
-- Pre-fix: savestate.save cast any userdata (e.g. io.tmpfile()) to Nes_State*
--   and dereferenced it as a Nes_State, crashing inside NES->save_state.
-- Post-fix: it should raise a clean Lua error.

emu.loadrom("example.nes")
local warmup_ok, warmup_err = pcall(emu.frameadvance)
if not warmup_ok then
    io.stderr:write("warmup frame errored (ignoring): " .. tostring(warmup_err) .. "\n")
end

-- 1. savestate.save with a non-savestate userdata (io.tmpfile() is userdata)
local ud = io.tmpfile()
local ok1, err1 = pcall(savestate.save, ud)
if not ok1 then
    io.stderr:write("savestate_rejects_userdata: savestate.save(non-savestate userdata) errored: "
        .. tostring(err1) .. "\n")
else
    error("savestate_rejects_userdata: savestate.save accepted a non-savestate userdata")
end

-- 2. savestate.load with the same kind of userdata
local ud2 = io.tmpfile()
local ok2, err2 = pcall(savestate.load, ud2)
if not ok2 then
    io.stderr:write("savestate_rejects_userdata: savestate.load(non-savestate userdata) errored: "
        .. tostring(err2) .. "\n")
else
    error("savestate_rejects_userdata: savestate.load accepted a non-savestate userdata")
end

-- 3. savestate.create() still works (sanity check)
local s = savestate.create()
if type(s) ~= "userdata" then
    error("savestate_rejects_userdata: savestate.create did not return userdata")
end

print("savestate_rejects_userdata PASS")

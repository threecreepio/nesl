-- Regression test: emu.print must not segfault on non-string-coercible arg.
-- Pre-fix: emu_print called puts(NULL) and crashed with SIGSEGV.
--   lua_tostring returns NULL for values that are not string or number,
--   i.e. tables, userdata, functions, booleans, nil.
-- Post-fix: emu.print should silently no-op (or error) on a non-coercible value.
--
-- The test invokes emu.print with each of the four problem types; on the
-- unfixed binary, at least one of these will segfault. On the fixed binary
-- we must reach the final print().

local function try(label, val)
    local ok, err = pcall(emu.print, val)
    if not ok then
        io.stderr:write(string.format("emu_print_nontype: emu.print(%s) errored (acceptable): %s\n",
            label, tostring(err)))
    else
        io.stderr:write(string.format("emu_print_nontype: emu.print(%s) returned ok\n", label))
    end
end

try("table", {})
try("boolean", true)
try("function", function() end)
try("nil", nil)
try("userdata", io.tmpfile())

-- Reaching here means none of the calls crashed the process.
print("emu_print_nontype PASS")

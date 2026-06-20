-- Regression test: loadRomFile must not write past romData[].
-- Pre-fix: when st.st_size == sizeof(romData) (10 MB),
--   romData[st.st_size] = 0 was a 1-byte OOB write on the global array.
-- Post-fix: loadRomFile must reject any file of size >= sizeof(romData)
--   (the +1 sentinel needs one byte of headroom).
--
-- The trick: synthesize a 10MB file that is a valid iNES header
-- (NES\x1a, 1x16KB PRG, 1x8KB CHR, mapper 0, no special flags). On the
-- unfixed binary, this file is ACCEPTED and parsed as a real ROM, so
-- emu.loadrom returns success. On the fixed binary, it is REJECTED
-- because 10MB == sizeof(romData) and the +1 sentinel would overflow.
--
-- The test asserts loadrom raises an error. This is what makes it a
-- real regression test: the unfixed code would proceed to corrupt the
-- OOB byte and accept the ROM.

local function make_valid_ines(size_minus_header)
    -- iNES header: "NES" + 0x1a + prg_count + chr_count + flags + flags2
    -- + 8 bytes of padding. Use string.char for 0x1a to avoid Lua's
    -- escape-table ambiguity (a 4-char escape in a literal would be
    -- parsed as 4 bytes and corrupt the header).
    local header = "NES" .. string.char(0x1a, 0x01, 0x01, 0x00, 0x00) ..
                    string.rep("\0", 8)
    if size_minus_header <= 0 then return header end
    return header .. string.rep("\0", size_minus_header)
end

-- Warm up: ensure the binary is working at all.
emu.loadrom("example.nes")
local warmup_ok, warmup_err = pcall(emu.frameadvance)
if not warmup_ok then
    io.stderr:write("warmup frame errored (ignoring): " .. tostring(warmup_err) .. "\n")
end

-- Build a 10MB file with a valid iNES header.
local tenmb = make_valid_ines(1024 * 1024 * 10 - 16)
local f = io.open("tenmb.bin", "wb")
f:write(tenmb)
f:close()

local ok, err = pcall(emu.loadrom, "tenmb.bin")
os.remove("tenmb.bin")

if ok then
    -- loadrom must REJECT a file of size == sizeof(romData). The
    -- unfixed code accepts it and OOB-writes; fail loudly.
    error("rom_load_oob_boundary: loadrom accepted 10MB valid-iNES file (expected rejection): " .. tostring(err))
end

io.stderr:write("rom_load_oob_boundary: loadrom(10MB) rejected: " .. tostring(err) .. "\n")
print("rom_load_oob_boundary PASS")

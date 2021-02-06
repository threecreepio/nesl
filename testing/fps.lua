emu.loadrom("example.nes")

local s = os.clock()
local f = 0
while true do
    local n = os.clock()
    f = f + 1
    emu.frameadvance()
    if n > s + 1 then
        print(string.format("FPS: %d", f))
        f = 0
        s = n
    end
end

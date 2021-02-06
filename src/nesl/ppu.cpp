#include "../nesl.h"

int ppu_readbyte(lua_State* L) {
    uint16_t addr = lua_tonumber(L, 1);
    int retval = NES->emu.ppu.peekaddr(addr);
    lua_pushinteger(L, retval);
    return 1;
}

static int ppu_readbyterange(lua_State* L) {
    int range_start = luaL_checkinteger(L, 1);
    int range_size = luaL_checkinteger(L, 2);
    if (range_size < 0)
        return 0;

    char* buf = (char*)malloc(range_size);
    if (buf == 0) {
        return luaL_error(L, "allocating space for readbyterange failed");
    }

    for (int i = 0; i < range_size; i++) {
        buf[i] = NES->emu.ppu.peekaddr(range_start + i);
    }

    lua_pushlstring(L, buf, range_size);
    return 1;
}

static const struct luaL_reg ppulib[] = {
    {"readbyte", ppu_readbyte},
    {"readbyterange", ppu_readbyterange},

    {NULL,NULL}
};

void ppulib_register(lua_State* L) {
    luaL_register(L, "ppu", ppulib);
}

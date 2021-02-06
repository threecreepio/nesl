#include "../nesl.h"

int rom_readbyte(lua_State* L) {
    int addr = luaL_checkinteger(L, 1);
    if (addr < 0 || addr > romDataLength)
        luaL_error(L, "invalid argument addr");
    lua_pushinteger(L, romData[addr]);
    return 1;
}

int rom_readbytesigned(lua_State* L) {
    int addr = luaL_checkinteger(L, 1);
    if (addr < 0 || addr > romDataLength)
        luaL_error(L, "invalid argument addr");
    lua_pushinteger(L, (signed)romData[addr]);
    return 1;
}

static int rom_readbyterange(lua_State* L) {
    size_t range_start = luaL_checkinteger(L, 1);
    size_t range_size = luaL_checkinteger(L, 2);
    if (range_size < 0)
        return 0;
    if (range_start < 0 || range_start >= romDataLength)
        luaL_error(L, "invalid argument range_start");
    if (range_size < 0 || range_start + range_size >= romDataLength)
        luaL_error(L, "invalid argument range_size");

    char* buf = (char*)malloc(range_size);
    if (buf == 0) {
        return luaL_error(L, "allocating space for readbyterange failed");
    }

    for (int i = 0; i < range_size; i++) {
        buf[i] = romData[range_start + i];
    }

    lua_pushlstring(L, buf, range_size);
    return 1;
}

int rom_getfilename(lua_State* L) {
    lua_pushstring(L, romFileName);
    return 1;
}

static const struct luaL_reg romlib[] = {
    {"getfilename", rom_getfilename},
    {"gethash", unimplemented},
    {"readbyte", rom_readbyte},
    {"readbytesigned", rom_readbytesigned},
    // alternate naming scheme for unsigned
    {"readbyteunsigned", rom_readbyte},
    {"readbyterange", rom_readbyterange},
    {"writebyte", unimplemented},
    {NULL,NULL}
};

void romlib_register(lua_State* L) {
    luaL_register(L, "rom,", romlib);
}

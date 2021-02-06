#include "../nesl.h"


int readword(lua_State* L) {
    uint16_t addrlo = luaL_checkinteger(L, 1);
    uint16_t addrhi = addrlo + 1;
    if (lua_type(L, 2) == LUA_TNUMBER) {
        addrhi = luaL_checkinteger(L, 2);
    }
    return (uint16_t)NES->emu.read(addrlo) | (((uint16_t)NES->emu.read(addrhi)) << 8);
}

void memory_registerexec_tracecb(uint32_t* trace) {
    /*
    scratch[0] = a;
    scratch[1] = x;
    scratch[2] = y;
    scratch[3] = sp;
    scratch[4] = pc - 1;
    scratch[5] = status;
    scratch[6] = opcode;
    */

    printf("got exec cb for %04X!\n", trace[4]);
}

struct hook { uint16_t start; uint16_t end; uint32_t call; };
struct hook hooks[64];

int hooks_count = 0;
uint16_t hooks_start[64] = {};
uint16_t hooks_end[64] = {};
int hooks_callid[64] = {};


static int memory_registerexec(lua_State* L) {
    // registerexec takes: address, length, callback
    luaL_checktype(L, 1, LUA_TNUMBER);

    uint16_t addr = lua_tonumber(L, 1);
    uint16_t size = lua_tonumber(L, 2);
    uint16_t cb = lua_tonumber(L, 2);

    for (int i = 0; i < hooks_count - 1; ++i) {
        if (hooks[i].start < addr) continue;
        if (hooks[i + i].start > addr) break;
        for (int j = i + 1; j < hooks_count; ++j) {
            hooks[j] = hooks[i];
        }
    }

    // NES->emu.set_tracecb(memory_registerexec_tracecb);

    /*
    if (!lua_isnil(L, 1)) \
        luaL_checktype(L, 1, LUA_TFUNCTION); \
        lua_settop(L, 1); \
        lua_getfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[##id]); \
lua_insert(L, 1); \
lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[##id]); \
return 1; \
*/

    return 0;

}

int memory_readbyte(lua_State* L) {
    lua_pushinteger(L, NES->emu.read(lua_tonumber(L, 1)));
    return 1;
}

int memory_readword(lua_State* L) {
    lua_pushinteger(L, readword(L));
    return 1;
}

int memory_readwordsigned(lua_State* L) {
    lua_pushinteger(L, (int16_t)readword(L));
    return 1;
}

int memory_getregister(lua_State* L) {
    const char* str = luaL_checkstring(L, 1);
    if (str == "a") {
        lua_pushinteger(L, NES->emu.r.a);
        return 1;
    }
    if (str == "x") {
        lua_pushinteger(L, NES->emu.r.x);
        return 1;
    }
    if (str == "y") {
        lua_pushinteger(L, NES->emu.r.y);
        return 1;
    }
    if (str == "pc") {
        lua_pushinteger(L, (uint16_t)NES->emu.r.pc);
        return 1;
    }
    if (str == "s") {
        lua_pushinteger(L, NES->emu.r.sp);
        return 1;
    }
    if (str == "p") {
        lua_pushinteger(L, NES->emu.r.status);
        return 1;
    }
    return 0;
}

int memory_setregister(lua_State* L) {
    const char* str = luaL_checkstring(L, 1);
    int value = luaL_checknumber(L, 1);
    if (str == "a") {
        NES->emu.r.a = value;
        return 1;
    }
    if (str == "x") {
        NES->emu.r.x = value;
        return 1;
    }
    if (str == "y") {
        NES->emu.r.y = value;
        return 1;
    }
    if (str == "pc") {
        NES->emu.r.pc = value;
        return 1;
    }
    if (str == "s") {
        NES->emu.r.sp = value;
        return 1;
    }
    if (str == "p") {
        NES->emu.r.status = value;
        return 1;
    }
    return 0;
}

int memory_readbytesigned(lua_State* L) {
    lua_pushinteger(L, (int8_t)NES->emu.read(lua_tonumber(L, 1)));
    return 1;
}

int memory_writebyte(lua_State* L) {
    uint16_t addr = lua_tonumber(L, 1);
    uint8_t value = lua_tonumber(L, 2);
    if (addr < NES->low_mem_size) {
        NES->emu.write(addr, value);
    }
    return 0;
}

static int memory_readbyterange(lua_State* L) {
    int range_start = luaL_checkinteger(L, 1);
    int range_size = luaL_checkinteger(L, 2);
    if (range_size < 0)
        return 0;

    char* buf = (char*)malloc(range_size);
    if (buf == 0) {
        return luaL_error(L, "allocating space for readbyterange failed");
    }

    for (int i = 0; i < range_size; i++) {
        buf[i] = NES->emu.read(range_start + i);
    }

    lua_pushlstring(L, buf, range_size);
    return 1;
}

const struct luaL_reg memorylib[] = {
    {"readbyte", memory_readbyte},
    {"readbyterange", memory_readbyterange},
    {"readbytesigned", memory_readbytesigned},
    {"readbyteunsigned", memory_readbyte},	// alternate naming scheme for unsigned
    {"readword", memory_readword},
    {"readwordsigned", memory_readwordsigned},
    {"readwordunsigned", memory_readword},	// alternate naming scheme for unsigned
    {"writebyte", memory_writebyte},
    {"getregister", memory_getregister},
    {"setregister", memory_setregister},

    //{"registerwrite", memory_registerwrite},
    //{"registerexec", memory_registerexec},
    //{"register", memory_registerwrite},
    //{"registerrun", memory_registerexec},
    //{"registerexecute", memory_registerexec},
    {NULL,NULL}
};

void memorylib_register(lua_State *L) {
    luaL_register(L, "memory", memorylib);
}

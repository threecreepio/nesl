#include "../nesl.h"
#include <list>

int readword(lua_State* L) {
    uint16_t addrlo = luaL_checkinteger(L, 1);
    uint16_t addrhi = addrlo + 1;
    if (lua_type(L, 2) == LUA_TNUMBER) {
        addrhi = luaL_checkinteger(L, 2);
    }
    return (uint16_t)NES->emu.read(addrlo) | (((uint16_t)NES->emu.read(addrhi)) << 8);
}

struct hook { uint16_t start; uint16_t end; int call; };
std::list<struct hook> exechooks;
std::list<struct hook> memhooks;

void memory_registerexec_trace(nes_addr_t addr) {
    bool matching = false;
    for (auto const& i : exechooks) {
        if (matching) {
            if (i.start > addr) break;
            if (i.end < addr) continue;
        }
        else {
            if (i.start > addr) continue;
            if (i.end < addr) continue;
            matching = true;
        }
        lua_rawgeti(L, LUA_REGISTRYINDEX, i.call);
        lua_pushnumber(L, addr);
        lua_pushnumber(L, 1);
        lua_pushnumber(L, 0);
        lua_call(L, 3, 0);
    }
}

void memory_registerwrite_trace(nes_addr_t addr, int val) {
    bool matching = false;
    for (auto const& i : memhooks) {
        if (matching) {
            if (i.start > addr) break;
            if (i.end < addr) continue;
        }
        else {
            if (i.start > addr) continue;
            if (i.end < addr) continue;
            matching = true;
        }
        lua_rawgeti(L, LUA_REGISTRYINDEX, i.call);
        lua_pushnumber(L, addr);
        lua_pushnumber(L, 1);
        lua_pushnumber(L, val);
        lua_call(L, 3, 0);
    }
}


static int memory_registerexec(lua_State* L) {
    // registerexec takes: address, length, callback
    luaL_checktype(L, 1, LUA_TNUMBER);

    int size = 1;
    int fn = 2;
    uint16_t start = lua_tonumber(L, 1);
    if (lua_isnumber(L, 2)) {
        fn = 3;
        int size = lua_tonumber(L, 2);
        if (size == 0) return 0;
    }

    uint16_t end = (start + size) - 1;
    if (end < start) {
        uint16_t x = start;
        start = end;
        end = x;
    }

    if (lua_isnil(L, fn)) {
        exechooks.remove_if([L, start, end](const struct hook& i) {
            bool overlap = i.start <= end && i.end >= start;
            if (overlap) {
                luaL_unref(L, LUA_REGISTRYINDEX, i.call);
            }
            return overlap;
        });
    }
    else {
        int cb = luaL_ref(L, LUA_REGISTRYINDEX);
        struct hook h = { start, end, cb };
        exechooks.push_back(h);
        exechooks.sort([](const struct hook& a, const struct hook& b) {
            return a.start == b.start ? a.end < b.end : a.start < b.start;
        });
    }

    NES->emu.set_tracecb(exechooks.size() ? memory_registerexec_trace : 0);
    return 0;
}


static int memory_registerwrite(lua_State* L) {
    // registerexec takes: address, length, callback
    luaL_checktype(L, 1, LUA_TNUMBER);

    int size = 1;
    int fn = 2;
    uint16_t start = lua_tonumber(L, 1);
    if (lua_isnumber(L, 2)) {
        fn = 3;
        int size = lua_tonumber(L, 2);
        if (size == 0) return 0;
    }
    uint16_t end = (start + size) - 1;
    if (end < start) {
        uint16_t x = start;
        start = end;
        end = x;
    }

    if (lua_isnil(L, fn)) {
        memhooks.remove_if([L, start, end](const struct hook& i) {
            bool overlap = i.start <= end && i.end >= start;
            if (overlap) {
                luaL_unref(L, LUA_REGISTRYINDEX, i.call);
            }
            return overlap;
            });
    }
    else {
        int cb = luaL_ref(L, LUA_REGISTRYINDEX);
        struct hook h = { start, end, cb };
        memhooks.push_back(h);
        memhooks.sort([](const struct hook& a, const struct hook& b) {
            return a.start == b.start ? a.end < b.end : a.start < b.start;
            });
    }

    NES->emu.set_memtracecb(memhooks.size() ? memory_registerwrite_trace : 0);
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
    if (strcmp(str, "a") == 0) {
        lua_pushinteger(L, NES->emu.r.a);
        return 1;
    }
    if (strcmp(str, "x") == 0) {
        lua_pushinteger(L, NES->emu.r.x);
        return 1;
    }
    if (strcmp(str, "y") == 0) {
        lua_pushinteger(L, NES->emu.r.y);
        return 1;
    }
    if (strcmp(str, "pc") == 0) {
        lua_pushinteger(L, (uint16_t)NES->emu.r.pc);
        return 1;
    }
    if (strcmp(str, "s") == 0) {
        lua_pushinteger(L, NES->emu.r.sp);
        return 1;
    }
    if (strcmp(str, "p") == 0) {
        lua_pushinteger(L, NES->emu.r.status);
        return 1;
    }
    return 0;
}

int memory_setregister(lua_State* L) {
    const char* str = luaL_checkstring(L, 1);
    int value = luaL_checknumber(L, 1);
    if (strcmp(str, "a") == 0) {
        NES->emu.r.a = value;
        return 1;
    }
    if (strcmp(str, "x") == 0) {
        NES->emu.r.x = value;
        return 1;
    }
    if (strcmp(str, "y") == 0) {
        NES->emu.r.y = value;
        return 1;
    }
    if (strcmp(str, "pc") == 0) {
        NES->emu.r.pc = value;
        return 1;
    }
    if (strcmp(str, "s") == 0) {
        NES->emu.r.sp = value;
        return 1;
    }
    if (strcmp(str, "p") == 0) {
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

    {"registerwrite", memory_registerwrite},
    {"registerexec", memory_registerexec},
    {"register", memory_registerwrite},
    {"registerrun", memory_registerexec},
    {"registerexecute", memory_registerexec},
    {NULL,NULL}
};

void memorylib_register(lua_State *L) {
    luaL_register(L, "memory", memorylib);
}

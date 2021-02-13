#include "../nesl.h"
#include <list>
#include <vector>

int readword(lua_State* L) {
    uint16_t addrlo = luaL_checkinteger(L, 1);
    uint16_t addrhi = addrlo + 1;
    if (lua_type(L, 2) == LUA_TNUMBER) {
        addrhi = luaL_checkinteger(L, 2);
    }
    return (uint16_t)NES->emu.read(addrlo) | (((uint16_t)NES->emu.read(addrhi)) << 8);
}

struct hook { int id; int registrations; };
std::vector<struct hook> hooks;
int hookregistrations_total[2] = { 0,0 };
int hookregistrations[2][0xFFFF];

void memory_tracecb(int type, nes_addr_t addr, int val) {
    int reg = hookregistrations[type][addr];
    if (reg == 0) return;
    lua_rawgeti(L, LUA_REGISTRYINDEX, reg);
    lua_pushnumber(L, addr);
    lua_pushnumber(L, 1);
    lua_pushnumber(L, val);
    lua_call(L, 3, 0);
}

static int memory_registerhook(lua_State* L, int type) {
    // registerexec takes: address, length, callback
    luaL_checktype(L, 1, LUA_TNUMBER);
    int* regs = hookregistrations[type];

    int size = 1;
    int fn = 2;
    int start = lua_tonumber(L, 1);
    if (lua_isnumber(L, 2)) {
        fn = 3;
        size = lua_tonumber(L, 2);
        if (size == 0) return 0;
    }

    bool removing = lua_isnil(L, fn);

    int end = (start + size) - 1;
    if (end < start) {
        int x = start;
        start = end;
        end = x;
    }

    if (removing) {
        for (int i = start; i < end; ++i) {
            int addr = i % 0xFFFF;

            int hook_idx = regs[addr];
            if (hook_idx == 0) continue;
            regs[addr] = 0;
            struct hook hook = hooks[hook_idx];
            hook.registrations--;
            hookregistrations_total[type] -= 1;
            if (hook.registrations <= 0) {
                luaL_unref(L, LUA_REGISTRYINDEX, hook.id);
            }
        }
    } else {
        int hook_id = luaL_ref(L, LUA_REGISTRYINDEX);
        struct hook hook = { hook_id, end - start };
        hooks.push_back(hook);
        int hook_idx = hooks.size();
        for (int i = start; i < end; ++i) {
            int addr = i % 0xFFFF;
            if (regs[addr] == 0) hookregistrations_total[type] += 1;
            regs[addr] = hook_idx;
        }
    }

    NES->emu.set_tracing(hookregistrations_total[0] + hookregistrations_total[0] > 0);
    return 0;

}

static int memory_registerexec(lua_State * L) {
    return memory_registerhook(L, 0);
}


static int memory_registerwrite(lua_State* L) {
    return memory_registerhook(L, 1);
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
    NES->emu.write(addr, value);
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

void memorylib_register(lua_State* L) {
    luaL_register(L, "memory", memorylib);
}

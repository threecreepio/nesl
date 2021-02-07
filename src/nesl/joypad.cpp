#include "../nesl.h"

static const char* button_mappings[] = {
    "A", "B", "select", "start", "up", "down", "left", "right"
};

static int joypad_get_fake(lua_State* L) {
    int which = luaL_checkinteger(L, 1) - 1;
    if (which < 0 || which > 1) {
        return luaL_error(L, "Invalid input port (valid range 1-4, specified %d)", which);
    }
    lua_newtable(L);
    return 1;
}

static int joypad_get_raw(lua_State* L) {
    int which = luaL_checkinteger(L, 1) - 1;
    if (which < 0 || which > 1) {
        return luaL_error(L, "Invalid input port (valid range 1-4, specified %d)", which);
    }
    lua_pushinteger(L, NES->emu.current_joypad[which % 2]);
    return 1;
}

static int joypad_set(lua_State* L) {
    // Which joypad we're tampering with
    int which = luaL_checkinteger(L, 1) - 1;
    if (which < 0 || which > 3) {
        return luaL_error(L, "Invalid output port (valid range 1-4, specified %d)", which);
    }

    if (lua_isnumber(L, 2)) {
        NES->emu.current_joypad[which % 2] = lua_tointeger(L, 2);
        return 0;
    }

    // And the table of buttons.
    luaL_checktype(L, 2, LUA_TTABLE);

    int val = 0;
    for (int i = 0; i < 8; ++i) {
        lua_getfield(L, 2, button_mappings[i]);
        bool isset = !lua_isnil(L, -1) && lua_toboolean(L, -1);
        lua_pop(L, 1);

        if (isset) {
            val |= 1 << i;
        }
    }

    NES->emu.current_joypad[which % 2] = val;
    return 0;
}

static const struct luaL_reg joypadlib[] = {
    {"get", joypad_get_fake},
    {"getraw", joypad_get_raw},
    {"getdown", joypad_get_fake},
    {"getup", joypad_get_fake},
    {"getimmediate", joypad_get_fake},
    {"set", joypad_set},
    // alternative names
    {"read", joypad_get_fake},
    {"write", joypad_set},
    {"readdown", joypad_get_fake},
    {"readup", joypad_get_fake},
    {"readimmediate", joypad_get_fake},
    {NULL,NULL}
};

static const struct luaL_reg inputlib[] = {
    {"get", joypad_get_fake},
    {"popup", donothing},
    {"openfilepopup", donothing},
    {"savefilepopup", donothing},
    // alternative names
    {"read", joypad_get_fake},
    {NULL,NULL}
};

void joypadlib_register(lua_State* L) {
    luaL_register(L, "joypad", joypadlib);
    luaL_register(L, "input", inputlib);
    //luaL_register(L, "zapper", zapperlib);
}

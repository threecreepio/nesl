#include "../nesl.h"

static const char* button_mappings[] = {
    "A", "B", "select", "start", "up", "down", "left", "right"
};

static int input_get(lua_State* L) {
    lua_newtable(L);
    return 1;
}

static int joypad_getstate(lua_State* L, int state) {
    int which = luaL_checkinteger(L, 1) - 1;
    if (which < 0 || which > 1) {
        return luaL_error(L, "Invalid input port (valid range 1-4, specified %d)", which);
    }
    int joypad = NES->emu.current_joypad[which % 2];
    lua_newtable(L);
    for (int i = 0; i < 8; ++i) {
        int down = (joypad & (1 << i)) ? 1 : 0;
        if (state < 0 || state == down) {
            lua_pushboolean(L, down == 1);
            lua_setfield(L, 2, button_mappings[i]);
        }
    }
    return 1;
}

static int joypad_get(lua_State* L) {
    return joypad_getstate(L, -1);
}

static int joypad_getdown(lua_State* L) {
    return joypad_getstate(L, 1);
}

static int joypad_getup(lua_State* L) {
    return joypad_getstate(L, 0);
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
        joypads[which % 2] = lua_tointeger(L, 2);
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

    joypads[which % 2] = val;
    return 0;
}

static const struct luaL_reg joypadlib[] = {
    {"get", joypad_get},
    {"getraw", joypad_get_raw},
    {"getdown", joypad_getdown},
    {"getup", joypad_getup},
    {"getimmediate", donothing},
    {"set", joypad_set},
    // alternative names
    {"read", joypad_get},
    {"write", joypad_set},
    {"readdown", joypad_getdown},
    {"readup", joypad_getup},
    {"readimmediate", donothing},
    {NULL,NULL}
};

static const struct luaL_reg inputlib[] = {
    {"get", input_get},
    {"popup", donothing},
    {"openfilepopup", donothing},
    {"savefilepopup", donothing},
    // alternative names
    {"read", input_get},
    {NULL,NULL}
};

void joypadlib_register(lua_State* L) {
    luaL_register(L, "joypad", joypadlib);
    luaL_register(L, "input", inputlib);
    //luaL_register(L, "zapper", zapperlib);
}

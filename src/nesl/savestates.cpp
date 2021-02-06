#include "../nesl.h"

int savestate_gc(lua_State* L) {
    Nes_State* ss = (Nes_State*)lua_touserdata(L, 1);
    delete ss;
    return 0;
}

int savestate_object(lua_State* L) {
    Nes_State* ss = new (lua_newuserdata(L, sizeof(Nes_State))) Nes_State();

    lua_newtable(L);
    lua_pushstring(L, "Savestate");
    lua_setfield(L, -2, "__metatable");
    lua_pushcfunction(L, savestate_gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);

    return 1;
}

int savestate_save(lua_State* L) {
    Nes_State* ss = (Nes_State*)lua_touserdata(L, 1);
    if (!ss) {
        luaL_error(L, "Invalid savestate.save object");
        return 0;
    }
    callhook(CALL_BEFORESAVE);
    NES->save_state(ss);
    return 0;
}

int savestate_load(lua_State* L) {
    Nes_State* ss = (Nes_State*)lua_touserdata(L, 1);
    if (!ss) {
        luaL_error(L, "Invalid savestate.save object");
        return 0;
    }
    NES->load_state(*ss);
    callhook(CALL_AFTERLOAD);
    return 0;
}

static int sethook_savestate_registersave(lua_State* L) {
    return sethook_by_id(L, CALL_BEFORESAVE);
}

static int sethook_savestate_registerload(lua_State* L) {
    return sethook_by_id(L, CALL_AFTERLOAD);
}

const struct luaL_reg savestatelib[] = {
    {"create", savestate_object},
    {"object", savestate_object},
    {"save", savestate_save},
    {"persist", donothing},
    {"load", savestate_load},

    {"registersave", sethook_savestate_registersave},
    {"registerload", sethook_savestate_registerload},
    //{"loadscriptdata", savestate_loadscriptdata},
    {NULL,NULL}
};

void savestatelib_register(lua_State* L) {
    luaL_register(L, "savestate", savestatelib);
}

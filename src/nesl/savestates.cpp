#include "../nesl.h"

static const char* SAVESTATE_MT = "nesl.savestate";

static int savestate_gc(lua_State* L) {
    // Nes_State is POD (no owned resources, no user-defined destructor),
    // so this call is currently a no-op. We still invoke the destructor
    // explicitly so that if Nes_State ever grows a real destructor, the
    // GC will pick it up automatically without needing a code change here.
    Nes_State* ss = (Nes_State*)lua_touserdata(L, 1);
    ss->~Nes_State();
    return 0;
}

int savestate_object(lua_State* L) {
    Nes_State* ss = new (lua_newuserdata(L, sizeof(Nes_State))) Nes_State();
    luaL_getmetatable(L, SAVESTATE_MT);
    lua_setmetatable(L, -2);
    return 1;
}

int savestate_save(lua_State* L) {
    Nes_State* ss = (Nes_State*)luaL_checkudata(L, 1, SAVESTATE_MT);
    callhook(CALL_BEFORESAVE);
    NES->save_state(ss);
    return 0;
}

int savestate_load(lua_State* L) {
    Nes_State* ss = (Nes_State*)luaL_checkudata(L, 1, SAVESTATE_MT);
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
    luaL_newmetatable(L, SAVESTATE_MT);
    lua_pushcfunction(L, savestate_gc);
    lua_setfield(L, -2, "__gc");
    lua_pop(L, 1);
}

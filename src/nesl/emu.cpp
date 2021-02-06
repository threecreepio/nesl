#include "../nesl.h"
#include <direct.h>
using namespace std;

static int sethook_emu_registerbefore(lua_State* L) {
    return sethook_by_id(L, CALL_BEFOREEMULATION);
}
static int sethook_emu_registerafter(lua_State* L) {
    return sethook_by_id(L, CALL_AFTEREMULATION);
}
static int sethook_emu_registerexit(lua_State* L) {
    return sethook_by_id(L, CALL_BEFOREEXIT);
}

int emu_pause(lua_State* L) {
    fprintf(stderr, "> Lua paused, press enter to continue\n");
    getchar();
    return 0;
}

int emu_framecount(lua_State* L) {
    int frame = NES->emu.nes.frame_count - 1;
    if (frame < 0) frame = 0;
    lua_pushinteger(L, frame);
    return 1;
}

int emu_frameadvance(lua_State* L) {
    callhook(CALL_BEFOREEMULATION);
    if (NES->emu.nes.frame_count > 1) {
        std::string savepath;
        if (screenshot_pending[0]) {
            savepath = std::string(screenshot_pending);
            screenshot_pending[0] = 0;
            NES->emu.ppu.host_pixels = (uint8_t*)NES->host_pixels;
        }
        NES->emu.emulate_frame();
        if (!savepath.empty()) {
            NES->emu.ppu.host_pixels = 0;
            screenshots_save(savepath);
        }
    }
    else {
        NES->emu.nes.frame_count += 1;
    }
    callhook(CALL_AFTEREMULATION);
    NES->emu.current_joypad[0] = 0;
    NES->emu.current_joypad[1] = 0;
    return 0;
}

int emu_poweron(lua_State* L) {
    NES->reset(true, false);
    return 0;
}

int emu_softreset(lua_State* L) {
    NES->reset(false, false);
    return 0;
}

int emu_loadrom(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    int err = loadRomFile(path);
    if (err) {
        return luaL_error(L, "Failed to load rom");
    }
    const char *errstr = reloadRom();
    if (errstr) {
        return luaL_error(L, errstr);
    }
    return 0;
}

int emu_getdir(lua_State* L) {
    char cwd[0x2000];
    char* res = getcwd(&cwd[0], 0x2000);
    lua_pushstring(L, cwd);
    return 1;
}

int emu_print(lua_State* L) {
    const char* v = lua_tostring(L, 1);
    puts(v);
    return 0;
}

int emu_exit(lua_State* L) {
    callhook(CALL_BEFOREEXIT);
    terminate();
    return 0;
}

static void emu_exec_count_hook(lua_State* L, lua_Debug* dbg) {
    luaL_error(L, "exec_count timeout");
}

static int emu_exec_count(lua_State* L) {
    int count = (int)luaL_checkinteger(L, 1);
    lua_pushvalue(L, 2);
    lua_sethook(L, emu_exec_count_hook, LUA_MASKCOUNT, count);
    int ret = lua_pcall(L, 0, 0, 0);
    lua_sethook(L, NULL, 0, 0);
    lua_settop(L, 0);
    lua_pushinteger(L, ret);
    return 1;
}

static const struct luaL_reg emulib[] = {
    {"framecount", emu_framecount},
    {"frameadvance", emu_frameadvance},
    {"poweron", emu_poweron},
    {"softreset", emu_softreset},
    {"loadrom", emu_loadrom},
    {"speedmode", donothing},
    // {"debuggerloop", emu_debuggerloop},
    // {"debuggerloopstep", emu_debuggerloopstep},
    {"paused", donothing},
    {"pause", emu_pause},
    {"unpause", donothing},
    {"exec_count", emu_exec_count},
    {"exec_time", unimplemented},
    {"setrenderplanes", donothing},
    {"message", donothing},
    {"lagcount", unimplemented},
    {"lagged", unimplemented},
    {"setlagflag", unimplemented},
    {"emulating", donothing},
    {"registerbefore", sethook_emu_registerbefore},
    {"registerafter", sethook_emu_registerafter},
    {"registerexit", sethook_emu_registerexit},
    {"addgamegenie", unimplemented},
    {"delgamegenie", unimplemented},
    {"getscreenpixel", unimplemented},
    {"readonly", donothing},
    {"setreadonly", donothing},
    {"getdir", emu_getdir},
    {"print", emu_print}, // sure, why not
    {"exit", emu_exit}, // useful for run-and-close scripts
    {NULL,NULL}
};

void emulib_register(lua_State* L) {
    luaL_register(L, "emu", emulib);
    luaL_register(L, "FCEU", emulib);
}

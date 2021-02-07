#include "nesl.h"
#include "./lua_bitops.cpp"
#include <fstream>
#include <direct.h>
#include <bitset>
#include <signal.h>
using namespace std;

char romFileName[0x2000] = {};
uint8_t* romData = 0;
size_t romDataLength = 0;
lua_State* L = 0;
Nes_Emu* NES = 0;

static const char* luaCallIDStrings[] = {
    "CALL_BEFOREEMULATION",
    "CALL_AFTEREMULATION",
    "CALL_BEFOREEXIT",
    "CALL_BEFORESAVE",
    "CALL_AFTERLOAD",
};

static int tobitstring(lua_State* L) {
    std::bitset<8> bits(luaL_checkinteger(L, 1));
    std::string temp = bits.to_string().insert(4, " ");
    const char* result = temp.c_str();
    lua_pushstring(L, result);
    return 1;
}

static int addressof(lua_State* L) {
    const void* ptr = lua_topointer(L, -1);
    lua_pushinteger(L, (lua_Integer)ptr);
    return 1;
}

void callhook(LUAHookId calltype) {
    const char* idstring = luaCallIDStrings[calltype];
    if (!L) return;

    lua_settop(L, 0);
    lua_getfield(L, LUA_REGISTRYINDEX, idstring);

    int errorcode = 0;
    if (lua_isfunction(L, -1)) {
        errorcode = lua_pcall(L, 0, 0, 0);
    } else {
        lua_pop(L, 1);
    }
}

int sethook_by_id(lua_State* L, int id) {
    if (!lua_isnil(L, 1)) luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_settop(L, 1);
    lua_getfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[id]);
    lua_insert(L, 1);
    lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[id]);
    return 1;
}

int loadRomFile(const char* path) {
    if (romData) {
        free(romData);
    }

    struct stat st;
    int err = stat(path, &st);
    if (err) return err;

    romDataLength = st.st_size;
    romData = (uint8_t*) malloc(st.st_size);
    if (romData == 0) return 1;

    FILE *f = fopen(path, "rb");
    if (f == 0) return 2;

    fread(romData, 1, st.st_size, f);
    fclose(f);
    return 0;
}

const char* reloadRom(void) {
    Mem_File_Reader r(romData, romDataLength);
    Auto_File_Reader a(r);
    const char *d = NES->load_ines(a);
    if (d) return d;
    NES->reset(true, false);
    NES->emu.nes.frame_count -= 1;
    return 0;
}

int donothing(lua_State* L) {
    return 0;
}

int unimplemented(lua_State* L) {
    return luaL_error(L, "Method not implemented");
}

void sigint(int v) {
    screenshots_exit();
}

void nesl_terminate(void) {
    screenshots_exit();
#if DEBUG
    lua_close(L);
    delete emu;
#endif
    exit(0);
}

int main(int argc, char** argv) {
    signal(SIGINT, sigint);

    romData = 0;
    int err;

    if (argc < 2) {
        fprintf(stderr, "usage: nesl script.lua [startup-rom.nes]\n");
        return 1;
    }

    if (argc > 2) {
        int err = loadRomFile(argv[2]);
        if (err != 0) {
            fprintf(stderr, "Failed to load rom\n");
            return -1;
        }
    }

    L = luaL_newstate();
    luaL_openlibs(L);
    emulib_register(L);
    memorylib_register(L);
    screenshotlib_register(L);
    ppulib_register(L);
    savestatelib_register(L);
    joypadlib_register(L);
    romlib_register(L);
    lua_settop(L, 0);
    //luaL_register(L, "debugger", debuggerlib);
    luaL_register(L, "bit", bit_funcs); // LuaBitOp library

    lua_register(L, "tobitstring", tobitstring);
    lua_register(L, "addressof", addressof);

    lua_register(L, "AND", bit_band);
    lua_register(L, "OR", bit_bor);
    lua_register(L, "XOR", bit_bxor);
    lua_register(L, "SHIFT", bit_bshift_emulua);
    lua_register(L, "BIT", bitbit);

    err = luaL_loadfile(L, argv[1]);
    if (err != 0) {
        const char* errstr = lua_tostring(L, -1);
        fprintf(stderr, "%s\n", errstr);
        return -err;
    }

    std::string getfilepath = argv[1];
    getfilepath = getfilepath.substr(0, getfilepath.find_last_of("/\\") + 1);
    if (getfilepath.length() > 0) {
        err = _chdir(getfilepath.c_str());
        if (err != 0) {
            fprintf(stderr, "Failed to load lua file\n");
            return -err;
        }
    }

    register_optional_mappers();
    NES = new Nes_Emu();
    if (romData) {
        const char* errstr = reloadRom();
        if (errstr != 0) {
            fprintf(stderr, "Could not load rom,%s\n", errstr);
            return -1;
        }
    }

    err = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (err != 0) {
        const char* errstr = lua_tostring(L, -1);
        fprintf(stderr, "%s\n", errstr);
        return -1;
    }

    callhook(CALL_BEFOREEXIT);
    terminate();
    return 0;
}

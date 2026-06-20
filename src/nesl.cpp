#include "nesl.h"
#include "./lua_bitops.cpp"
#include "./nesl/signal_handler.h"
#include <fstream>
#include <string.h>
#ifdef WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
    #include <unistd.h>
#endif
#include <bitset>
#include <signal.h>
using namespace std;

char romFileName[0x2000] = {};
uint8_t romData[1024 * 1024 * 10];
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
    romDataLength = 0;

    struct stat st;
    int err = stat(path, &st);
    if (err) return err;

    // rom file size must be less than 10MB; the +1 sentinel below needs one
    // byte of headroom in romData[].
    if (st.st_size >= (long)sizeof(romData)) return 1;
    romDataLength = st.st_size;

    FILE *f = fopen(path, "rb");
    if (f == 0) return 2;

    fread(romData, 1, st.st_size, f);
    romData[st.st_size] = 0;
    fclose(f);

    // Record the basename of the path for rom.getfilename(). Use a
    // bounded copy to avoid the 0x2000-byte buffer ever being
    // overflowed; truncate if the path is longer than the buffer.
    const char* base = strrchr(path, '/');
#ifdef WIN32
    if (base == NULL) base = strrchr(path, '\\');
#endif
    if (base != NULL) {
        base += 1;
    } else {
        base = path;
    }
    size_t base_len = strlen(base);
    if (base_len >= sizeof(romFileName)) {
        base_len = sizeof(romFileName) - 1;
    }
    memcpy(romFileName, base, base_len);
    romFileName[base_len] = '\0';

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

// Lua panic handler. When a Lua runtime error bypasses pcall (e.g.
// out-of-memory during table operations, or an unprotected C call
// into Lua), Lua invokes this function. Without a panic handler the
// default behavior is to call abort() with no diagnostic. We print
// the panic message and a stack frame walk to stderr before
// aborting, so the failure mode is at least diagnosable.
//
// Lua contract: this function must NOT return. If it does, Lua
// falls back to its own default (which is to call abort()).
//
// We walk the Lua stack with lua_getstack/lua_getinfo because the
// vendored Lua is 5.1, which does not have luaL_traceback (added
// in 5.2). lua_getstack is safe to call from a panic context.
static int nesl_lua_panic(lua_State* L) {
    const char* msg = lua_tostring(L, -1);
    if (msg == NULL) msg = "<no message>";
    fprintf(stderr, "\nnesl: Lua panic: %s\n", msg);
    fprintf(stderr, "nesl: stack frames (innermost first):\n");

    lua_Debug ar;
    for (int level = 0; lua_getstack(L, level, &ar); ++level) {
        if (lua_getinfo(L, "Sl", &ar) == 0) break;
        const char* source = ar.source ? ar.source : "?";
        fprintf(stderr, "  [%d] %s:%d\n", level, source, ar.currentline);
    }

    fflush(stderr);
    abort();
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
    nesl_install_signal_handlers();

    romData[0] = 0;
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
    lua_atpanic(L, nesl_lua_panic);
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
        err = chdir(getfilepath.c_str());
        if (err != 0) {
            fprintf(stderr, "Failed to load lua file\n");
            return -err;
        }
    }

    register_optional_mappers();
    NES = new Nes_Emu();
    if (romDataLength > 0) {
        const char* errstr = reloadRom();
        if (errstr != 0) {
            fprintf(stderr, "Could not load rom, %s\n", errstr);
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
    nesl_terminate();
    return 0;
}

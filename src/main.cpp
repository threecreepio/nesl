#include "nes_emu/Nes_Emu.h"
#include "nes_emu/Nes_File.h"
#include "nes_emu/Nes_State.h"
#include <stdio.h>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include <bitset>
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "./lua_bitops.cpp"
}

using namespace std;

char romFileName[0x2000] = { 0 };
uint8_t* romData = 0;
size_t romDataLength = 0;
lua_State* L = 0;
Nes_Emu* NES = 0;

int luahook_emu_registerbefore = 0;
int luahook_emu_registerafter = 0;

static const char* luaCallIDStrings[] =
{
    "CALL_BEFOREEMULATION",
    "CALL_AFTEREMULATION",
    "CALL_BEFOREEXIT",
    "CALL_BEFORESAVE",
    "CALL_AFTERLOAD",
};

enum LUAHookId {
    CALL_BEFOREEMULATION,
    CALL_AFTEREMULATION,
    CALL_BEFOREEXIT,
    CALL_BEFORESAVE,
    CALL_AFTERLOAD
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

static void callhook(LUAHookId calltype)
{
    const char* idstring = luaCallIDStrings[calltype];

    if (!L)
        return;

    lua_settop(L, 0);
    lua_getfield(L, LUA_REGISTRYINDEX, idstring);

    int errorcode = 0;
    if (lua_isfunction(L, -1))
    {
        errorcode = lua_pcall(L, 0, 0, 0);
        if (errorcode) {

        }
    }
    else
    {
        lua_pop(L, 1);
    }
}

#define make_hook(name, id) \
static int sethook_##name(lua_State* L) { \
if (!lua_isnil(L, 1)) \
luaL_checktype(L, 1, LUA_TFUNCTION); \
lua_settop(L, 1); \
lua_getfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[##id]); \
lua_insert(L, 1); \
lua_setfield(L, LUA_REGISTRYINDEX, luaCallIDStrings[##id]); \
return 1; \
}

make_hook(emu_registerbefore, CALL_BEFOREEMULATION)
make_hook(emu_registerafter, CALL_AFTEREMULATION)
make_hook(emu_registerexit, CALL_BEFOREEXIT)
make_hook(savestate_registersave, CALL_BEFORESAVE)
make_hook(savestate_registerload, CALL_AFTERLOAD)


void memory_registerexec_tracecb(uint32_t *trace) {
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

const char* loadRomFile(const char* path) {
    if (romData) {
        free(romData);
    }
    strcpy(&romFileName[0], path);


    streampos begin, end;
    ifstream file(path, ios::binary);
    file.seekg(0, ios::end);
    end = file.tellg();
    file.seekg(0, ios::beg);
    begin = file.tellg();
    romDataLength = end - begin;
    romData = (uint8_t*) malloc(romDataLength);
    file.read((char*) &romData[0], romDataLength);
    file.seekg(0, ios::end);
    file.close();

    return 0;
}


int emu_pause(lua_State* L) {
    fprintf(stderr, "> Lua paused, press any key to continue.\n");
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
        NES->emu.emulate_frame();
    }
    else {
        NES->emu.nes.frame_count += 1;
    }
    callhook(CALL_AFTEREMULATION);
    NES->emu.current_joypad[0] = 0;
    NES->emu.current_joypad[1] = 0;
    return 0;
}

const char* reloadRom(void) {
    Mem_File_Reader r(romData, romDataLength);
    Auto_File_Reader a(r);
    const char *d = NES->load_ines(a);
    NES->reset(true, false);
    return d;
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
    const char* result = loadRomFile(path);
    if (result != 0) {
        return luaL_error(L, result);
    }
    reloadRom();
    return 0;
}

int emu_getdir(lua_State* L) {
    char cwd[0x2000];
    char* res = getcwd(&cwd[0], 0x2000);
    lua_pushstring(L, cwd);
    return 1;
}

int rom_getfilename(lua_State* L) {
    lua_pushstring(L, romFileName);
    return 1;
}


int emu_print(lua_State* L) {
    const char* v = lua_tostring(L, 1);
    puts(v);
    return 0;
}

int emu_exit(lua_State* L) {
    int retval = lua_tonumber(L, 1);
    callhook(CALL_BEFOREEXIT);
#if DEBUG
    lua_close(L);
    delete emu;
#endif
    exit(retval);
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

int donothing(lua_State* L) {
    return 0;
}

int readword(lua_State* L) {
    uint16_t addrlo = luaL_checkinteger(L, 1);
    uint16_t addrhi = addrlo + 1;
    if (lua_type(L, 2) == LUA_TNUMBER) {
        addrhi = luaL_checkinteger(L, 2);
    }
    return (uint16_t)NES->emu.read(addrlo) | (((uint16_t)NES->emu.read(addrhi)) << 8);
}

int rom_readbyte(lua_State* L) {
    int addr = luaL_checkinteger(L, 1);
    if (addr < 0 || addr > romDataLength)
        luaL_error(L, "invalid argument addr");
    lua_pushinteger(L, romData[addr]);
    return 1;
}

int rom_readbytesigned(lua_State* L) {
    int addr = luaL_checkinteger(L, 1);
    if (addr < 0 || addr > romDataLength)
        luaL_error(L, "invalid argument addr");
    lua_pushinteger(L, (signed) romData[addr]);
    return 1;
}

static int rom_readbyterange(lua_State* L) {
    size_t range_start = luaL_checkinteger(L, 1);
    size_t range_size = luaL_checkinteger(L, 2);
    if (range_size < 0)
        return 0;
    if (range_start < 0 || range_start >= romDataLength)
        luaL_error(L, "invalid argument range_start");
    if (range_size < 0 || range_start + range_size >= romDataLength)
        luaL_error(L, "invalid argument range_size");

    char* buf = (char*)malloc(range_size);
    if (buf == 0) {
        return luaL_error(L, "allocating space for readbyterange failed");
    }

    for (int i = 0; i < range_size; i++) {
        buf[i] = romData[range_start + i];
    }

    lua_pushlstring(L, buf, range_size);
    return 1;
}

int unimplemented(lua_State* L) {
    return luaL_error(L, "unimplemented");
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

int ppu_readbyte(lua_State* L) {
    uint16_t addr = lua_tonumber(L, 1);
    int retval = NES->emu.ppu.peekaddr(addr);
    lua_pushinteger(L, retval);
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

static int joypad_set(lua_State* L) {
    // Which joypad we're tampering with
    int which = luaL_checkinteger(L, 1) - 1;
    if (which < 0 || which > 3) {
        return luaL_error(L, "Invalid output port (valid range 1-4, specified %d)", which);
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

static int memory_readbyterange(lua_State* L) {
    int range_start = luaL_checkinteger(L, 1);
    int range_size = luaL_checkinteger(L, 2);
    if (range_size < 0)
        return 0;

    char* buf = (char*) malloc(range_size);
    if (buf == 0) {
        return luaL_error(L, "allocating space for readbyterange failed");
    }

    for (int i = 0; i < range_size; i++) {
        buf[i] = NES->emu.read(range_start + i);
    }

    lua_pushlstring(L, buf, range_size);
    return 1;
}


static int ppu_readbyterange(lua_State* L) {
    int range_start = luaL_checkinteger(L, 1);
    int range_size = luaL_checkinteger(L, 2);
    if (range_size < 0)
        return 0;

    char* buf = (char*)malloc(range_size);
    if (buf == 0) {
        return luaL_error(L, "allocating space for readbyterange failed");
    }

    for (int i = 0; i < range_size; i++) {
        buf[i] = NES->emu.ppu.peekaddr(range_start + i);
    }

    lua_pushlstring(L, buf, range_size);
    return 1;
}


static const struct luaL_reg guilib[] = {
    {"pixel", donothing},
    {"getpixel", donothing},
    {"line", donothing},
    {"box", donothing},
    {"text", donothing},

    {"parsecolor", donothing},

    {"savescreenshot",   donothing},
    {"savescreenshotas", donothing},
    {"gdscreenshot", donothing},
    {"gdoverlay", donothing},
    {"opacity", donothing},
    {"transparency", donothing},

    {"register", donothing},

    {"popup", donothing},
    // alternative names
    {"drawtext", donothing},
    {"drawbox", donothing},
    {"drawline", donothing},
    {"drawpixel", donothing},
    {"setpixel", donothing},
    {"writepixel", donothing},
    {"rect", donothing},
    {"drawrect", donothing},
    {"drawimage", donothing},
    {"image", donothing},
    {NULL,NULL}
};

static const struct luaL_reg joypadlib[] = {
    {"get", joypad_get_fake},
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


static const struct luaL_reg savestatelib[] = {
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

static const struct luaL_reg memorylib[] = {
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

static const struct luaL_reg inputlib[] = {
    {"get", joypad_get_fake},
    {"popup", donothing},
    {"openfilepopup", donothing},
    {"savefilepopup", donothing},
    // alternative names
    {"read", joypad_get_fake},
    {NULL,NULL}
};

static const struct luaL_reg romlib[] = {
    {"getfilename", rom_getfilename},
    {"gethash", unimplemented},
    {"readbyte", rom_readbyte},
    {"readbytesigned", rom_readbytesigned},
    // alternate naming scheme for unsigned
    {"readbyteunsigned", rom_readbyte},
    {"readbyterange", rom_readbyterange},
    {"writebyte", unimplemented},
    {NULL,NULL}
};

static const struct luaL_reg ppulib[] = {
    {"readbyte", ppu_readbyte},
    {"readbyterange", ppu_readbyterange},

    {NULL,NULL}
};

int main(int argc, char** argv) {
    int err;

    if (argc < 2) {
        fprintf(stderr, "usage: nesl script.lua [startup-rom.nes]\n");
        return 1;
    }

    if (argc > 2) {
        const char* errstr = loadRomFile(argv[2]);
        if (errstr != 0) {
            fprintf(stderr, "could not load rom file,%s\n", errstr);
            return -1;
        }
    }

    std::string getfilepath = argv[1];
    getfilepath = getfilepath.substr(0, getfilepath.find_last_of("/\\") + 1);
    if (getfilepath.length() > 0) {
        err = chdir(getfilepath.c_str());
        if (err != 0) {
            fprintf(stderr, "failed to load lua file, chdir error %d.\n", err);
            return -err;
        }
    }

    L = luaL_newstate();
    luaL_openlibs(L);
    luaL_register(L, "emu", emulib);
    luaL_register(L, "FCEU", emulib); // kept for backward compatibility
    luaL_register(L, "memory", memorylib);
    luaL_register(L, "ppu", ppulib);
    luaL_register(L, "rom", romlib);
    luaL_register(L, "joypad", joypadlib);
    //luaL_register(L, "zapper", zapperlib);
    luaL_register(L, "input", inputlib);
    lua_settop(L, 0); // clean the stack, because each call to luaL_register leaves a table on top (eventually overflows)
    luaL_register(L, "savestate", savestatelib);
    //luaL_register(L, "movie", movielib);
    luaL_register(L, "gui", guilib);
    //luaL_register(L, "sound", soundlib);
    //luaL_register(L, "debugger", debuggerlib);
    //luaL_register(L, "cdlog", cdloglib);
    //luaL_register(L, "taseditor", taseditorlib);
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

    register_optional_mappers();
    NES = new Nes_Emu();
    if (romData) {
        reloadRom();
    }

    err = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (err != 0) {
        const char* errstr = lua_tostring(L, -1);
        fprintf(stderr, "%s\n", errstr);
        return -1;
    }

    callhook(CALL_BEFOREEXIT);
#if DEBUG
    lua_close(L);
    delete emu;
#endif

    return 0;
}

#pragma once
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#include "nes_emu/Nes_Emu.h"
#include "nes_emu/Nes_File.h"
#include "nes_emu/Nes_State.h"
#include <string>

extern Nes_Emu* NES;
extern char romFileName[0x2000];
extern uint8_t* romData;
extern size_t romDataLength;
extern lua_State* L;
extern uint8_t joypads[2];

enum LUAHookId {
    CALL_BEFOREEMULATION,
    CALL_AFTEREMULATION,
    CALL_BEFOREEXIT,
    CALL_BEFORESAVE,
    CALL_AFTERLOAD
};

int loadRomFile(const char* path);
const char* reloadRom(void);

int sethook_by_id(lua_State* L, int id);
void callhook(LUAHookId calltype);
int donothing(lua_State* L);
int unimplemented(lua_State* L);
void nesl_terminate(void);

extern bool screenshotpending;
void screenshots_exit(void);
void screenshots_beforeframe(void);
int screenshots_afterframe(void);
int gui_savescreenshotas(lua_State* L);
int gui_setscreenshottarball(lua_State* L);

void memorylib_register(lua_State* L);
void savestatelib_register(lua_State* L);
void ppulib_register(lua_State* L);
void emulib_register(lua_State* L);
void screenshotlib_register(lua_State* L);
void joypadlib_register(lua_State* L);
void romlib_register(lua_State* L);


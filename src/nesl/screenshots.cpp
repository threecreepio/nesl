#define STB_IMAGE_WRITE_IMPLEMENTATION
#define M_PI 3.14159265358979323846
extern "C" {
#include <stb_image_write.h>
}
#include "../nesl.h"
#include <microtar.h>
#include <direct.h>
#include <list>

#define BITMAP_WIDTH 256
#define BITMAP_HEIGHT 240
#define BITMAP_SIZE (BITMAP_WIDTH * BITMAP_HEIGHT)

#define QNES_WIDTH 272
#define QNES_HEIGHT 242

uint8_t colortable[0x40][0x3] = {};
bool screenshotinitialized = 0;
bool screenshotpending = 0;

extern bool screenshotpending;
uint8_t screenshotdata[QNES_WIDTH * QNES_HEIGHT];
char screenshotpath[0x2000];

mtar_t* tar = 0;
#define BMP_PALETTE_OFFSET 0x36
#define BMP_DATA_OFFSET 0x36 + (0x20 * 4)
uint8_t bmptmp[BMP_DATA_OFFSET + BITMAP_SIZE] = {
    // BMP
    'B', 'M',
    0xB8, 0xF0, 0x00, 0x00, // file size.. 54 + (0x20 * 4) + (256 * 240) = 
    0x00, 0x00, 0x00, 0x00,
    0xB6, 0x00, 0x00, 0x00, // offset to file data
    // DIB
    0x28, 0x00, 0x00, 0x00, // 40 bytes
    0x00, 0x01, 0x00, 0x00, // bitmap width
    0x10, 0xFF, 0xFF, 0xFF, // bitmap height
    0x01, 0x00,             // color planes
    0x08, 0x00,             // 8 bits per pixel
    0x00, 0x00, 0x00, 0x00, // compression
    0x02, 0xF0, 0x00, 0x00, // bitmap data length
    0x12, 0x0B, 0x00, 0x00, // print resolution..
    0x12, 0x0B, 0x00, 0x00, // print resolution..
    0x20, 0x00, 0x00, 0x00, // palette size
    0x20, 0x00, 0x00, 0x00, // important colors.. they're all important to us!
    // PALETTE DATA ..
};

void tarball_close() {
    if (tar == 0) return;
    mtar_finalize(tar);
    mtar_close(tar);
    free(tar);
    tar = 0;
}

int tarball_open(std::string path) {
    tarball_close();
    tar = (mtar_t*)malloc(sizeof(mtar_t));
    return mtar_open(tar, path.c_str(), "wb");
}


static int mkdirp(char* file) {
    const char* offset = file;
    while (true) {
        const char* next = strchr(offset, '\\');
        if (!next) next = strchr(offset, '/');
        if (!next) break;
        int off = next - file;
        if (off > 1) {
            file[off] = '\0';
            int err = mkdir(file);
            file[off] = '/';
            if (err && err != -1) return err;
        }
        offset = next + 1;
    }
    return 0;
}


int screenshots_save2(char *path, const uint8_t *src, const uint8_t *lut) {
    if (!tar) {
        int err = mkdirp(path);
        if (err) {
            return luaL_error(L, "Failed to create screenshot path");
        }
    }

    const char* extname = strrchr(path, '.');
    if (strcmp(extname, ".bmp") == 0) {
        for (int i = 0; i < 0x20; ++i) {
            short lutclr = i & 0b00000011 ? lut[i] : lut[0];
            bmptmp[BMP_PALETTE_OFFSET + (i * 4) + 0] = colortable[lutclr][2]; // BLUE
            bmptmp[BMP_PALETTE_OFFSET + (i * 4) + 1] = colortable[lutclr][1]; // GREEN
            bmptmp[BMP_PALETTE_OFFSET + (i * 4) + 2] = colortable[lutclr][0]; // RED
        }
        for (int y = 0; y < BITMAP_HEIGHT; ++y) {
            const int host_y = QNES_WIDTH * y;
            for (int x = 0; x < BITMAP_WIDTH; ++x) {
                const int destpx = BMP_DATA_OFFSET + x + (y * BITMAP_WIDTH);
                bmptmp[destpx] = src[host_y + x + 8];
            }
        }
        if (tar) {
            mtar_write_file_header(tar, path, BMP_DATA_OFFSET + BITMAP_SIZE);
            mtar_write_data(tar, bmptmp, BMP_DATA_OFFSET + BITMAP_SIZE);
        } else {
            FILE* f = fopen(path, "wb");
            if (f == 0) return 1;
            fwrite(bmptmp, 1, BMP_DATA_OFFSET + BITMAP_SIZE, f);
            fclose(f);
        }
    } else {
        size_t index = 0;
        size_t sindex = 0;
        uint8_t *pngtmp = new uint8_t[BITMAP_WIDTH * BITMAP_HEIGHT * 3];
        for (int y = 0; y < BITMAP_HEIGHT; ++y) {
            int host_y = QNES_WIDTH * y;
            for (int x = 0; x < BITMAP_WIDTH; ++x) {
                char palclr = src[host_y + x + 8];
                short clr = palclr & 0b00000011 ? lut[palclr] : lut[0];
                pngtmp[index++] = colortable[clr][0];
                pngtmp[index++] = colortable[clr][1];
                pngtmp[index++] = colortable[clr][2];
            }
        }
        if (tar) {
            int len;
            uint8_t* bin = stbi_write_png_to_mem(pngtmp, BITMAP_WIDTH * 3, BITMAP_WIDTH, BITMAP_HEIGHT, 3, &len);
            if (bin == 0) {
                delete[] pngtmp;
                return 1;
            }
            mtar_write_file_header(tar, path, len);
            mtar_write_data(tar, bin, len);
            free(bin);
        }
        else {
            stbi_write_png_compression_level = 0;
            int result = stbi_write_png(path, BITMAP_WIDTH, BITMAP_HEIGHT, 3, pngtmp, BITMAP_WIDTH * 3);
        }
        delete[] pngtmp;
    }
    return 0;
}

// Converted from Kevin Horton's qbasic palette generator.
static void create_ntsc_palette(void)
{
    static int ntsctint = 46 + 10;
    static int ntschue = 72;

    int x, z;
    int r, g, b;
    double s, luma, theta;
    static uint8_t cols[16] = { 0,24,21,18,15,12,9,6,3,0,33,30,27,0,0,0 };
    static uint8_t br1[4] = { 6,9,12,12 };
    static double br2[4] = { .29,.45,.73,.9 };
    static double br3[4] = { 0,.24,.47,.77 };

    for (x = 0; x <= 3; x++)
    {
        for (z = 0; z < 16; z++)
        {
            s = (double)ntsctint / 128;
            luma = br2[x];
            if (z == 0) { s = 0; luma = ((double)br1[x]) / 12; }

            if (z >= 13)
            {
                s = luma = 0;
                if (z == 13)
                    luma = br3[x];
            }

            theta = (double)M_PI * (double)(((double)cols[z] * 10 + (((double)ntschue / 2) + 300)) / (double)180);
            r = (int)((luma + s * sin(theta)) * 256);
            g = (int)((luma - (double)27 / 53 * s * sin(theta) + (double)10 / 53 * s * cos(theta)) * 256);
            b = (int)((luma - s * cos(theta)) * 256);


            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            if (r < 0) r = 0;
            if (g < 0) g = 0;
            if (b < 0) b = 0;

            int ofs = (x << 4) + z;
            colortable[ofs][0] = r;
            colortable[ofs][1] = g;
            colortable[ofs][2] = b;
        }
    }
}


void screenshots_beforeframe(void) {
    NES->emu.ppu.host_pixels = screenshotdata;
}

int screenshots_afterframe(void) {
    screenshotpending = false;
    int err = screenshots_save2(screenshotpath, screenshotdata, NES->emu.ppu.palette);
    NES->emu.ppu.host_pixels = 0;
    if (err) {
        return luaL_error(L, "Failed to save screenshot");
    }
}

int gui_savescreenshotas(lua_State* L) {
    if (!screenshotinitialized) {
        screenshotinitialized = true;
        create_ntsc_palette();
    }
    screenshotpending = true;
    const char* path = luaL_checkstring(L, 1);
    strncpy(screenshotpath, path, 0x2000);
    return 0;
}

int gui_screenshotarchive(lua_State* L) {
    tarball_close();
    if (lua_isnil(L, 1)) return 0;
    const char* path = luaL_checkstring(L, 1);
    if (tarball_open(path)) {
        return luaL_error(L, "Failed to create tarball");
    }
    return 0;
}

void screenshots_exit(void) {
    tarball_close();
}

static const struct luaL_reg guilib[] = {
    {"pixel", donothing},
    {"getpixel", donothing},
    {"line", donothing},
    {"box", donothing},
    {"text", donothing},

    {"parsecolor", donothing},

    {"savescreenshot",   donothing},
    {"screenshotarchive",   gui_screenshotarchive},
    {"savescreenshotas", gui_savescreenshotas},
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

void screenshotlib_register(lua_State* L) {
    luaL_register(L, "gui", guilib);
}

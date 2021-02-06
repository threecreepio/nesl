#define STB_IMAGE_WRITE_IMPLEMENTATION
#define M_PI 3.14159265358979323846
extern "C" {
#include <stb_image_write.h>
}
#include "../nesl.h"
#include <microtar.h>

char screenshot_pending[0x2000];
uint8_t colortable[0x40][0x3] = {};

mtar_t* tar = 0;
#define BMP_PALETTE_OFFSET 0x36
#define BMP_DATA_OFFSET 0xB6
uint8_t bmptmp[BMP_DATA_OFFSET] = {
    // BMP
    'B', 'M',
    0xD8, 0x01, 0x01, 0x00, // file size.. 54 + (32 * 3) + (272 * 242) = 
    0x00, 0x00, 0x00, 0x00,
    0xB6, 0x00, 0x00, 0x00, // offset to file data
    // DIB
    0x28, 0x00, 0x00, 0x00, // 40 bytes
    0x10, 0x01, 0x00, 0x00, // 272 px width
    0x0E, 0xFF, 0xFF, 0xFF, // 242 px height
    0x01, 0x00,             // color planes
    0x08, 0x00,             // 8 bits per pixel
    0x00, 0x00, 0x00, 0x00, // compression
    0x22, 0x01, 0x01, 0x00, // bitmap data length
    0x13, 0x0B, 0x00, 0x00, // print resolution..
    0x13, 0x0B, 0x00, 0x00, // print resolution..
    0x20, 0x00, 0x00, 0x00, // palette size
    0x20, 0x00, 0x00, 0x00, // important colors.. they're all important to us!
    // PALETTE...
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

void screenshots_save(std::string path) {
    uint8_t* src = (uint8_t*)NES->host_pixels;
    const uint8_t* lut = NES->emu.ppu.palette;
    size_t W = 272;
    size_t H = 242;
    size_t SIZE = W * H;

    const char* extname = strrchr(path.c_str(), '.');
    if (strcmp(extname, ".bmp") == 0) {
        for (int i = 0; i < 0x20; ++i) {
            short lutclr = i & 0b00000011 ? lut[i] : lut[0];
            bmptmp[BMP_PALETTE_OFFSET + (i * 4) + 0] = colortable[lutclr][2]; // BLUE
            bmptmp[BMP_PALETTE_OFFSET + (i * 4) + 1] = colortable[lutclr][1]; // GREEN
            bmptmp[BMP_PALETTE_OFFSET + (i * 4) + 2] = colortable[lutclr][0]; // RED
        }
        if (tar) {
            mtar_write_file_header(tar, path.c_str(), BMP_DATA_OFFSET + SIZE);
            mtar_write_data(tar, bmptmp, BMP_DATA_OFFSET);
            mtar_write_data(tar, src, SIZE);
        } else {
            FILE* f = fopen(path.c_str(), "wb");
            fwrite(bmptmp, 1, BMP_DATA_OFFSET, f);
            fwrite(src, 1, SIZE, f);
            fclose(f);
        }
    } else {
        size_t index = 0;
        size_t sindex = 0;
        uint8_t *pngtmp = new uint8_t[0x30000];
        for (int y = 0; y < H - 1; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                char palclr = src[x + (y * W)];
                short clr = palclr & 0b00000011 ? lut[palclr] : lut[0];
                pngtmp[index++] = colortable[clr][0];
                pngtmp[index++] = colortable[clr][1];
                pngtmp[index++] = colortable[clr][2];
            }
        }
        if (tar) {
            int len;
            uint8_t* bin = stbi_write_png_to_mem(pngtmp, W * 3, W, H, 3, &len);
            mtar_write_file_header(tar, path.c_str(), len);
            mtar_write_data(tar, bin, len);
            free(bin);
        }
        else {
            stbi_write_png(path.c_str(), W, H, 0, pngtmp, W * 3);
        }
        delete[] pngtmp;
    }
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

            colortable[(x << 4) + z][0] = r;
            colortable[(x << 4) + z][1] = g;
            colortable[(x << 4) + z][2] = b;
        }
    }
}

int gui_savescreenshotas(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    strncpy_s(screenshot_pending, path, sizeof(screenshot_pending));
    return 0;
}

int gui_setscreenshottarball(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    tarball_close();
    if (tarball_open(path)) {
        return luaL_error(L, "Failed to create tarball");
    }
    return 0;
}

void screenshots_exit() {
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
    {"setscreenshottarball",   gui_setscreenshottarball},
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
    create_ntsc_palette();
    luaL_register(L, "gui", guilib);
}

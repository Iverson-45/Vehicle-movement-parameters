#include "st7567s.h"



static uint8_t fb[ST7567_PAGES][ST7567_WIDTH];


static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;


static const uint8_t font5x7[][6] = {
#include "font5x7.inc"
};

static void st7567_cmd(const uint8_t *cmds, uint8_t len)
{
    I2C_WriteRegister(DISPLAY_I2C_ADDR, ST7567_CONTROL_CMD, (uint8_t *)cmds, len);
}

static void st7567_data(const uint8_t *data, uint16_t len)
{
    const uint16_t CHUNK = 32;
    uint16_t sent = 0;
    while (sent < len) {
        uint16_t tosend = (len - sent) > CHUNK ? CHUNK : (len - sent);
        I2C_WriteRegister(DISPLAY_I2C_ADDR, ST7567_CONTROL_DATA, (uint8_t *)&data[sent], tosend);
        sent += tosend;
    }
}

void ST7567_DisplayOn(void)    { uint8_t c = 0xAF; st7567_cmd(&c,1); }
void ST7567_DisplayOff(void)   { uint8_t c = 0xAE; st7567_cmd(&c,1); }
void ST7567_Invert(bool inv)   { uint8_t c = inv ? 0xA7 : 0xA6; st7567_cmd(&c,1); }
void ST7567_SetContrast(uint8_t val) {
    uint8_t cmds[] = {0x81, val};
    st7567_cmd(cmds, 2);
}
void ST7567_Sleep(bool enable) {
    if (enable) {
        uint8_t c = 0xAC; // power save command sometimes 0xAC/0xAD
        st7567_cmd(&c,1);
    } else {
        uint8_t c = 0xAD;
        st7567_cmd(&c,1);
    }
}

static void st7567_set_pos(uint8_t page, uint8_t col)
{
    uint8_t c1 = 0xB0 | (page & 0x0F);
    uint8_t c2 = 0x00 | (col & 0x0F);
    uint8_t c3 = 0x10 | ((col >> 4) & 0x0F);
    uint8_t arr[3] = {c1, c2, c3};
    st7567_cmd(arr, 3);
}

void ST7567_Clear(void)
{
    memset(fb, 0x00, sizeof(fb));
}

void ST7567_Flush(void)
{
    for (uint8_t page = 0; page < ST7567_PAGES; page++) {
        st7567_set_pos(page, 0);
        st7567_data(fb[page], ST7567_WIDTH);
    }
}

void ST7567_SetPixel(uint16_t x, uint16_t y, bool color)
{
    if (x >= ST7567_WIDTH || y >= ST7567_HEIGHT) return;
    uint16_t page = y / 8;
    uint8_t bit = y % 8;
    if (color)
        fb[page][x] |= (1 << bit);
    else
        fb[page][x] &= ~(1 << bit);
}

bool ST7567_GetPixel(uint16_t x, uint16_t y)
{
    if (x >= ST7567_WIDTH || y >= ST7567_HEIGHT) return false;
    uint16_t page = y / 8;
    uint8_t bit = y % 8;
    return (fb[page][x] >> bit) & 0x1;
}

void ST7567_DrawLine(int x0, int y0, int x1, int y1, bool color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
        ST7567_SetPixel((uint16_t)x0, (uint16_t)y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2*err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void ST7567_DrawRect(int x, int y, int w, int h, bool color)
{
    ST7567_DrawLine(x, y, x+w-1, y, color);
    ST7567_DrawLine(x, y+h-1, x+w-1, y+h-1, color);
    ST7567_DrawLine(x, y, x, y+h-1, color);
    ST7567_DrawLine(x+w-1, y, x+w-1, y+h-1, color);
}

void ST7567_FillRect(int x, int y, int w, int h, bool color)
{
    if (w<=0 || h<=0) return;
    for (int yy = y; yy < y+h; yy++) {
        for (int xx = x; xx < x+w; xx++) {
            ST7567_SetPixel(xx, yy, color);
        }
    }
}

void ST7567_DrawCircle(int x0, int y0, int r, bool color)
{
    int x = r, y = 0;
    int err = 0;
    while (x >= y) {
        ST7567_SetPixel(x0 + x, y0 + y, color);
        ST7567_SetPixel(x0 + y, y0 + x, color);
        ST7567_SetPixel(x0 - y, y0 + x, color);
        ST7567_SetPixel(x0 - x, y0 + y, color);
        ST7567_SetPixel(x0 - x, y0 - y, color);
        ST7567_SetPixel(x0 - y, y0 - x, color);
        ST7567_SetPixel(x0 + y, y0 - x, color);
        ST7567_SetPixel(x0 + x, y0 - y, color);
        y += 1;
        if (err <= 0) {
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

void ST7567_DrawBitmap(const uint8_t *bmp, uint16_t w, uint16_t h, int x, int y)
{
    for (uint16_t j = 0; j < h; j++) {
        for (uint16_t i = 0; i < w; i++) {
            uint32_t idx = (j * (w) + i);
            uint8_t byte = bmp[idx/8];
            uint8_t bit = 7 - (idx % 8);
            bool pixel = (byte >> bit) & 0x1;
            ST7567_SetPixel(x + i, y + j, pixel);
        }
    }
}

void ST7567_ScrollH(int offset)
{
    uint8_t tmp[ST7567_PAGES][ST7567_WIDTH];
    memset(tmp,0,sizeof(tmp));
    for (uint8_t p=0;p<ST7567_PAGES;p++){
        for (uint16_t x=0;x<ST7567_WIDTH;x++){
            int nx = x + offset;
            if (nx>=0 && nx<ST7567_WIDTH) tmp[p][nx] = fb[p][x];
        }
    }
    memcpy(fb,tmp,sizeof(fb));
}

void ST7567_ScrollV(int pages)
{
    uint8_t tmp[ST7567_PAGES][ST7567_WIDTH];
    memset(tmp,0,sizeof(tmp));
    for (int p=0;p<ST7567_PAGES;p++){
        int np = p + pages;
        if (np>=0 && np<ST7567_PAGES){
            memcpy(tmp[np], fb[p], ST7567_WIDTH);
        }
    }
    memcpy(fb,tmp,sizeof(fb));
}

void ST7567_SetCursor(uint8_t col, uint8_t row)
{
    cursor_x = col;
    cursor_y = row;
}

void ST7567_Putc(char c)
{
    if (c < 32 || c > 127) c = '?';
    const uint8_t *ch = font5x7[c - 32];

    for (int i=0;i<5;i++){
        for (int b=0;b<8;b++){
            bool pix = (ch[i] >> b) & 0x1;
            ST7567_SetPixel(cursor_x + i, cursor_y + b, pix);
        }
    }
    for (int b=0;b<8;b++) ST7567_SetPixel(cursor_x + 5, cursor_y + b, 0);
    cursor_x += 6;
    if (cursor_x + 6 >= ST7567_WIDTH) {
        cursor_x = 0;
        cursor_y += 8;
        if (cursor_y >= ST7567_HEIGHT) cursor_y = 0;
    }
}

void ST7567_Puts(const char *s)
{
    while (*s) { ST7567_Putc(*s++); }
}

void ST7567_Printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ST7567_Puts(buf);
}

void ST7567_Init(void)
{
    ST7567_Clear();

    uint8_t init_seq[] = {
        0xAE,       // Display OFF
        0xA2,       // Set LCD bias 1/9 (example) - ST7567 spec
        0xA0,       // SEG direction (A0 or A1) - A0 normal, A1 remap. Zmień jeśli obraz odwrócony
        0xC8,       // COM scan direction (normal/reverse)
        0x2F,       // Power control: Booster, Regulator, Follower ON
        0x26,       // Set VOP contrast reg (example)
        0x81, 0x14, // Electronic volume (contrast) - adjust as needed
        0x40,       // Set start line = 0x40
        0xAF        // Display ON
    };

    st7567_cmd(init_seq, sizeof(init_seq));
    ST7567_SetContrast(0x10);
    ST7567_Flush();
}

void ST7567_SetRotation(bool inverted)
{
    uint8_t cmds[5];
    uint8_t len = 0;
    const uint8_t OFFSET = 4;

    if (inverted) {
        cmds[len++] = 0xA1;
        cmds[len++] = 0xC0;
    } else {
        cmds[len++] = 0xA0;
        cmds[len++] = 0xC8;
    }

    cmds[len++] = 0x40;
    cmds[len++] = 0x00 | (OFFSET & 0x0F);
    cmds[len++] = 0x10;

    st7567_cmd(cmds, len);
}


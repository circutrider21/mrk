#include <generic/fbcon.h>
#include <lib/stivale2.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "font.inc"

volatile uint8_t *framebuffer = 0;
uint16_t width = 0;
uint16_t height = 0;
uint16_t pitch = 0;
static uint32_t cursor_x, cursor_y = 0;

static const uint32_t color_table[16] = {
    TERM_BLACK, TERM_RED, TERM_GREEN, TERM_YELLOW,
    TERM_BLUE, TERM_MAGENTA, TERM_CYAN, TERM_WHITE,
    TERM_GREY, TERM_LIGHT_RED, TERM_LIGHT_GREEN, TERM_LIGHT_YELLOW,
    TERM_LIGHT_BLUE, TERM_LIGHT_MAGENTA, TERM_LIGHT_CYAN, TERM_LIGHT_WHITE
};

static uint32_t fgcolor = TERM_LIGHT_WHITE;
static uint32_t bgcolor = TERM_GREY;

void init_fbcon() {
    struct stivale2_struct_tag_framebuffer* fb_tag = stivale2_query(STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);
    framebuffer = (volatile uint8_t*)fb_tag->framebuffer_addr;

    width = fb_tag->framebuffer_width;
    height = fb_tag->framebuffer_height;
    pitch = fb_tag->framebuffer_pitch;

    fbcon_clear();
}

static void put_px(uint32_t x, uint32_t y, uint32_t color) {
    ((volatile uint32_t*)(framebuffer + (pitch * y)))[x] = color;
}
static void fbcon_setfg(uint32_t color) {
    fgcolor = color;
}
static void fbcon_setbg(uint32_t color) {
    bgcolor = color;
}

void fbcon_clear() {
    for (size_t y = 0; y < height; y++)
        for (size_t x = 0; x < width; x++)
            put_px(x, y, bgcolor);

    cursor_x = 0;
    cursor_y = 0;
}

#define STATE_READY        1
#define STATE_WAIT_COMMAND 2
#define STATE_WAIT_PARAM   3

// Simple ansi escape sequence parser
static bool ansi_parse(uint8_t byte) {
    static int termios[10] = { 0 };
    static int param_count = 0;
    static int state = STATE_READY;

    bool status = false;

    if (state == STATE_READY) {
        if (byte == 0x1B) {
            state = STATE_WAIT_COMMAND;
        } else {
            status = false; goto exit;
        }
    } else if (state == STATE_WAIT_COMMAND) {
        if (byte == '[') {
            param_count = 1;
            termios[0] = 0;
            state = STATE_WAIT_PARAM;
        } else if (byte == 'c') {
            fbcon_clear();
            status = true; goto exit;
        } else {
            status = false; goto exit;
        }
    } else if (state == STATE_WAIT_PARAM) {
        if (byte == ';') {
            termios[param_count++] = 0;
        } else if (byte == 'm') {
            if (termios[0] == 0) {
                fbcon_setfg(TERM_WHITE);
                fbcon_setbg(TERM_GREY);
            } else if (termios[0] >= 30 && termios[0] <= 37) {
                if (param_count == 2 && termios[1] == 1) {
                    fbcon_setfg(color_table[termios[0] - 30 + 8]);
                } else {
                    fbcon_setfg(color_table[termios[0] - 30]);
                }
            } else if (termios[0] >= 40 && termios[0] <= 47) {
                if (param_count == 2 && termios[1] == 1) {
                    fbcon_setbg(color_table[termios[0] - 40 + 8]);
                } else {
                    fbcon_setbg(color_table[termios[0] - 40]);
                }
            }
            status = true; goto exit;
        } else if (byte >= '0' && byte <= '9') {
            termios[param_count - 1] *= 10;
            termios[param_count - 1] += byte - '0';
        } else {
            status = false; goto exit;
        }
    } else {
        status = false; goto exit;
    }

    return true;

exit:
    state = STATE_READY;
    param_count = 0;
    return status;
}

static void putat(unsigned int x, unsigned int y, const char c) {
	uint32_t* fb = (uint32_t*)framebuffer;
	uintptr_t line = (uintptr_t)fb + y * 16 * (uint32_t)(pitch) + x * 8;
	
	for(size_t i = 0; i < 16; i++) {
		uint32_t* dest = (uint32_t*)line;
		uint8_t dc = (c >= 32 && c <= 127) ? c : 127;
		char fontbits = font_bitmap[(dc - 32) * 16 + i];
		
		for(size_t j = 0; j < 8; j++) {
			int bit = (1 << ((8 - 1) - j));
			*dest++ = (fontbits & bit) ? fgcolor : bgcolor;
		}

		line += (pitch);
	}

	asm volatile ("" : : : "memory");
}

void fbcon_put(char c) {
    if(ansi_parse(c))
        return;

    if(c == '\n') {
	cursor_x = 0;
	cursor_y++;
    } else if(cursor_x >= width / 8) {
	cursor_x = 0;
	cursor_y++;
    } else if(cursor_y >= height / 16) {
	fbcon_clear();
    } else {
        putat(cursor_x * 4, cursor_y, c);	
	cursor_x++;
    }
}

void fbcon_puts(char *str) {
    if(framebuffer == NULL)
	return;

    int index = 0;
    while (str[index] != '\0') {
        fbcon_put(str[index++]);
    }
}


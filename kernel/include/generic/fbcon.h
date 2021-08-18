#ifndef GENERIC_FBCON_H
#define GENERIC_FBCON_H

typedef enum {
    TERM_BLACK = 0x000000,
    TERM_RED = 0xaa0000,
    TERM_GREEN = 0x00aa00,
    TERM_YELLOW = 0xaaaa00,
    TERM_BLUE = 0x0000aa,
    TERM_MAGENTA = 0xaa00aa,
    TERM_CYAN = 0x00aaaa,
    TERM_WHITE = 0xaaaaaa,
    TERM_GREY =  0x555555,

    TERM_LIGHT_RED = 0xff5555,
    TERM_LIGHT_GREEN = 0x55ff55,
    TERM_LIGHT_YELLOW = 0xffff55,
    TERM_LIGHT_BLUE = 0x5555ff,
    TERM_LIGHT_MAGENTA = 0xff55ff,
    TERM_LIGHT_CYAN = 0x55ffff,
    TERM_LIGHT_WHITE = 0xffffff
} termcolor;

void init_fbcon();
void fbcon_clear();
void fbcon_putc(char c);
void fbcon_puts(char* str);

#endif // GENERIC_FBCON_H


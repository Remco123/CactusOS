#ifndef CURSOR_H
#define CURSOR_H

#include <types.h>

#define ALPHA 0
#define BLACK 1
#define WHITE 2

#define CURSOR_W 12
#define CURSOR_H 19

LIBCactusOS::uint8_t cursorBitmap[CURSOR_W * CURSOR_H] = 
{
    BLACK,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,
    BLACK,BLACK,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,
    BLACK,WHITE,BLACK,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,
    BLACK,WHITE,WHITE,BLACK,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,
    BLACK,WHITE,WHITE,WHITE,BLACK,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,
    BLACK,WHITE,WHITE,WHITE,WHITE,BLACK,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,
    BLACK,WHITE,WHITE,WHITE,WHITE,WHITE,BLACK,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,
    BLACK,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,BLACK,ALPHA,ALPHA,ALPHA,ALPHA,
    BLACK,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,BLACK,ALPHA,ALPHA,ALPHA,
    BLACK,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,BLACK,ALPHA,ALPHA,
    BLACK,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,BLACK,ALPHA,
    BLACK,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,BLACK,
    BLACK,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,BLACK,BLACK,BLACK,BLACK,BLACK,
    BLACK,WHITE,WHITE,WHITE,BLACK,WHITE,WHITE,BLACK,ALPHA,ALPHA,ALPHA,ALPHA,
    BLACK,WHITE,WHITE,BLACK,ALPHA,BLACK,WHITE,WHITE,BLACK,ALPHA,ALPHA,ALPHA,
    BLACK,WHITE,BLACK,ALPHA,ALPHA,BLACK,WHITE,WHITE,BLACK,ALPHA,ALPHA,ALPHA,
    BLACK,BLACK,ALPHA,ALPHA,ALPHA,ALPHA,BLACK,WHITE,WHITE,BLACK,ALPHA,ALPHA,
    ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,BLACK,WHITE,WHITE,BLACK,ALPHA,ALPHA,
    ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,ALPHA,BLACK,BLACK,ALPHA,ALPHA,ALPHA
};

#endif
#include <cbm.h>
#include <conio.h>
#include <stdlib.h>
#include "dungeon.h"

#include "../mega65-libc/cc65/src/memory.c"
#include "../mega65-libc/cc65/src/conio.c"

#ifdef __CX16__
const int xDungeonSize = 80;
const int yDungeonSize = 58;
const int minDungeonRoomCount = 10;
const int minRoomSize = 2;
#else
const int xDungeonSize = 40;
const int yDungeonSize = 24;
const int minDungeonRoomCount = 0;
const int minRoomSize = 2;
#endif

const char signs[] = {' ', '.', '#', 'X', '*'};

void dumpDungeon(dungeonDescriptor *desc)
{
    byte *canvas;
    byte width;
    byte height;
    register byte x, y;

    canvas = desc->canvas;
    width = desc->width;
    height = desc->height;

    clrscr();

    for (x = 0; x < width; ++x)
    {
        for (y = 0; y < height; ++y)
        {
            cputcxy(x, y, canvas[x + (width * y)]);
        }
    }
}

unsigned int debugMem(void)
{
    unsigned int t;
    int *m;
    m = malloc(1024);
    t = (unsigned int)m;
    free(m);
    return t;
}

void main()
{
    dungeonDescriptor *aDungeon;
    mega65_io_enable();
    POKE (VIC_BASE + 0x60,0x00);
    POKE (VIC_BASE + 0x61,0xf0);
    POKE (VIC_BASE + 0x62,0x04);
    POKE (VIC_BASE + 0x63,0x00);
    srand(439);
    do
    {
        aDungeon = createDungeon(xDungeonSize,
                                 yDungeonSize,
                                 minDungeonRoomCount,
                                 minRoomSize);
        dumpDungeon(aDungeon);
        gotoxy(0, 0);
        cputs("press any key for new dungeon");
        cgetc();

    } while (1);
}

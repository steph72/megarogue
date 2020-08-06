#include <cbm.h>
#include <conio.h>
#include <stdlib.h>
#include "dungeon.h"

#include "../mega65-libc/cc65/src/memory.c"
#include "../mega65-libc/cc65/src/conio.c"

const int xDungeonSize = 80;
const int yDungeonSize = 24;
const int minDungeonRoomCount = 0;
const int minRoomSize = 2;

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
    SET_H640();
    setscreenaddr(0x48000UL);
    conioinit();
    textcolor(5);
    bordercolor(0);
    bgcolor(0);
    srand(439);
    clrscr();

    do
    {
        aDungeon = createDungeon(xDungeonSize,
                                 yDungeonSize,
                                 minDungeonRoomCount,
                                 minRoomSize);
        dumpDungeon(aDungeon);
        gotoxy(0, 24);
        cputs("press any key for new dungeon");
        cgetc();
        deallocDungeon(aDungeon);

    } while (1);
}

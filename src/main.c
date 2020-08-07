#include <cbm.h>
#include <conio.h>
#include <stdlib.h>
#include "dungeon.h"
#include <c64.h>

#include "../mega65-libc/cc65/src/memory.c"
#include "../mega65-libc/cc65/src/conio.c"

const int xDungeonSize = 80;
const int yDungeonSize = 24;
const int minDungeonRoomCount = 0;
const int minRoomSize = 2;

dungeonDescriptor *gDungeon;

typedef struct
{
    char sign;
    byte color;
} dungeonCharacter;

const dungeonCharacter kDungeonSigns[] = {
    {' ', COLOR_CYAN},
    {'.', COLOR_GRAY1},
    {'+', COLOR_BROWN},
    {'/', COLOR_BROWN},
    {'#', COLOR_GRAY2},
    {'<', COLOR_LIGHTBLUE},
    {'>', COLOR_LIGHTBLUE},
    {0, COLOR_YELLOW}};

void displayDungeonAt(byte x, byte y)
{
    dungeonCharacter c;
    c = kDungeonSigns[displayItemForPos(x, y)];
    textcolor(c.color);
    cputcxy(x, 1 + y, c.sign);
}

byte isItemBlockingView(dungeonItem *anItem)
{
    return (anItem->itemID == kDungeonItemClosedDoor || anItem->itemID == kDungeonItemWall);
}

// quick and dirty fov via bresenham's line drawing algorithm
void look_bh(int x0, int y0, int x1, int y1)
{

    int dx, dy;
    int x, y;
    int n;
    int x_inc, y_inc;
    int error;

    dungeonItem anItem;
    byte blocksView;

    dx = abs(x1 - x0);
    dy = abs(y1 - y0);
    x_inc = (x1 > x0) ? 1 : -1;
    y_inc = (y1 > y0) ? 1 : -1;

    x = x0;
    y = y0;

    do
    {
        getDungeonItem(x, y, &anItem);
        blocksView = isItemBlockingView(&anItem);
        displayDungeonAt(x, y);
        x += x_inc;
        y += y_inc;

    } while (x != x1 && y != y1 && !blocksView);

    x = x0;
    y = y0;
    n = 1 + dx + dy;
    error = dx - dy;

    dx *= 2;
    dy *= 2;

    for (; n > 0; --n)
    {

        if (x < 0 || y < 0 || x > _gDungeonWidth - 1 ||
            y > _gDungeonHeight - 1)
        {
            continue;
        }

        getDungeonItem(x, y, &anItem);

        blocksView = isItemBlockingView(&anItem);

        displayDungeonAt(x, y);

        if (blocksView)
        {
            return;
        }

        if (error > 0)
        {
            x += x_inc;
            error -= dy;
        }
        else
        {
            y += y_inc;
            error += dx;
        }
    }
}

#define L_DISTANCE 5

// perform bresenham for 4 edges
void look(int x, int y)
{
    signed char xi1, xi2, yi1, yi2;

    yi1 = y - L_DISTANCE;
    if (yi1 < 0)
        yi1 = 0;

    yi2 = y + L_DISTANCE;
    if (yi2 > _gDungeonHeight - 1)
        yi2 = _gDungeonHeight - 1;

    for (xi1 = x - L_DISTANCE; xi1 <= x + L_DISTANCE; ++xi1)
    {
        look_bh(x, y, xi1, yi1);
        look_bh(x, y, xi1, yi2);
    }

    xi1 = x - L_DISTANCE;
    if (xi1 < 0)
        xi1 = 0;

    xi2 = x + L_DISTANCE;
    if (xi2 > _gDungeonWidth - 1)
        xi2 = _gDungeonWidth - 1;

    for (yi1 = y - L_DISTANCE; yi1 <= y + L_DISTANCE; ++yi1)
    {
        look_bh(x, y, xi1, yi1);
        look_bh(x, y, xi2, yi1);
    }
}

void dumpDungeon(dungeonDescriptor *desc)
{
    byte width;
    byte height;
    register byte x, y;

    width = desc->width;
    height = desc->height;

    clrscr();
    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            displayDungeonAt(x, y);
        }
    }
}

void descendIfPossible()
{
    dungeonItem anItem;
    getDungeonItem(gPlayerCoords.x, gPlayerCoords.y, &anItem);
    if (anItem.itemID == kDungeonItemStairDown)
    {
        deallocDungeon(gDungeon);

        gDungeon = createDungeonLevel(xDungeonSize,
                                      yDungeonSize,
                                      minDungeonRoomCount,
                                      minRoomSize);
                            
        clrscr();
    }
}

void displayStatus(char *s)
{
    cputncxy(0, 0, 80, ' ');
    cputsxy(0, 0, s);
}

#define kExtendedCommandOpen 1
#define kExtendedCommandClose 2

void testDungeon(dungeonDescriptor *aDungeon)
{
    byte cmd;
    byte extendedCommand;
    byte phase;

    signed char deltax, deltay;
    clrscr();
    phase = 0;

    displayStatus("j,k,l,i = move   o = open   c = close   d = descend");

    do
    {
        look(gPlayerCoords.x, gPlayerCoords.y);

        cmd = cgetc();
        deltax = 0;
        deltay = 0;


        switch (cmd)
        {

        case 99: // "c"
            extendedCommand = kExtendedCommandClose;
            displayStatus("close - which direction?");
            phase = 2;
            break;

        case 100: // "d"
            descendIfPossible();
            break;

        case 111: // "o"
            extendedCommand = kExtendedCommandOpen;
            displayStatus("open - which direction?");
            phase = 2;
            break;

        case 105: // "i"
            deltay = -1;
            break;

        case 107: // "k"
            deltay = 1;
            break;

        case 106: // "j"
            deltax = -1;
            break;

        case 108: // "l"
            deltax = 1;
            break;

        default:
            break;
        }

        if (!phase)
        {
            movePlayerDelta(deltax, deltay);
        }
        else
        {
            phase--;
            if (!phase)
            {
                switch (extendedCommand)
                {
                case kExtendedCommandOpen:

                    if (handleDoorDelta(deltax, deltay, 1))
                    {
                        displayStatus("you open the door.");
                    }
                    else
                    {
                        displayStatus("nothing happens");
                    }
                    break;

                case kExtendedCommandClose:
                    if (handleDoorDelta(deltax, deltay, 0))
                    {
                        displayStatus("you close the door");
                    }
                    else
                    {
                        displayStatus("nothing happens");
                    }
                    break;

                default:
                    break;
                }
            }
        }

    } while (cmd != 'q');
    return;
}

void main()
{
    mega65_io_enable();
    setscreenaddr(0x4f000UL);
    conioinit();
    setscreensize(80, 25);
    textcolor(5);
    bordercolor(0);
    bgcolor(0);
    
    srand(CIA1.ta_lo + CIA2.ta_lo);
    clrscr();

    do
    {
        gohome();
        cputs("creating...");
        gDungeon = createDungeonLevel(xDungeonSize,
                                      yDungeonSize,
                                      minDungeonRoomCount,
                                      minRoomSize);
        // dumpDungeon(aDungeon);
        testDungeon(gDungeon);
        cputsxy(0, 0, "press any key for new dungeon");
        cgetc();
        deallocDungeon(gDungeon);
    } while (1);
}

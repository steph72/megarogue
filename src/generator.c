#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <conio.h>
#include <stdio.h>
#include <memory.h>

#include "dungeon.h"

#define DDEBUG

// maximum tries to lay out a dungeon before starting over
#define MAXTRIES 64 

// divisor for calculating the minimum room count
// higher values mean fewer rooms per dungeon space
// 120 == 16 rooms in 80x24 dungeon / 8 rooms in 40x24  
// 160 == 12   "                    / 6        "
#define MIN_ROOM_COUNT_DIVISOR 120   

const dungeonItemID kDungeonItemTempWallLeft = 0xf0;
const dungeonItemID kDungeonItemTempWallRight = 0xf1;
const dungeonItemID kDungeonItemTempWallTop = 0xf2;
const dungeonItemID kDungeonItemTempWallBottom = 0xf3;
const dungeonItemID kDungeonItemTempHallway = 0xf4;

const dungeonItemID _doorIDs[] = {3, 2, 1};
// cc65 bug? can't put constants kDungeonItemOpenDoor, etc. here...

int _gMinRooms;
int _gMinRoomSize;

typedef enum _dir
{
    dir_horizontal,
    dir_vertical
} direction;

typedef struct _rect
{
    byte x0, y0, x1, y1;
} rect;

typedef struct _frame
{
    rect frameRect;
    rect roomRect;
    direction splitDir;
    byte isConnectedToSibling;
    struct _frame *parent;
    struct _frame *subframe1;
    struct _frame *subframe2;
} frame;

/* -------------------------------- code ---------------------------------- */

void putCanvas(byte x, byte y, byte elem)
{
    dungeonItem anItem;
    anItem.itemID = elem;
    anItem.enclosedItem = NULL;
    putDungeonItem(x, y, &anItem);
}

byte getCanvas(byte x, byte y)
{
    dungeonItem anItem;
    getDungeonItem(x,y,&anItem);
    return (anItem.itemID);
}

byte midX(rect *aRect)
{
    return aRect->x0 + ((aRect->x1 - aRect->x0) / 2);
}

byte midY(rect *aRect)
{
    return aRect->y0 + ((aRect->y1 - aRect->y0) / 2);
}

byte isPointInRect(byte x, byte y, rect *aRect)
{
    return (x >= aRect->x0 && x <= aRect->x1 && y >= aRect->y0 && y <= aRect->y1);
}

frame *newFrame(byte x0, byte y0, byte x1, byte y1)
{
    frame *aFrame;
    aFrame = (frame *)malloc(sizeof(frame));
    aFrame->frameRect.x0 = x0;
    aFrame->frameRect.y0 = y0;
    aFrame->frameRect.x1 = x1;
    aFrame->frameRect.y1 = y1;
    aFrame->isConnectedToSibling = 0;
    aFrame->subframe1 = NULL;
    aFrame->subframe2 = NULL;
    aFrame->parent = NULL;
    return aFrame;
}

byte isLeaf(frame *aFrame)
{
    return (aFrame->subframe1 == NULL && aFrame->subframe2 == NULL);
}

void splitFrame(frame *aFrame, byte minFrameSize)
{
    direction splitDir;
    byte splitPoint;
    byte frameWidth;
    byte frameHeight;
    int frameSizeThreshhold;
    frame *subframe1 = NULL;
    frame *subframe2 = NULL;

    frameSizeThreshhold = (minFrameSize * 35) / 10;

    frameWidth = aFrame->frameRect.x1 - aFrame->frameRect.x0;
    frameHeight = aFrame->frameRect.y1 - aFrame->frameRect.y0;

    splitDir = (rand() & 1) ? dir_horizontal : dir_vertical;
    aFrame->splitDir = splitDir;

    if ((splitDir == dir_horizontal) && frameWidth >= frameSizeThreshhold)
    {
        splitPoint = aFrame->frameRect.x0 + minFrameSize + (rand() % (frameWidth - (minFrameSize * 2)));
        subframe1 = newFrame(aFrame->frameRect.x0, aFrame->frameRect.y0, splitPoint, aFrame->frameRect.y1);
        subframe2 = newFrame(splitPoint, aFrame->frameRect.y0, aFrame->frameRect.x1, aFrame->frameRect.y1);
    }
    else if ((splitDir == dir_vertical) && frameHeight >= frameSizeThreshhold)
    {
        splitPoint = aFrame->frameRect.y0 + minFrameSize + (rand() % (frameHeight - (minFrameSize * 2)));

        subframe1 = newFrame(aFrame->frameRect.x0, aFrame->frameRect.y0, aFrame->frameRect.x1, splitPoint);
        subframe2 = newFrame(aFrame->frameRect.x0, splitPoint, aFrame->frameRect.x1, aFrame->frameRect.y1);
    }

    if (subframe1)
    {
        subframe1->parent = aFrame;
        subframe2->parent = aFrame;
        aFrame->subframe1 = subframe1;
        aFrame->subframe2 = subframe2;
        splitFrame(subframe1, minFrameSize);
        splitFrame(subframe2, minFrameSize);
    }
}

void deallocFrames(frame *aFrame)
{
    if (!aFrame)
    {
        return;
    }
    deallocFrames(aFrame->subframe1);
    deallocFrames(aFrame->subframe2);
    free(aFrame);
}

byte _countLeafFrames(frame *startFrame, byte currentCount)
{

    if (isLeaf(startFrame))
    {
        return currentCount + 1;
    }
    else
    {
        return _countLeafFrames(startFrame->subframe1, currentCount) +
               _countLeafFrames(startFrame->subframe2, currentCount);
    }
}

byte countLeafFrames(frame *startFrame)
{
    return _countLeafFrames(startFrame, 0);
}

frame *createFrames(byte width, byte height, byte minFrameCount, byte minFrameSize)
{
    frame *startFrame = NULL;
    do
    {
        cputc('.');
        deallocFrames(startFrame);
        startFrame = newFrame(0, 0, width - 1, height - 1);
        splitFrame(startFrame, minFrameSize);
    } while (countLeafFrames(startFrame) < minFrameCount);
    return startFrame;
}

void createRoomForFrame(frame *aFrame)
{
    byte roomWidth;
    byte roomHeight;
    byte randomSize;
    byte x0, y0, x1, y1;

    x0 = aFrame->frameRect.x0;
    y0 = aFrame->frameRect.y0;
    x1 = aFrame->frameRect.x1;
    y1 = aFrame->frameRect.y1;

    x0 = (x0 == 0) ? x0 : x0 + 1;
    y0 = (y0 == 0) ? y0 : y0 + 1;
    x1 = (x1 == _gDungeonWidth - 1) ? x1 : x1 - 1;
    y1 = (y1 == _gDungeonHeight - 1) ? y1 : y1 - 1;

    roomWidth = x1 - x0 - 2;
    roomHeight = y1 - y0 - 2;

    if (roomWidth > (_gMinRoomSize + 2))
    {
        randomSize = rand() % (roomWidth - _gMinRoomSize);
        x0 += randomSize / 2;
        x1 -= randomSize / 2;
    }

    if (roomHeight > (_gMinRoomSize + 2))
    {
        randomSize = rand() % (roomHeight - _gMinRoomSize);
        y0 += randomSize / 2;
        y1 -= randomSize / 2;
    }

    aFrame->roomRect.x0 = x0;
    aFrame->roomRect.x1 = x1;
    aFrame->roomRect.y0 = y0;
    aFrame->roomRect.y1 = y1;

    roomWidth = x1 - x0 - 2;
    roomHeight = y1 - y0 - 2;
}

void instantiateRoomInDungeon(frame *aFrame, int *currentRoomIdx, room *roomList)
{
    register byte x;
    register byte y;
    byte x0, x1, y0, y1;

    x0 = aFrame->roomRect.x0;
    y0 = aFrame->roomRect.y0;
    x1 = aFrame->roomRect.x1;
    y1 = aFrame->roomRect.y1;

    roomList[*currentRoomIdx].x0 = x0;
    roomList[*currentRoomIdx].x1 = x1;
    roomList[*currentRoomIdx].y0 = y0;
    roomList[*currentRoomIdx].y1 = y1;

    *currentRoomIdx += 1;

    for (x = x0; x <= x1; ++x)
    {
        for (y = y0; y <= y1; ++y)
        {

            if (x == x0)
            {
                putCanvas(x, y, kDungeonItemTempWallLeft);
            }
            else if (x == x1)
            {
                putCanvas(x, y, kDungeonItemTempWallRight);
            }
            else if (y == y0)
            {
                putCanvas(x, y, kDungeonItemTempWallTop);
            }
            else if (y == y1)
            {
                putCanvas(x, y, kDungeonItemTempWallBottom);
            }
            else
            {
                putCanvas(x, y, kDungeonItemFloor);
            }
        }
    }
}

void instantiateRooms(frame *startFrame, int *currentRoomIdx, room *roomList)
{
    if (!startFrame)
    {
        return;
    }

    if (isLeaf(startFrame))
    {
        instantiateRoomInDungeon(startFrame, currentRoomIdx, roomList);
    }
    else
    {
        instantiateRooms(startFrame->subframe1, currentRoomIdx, roomList);
        instantiateRooms(startFrame->subframe2, currentRoomIdx, roomList);
    }
}

void createRooms(frame *startFrame)
{
    if (isLeaf(startFrame))
    {
        createRoomForFrame(startFrame);
    }
    else
    {
        createRooms(startFrame->subframe1);
        createRooms(startFrame->subframe2);
    }
}

void connectRects(rect *rect1, rect *rect2)
{

    byte xc1, yc1, xc2, yc2;
    byte x, y;
    byte floorElement;

    int xdiff, ydiff;
    int xstep, ystep;

    floorElement = kDungeonItemTempHallway;

    xc1 = midX(rect1);
    yc1 = midY(rect1);
    xc2 = midX(rect2);
    yc2 = midY(rect2);

    if (xc2 > xc1)
    {
        xstep = 1;
        xdiff = xc2 - xc1;
    }
    else if (xc1 > xc2)
    {
        xstep = -1;
        xdiff = xc1 - xc2;
    }
    else
    {
        xstep = 0;
        xdiff = 0;
    }

    if (yc2 > yc1)
    {
        ystep = 1;
        ydiff = yc2 - yc1;
    }
    else if (yc1 > yc2)
    {
        ystep = -1;
        ydiff = yc1 - yc2;
    }
    else
    {
        ystep = 0;
        ydiff = 0;
    }

    x = xc1;
    y = yc1;

    if (ydiff < xdiff)
    {
        for (x = xc1; x != xc2; x += xstep)
        {
            if (isPointInRect(x, y, rect1))
            {
                putCanvas(x, y, floorElement);
            }
            else
            {
                while (y != yc2)
                {
                    putCanvas(x, y, floorElement);
                    y += ystep;
                }

                putCanvas(x, y, floorElement);
            }
        }
    }

    if (xdiff < ydiff)
    {
        for (y = yc1; y != yc2; y += ystep)
        {
            if (isPointInRect(x, y, rect1))
            {

                putCanvas(x, y, floorElement);
            }
            else
            {
                while (x != xc2)
                {
                    putCanvas(x, y, floorElement);
                    x += xstep;
                }
                putCanvas(x, y, floorElement);
            }
        }
    }
}

void connectRoomToSibling(frame *startFrame)
{

    rect *rect1 = NULL;
    rect *rect2 = NULL;

    frame *parent = NULL;
    frame *otherFrame = NULL;

    if (startFrame->isConnectedToSibling)
    {
        return;
    }

    rect1 = &(startFrame->roomRect);
    parent = startFrame->parent;

    if (parent->subframe1 == startFrame)
    {
        otherFrame = parent->subframe2;
    }
    else
    {
        otherFrame = parent->subframe1;
    }

    if (isLeaf(otherFrame))
    {
        rect2 = &(otherFrame->roomRect);
    }
    else
    {
        rect2 = &(otherFrame->frameRect);
    }

    connectRects(rect1, rect2);

    startFrame->isConnectedToSibling = true;
    otherFrame->isConnectedToSibling = true;
}

void createHallwaysBetweenRooms(frame *startFrame)
{

    if (isLeaf(startFrame))
    {
        connectRoomToSibling(startFrame);
    }
    else
    {
        createHallwaysBetweenRooms(startFrame->subframe1);
        createHallwaysBetweenRooms(startFrame->subframe2);
    }
}

void createOtherHallways(frame *startFrame)
{

    if (isLeaf(startFrame))
    {
        return;
    }

    if (!(startFrame->subframe1->isConnectedToSibling))
    {
        connectRects(&startFrame->subframe1->frameRect, &startFrame->subframe2->frameRect);
        startFrame->subframe1->isConnectedToSibling = true;
        startFrame->subframe2->isConnectedToSibling = true;
    }

    createOtherHallways(startFrame->subframe1);
    createOtherHallways(startFrame->subframe2);
}

void createHallways(frame *startFrame)
{
    createHallwaysBetweenRooms(startFrame);
    createOtherHallways(startFrame);
}

char checkIsWallOfRoomAt(byte x, byte y)
{
    byte elem;
    elem = getCanvas(x, y);

    return (elem >= kDungeonItemTempWallLeft && elem <= kDungeonItemTempWallBottom) ? elem : 0;
}

dungeonItemID randomDoorItem(void)
{
    return _doorIDs[rand() % 3];
}

void postprocessDungeon(void)
{
    register byte x, y;
    signed char xd, yd;

    for (x = 1; x < _gDungeonWidth - 1; ++x)
    {
        for (y = 1; y < _gDungeonHeight - 1; ++y)
        {
            if (getCanvas(x, y) == kDungeonItemTempHallway)
            {

                if (checkIsWallOfRoomAt(x - 1, y) && (checkIsWallOfRoomAt(x - 1, y) == checkIsWallOfRoomAt(x + 1, y)))
                {
                    putCanvas(x, y, randomDoorItem());
                }
                else if (checkIsWallOfRoomAt(x, y - 1) && (checkIsWallOfRoomAt(x, y - 1) == checkIsWallOfRoomAt(x, y + 1)))
                {
                    putCanvas(x, y, randomDoorItem());
                }
                else
                {

                    for (xd = -1; xd <= 1; ++xd)
                    {
                        for (yd = -1; yd <= 1; ++yd)
                        {
                            if (getCanvas(x + xd, y + yd) == kDungeonItemNothing)
                            {
                                putCanvas(x + xd, y + yd, kDungeonItemWall);
                            }
                        }
                    }
                    putCanvas(x, y, kDungeonItemFloor);
                }
            }
        }
    }
    for (x = 0; x < _gDungeonWidth; ++x)
    {
        for (y = 0; y < _gDungeonHeight; ++y)
        {
            if (checkIsWallOfRoomAt(x, y))
            {
                putCanvas(x, y, kDungeonItemWall);
            }
        }
    }
}

dungeonDescriptor *createDungeon(byte width,
                                 byte height,
                                 byte minRoomCount,
                                 byte minRoomSize)
{

    int currentRoomIdx = 0;
    frame *startFrame = NULL;

    // create dungeon structure and canvas array
    dungeonDescriptor *ddesc = (dungeonDescriptor *)malloc(sizeof(dungeonDescriptor));

    if (minRoomSize < 1)
    {
        minRoomSize = 2;
    }

    if (!minRoomCount)
    {
        minRoomCount = (width * height) / MIN_ROOM_COUNT_DIVISOR;
        // minRoomCount /= (minRoomSize-1);
#ifdef DDEBUG
        gohome();
        cputs("calculated min room count: ");
        cputdec(minRoomCount,0,0);
        cprintf("\n");
#endif
    }

    ddesc->width = width;
    ddesc->height = height;
    ddesc->canvas = DUNGEON_BASE_ADDRESS;

    _gDungeonHeight = height;
    _gDungeonWidth = width;
    _gMinRooms = minRoomCount;
    _gMinRoomSize = minRoomSize;
    _gCanvas = DUNGEON_BASE_ADDRESS;

    lfill(_gCanvas, kDungeonItemNothing, width * height * sizeof(dungeonItem));

    /*
            min size for frame is
              minRoomSize
              + 2 spaces for walls
              + 1 space for room between walls
        */

#ifdef DDEBUG
    cprintf("pass 1.1 create frames\n");
#endif
    deallocFrames(startFrame);
    startFrame = createFrames(width,
                              height,
                              minRoomCount,
                              minRoomSize + 3);
#ifdef DDEBUG
    cprintf("pass 1.2: create rooms\n");
#endif
    createRooms(startFrame);

    ddesc->numRooms = countLeafFrames(startFrame);
    ddesc->roomList = malloc(sizeof(room) * ddesc->numRooms);

#ifdef DDEBUG
    cprintf("pass 2: instantiating rooms\n");
#endif
    instantiateRooms(startFrame, &currentRoomIdx, ddesc->roomList);

#ifdef DDEBUG
    cprintf("pass 3: creating hallways\n");

#endif
    createHallways(startFrame);

#ifdef DDEBUG
    cprintf("pass 4: postprocess dungeon\n");
#endif
    postprocessDungeon();
    deallocFrames(startFrame);

#ifdef DDEBUG
    cprintf("pass 5: generate stairs\n");
#endif
    newDungeonItemAtRandomPos(ddesc, kDungeonItemStairUp, &ddesc->stairUp);
    newDungeonItemAtRandomPos(ddesc, kDungeonItemStairDown, &ddesc->stairDown);
    return ddesc;
}

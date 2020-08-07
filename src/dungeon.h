#include <stdint.h>

#ifndef __DUNGEON__
#define __DUNGEON__

extern const int kErrNoRoomForDungeon;

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#define DUNGEON_BASE_ADDRESS 0x40000U

typedef uint8_t byte;
typedef uint16_t dungeonItemID;
typedef uint32_t exPtr;

typedef struct
{
    byte x, y;
} coords;

typedef struct
{
    byte x0, y0, x1, y1;
} room;

typedef struct
{
    byte width;
    byte height;
    int numRooms;
    room *roomList;
    coords stairUp;
    coords stairDown;
    exPtr canvas;
} dungeonDescriptor;

typedef struct _dungeonItem
{
    dungeonItemID itemID;
    struct _dungeonItem *enclosedItem;
} dungeonItem;

extern const dungeonItemID kDungeonItemNothing;
extern const dungeonItemID kDungeonItemFloor;
extern const dungeonItemID kDungeonItemClosedDoor;
extern const dungeonItemID kDungeonItemOpenDoor;
extern const dungeonItemID kDungeonItemWall;
extern const dungeonItemID kDungeonItemStairUp;
extern const dungeonItemID kDungeonItemStairDown;

extern int _gDungeonWidth;
extern int _gDungeonHeight;
extern long _gCanvas;

extern coords gPlayerCoords;
extern dungeonItem gPlayerItem;

void getDungeonItem(byte x, byte y, dungeonItem *anItem);
void putDungeonItem(byte x, byte y, dungeonItem *anItem);

dungeonItemID displayItemForPos(byte x, byte y);

void newDungeonItemAtRandomPos(dungeonDescriptor *desc, dungeonItemID kind, coords *position);

dungeonDescriptor *createDungeonLevel(byte width,
                                      byte height,
                                      byte minRoomCount,
                                      byte minRoomSize);

byte movePlayerDelta(signed char xdiff, signed char ydiff);
byte handleDoorDelta(signed char xdiff, signed char ydiff, byte openClose);

void deallocDungeon(dungeonDescriptor *desc);

#endif
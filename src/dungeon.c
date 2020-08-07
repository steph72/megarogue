#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <conio.h>
#include <stdio.h>
#include <memory.h>

#include "dungeon.h"
#include "generator.h"

#define DDEBUG

const dungeonItemID kDungeonItemNothing = 0;
const dungeonItemID kDungeonItemFloor = 1;
const dungeonItemID kDungeonItemClosedDoor = 2;
const dungeonItemID kDungeonItemOpenDoor = 3;
const dungeonItemID kDungeonItemWall = 4;
const dungeonItemID kDungeonItemStairUp = 5;
const dungeonItemID kDungeonItemStairDown = 6;
const dungeonItemID kDungeonItemPlayer = 7;

coords gPlayerCoords;
dungeonItem gPlayerItem;

int _gDungeonWidth;
int _gDungeonHeight;
long _gCanvas;

void getDungeonItem(byte x, byte y, dungeonItem *anItem)
{

    lcopy(_gCanvas + (sizeof(dungeonItem) * (x + (y * _gDungeonWidth))),
          (long)anItem, sizeof(dungeonItem));
}

void putDungeonItem(byte x, byte y, dungeonItem *anItem)
{
    long destadr;
    destadr = _gCanvas + (sizeof(dungeonItem) * (x + (y * _gDungeonWidth)));
    lcopy((long)anItem, _gCanvas + (sizeof(dungeonItem) * (x + (y * _gDungeonWidth))),
          sizeof(dungeonItem));
}

dungeonItem *getTopmostItemForPos(byte x, byte y)
{
    dungeonItem anItem;
    dungeonItem *retItemPtr;
    getDungeonItem(x, y, &anItem);
    retItemPtr = &anItem;
    while (retItemPtr->enclosedItem != NULL)
    {
        retItemPtr = retItemPtr->enclosedItem;
    }
    return retItemPtr;
}

void addItemOnTop(byte x, byte y, dungeonItem *item)
{
    dungeonItem anItem;
    dungeonItem *itemPtr;
    // get dungeon item at position
    getDungeonItem(x, y, &anItem);
    // is it empty? add new item directly...
    if (anItem.enclosedItem == NULL)
    {
        anItem.enclosedItem = item;
        putDungeonItem(x, y, &anItem);
        return;
    }
    else
    {
        // not empty? traverse up and add
        itemPtr = &anItem;
        do
        {
            itemPtr = itemPtr->enclosedItem;
        } while (itemPtr->enclosedItem != NULL);
        itemPtr->enclosedItem = item;
    }
}

byte canMovePlayerTo(byte x, byte y)
{
    dungeonItem *anItem;
    anItem = getTopmostItemForPos(x, y);
    if (anItem->itemID == kDungeonItemWall || anItem->itemID == kDungeonItemClosedDoor)
    {
        return false;
    }
    return true;
}

void removePlayerFromCurrentPosition()
{
    dungeonItem anItem;
    getDungeonItem(gPlayerCoords.x, gPlayerCoords.y, &anItem);
    if (anItem.enclosedItem == &gPlayerItem)
    {
        anItem.enclosedItem = NULL;
        putDungeonItem(gPlayerCoords.x, gPlayerCoords.y, &anItem);
        return;
    }
    do
    {
        anItem = *anItem.enclosedItem;
    } while (anItem.enclosedItem != &gPlayerItem && anItem.enclosedItem != NULL);
    if (anItem.enclosedItem == NULL)
    {
        cputs("?? could not find player item!");
        exit(0);
    }
    anItem.enclosedItem = NULL;
}

byte movePlayer(byte x, byte y)
{
    if (!canMovePlayerTo(x, y))
    {
        return false;
    }
    removePlayerFromCurrentPosition();
    addItemOnTop(x, y, &gPlayerItem);
    gPlayerCoords.x = x;
    gPlayerCoords.y = y;
}

byte movePlayerDelta(signed char xdiff, signed char ydiff)
{
    return movePlayer(gPlayerCoords.x + xdiff, gPlayerCoords.y + ydiff);
}

byte handleDoorDelta(signed char xdiff, signed char ydiff, byte openClose)
{
    coords doorCoords;
    dungeonItem anItem;
    dungeonItemID expectedItem;
    dungeonItemID destItem;

    expectedItem = openClose ? kDungeonItemClosedDoor : kDungeonItemOpenDoor;
    destItem = openClose ? kDungeonItemOpenDoor : kDungeonItemClosedDoor;

    doorCoords.x = gPlayerCoords.x + xdiff;
    doorCoords.y = gPlayerCoords.y + ydiff;
    getDungeonItem(doorCoords.x, doorCoords.y, &anItem);
    if (anItem.itemID == expectedItem)
    {
        anItem.itemID = destItem;
        putDungeonItem(doorCoords.x, doorCoords.y, &anItem);
        return true;
    }
    return false;
}

dungeonItemID displayItemForPos(byte x, byte y)
{
    return (getTopmostItemForPos(x, y)->itemID);
}

void deallocDungeon(dungeonDescriptor *desc)
{
    free(desc->roomList);
    free(desc);
}

void newDungeonItemAtRandomPos(dungeonDescriptor *desc, dungeonItemID kind, coords *position)
{
    byte roomIdx, numRooms, x, y;
    room *roomList;
    dungeonItem anItem;

    roomList = desc->roomList;
    numRooms = desc->numRooms;

    roomIdx = rand() % numRooms;
    do
    {
        x = roomList[roomIdx].x0 + rand() % (roomList[roomIdx].x1 - roomList[roomIdx].x0);
        y = roomList[roomIdx].y0 + rand() % (roomList[roomIdx].y1 - roomList[roomIdx].y0);
        getDungeonItem(x, y, &anItem);

    } while (anItem.itemID != kDungeonItemFloor);
    anItem.itemID = kind;

    putDungeonItem(x, y, &anItem);
    position->x = x;
    position->y = y;
}

dungeonDescriptor *createDungeonLevel(byte width,
                                      byte height,
                                      byte minRoomCount,
                                      byte minRoomSize)
{
    dungeonDescriptor *desc;

    // todo: move player initialization elsewhere
    gPlayerItem.itemID = kDungeonItemPlayer;
    gPlayerItem.enclosedItem = NULL;
    desc = createDungeon(width, height, minRoomCount, minRoomSize);
    gPlayerCoords.x = desc->stairUp.x;
    gPlayerCoords.y = desc->stairUp.y;

    addItemOnTop(gPlayerCoords.x, gPlayerCoords.y, &gPlayerItem);

    return desc;
}
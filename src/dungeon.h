extern const int kErrNoRoomForDungeon;

typedef unsigned char byte;

typedef struct _room
{
    byte x0, y0, x1, y1;
} room;

typedef struct
{
    byte width;
    byte height;
    int numRooms;
    byte *canvas;
} dungeonDescriptor;

dungeonDescriptor *createDungeon(byte width,
                                 byte height,
                                 byte minRoomCount,
                                 byte minRoomSize);

void deallocDungeon(dungeonDescriptor *desc); 
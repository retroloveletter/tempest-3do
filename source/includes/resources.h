#ifndef RESOURCES_H
#define RESOURCES_H

#include "types.h"

typedef struct rez_envelope_typ
{
    long file_bytes;
    void *data;
    long seek_bytes;
    void *seek;
} rez_envelope_typ, *rez_envelope_typ_ptr;

/*
typedef struct
{
    Item item;
} SoundSample;
*/

enum REZ_TYPE
{
    REZ_FILE = 0,
    REZ_CEL,
    REZ_ANIM,
    REZ_SAMPLE,
    REZ_IMAGE,
    REZ_FONT,
    REZ_INSTRUMENT,
    REZ_CEL_LIST
};

Boolean seek_rez_data(rez_envelope_typ_ptr rez, int32 *data);

int32 load_resource(char *path, uint32 type, rez_envelope_typ_ptr rez_envelope);

void unload_resource(rez_envelope_typ_ptr rez_envelope, uint32 type);

void lock_disc_drive(void);

void unlock_disc_drive(void);

#endif // RESOURCES_H

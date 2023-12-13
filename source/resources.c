#include "resources.h"

// 3DO includes
#include "mem.h"
#include "animutils.h"
#include "fontlib.h"
#include "celutils.h"
#include "audio.h"
#include "blockfile.h"
#include "parse3do.h"
#include "kernel.h"
#include "semaphore.h"

/***************************************************************************************/
/* =================================== PRIVATE VARS ================================== */
/***************************************************************************************/

static Item disc_locker_sem = -1;

/***************************************************************************************/
/* ================================ PRIVATE FUNCTIONS ================================ */
/***************************************************************************************/

static int32 load_file(char *path, rez_envelope_typ_ptr rez_envelope)
{
    int32 ret_value = 0;

    // printf("Loading %s\n", path);

    rez_envelope->data = (void*) LoadFile(path, &rez_envelope->file_bytes, MEMTYPE_DRAM);

    if (!rez_envelope->data)
    {
        // printf("Load failed\n");
        ret_value = -1;
    }

    return(ret_value);
}

static void unload_file(rez_envelope_typ_ptr rez_envelope)
{
    if (rez_envelope)
        UnloadFile(rez_envelope->data);
}

static int32 load_cel(char *path, rez_envelope_typ_ptr rez_envelope)
{
    int32 ret_value = 0;
    
    rez_envelope->data = (void*) LoadCel(path, MEMTYPE_CEL);

    if (rez_envelope->data)
    {
        ((CCB*)rez_envelope->data)->ccb_Flags &= ~CCB_LAST; // Turn this off. Caller to control this.
        rez_envelope->file_bytes = GetFileSize(path);
    }
    else 
    {
        ret_value = -1;
    }

    return(ret_value);
}

static void unload_cel(rez_envelope_typ_ptr rez_envelope)
{
    if (rez_envelope)
        DeleteCel((CCB*)rez_envelope->data);
}

static void unload_cel_list(rez_envelope_typ_ptr rez_envelope)
{
    if (rez_envelope)
        DeleteCelList((CCB*)rez_envelope->data);
}

static int32 load_sample(char *path, rez_envelope_typ_ptr rez_envelope)
{
    int32 ret_value = 0;
    Item *item_ptr = (Item*)AllocMem(sizeof(Item), MEMTYPE_DRAM);

    *item_ptr = LoadSample(path);

    if (item_ptr && (*item_ptr) > -1)
    {
        rez_envelope->data = (void*) item_ptr;
        rez_envelope->file_bytes = GetFileSize(path);
    }    
    else 
    {
        ret_value = -1;
        if (item_ptr)
            FreeMem(item_ptr, sizeof(Item));
    }

    return(ret_value);
}

static void unload_sample(rez_envelope_typ_ptr rez_envelope)
{
    Item *item_ptr;

    if (rez_envelope)
    {
        item_ptr = (Item*) rez_envelope->data;
        UnloadSample(*item_ptr);
        FreeMem(item_ptr, sizeof(Item));
    }
}

static int32 load_font(char *path, rez_envelope_typ_ptr rez_envelope)
{
    int32 ret_value = 0;
    FontDescriptor *fd = LoadFont(path, MEMTYPE_DRAM);

    if (fd)
    {
        rez_envelope->data = (void*) fd;
        rez_envelope->file_bytes = GetFileSize(path);
    }
    else 
    {
        ret_value = -1;
    }
    
    return(ret_value);
}

static void unload_font(rez_envelope_typ_ptr rez_envelope)
{
    if (rez_envelope)
        UnloadFont((FontDescriptor*)rez_envelope->data);
}

static int32 load_instrument(char *path, rez_envelope_typ_ptr rez_envelope)
{
    int32 ret_value = 0;
    Item *item_ptr = (Item*)AllocMem(sizeof(Item), MEMTYPE_DRAM);

    *item_ptr = LoadInstrument(path, 0, 100);

    if (*item_ptr < 0)
    {
        if (item_ptr)
            FreeMem(item_ptr, sizeof(Item));

        ret_value = -1;
    }
    else 
    {
        rez_envelope->data = (void*) item_ptr;
        rez_envelope->file_bytes = GetFileSize(path);
    }

    return(ret_value);
}

static void unload_instrument(rez_envelope_typ_ptr rez_envelope)
{
    Item *item_ptr;

    if (rez_envelope)
    {
        item_ptr = (Item*) rez_envelope->data;
        UnloadInstrument(*item_ptr);
        FreeMem(item_ptr, sizeof(Item));
    }
}

#if 0
static void unload_image(void *rez)
{
    if (rez)
        UnloadImage(rez);
}

static void unload_anim(ANIM *rez)
{
    if (rez)
        UnloadAnim(rez);
}

static void *load_image(char *path, long *nbytes, ScreenContext *sc)
{
    void *ptr = LoadImage(path, NULL, NULL, sc);

    if (ptr)
        *nbytes = GetFileSize(path);
    else
        *nbytes = 0;

    return(ptr);
}

static void *load_anim(char *path)
{
    void *ptr = LoadAnim(path, MEMTYPE_CEL);
    return(ptr);
}
#endif 

/***************************************************************************************/
/* ================================= PUBLIC FUNCTIONS ================================ */
/***************************************************************************************/

Boolean seek_rez_data(rez_envelope_typ_ptr rez, int32 *data)
{
    int32 *buffer = (int32*) rez->seek;

    if (rez->seek_bytes > 0)
    {
        *data = *buffer;
        buffer++; // Skip 4 bytes
        rez->seek = (void*) buffer;
        rez->seek_bytes -= 4;
        return(TRUE);
    }
    else 
    {
        return(FALSE);
    }
}

int32 load_resource(char *path, uint32 type, rez_envelope_typ_ptr rez_envelope)
{
    int32 ret_value;

    memset((void*)rez_envelope, 0, sizeof(rez_envelope_typ));

    lock_disc_drive();

    switch(type)
    {
        case REZ_FILE:
            ret_value = load_file(path, rez_envelope);
            break;
        #if 0
        case REZ_IMAGE:
            ret = load_image(path, nbytes);
            break;
        case REZ_ANIM:
            ret = load_anim(path);
            break;
        #endif
        case REZ_CEL:
        case REZ_CEL_LIST:
            ret_value = load_cel(path, rez_envelope);
            break;
        case REZ_SAMPLE:
            ret_value = load_sample(path, rez_envelope);
            break;
        case REZ_FONT:
            ret_value = load_font(path, rez_envelope);
            break;
        case REZ_INSTRUMENT:
            ret_value = load_instrument(path, rez_envelope);
            break;
        default:
            break;
    }

    if (ret_value >= 0)
    {
        rez_envelope->seek_bytes = rez_envelope->file_bytes;
        rez_envelope->seek = rez_envelope->data;
    }

    unlock_disc_drive();

    return(ret_value);
}

void unload_resource(rez_envelope_typ_ptr rez_envelope, uint32 type)
{
    if (!rez_envelope)
        return;

    switch (type)
    {
        case REZ_FILE:
            unload_file(rez_envelope);
            break;
        #if 0
        case REZ_IMAGE:
            unload_image(rez);
            break;
        case REZ_ANIM:
            unload_anim((ANIM*)rez);
            break;
        #endif 
        case REZ_CEL:
            unload_cel(rez_envelope);
            break;
        case REZ_CEL_LIST:
            unload_cel_list(rez_envelope);
            break;
        case REZ_SAMPLE:
            unload_sample(rez_envelope);
            break;
        case REZ_FONT:
            unload_font(rez_envelope);
            break;
        case REZ_INSTRUMENT:
            unload_instrument(rez_envelope);
            break;
        default:
            break;
    }

    memset((void*)rez_envelope, 0, sizeof(rez_envelope_typ));
}

void lock_disc_drive(void)
{
    static Boolean first_run = TRUE;

    if (first_run)
    {
        disc_locker_sem = CreateSemaphore("disc_locker_sem", KernelBase->kb_CurrentTask->t.n_Priority);
        first_run = FALSE;
    }

    LockSemaphore(disc_locker_sem, SEM_WAIT);
}

void unlock_disc_drive(void)
{
    UnlockSemaphore(disc_locker_sem);
}

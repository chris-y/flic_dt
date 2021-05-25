#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/datatypes.h>
#include <proto/utility.h>

#include "endian.h"

/* we don't have our class library base yet, therefore we use the resources from the given DTHookContext */
#undef SysBase
#undef DOSBase
#undef UtilityBase
#define SysBase     (dthc -> dthc_SysBase)
#define UtilityBase (dthc -> dthc_UtilityBase)
#define DOSBase     (dthc -> dthc_DOSBase)

#undef IExec
#undef IDOS
#undef IUtility
#define IExec     (dthc -> dthc_IExec)
#define IUtility (dthc -> dthc_IUtility)
#define IDOS     (dthc -> dthc_IDOS)

struct minflcheader
{
	uint32 size;
	uint16 magic;
};

BOOL _start( struct DTHookContext *dthc )  // DTHook
{
	struct minflcheader *bufptr = (struct minflcheader *)dthc->dthc_Buffer;
	struct FileInfoBlock *fib = dthc->dthc_FIB;

	if((read_le16(&bufptr->magic) == 0xaf11) || (read_le16(&bufptr->magic) == 0xaf12))
	{
		if(read_le32(&bufptr->size) == fib->fib_Size) return(TRUE);
	}

    return(FALSE);
}

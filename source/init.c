/*
 * flic.datatype
 * (c) Chris Young
 */

#include <exec/exec.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <exec/types.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <stdarg.h>

#include "class.h"

#ifdef __amigaos4__
struct Interface *INewlib;
#else
#define RTF_NATIVE 0
struct ExecBase *SysBase;
struct Library *DOSBase;
struct Library *UtilityBase;
struct Library *IntuitionBase;
struct Library *DataTypesBase;
struct Library *GfxBase;
#endif

/* Version Tag */
#include "flic.datatype_rev.h"
#define LIBNAME "flic.datatype"

STATIC CONST UBYTE
#ifdef __GNUC__
__attribute__((used))
#endif
verstag[] = VERSTAG;

#ifdef __amigaos4__
static BPTR libExpunge(struct LibraryManagerInterface *Self);
#else
BPTR libExpunge (REG(a6, struct ClassBase *libBase));
#endif

/*
 * The system (and compiler) rely on a symbol named _start which marks
 * the beginning of execution of an ELF file. To prevent others from 
 * executing this library, and to keep the compiler/linker happy, we
 * define an empty _start symbol here.
 *
 * On the classic system (pre-AmigaOS4) this was usually done by
 * moveq #0,d0
 * rts
 *
 */
int32 _start(void);

int32 _start(void)
{
    /* If you feel like it, open DOS and print something to the user */
    return 100;
}

/* Open the library */
#ifdef __amigaos4__
static struct ClassBase *libOpen(struct LibraryManagerInterface *Self, ULONG version)
#else
struct ClassBase *libOpen(REG(a6, struct ClassBase *libBase), REG(d0, uint32 version))
#endif
 {
#ifdef __amigaos4__
    struct ClassBase *libBase = (struct ClassBase *)Self->Data.LibBase; 
#endif

    if (version > VERSION)
    {
        return NULL;
    }

    /* Add any specific open code here 
       Return 0 before incrementing OpenCnt to fail opening */


    /* Add up the open count */
    libBase->libNode.lib_OpenCnt++;
    return libBase;

}


/* Close the library */
#ifdef __amigaos4__
static BPTR libClose(struct LibraryManagerInterface *Self)
#else
BPTR libClose (REG(a6, struct ClassBase *libBase))
#endif
{
#ifdef __amigaos4__
    struct ClassBase *libBase = (struct ClassBase *)Self->Data.LibBase;
#endif
    /* Make sure to undo what open did */


    /* Make the close count */
    ((struct Library *)libBase)->lib_OpenCnt--;

	#ifndef __amigaos4__
	if (libBase->libNode.lib_OpenCnt) {
		return 0;
	}

	if (libBase->libNode.lib_Flags & LIBF_DELEXP) {
		return libExpunge(libBase);
	} else {
		return 0;
	}
#endif

    return 0;
}


static int openDTLibs (struct ClassBase *libBase);
static void closeDTLibs (struct ClassBase *libBase);

/* Expunge the library */
#ifdef __amigaos4__
static BPTR libExpunge(struct LibraryManagerInterface *Self)
#else
BPTR libExpunge (REG(a6, struct ClassBase *libBase))
#endif
{
#ifdef __amigaos4__
    struct ClassBase *libBase = (struct ClassBase *)Self->Data.LibBase;
    struct ExecIFace *IExec = libBase->IExec;
#endif
    /* If your library cannot be expunged, return 0 */
    BPTR result = (BPTR)NULL;

    if (libBase->libNode.lib_OpenCnt == 0)
    {
	    result = libBase->SegList;

		freeDTClass(libBase, libBase->DTClass);

		closeDTLibs(libBase);

        Remove(&libBase->libNode.lib_Node);
#ifdef __amigaos4__
        DeleteLibrary(&libBase->libNode);
#else
	FreeMem((BYTE *)libBase - libBase->libNode.lib_NegSize,
		libBase->libNode.lib_NegSize + libBase->libNode.lib_PosSize);
#endif
    }
    else
    {
        result = (BPTR)NULL;
        libBase->libNode.lib_Flags |= LIBF_DELEXP;
    }
    return result;
}

/* The ROMTAG Init Function */
#ifdef __amigaos4__
static struct ClassBase *libInit (struct ClassBase *libBase, BPTR seglist, struct ExecIFace *exec)
#else
struct ClassBase *libInit (REG(d0, struct ClassBase *libBase), REG(a0, BPTR seglist),
	REG(a6, struct ExecBase *exec))
#endif
{
#ifdef __amigaos4__
	struct ExecIFace *IExec = exec;
#else
	SysBase = exec;
#endif

    libBase->libNode.lib_Node.ln_Type = NT_LIBRARY;
    libBase->libNode.lib_Node.ln_Pri  = 0;
    libBase->libNode.lib_Node.ln_Name = LIBNAME;
    libBase->libNode.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
    libBase->libNode.lib_Version      = VERSION;
    libBase->libNode.lib_Revision     = REVISION;
    libBase->libNode.lib_IdString     = VSTRING;

#ifdef __amigaos4__
	libBase->IExec = exec;
#endif

    libBase->SegList = seglist;
	if (openDTLibs(libBase)) {
		libBase->DTClass = initDTClass(libBase);
		if (libBase->DTClass) {
			return libBase;
		}
		closeDTLibs(libBase);
	}

#ifdef __amigaos4__
	DeleteLibrary(&libBase->libNode);
#else
	FreeMem((BYTE *)libBase - libBase->libNode.lib_NegSize,
		libBase->libNode.lib_NegSize + libBase->libNode.lib_PosSize);
#endif
	
	return NULL;
}

static int openDTLibs (struct ClassBase *libBase) {
#ifdef __amigaos4__
	struct ExecIFace *IExec = libBase->IExec;
	libBase->DOSLib = OpenLibrary("dos.library", 52);
	if (!libBase->DOSLib) return FALSE;
	IDOS = (struct DOSIFace *)GetInterface(libBase->DOSLib, "main", 1, NULL);
	if (!IDOS) return FALSE;

	libBase->IntuitionLib = OpenLibrary("intuition.library", 52);
	if (!libBase->IntuitionLib) return FALSE;
	libBase->IIntuition = (struct IntuitionIFace *)GetInterface(libBase->IntuitionLib, "main", 1, NULL);
	if (!libBase->IIntuition) return FALSE;

	libBase->UtilityLib = OpenLibrary("utility.library", 52);
	if (!libBase->UtilityLib) return FALSE;
	libBase->IUtility = (struct UtilityIFace *)GetInterface(libBase->UtilityLib, "main", 1, NULL);
	if (!libBase->IUtility) return FALSE;

	libBase->DataTypesLib = OpenLibrary("datatypes.library", 52);
	if (!libBase->DataTypesLib) return FALSE;
	libBase->IDataTypes = (struct DataTypesIFace *)GetInterface(libBase->DataTypesLib, "main", 1, NULL);
	if (!libBase->IDataTypes) return FALSE;

	libBase->GraphicsLib = OpenLibrary("graphics.library", 52);
	if (!libBase->GraphicsLib) return FALSE;
	libBase->IGraphics = (struct GraphicsIFace *)GetInterface(libBase->GraphicsLib, "main", 1, NULL);
	if (!libBase->IGraphics) return FALSE;

	libBase->NewlibLib = OpenLibrary("newlib.library", 52);
	if (!libBase->NewlibLib) return FALSE;
	INewlib = GetInterface(libBase->NewlibLib, "main", 1, NULL);
	if (!INewlib) return FALSE;

	return TRUE;
#else
	if ((IntuitionBase = OpenLibrary("intuition.library", 39)) &&
		(GfxBase = OpenLibrary("graphics.library", 39)) &&
		(DOSBase = OpenLibrary("dos.library", 39)) &&
		(UtilityBase = OpenLibrary("utility.library", 39)) &&
		(DataTypesBase = OpenLibrary("datatypes.library", 39))) {
		return TRUE;
	} else {
		closeDTLibs(libBase);
		return FALSE;
	}
#endif
}

static void closeDTLibs (struct ClassBase *libBase) {
#ifdef __amigaos4__
	struct ExecIFace *IExec = libBase->IExec;
	DropInterface((struct Interface *)libBase->IGraphics);
	CloseLibrary(libBase->GraphicsLib);

	DropInterface((struct Interface *)libBase->IDataTypes);
	CloseLibrary(libBase->DataTypesLib);

	DropInterface((struct Interface *)libBase->IUtility);
	CloseLibrary(libBase->UtilityLib);

	DropInterface((struct Interface *)libBase->IIntuition);
	CloseLibrary(libBase->IntuitionLib);

	DropInterface((struct Interface *)IDOS);
	CloseLibrary(libBase->DOSLib);

	DropInterface((struct Interface *)INewlib);
	CloseLibrary(libBase->NewlibLib);
#else
	CloseLibrary(DataTypesBase);
	CloseLibrary(UtilityBase);
	CloseLibrary(DOSBase);
	CloseLibrary(GfxBase);
	CloseLibrary(IntuitionBase);
#endif
}

#ifdef __amigaos4__
/* ------------------- Manager Interface ------------------------ */
/* These are generic. Replace if you need more fancy stuff */
STATIC LONG _manager_Obtain(struct LibraryManagerInterface *Self)
{
    return Self->Data.RefCount++;
}

STATIC ULONG _manager_Release(struct LibraryManagerInterface *Self)
{
    return Self->Data.RefCount--;
}

/* Manager interface vectors */
STATIC CONST APTR lib_manager_vectors[] =
{
    (APTR)_manager_Obtain,
    (APTR)_manager_Release,
    NULL,
    NULL,
    (APTR)libOpen,
    (APTR)libClose,
    (APTR)libExpunge,
    NULL,
    (APTR)-1
};

/* "__library" interface tag list */
STATIC CONST struct TagItem lib_managerTags[] =
{
    { MIT_Name,        (Tag)"__library"       },
    { MIT_VectorTable, (Tag)lib_manager_vectors },
    { MIT_Version,     1                        },
    { TAG_DONE,        0                        }
};
#endif
/* ------------------- Library Interface(s) ------------------------ */

#ifdef __amigaos4__
Class *_DTClass_ObtainEngine(struct Interface *Self)
#else
Class *_DTClass_ObtainEngine(REG(a6, struct ClassBase *libBase))
#endif
{
#ifdef __amigaos4__
	struct ClassBase *libBase = (struct ClassBase *)Self->Data.LibBase;
#endif
	return libBase->DTClass;
}

#ifdef __amigaos4__
STATIC CONST APTR main_vectors[] =
{
    (APTR)_manager_Obtain,
    (APTR)_manager_Release,
    NULL,
    NULL,
    (APTR)_DTClass_ObtainEngine,
    (APTR)-1
};

/* Uncomment this line (and see below) if your library has a 68k jump table */
/* extern APTR VecTable68K[]; */

STATIC CONST struct TagItem mainTags[] =
{
    { MIT_Name,        (Tag)"main" },
    { MIT_VectorTable, (Tag)main_vectors },
    { MIT_Version,     1 },
    { TAG_DONE,        0 }
};

STATIC CONST CONST_APTR libInterfaces[] =
{
    lib_managerTags,
    mainTags,
    NULL
};

STATIC CONST struct TagItem libCreateTags[] =
{
    { CLT_DataSize,    sizeof(struct ClassBase) },
    { CLT_InitFunc,    (Tag)libInit },
    { CLT_Interfaces,  (Tag)libInterfaces},
    /* Uncomment the following line if you have a 68k jump table */
    /* { CLT_Vector68K, (Tag)VecTable68K }, */
    {TAG_DONE,         0 }
};

#else
APTR libReserved (REG(a6, struct ClassBase *libBase)) {
	return NULL;
}

struct InitTable {
	ULONG	it_Size;			/* data space size */
	APTR	it_FunctionTable;	/* pointer to function initializers */
	APTR	it_DataTable;		/* pointer to data initializers */
	APTR	it_InitRoutine;		/* routine to run */
};

CONST_APTR function_table[] = {
	(APTR)libOpen,
	(APTR)libClose,
	(APTR)libExpunge,
	(APTR)libReserved,
	(APTR)_DTClass_ObtainEngine,
	(APTR)-1
};

const struct InitTable init_table = {
	sizeof(struct ClassBase),
	(APTR)function_table,
	NULL,
	(APTR)libInit
};


#endif

/* ------------------- ROM Tag ------------------------ */
STATIC CONST struct Resident lib_res
#ifdef __GNUC__
__attribute__((used))
#endif
=
{
    RTC_MATCHWORD,
    (struct Resident *)&lib_res,
    (APTR)(&lib_res + 1),
    RTF_NATIVE|RTF_AUTOINIT, /* Add RTF_COLDSTART if you want to be resident */
    VERSION,
    NT_LIBRARY, /* Make this NT_DEVICE if needed */
    0, /* PRI, usually not needed unless you're resident */
    LIBNAME,
    VSTRING,
#ifdef __amigaos4__
    (APTR)libCreateTags
#else
    (APTR)&init_table
#endif
};

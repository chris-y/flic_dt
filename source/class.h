/*
 * flic.datatype
 * (c) Chris Young
 */

#ifndef FLIC_CLASS_H
#define FLIC_CLASS_H

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/datatypes.h>
#include <proto/graphics.h>

#include <exec/exec.h>
#include <dos/dos.h>
#include <utility/utility.h>
#include <datatypes/animationclass.h>

#ifndef __amigaos4__
#include "SDI_compiler.h"
#include <stdint.h>
#define uint16 uint16_t
#define int32 int32_t
#define uint32 uint32_t
#define uint8 uint8_t
#define int8 int8_t
#define int16 int16_t
#endif

#include "endian.h"

struct ClassBase {
	struct Library	libNode;
	uint16			Pad;
	Class			*DTClass;
	BPTR			SegList;
#ifdef __amigaos4__
	struct ExecIFace		*IExec;
	struct DOSIFace			*IDOS;
	struct IntuitionIFace	*IIntuition;
	struct UtilityIFace		*IUtility;
	struct DataTypesIFace	*IDataTypes;
	struct GraphicsIFace	*IGraphics;
//	struct NewlibIFace	*INewlib;

	struct Library	*DOSLib;
	struct Library	*IntuitionLib;
	struct Library	*UtilityLib;
	struct Library	*DataTypesLib;
	struct Library	*GraphicsLib;
	struct Library  *NewlibLib;
#endif
	struct Library	*SuperClassLib;
};

#if 0
#define DOSLib			libBase->DOSLib
#define IntuitionLib	libBase->IntuitionLib
#define UtilityLib		libBase->UtilityLib
#define DataTypesLib	libBase->DataTypesLib
#define GraphicsLib		libBase->GraphicsLib
#define SuperClassLib	libBase->SuperClassLib
#define NewlibLib       libBase->NewlibLib

//#define IExec		libBase->IExec
#define IIntuition	libBase->IIntuition
#define IUtility	libBase->IUtility
#define IDataTypes	libBase->IDataTypes
#define IGraphics	libBase->IGraphics
//#define INewlib     libBase->INewlib
#endif

#ifndef __amigaos4__
extern struct ExecBase *SysBase;
extern struct Library *DOSBase;
extern struct Library *UtilityBase;
extern struct Library *IntuitionBase;
extern struct Library *DataTypesBase;
extern struct Library *GfxBase;
#else
#define IDOS		libBase->IDOS
#endif



Class *initDTClass (struct ClassBase *libBase);
BOOL freeDTClass (struct ClassBase *libBase, Class *cl);

#define OK (0)
#define NOTOK DTERROR_INVALID_DATA
#define ERROR_EOF DTERROR_NOT_ENOUGH_DATA

#define ReadError(C) ((C == -1) ? IDOS->IoErr() : ERROR_EOF)
#define WriteError(C) IDOS->IoErr()

struct flcheader
{
	uint32 size;
	uint16 magic;
	uint16 frames;
	uint16 width;
	uint16 height;
	uint16 depth;
	uint32 speed;
	uint16 reserved;
	uint32 created;
	char creator[4];
	uint16 updated;
	char updater[4];
	uint16 aspectx;
	uint16 aspecty;
	char reserved2[38];
	uint32 oframe1;
	uint32 oframe2;
	char reserved3[40];
};

struct flcchunk
{
	uint32 size;
	uint16 type;
	uint16 chunks;
	char reserved[8];
};

struct flcdatchunk
{
	uint32 size;
	uint16 type;
};

struct flicinstdata
{
	BPTR file;
	uint32 frame;
	struct BitMap *bm;
	struct ColorMap *cmap;
	uint32 nextchunk;
};

#endif /* FLIC_CLASS_H */

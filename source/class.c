/*
 * flic.datatype - Chris Young
 */

#include "class.h"

#ifndef __amigaos4__
#include <clib/alib_protos.h>
#define IDoSuperMethodA DoSuperMethodA
#define ICoerceMethod CoerceMethod
#endif

#define RGB8to32(RGB) (((uint32)(RGB) << 24)|((uint32)(RGB) << 16)|((uint32)(RGB) << 8)|((uint32)(RGB)))


#ifdef __amigaos4__
static uint32 ClassDispatch(Class *cl, Object *o, Msg msg);
#else
static uint32 ClassDispatch(REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg));
#endif
static int32 WriteFLIC (Class *cl, Object *o, struct dtWrite *msg);
static struct BitMap *ConvertFLIC (Class *cl, Object *o, BPTR file,struct adtFrame *adf); //, uint32 index, uint32 *total);
static struct BitMap *GetFLIC (Class *cl, Object *o, struct TagItem *tags);
static struct BitMap *GetFrame(Class *, Object *, struct adtFrame *);

Class *initDTClass (struct ClassBase *libBase)
{
#ifdef __amigaos4__
	struct ExecIFace *IExec = libBase->IExec;
	struct IntuitionIFace *IIntuition = libBase->IIntuition;
#endif
	Class *cl;
	libBase->SuperClassLib = OpenLibrary("datatypes/animation.datatype", 39);
	if (libBase->SuperClassLib) {
		cl = MakeClass(libBase->libNode.lib_Node.ln_Name, ANIMATIONDTCLASS, NULL, sizeof(struct flicinstdata), 0);
		if (cl) {
			cl->cl_Dispatcher.h_Entry = (HOOKFUNC)ClassDispatch;
			cl->cl_UserData = (uint32)libBase;
			AddClass(cl);
		} else {
			CloseLibrary(libBase->SuperClassLib);
		}
	}
	return cl;
}

BOOL freeDTClass (struct ClassBase *libBase, Class *cl) {
#ifdef __amigaos4__
	struct ExecIFace *IExec = libBase->IExec;
	struct IntuitionIFace *IIntuition = libBase->IIntuition;
#endif
	BOOL res = TRUE;
	if (cl) {
		res = FreeClass(cl);
		if (res) {
			CloseLibrary(libBase->SuperClassLib);
		}
	}
	return res;
}



#ifdef __amigaos4__
static uint32 ClassDispatch(Class *cl, Object *o, Msg msg)
#else
static uint32 ClassDispatch(REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg))
#endif
{
	struct ClassBase *libBase = (struct ClassBase *)cl->cl_UserData;
#ifdef __amigaos4__
	struct ExecIFace *IExec = libBase->IExec;
	struct GraphicsIFace *IGraphics = libBase->IGraphics;
//	struct DOSIFace *IDOS = libBase->IDOS;
	struct IntuitionIFace *IIntuition = libBase->IIntuition;
#endif
	uint32 ret;

	switch (msg->MethodID) {

		case OM_NEW:
			ret = IDoSuperMethodA(cl, o, msg);
			if (ret) {
				int32 error;
				error = (int32)GetFLIC(cl, (Object *)ret, ((struct opSet *)msg)->ops_AttrList);
				if (error != OK) {
					ICoerceMethod(cl, (Object *)ret, OM_DISPOSE);
					ret = (uint32)NULL;
					SetIoErr(error);
				}
			}
			break;

			case ADTM_LOADFRAME:
				ret = (uint32)GetFrame(cl, o, (struct adtFrame *)msg);
				if(!ret) SetIoErr(DTERROR_INVALID_DATA);
			break;

			case ADTM_UNLOADFRAME:
			{
				struct adtFrame *adf = (struct adtFrame *)msg;

				if(adf->alf_BitMap) FreeBitMap(adf->alf_BitMap);
				adf->alf_BitMap = NULL;
				if(adf->alf_CMap) FreeColorMap(adf->alf_CMap);
				adf->alf_CMap = NULL;

/*
				struct flicinstdata *prevframe = INST_DATA(cl,o);

				if(prevframe->bm) IGraphics->FreeBitMap(prevframe->bm);
				prevframe->bm=adf->alf_BitMap;
				adf->alf_BitMap = NULL;

				if(prevframe->cmap) IGraphics->FreeColorMap(prevframe->cmap);
				prevframe->cmap=adf->alf_CMap;
				adf->alf_CMap=NULL;

				prevframe->frame = adf->alf_Frame;
*/
			}
			break;

			case OM_DISPOSE:
			{
				struct flicinstdata *prevframe = INST_DATA(cl,o);

				if(prevframe->bm) FreeBitMap(prevframe->bm);
				prevframe->bm=NULL;
				if(prevframe->cmap) FreeColorMap(prevframe->cmap);
				prevframe->cmap=NULL;

				ret = IDoSuperMethodA(cl, o, msg);
			}
			break;

			/* fall through and let superclass deal with it */

		default:
			ret = IDoSuperMethodA(cl, o, msg);
			break;

	}

	return ret;
}

static int32 WriteFLIC (Class *cl, Object *o, struct dtWrite *msg) {
	return ERROR_NOT_IMPLEMENTED;
}

static struct BitMap *ConvertFLIC (Class *cl, Object *o, BPTR file,struct adtFrame *adf)
{
	struct ClassBase *libBase = (struct ClassBase *)cl->cl_UserData;
#ifdef __amigaos4__
	struct ExecIFace *IExec = libBase->IExec;
	struct GraphicsIFace *IGraphics = libBase->IGraphics;
//	struct DOSIFace *IDOS = libBase->IDOS;
	struct DataTypesIFace *IDataTypes = libBase->IDataTypes;
#endif
	int error = 0;
	struct BitMap *bm,*tempbm;
	struct flcheader head;
	struct flcchunk chunk;
	struct flcdatchunk datachunk;
	UBYTE *buffer;
	struct RastPort rp,temprp;
	ULONG x=0,y=0;
	struct ColorRegister *cmap = NULL, *ocmap = NULL;
	struct ColorMap *clrmap = NULL;
	uint32 *cregs = NULL;
	int tmp;
	uint32 curframe=0,framereq=0;
	uint32 filepos = 128;
	uint32 size;
	uint32 speed;
	UBYTE *tempmem;
	struct flicinstdata *prevframe;

//IExec->DebugPrintF("convertico\n");
	prevframe = INST_DATA(cl,o);

	if(file)
	{
		prevframe->file = file;
	}
	else
	{
		file = prevframe->file;
	}

	if(adf)
	{
		framereq = adf->alf_TimeStamp; //+1;
//IExec->DebugPrintF("%ld\n",framereq);
	}

	framereq++; // starts at 0, we start at 1

//IExec->DebugPrintF("Seeking frame %ld\n",framereq);

	Seek(file,0,OFFSET_BEGINNING);

	Read(file,&head,sizeof(struct flcheader));		

	if(!adf)
	{
		if(read_le16(&head.magic) == 0xAF12)
		{
			speed = 1000/read_le32(&head.speed);
		}
		else if(read_le16(&head.magic) == 0xAF11)
		{
			speed = 70/read_le32(&head.speed);
		}

		SetDTAttrs(o, NULL, NULL,
			PDTA_NumColors,256,
			TAG_END);

		GetDTAttrs(o,
			PDTA_ColorRegisters,	&cmap,
			PDTA_CRegs,				&cregs,
			TAG_END);

	}

	//IExec->DebugPrintF("%ld x %ld x %ld  offset %ld\n",read_le16(&head.width),read_le16(&head.height),read_le16(&head.depth),read_le32(&head.oframe1));

	if(framereq > read_le16(&head.frames)) return 0;

	if((prevframe->frame <= framereq) && (prevframe->bm))
	{
		bm = prevframe->bm;
		prevframe->bm = NULL;
		if(prevframe->cmap)
		{
			clrmap = prevframe->cmap;
			prevframe->cmap=NULL;
		}
	}
	else
	{
		bm = AllocBitMap(read_le16(&head.width),read_le16(&head.height),read_le16(&head.depth),0,NULL);

		if(prevframe->bm)
		{
			FreeBitMap(prevframe->bm);
			prevframe->bm=NULL;
		}
		if(prevframe->cmap)
		{
			FreeColorMap(prevframe->cmap);
			prevframe->cmap=NULL;
		}
	}

	InitRastPort(&rp);
	InitRastPort(&temprp);
	rp.BitMap = bm;

	tempbm = AllocBitMap(read_le16(&head.width),1,read_le16(&head.depth),0,NULL);
	temprp.BitMap = tempbm;

	tempmem = AllocVec(read_le16(&head.width),MEMF_ANY);

/*
	ocmap=cmap;

	for(tmp=0;tmp<256;tmp++)
	{
		ocmap->red = tmp;
		ocmap->green = tmp;
		ocmap->blue = tmp;
		ocmap++;
	}
*/

	if(read_le32(&head.oframe1))
	{
		filepos = read_le32(&head.oframe1);
	}
	else
	{
		filepos=128;
	}

//IExec->DebugPrintF("start filepos %lld frames %ld\n",filepos,read_le16(&head.frames));

//IExec->DebugPrintF("prevframe %ld framereq %ld  sizeof %ld\n",prevframe->frame,framereq,sizeof(struct flcchunk));

	Seek(file,filepos,OFFSET_BEGINNING);

	if((prevframe->frame) && (prevframe->frame < framereq))
	{
		filepos = prevframe->nextchunk;
		Seek(file,filepos,OFFSET_BEGINNING);
		curframe = prevframe->frame;
#if 0
		curframe=0;

		while(curframe < prevframe->frame)
		{
			Read(file,&chunk,sizeof(struct flcchunk));
			size = read_le32(&chunk.size);
			Seek(file,size - sizeof(struct flcchunk),OFFSET_CURRENT);
			filepos+=size;

//IExec->DebugPrintF("skip %ld bytes (magic %lx) curframe %ld prevframe %ld\n",size,chunk.type,curframe,prevframe->frame);

//IExec->DebugPrintF("current file position = %lld  filepos %lld\n",IDOS->GetFilePosition(file),filepos);

			curframe++;
		}

//		curframe--;
#endif
	}
	else if(prevframe->frame == framereq)
	{
		curframe=framereq;
	}

//IExec->DebugPrintF("%ld (current frame) filepos %ld\n",curframe,filepos);

	while(curframe<framereq)
	{
		uint32 filepos2;
//		IDOS->ChangeFilePosition(file,filepos,OFFSET_BEGINNING);
		Read(file,&chunk,sizeof(struct flcchunk));
		filepos2=sizeof(struct flcchunk);

		if(read_le16(&chunk.chunks))
		{
			uint32 size2;
			int i;

			for(i=0;i<read_le16(&chunk.chunks);i++)
			{
//				IDOS->ChangeFilePosition(file,filepos+filepos2,OFFSET_BEGINNING);
				Read(file,&datachunk,6);

				size2 = read_le32(&datachunk.size); // - sizeof(struct flcdatchunk);
				filepos2+=size2;

			//	size = size - size2;
#ifdef __amigaos4__
				DebugPrintF("[flic.datatype] CHUNK %ld SIZE %ld\n",read_le16(&datachunk.type),size2);
#endif
				switch(read_le16(&datachunk.type))
				{
					case 4:
					{
						uint8 skip,count,r,g,b;
						uint16 packets,count2;
						uint32 idx=0;

						Read(file,&packets,sizeof(packets));

						if(adf)
						{
							if(!clrmap) clrmap=GetColorMap(256);
						}
						else
						{
							ocmap=cmap;
						}

						for(tmp=0;tmp<read_le16(&packets);tmp++)
						{
							uint16 i;

							Read(file,&skip,1);

							if(adf)
							{
								idx+=skip;
							}
							else
							{
								ocmap+=skip;
							}

							Read(file,&count,1);
							if(count==0)
							{
								count2=256;
							}
							else
							{
								count2=count;
							}

							while(count2)
							{
								Read(file,&r,1);
								Read(file,&g,1);
								Read(file,&b,1);

								if(adf)
								{
									SetRGB32CM(clrmap,idx,r<<24,g<<24,b<<24);
									idx++;
								}
								else
								{
									ocmap->red = r;
									ocmap->green = g;
									ocmap->blue = b;

									ocmap++;
								}

								count2--;
							}
						}
					}
					break;

					case 7:
					{
						uint16 word,lines,i;
						int16 lineskip;

						Read(file,&word,2);
						lines = read_le16(&word);
						y=0;
						for(i=0;i<lines;i++)
						{
							uint8 colskip,pixel;
							int8 ptype;
							uint32 j;

							x=0;

							Read(file,&word,2);
							while(read_le16(&word) & 0xc000)
							{
								if((read_le16(&word) & 0xc000) == 0xc000)
								{
									y+=(0x10000-read_le16(&word));
								}
								if((read_le16(&word) & 0xc000) == 0x4000)
								{
									//put low order byte in last byte of line
									SetAPen(&rp,read_le16(&word) & 0x00ff);
									WritePixel(&rp,read_le16(&head.width)-1,y);
								}
								Read(file,&word,2);
							}

							if((read_le16(&word) & 0xc000) == 0x0000)
							{
								if(read_le16(&word))
								{
									for(j=0;j<read_le16(&word);j++)
									{

										Read(file,&colskip,1);
										x+=colskip;
										Read(file,&ptype,1);

										if(ptype>0)
										{
											Read(file,tempmem,ptype*2);
											WritePixelLine8(&rp,x,y,(ptype*2),tempmem,&temprp);
											x+=(ptype*2);
										}
										else if(ptype<0)
										{
											uint8 oldx,oldptype;
	
											Read(file,tempmem,2);

											while(ptype<0)
											{
												WritePixelLine8(&rp,x,y,2,tempmem,&temprp);
												x+=2;
												ptype++;
											}
										}
									}
								}
							}
							y++;
						}
					}
					break;

					case 11:
					{
						uint8 skip,count,r,g,b;
						uint16 packets,count2;
						uint32 idx=0;

						Read(file,&packets,sizeof(packets));

						if(adf)
						{
							if(!clrmap) clrmap=GetColorMap(256);
						}
						else
						{
							ocmap=cmap;
						}

						for(tmp=0;tmp<read_le16(&packets);tmp++)
						{
							uint16 i;

							Read(file,&skip,1);

							if(adf)
							{
								idx+=skip;
							}
							else
							{
								ocmap+=skip;
							}

							Read(file,&count,1);
							if(count==0)
							{
								count2=256;
							}
							else
							{
								count2=count;
							}

							while(count2)
							{
								Read(file,&r,1);
								Read(file,&g,1);
								Read(file,&b,1);

								if(adf)
								{
									SetRGB32CM(clrmap,idx,r<<26,g<<26,b<<26);
									idx++;
								}
								else
								{
									ocmap->red = r<<2;
									ocmap->green = g<<2;
									ocmap->blue = b<<2;

									ocmap++;
								}

								count2--;
							}
						}
					}
					break;

					case 12:
					{
						uint16 word,lines,i;
						int16 lineskip;

						Read(file,&word,2);
						y=read_le16(&word);

						Read(file,&word,2);
						lines = read_le16(&word);

						for(i=0;i<lines;i++)
						{
							uint8 colskip,pixel,pkts;
							int8 ptype;
							uint32 j;

							x=0;

							Read(file,&pkts,1);

							for(j=0;j<pkts;j++)
							{
								Read(file,&colskip,1);
								x+=colskip;
								Read(file,&ptype,1);

								if(ptype>0)
								{
									Read(file,tempmem,ptype);
									WritePixelLine8(&rp,x,y,ptype,tempmem,&temprp);
									x+=ptype;
								}
								else if(ptype<0)
								{
									Read(file,&pixel,1);
									SetAPen(&rp,pixel);
	
									while(ptype<0)
									{
										WritePixel(&rp,x,y);
										x++;
										ptype++;
									}
								}
							}
							y++;
						}
					}
					break;

					case 13:
						SetAPen(&rp,0);
						for(y=0;read_le16(&head.height);y++)
						{
							for(x=0;x<read_le16(&head.width);x++)
							{
								WritePixel(&rp,x,y);
							}
						}
					break;

					case 15:

						x=0;
						y=0;

						while(y<read_le16(&head.height))
						{
							uint8 pixel,packets;
							int8 ptype;

							if(x==0)
							{
								Read(file,&packets,1);
							}

							Read(file,&ptype,1);
							if(ptype<=0)
							{
								Read(file,tempmem,-ptype);
								WritePixelLine8(&rp,x,y,-ptype,tempmem,&temprp);
								x-=ptype;
							}
							else
							{
								Read(file,&pixel,1);
								SetAPen(&rp,pixel);
								while(ptype>0)
								{
									WritePixel(&rp,x,y);
									x++;
									ptype--;
								}
							}

							if(x>=read_le16(&head.width))
							{
								x=0;
								y++;
							}
						}
					break;

					case 16:
						for(y=0;read_le16(&head.height);y++)
						{
							Read(file,tempmem,read_le16(&head.width));
							WritePixelLine8(&rp,x,y,read_le16(&head.width),tempmem,&temprp);
						}
					break;

					default:
#ifdef __amigaos4__
						DebugPrintF("[flic.datatype] Unsupported chunk %ld\n",read_le16(&datachunk.type));
#endif
						if(read_le16(&datachunk.type)>50)
						{
#ifdef __amigaos4__
							DebugPrintF("[flic.datatype] FATAL ERROR\n");
#endif
							return 0;
						}
//						IDOS->ChangeFilePosition(file,size2,OFFSET_CURRENT);
					break;
				}
#ifdef __amigaos4__
				if(GetFilePosition(file) != (filepos+filepos2))
				{
					DebugPrintF("[flic.datatype] WARNING: File position mismatch (%lld should be %lld after chunk %ld)\n",GetFilePosition(file),filepos+filepos2,read_le16(&datachunk.type));
					Seek(file,filepos+filepos2,OFFSET_BEGINNING);
				}
#endif
			}
		}
		size = read_le32(&chunk.size); // - sizeof(struct flcchunk);
		filepos+=size; // NEXT chunk!
//IExec->DebugPrintF("filepos %lld (next chunk)\n",filepos);
		curframe++;
	}

	FreeBitMap(tempbm);
	FreeVec(tempmem);

	if(!adf)
	{
		if(cmap)
		{
			ocmap = cmap;

			for(tmp=0;tmp<256;tmp++)
			{
				*cregs++ = RGB8to32(ocmap->red);
				*cregs++ = RGB8to32(ocmap->green);
				*cregs++ = RGB8to32(ocmap->blue);
				ocmap++;
			}
		}

		SetDTAttrs(o, NULL, NULL,
//				DTA_ObjName,IDOS->FilePart(filename),
                DTA_TotalHoriz,read_le16(&head.width),
                DTA_TotalVert, read_le16(&head.height),
                DTA_Repeat,    1,
                ADTA_Width,    read_le16(&head.width),
                ADTA_Height,   read_le16(&head.height),
                ADTA_Depth,    read_le16(&head.depth),
                ADTA_Frames,   read_le16(&head.frames),
                ADTA_FramesPerSecond, speed,
                ADTA_ModeID,                                       0,
                ADTA_KeyFrame,                                     bm,
				TAG_END);

		return (struct BitMap *)error;
	}
	else
	{
/*
		struct ViewPort vp;
*/
		ULONG table[(256*3)];
		uint16 i;

		adf->alf_Frame = curframe;
		adf->alf_BitMap = bm;
		adf->alf_CMap = clrmap;
		adf->alf_Duration = 1;

		prevframe->frame=curframe;
		prevframe->bm=AllocBitMap(read_le16(&head.width),read_le16(&head.height),read_le16(&head.depth),0,NULL);
		BltBitMap(bm,0,0,prevframe->bm,0,0,read_le16(&head.width),read_le16(&head.height),0x0c0,0xff,NULL);

		prevframe->cmap = GetColorMap(256);

		prevframe->nextchunk = filepos;

/*
		IGraphics->InitVPort(&vp);

		vp.ColorMap = prevframe->cmap;

		table[0] = 256<<16 + 0;
		table[258]=0;
*/
		GetRGB32(clrmap,0,256,&table);

		for(i=0;i<768;i+=3)
		{
			SetRGB32CM(prevframe->cmap,i/3,table[i],table[i+1],table[i+2]);
		}

		return bm;
	}
}

static struct BitMap *GetFLIC (Class *cl, Object *o, struct TagItem *tags) {
	struct ClassBase *libBase = (struct ClassBase *)cl->cl_UserData;
#ifdef __amigaos4__
	struct UtilityIFace *IUtility = libBase->IUtility;
	struct DataTypesIFace *IDataTypes = libBase->IDataTypes;
#endif
	struct BitMapHeader *bmh = NULL;
	char *filename;
	int32 srctype;
	struct BitMap *error = NULL; //ERROR_OBJECT_NOT_FOUND;
	BPTR file = (BPTR)NULL;
	struct BitMap *bm;

#ifndef __amigaos4__
	filename = (char *)GetTagData(DTA_Name, (uint32)"Untitled", tags);
#endif

	GetDTAttrs(o,
		DTA_Handle,			&file,
		DTA_SourceType,		&srctype,
#ifdef __amigaos4__
		DTA_Name,			&filename,
#endif
		TAG_END);

		SetDTAttrs(o, NULL, NULL,
				DTA_ObjName, FilePart(filename),
				TAG_DONE);

	/* Do we have everything we need? */
	if (file && srctype == DTST_FILE) {

		error = ConvertFLIC(cl, o, file,NULL); //, whichpic, numpics);
	}

	return error;
}

static struct BitMap *GetFrame (Class *cl, Object *o, struct adtFrame *adf) {
	struct ClassBase *libBase = (struct ClassBase *)cl->cl_UserData;
	char *filename;
	int32 srctype;
	int32 error = 0; //ERROR_OBJECT_NOT_FOUND;
	BPTR file = (BPTR)NULL;
	struct BitMap *bm;

/*
	IDataTypes->GetDTAttrs(o,
		DTA_Handle,			&file,
		TAG_END);
*/

	/* Do we have everything we need? */
//	if (file && srctype == DTST_FILE) {
		bm = ConvertFLIC(cl, o, 0,adf); //, whichpic, numpics);
//	}

	return bm;
}

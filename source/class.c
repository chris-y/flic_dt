/*
 * pnm.datatype - Chris Young
 */

#include "class.h"

#define RGB8to32(RGB) (((uint32)(RGB) << 24)|((uint32)(RGB) << 16)|((uint32)(RGB) << 8)|((uint32)(RGB)))


uint32 ClassDispatch (Class *cl, Object *o, Msg msg);

Class *initDTClass (struct ClassBase *libBase) {
	struct ExecIFace *IExec = libBase->IExec;
	Class *cl;
	SuperClassLib = IExec->OpenLibrary("datatypes/animation.datatype", 44);
	if (SuperClassLib) {
		cl = IIntuition->MakeClass(libBase->libNode.lib_Node.ln_Name, ANIMATIONDTCLASS, NULL, sizeof(struct flicinstdata), 0);
		if (cl) {
			cl->cl_Dispatcher.h_Entry = (HOOKFUNC)ClassDispatch;
			cl->cl_UserData = (uint32)libBase;
			IIntuition->AddClass(cl);
		} else {
			IExec->CloseLibrary(SuperClassLib);
		}
	}
	return cl;
}

BOOL freeDTClass (struct ClassBase *libBase, Class *cl) {
	struct ExecIFace *IExec = libBase->IExec;
	BOOL res = TRUE;
	if (cl) {
		res = IIntuition->FreeClass(cl);
		if (res) {
			IExec->CloseLibrary(SuperClassLib);
		}
	}
	return res;
}

static int32 WriteICO (Class *cl, Object *o, struct dtWrite *msg);
static int32 ConvertICO (Class *cl, Object *o, BPTR file,struct adtFrame *adf); //, uint32 index, uint32 *total);
static int32 GetICO (Class *cl, Object *o, struct TagItem *tags);
static struct BitMap *GetFrame(Class *, Object *, struct adtFrame *);

uint32 ClassDispatch (Class *cl, Object *o, Msg msg) {
	struct ClassBase *libBase = (struct ClassBase *)cl->cl_UserData;

	uint32 ret;

	switch (msg->MethodID) {

		case OM_NEW:
			ret = IIntuition->IDoSuperMethodA(cl, o, msg);
			if (ret) {
				int32 error;
				error = GetICO(cl, (Object *)ret, ((struct opSet *)msg)->ops_AttrList);
				if (error != OK) {
					IIntuition->ICoerceMethod(cl, (Object *)ret, OM_DISPOSE);
					ret = (uint32)NULL;
					IDOS->SetIoErr(error);
				}
			}
			break;

			case ADTM_LOADFRAME:
				ret = GetFrame(cl, o, (struct adtFrame *)msg);
				if(!ret) IDOS->SetIoErr(DTERROR_INVALID_DATA);
			break;

			case ADTM_UNLOADFRAME:
			{
				struct adtFrame *adf = (struct adtFrame *)msg;

				if(adf->alf_BitMap) IGraphics->FreeBitMap(adf->alf_BitMap);
				adf->alf_BitMap = NULL;
				if(adf->alf_CMap) IGraphics->FreeColorMap(adf->alf_CMap);
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

				if(prevframe->bm) IGraphics->FreeBitMap(prevframe->bm);
				prevframe->bm=NULL;
				if(prevframe->cmap) IGraphics->FreeColorMap(prevframe->cmap);
				prevframe->cmap=NULL;

				ret = IIntuition->IDoSuperMethodA(cl, o, msg);
			}
			break;

			/* fall through and let superclass deal with it */

		default:
			ret = IIntuition->IDoSuperMethodA(cl, o, msg);
			break;

	}

	return ret;
}

static int32 WriteICO (Class *cl, Object *o, struct dtWrite *msg) {
	return ERROR_NOT_IMPLEMENTED;
}

static int32 ConvertICO (Class *cl, Object *o, BPTR file,struct adtFrame *adf)
{
	struct ClassBase *libBase = (struct ClassBase *)cl->cl_UserData;
	struct ExecIFace *IExec = libBase->IExec;
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

	IDOS->Seek(file,0,OFFSET_BEGINNING);

	IDOS->Read(file,&head,sizeof(struct flcheader));		

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

		IDataTypes->SetDTAttrs(o, NULL, NULL,
			PDTA_NumColors,256,
			TAG_END);

		IDataTypes->GetDTAttrs(o,
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
		bm = IGraphics->AllocBitMap(read_le16(&head.width),read_le16(&head.height),read_le16(&head.depth),0,NULL);

		if(prevframe->bm)
		{
			IGraphics->FreeBitMap(prevframe->bm);
			prevframe->bm=NULL;
		}
		if(prevframe->cmap)
		{
			IGraphics->FreeColorMap(prevframe->cmap);
			prevframe->cmap=NULL;
		}
	}

	IGraphics->InitRastPort(&rp);
	IGraphics->InitRastPort(&temprp);
	rp.BitMap = bm;

	tempbm = IGraphics->AllocBitMap(read_le16(&head.width),1,read_le16(&head.depth),0,NULL);
	temprp.BitMap = tempbm;

	tempmem = IExec->AllocVec(read_le16(&head.width),MEMF_ANY);

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

	IDOS->Seek(file,filepos,OFFSET_BEGINNING);

	if((prevframe->frame) && (prevframe->frame < framereq))
	{
		filepos = prevframe->nextchunk;
		IDOS->Seek(file,filepos,OFFSET_BEGINNING);
		curframe = prevframe->frame;
#if 0
		curframe=0;

		while(curframe < prevframe->frame)
		{
			IDOS->Read(file,&chunk,sizeof(struct flcchunk));
			size = read_le32(&chunk.size);
			IDOS->Seek(file,size - sizeof(struct flcchunk),OFFSET_CURRENT);
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
		IDOS->Read(file,&chunk,sizeof(struct flcchunk));
		filepos2=sizeof(struct flcchunk);

		if(read_le16(&chunk.chunks))
		{
			uint32 size2;
			int i;

			for(i=0;i<read_le16(&chunk.chunks);i++)
			{
//				IDOS->ChangeFilePosition(file,filepos+filepos2,OFFSET_BEGINNING);
				IDOS->Read(file,&datachunk,6);

				size2 = read_le32(&datachunk.size); // - sizeof(struct flcdatchunk);
				filepos2+=size2;

			//	size = size - size2;

				IExec->DebugPrintF("[flic.datatype] CHUNK %ld SIZE %ld\n",read_le16(&datachunk.type),size2);

				switch(read_le16(&datachunk.type))
				{
					case 4:
					{
						uint8 skip,count,r,g,b;
						uint16 packets,count2;
						uint32 idx=0;

						IDOS->Read(file,&packets,sizeof(packets));

						if(adf)
						{
							if(!clrmap) clrmap=IGraphics->GetColorMap(256);
						}
						else
						{
							ocmap=cmap;
						}

						for(tmp=0;tmp<read_le16(&packets);tmp++)
						{
							uint16 i;

							IDOS->Read(file,&skip,1);

							if(adf)
							{
								idx+=skip;
							}
							else
							{
								ocmap+=skip;
							}

							IDOS->Read(file,&count,1);
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
								IDOS->Read(file,&r,1);
								IDOS->Read(file,&g,1);
								IDOS->Read(file,&b,1);

								if(adf)
								{
									IGraphics->SetRGB32CM(clrmap,idx,r<<24,g<<24,b<<24);
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

						IDOS->Read(file,&word,2);
						lines = read_le16(&word);
						y=0;
						for(i=0;i<lines;i++)
						{
							uint8 colskip,pixel;
							int8 ptype;
							uint32 j;

							x=0;

							IDOS->Read(file,&word,2);
							while(read_le16(&word) & 0xc000)
							{
								if((read_le16(&word) & 0xc000) == 0xc000)
								{
									y+=(0x10000-read_le16(&word));
								}
								if((read_le16(&word) & 0xc000) == 0x4000)
								{
									//put low order byte in last byte of line
									IGraphics->SetAPen(&rp,read_le16(&word) & 0x00ff);
									IGraphics->WritePixel(&rp,read_le16(&head.width)-1,y);
								}
								IDOS->Read(file,&word,2);
							}

							if((read_le16(&word) & 0xc000) == 0x0000)
							{
								if(read_le16(&word))
								{
									for(j=0;j<read_le16(&word);j++)
									{

										IDOS->Read(file,&colskip,1);
										x+=colskip;
										IDOS->Read(file,&ptype,1);

										if(ptype>0)
										{
											IDOS->Read(file,tempmem,ptype*2);
											IGraphics->WritePixelLine8(&rp,x,y,(ptype*2),tempmem,&temprp);
											x+=(ptype*2);
										}
										else if(ptype<0)
										{
											uint8 oldx,oldptype;
	
											IDOS->Read(file,tempmem,2);

											while(ptype<0)
											{
												IGraphics->WritePixelLine8(&rp,x,y,2,tempmem,&temprp);
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

						IDOS->Read(file,&packets,sizeof(packets));

						if(adf)
						{
							if(!clrmap) clrmap=IGraphics->GetColorMap(256);
						}
						else
						{
							ocmap=cmap;
						}

						for(tmp=0;tmp<read_le16(&packets);tmp++)
						{
							uint16 i;

							IDOS->Read(file,&skip,1);

							if(adf)
							{
								idx+=skip;
							}
							else
							{
								ocmap+=skip;
							}

							IDOS->Read(file,&count,1);
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
								IDOS->Read(file,&r,1);
								IDOS->Read(file,&g,1);
								IDOS->Read(file,&b,1);

								if(adf)
								{
									IGraphics->SetRGB32CM(clrmap,idx,r<<26,g<<26,b<<26);
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

						IDOS->Read(file,&word,2);
						y=read_le16(&word);

						IDOS->Read(file,&word,2);
						lines = read_le16(&word);

						for(i=0;i<lines;i++)
						{
							uint8 colskip,pixel,pkts;
							int8 ptype;
							uint32 j;

							x=0;

							IDOS->Read(file,&pkts,1);

							for(j=0;j<pkts;j++)
							{
								IDOS->Read(file,&colskip,1);
								x+=colskip;
								IDOS->Read(file,&ptype,1);

								if(ptype>0)
								{
									IDOS->Read(file,tempmem,ptype);
									IGraphics->WritePixelLine8(&rp,x,y,ptype,tempmem,&temprp);
									x+=ptype;
								}
								else if(ptype<0)
								{
									IDOS->Read(file,&pixel,1);
									IGraphics->SetAPen(&rp,pixel);
	
									while(ptype<0)
									{
										IGraphics->WritePixel(&rp,x,y);
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
						IGraphics->SetAPen(&rp,0);
						for(y=0;read_le16(&head.height);y++)
						{
							for(x=0;x<read_le16(&head.width);x++)
							{
								IGraphics->WritePixel(&rp,x,y);
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
								IDOS->Read(file,&packets,1);
							}

							IDOS->Read(file,&ptype,1);
							if(ptype<=0)
							{
								IDOS->Read(file,tempmem,-ptype);
								IGraphics->WritePixelLine8(&rp,x,y,-ptype,tempmem,&temprp);
								x-=ptype;
							}
							else
							{
								IDOS->Read(file,&pixel,1);
								IGraphics->SetAPen(&rp,pixel);
								while(ptype>0)
								{
									IGraphics->WritePixel(&rp,x,y);
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
							IDOS->Read(file,tempmem,read_le16(&head.width));
							IGraphics->WritePixelLine8(&rp,x,y,read_le16(&head.width),tempmem,&temprp);
						}
					break;

					default:
						IExec->DebugPrintF("[flic.datatype] Unsupported chunk %ld\n",read_le16(&datachunk.type));
						if(read_le16(&datachunk.type)>50)
						{
							IExec->DebugPrintF("[flic.datatype] FATAL ERROR\n");
							return 0;
						}
//						IDOS->ChangeFilePosition(file,size2,OFFSET_CURRENT);
					break;
				}
				if(IDOS->GetFilePosition(file) != (filepos+filepos2))
				{
					IExec->DebugPrintF("[flic.datatype] WARNING: File position mismatch (%lld should be %lld after chunk %ld)\n",IDOS->GetFilePosition(file),filepos+filepos2,read_le16(&datachunk.type));
					IDOS->Seek(file,filepos+filepos2,OFFSET_BEGINNING);
				}
			}
		}
		size = read_le32(&chunk.size); // - sizeof(struct flcchunk);
		filepos+=size; // NEXT chunk!
//IExec->DebugPrintF("filepos %lld (next chunk)\n",filepos);
		curframe++;
	}

	IGraphics->FreeBitMap(tempbm);
	IExec->FreeVec(tempmem);

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

		IDataTypes->SetDTAttrs(o, NULL, NULL,
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

		return error;
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
		prevframe->bm=IGraphics->AllocBitMap(read_le16(&head.width),read_le16(&head.height),read_le16(&head.depth),0,NULL);
		IGraphics->BltBitMap(bm,0,0,prevframe->bm,0,0,read_le16(&head.width),read_le16(&head.height),0x0c0,0xff,NULL);

		prevframe->cmap = IGraphics->GetColorMap(256);

		prevframe->nextchunk = filepos;

/*
		IGraphics->InitVPort(&vp);

		vp.ColorMap = prevframe->cmap;

		table[0] = 256<<16 + 0;
		table[258]=0;
*/
		IGraphics->GetRGB32(clrmap,0,256,&table);

		for(i=0;i<768;i+=3)
		{
			IGraphics->SetRGB32CM(prevframe->cmap,i/3,table[i],table[i+1],table[i+2]);
		}

		return bm;
	}
}

static int32 GetICO (Class *cl, Object *o, struct TagItem *tags) {
	struct ClassBase *libBase = (struct ClassBase *)cl->cl_UserData;
	struct BitMapHeader *bmh = NULL;
	char *filename;
	int32 srctype;
	int32 error = 0; //ERROR_OBJECT_NOT_FOUND;
	BPTR file = (BPTR)NULL;
	struct BitMap *bm;

//	filename = (char *)IUtility->GetTagData(DTA_Name, (uint32)"Untitled", tags);

	IDataTypes->GetDTAttrs(o,
		DTA_Handle,			&file,
		DTA_SourceType,		&srctype,
		DTA_Name,			&filename,
		TAG_END);

		IDataTypes->SetDTAttrs(o, NULL, NULL,
				DTA_ObjName,IDOS->FilePart(filename),
				TAG_DONE);

	/* Do we have everything we need? */
	if (file && srctype == DTST_FILE) {

		error = ConvertICO(cl, o, file,NULL); //, whichpic, numpics);
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
		bm = ConvertICO(cl, o, 0,adf); //, whichpic, numpics);
//	}

	return bm;
}

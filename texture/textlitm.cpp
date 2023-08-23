/*------------------------------------------------------------------------------ * WalkThrough™ - the real time 3-D CAD system. * Version 0.1 * * Copyright © 1989 by Virtus Corporation * All Rights Reserved * Written by  	David A. Smith *				David W. Easter *				Mark J. Uland * Suite 204 * 117 Edinburgh South * Cary, North Carolina    27511 * (919) 467-9700 *------------------------------------------------------------------------------ * TextureListItem.c *------------------------------------------------------------------------------ */ #include "VTypes.h"#include "TEXTLITM.h"#include "CADCNTRL.h" #include "WalkBuff.h" /* DWE */#include "Misc.h" /* DWE */#include "AppText.h"#include "Observer.h"#include "TEXTITEM.h"#include "VGraphic.h"#include "VGUtil.h"#include <stdio.h>#include <string.h>#define TEXT_START	8#define CELL_HEIGHT 36#define TEXTUREMENU_CHECK	18	// jam 1 Sept 94void TextureListItem::cListItem(char *str)	{	controller = NULL;	strcpy(name,str);	texture_item = NULL;	}	void TextureListItem::SetList(struct List *l)	{	list_pane = l;	}			void TextureListItem::dListItem()	{	EditItem::dListItem();	}	int TextureListItem::GetHeight(void){ return(CELL_HEIGHT);}void TextureListItem::SetController(Controller *cntrl) { controller = cntrl; }		void TextureListItem::GetName(char *str){ 	if (texture_item) 		texture_item->GetName(str);}TextureItem *TextureListItem::GetTextureItem() { return texture_item; }void TextureListItem::SetTextureItem(TextureItem *ti) { texture_item = ti; }/*----------------------------------------------------------------------------------*//* Draw() */voidTextureListItem::DrawRow(Rect *rr){//JAM PORTTOWINQQQRect r;RGBColor c;char str[256];#if MACINTOSHCIconHandle gLockIcon = GetCIcon(257);#elif WINDOWS//JAM PORTTOWINQQQ#endif//		PenNormal();		VSetPenNormal();	//JAM MERGE-VPRO-FORWIN		rr->left += (35);		VForeColor(&vWhite);//		PaintRect(rr);							/* !!! MAC !!! */        VPaintRect(rr);			//JAM MERGE-VPRO-FORWIN		VForeColor(&vBlack);		//		TextFont(geneva);						/* !!! MAC !!! *///		TextSize(9);							/* !!! MAC !!! */		VSetFont(vAppFont,vMediumFont,vNormal);     //JAM MERGE-VPRO-FORWIN		VForeColor(&vBlack);		VBackColor(&vWhite);				if (texture_item)		{		Rect r, r2;		char str1[256], str2[256];		#if MACINTOSH			texture_item->GetName(str);			C2Pstr(str);//			MoveTo(rr->left + 6, rr->bottom - 20);			VMoveTo(rr->left + 6, rr->bottom - 20);			//JAM MERGE-VPRO-FORWIN//			TextFace(bold);				/* !!! MAC !!! */			VSetFont(vAppFont,vMediumFont,vBold);     //JAM MERGE-VPRO-FORWIN			DrawString((StringPtr)str);			/* !!! */			texture_item->GetTypeStr(str1);			texture_item->GetInfoStr(str2);			strcat(str1, " ("); 			strcat(str1, str2); 			strcat(str1, ")"); 			C2Pstr(str1);//			MoveTo(rr->left + 6, rr->bottom - 8);			VMoveTo(rr->left + 6, rr->bottom - 8);  //JAM MERGE-VPRO-FORWIN			VSetFont(vAppFont,vMediumFont,vNormal);     //JAM MERGE-VPRO-FORWIN//			TextFace(0);				/* !!! */			DrawString((StringPtr)str1);			/* !!! */			SetRect(&r, rr->right - 16, rr->top + 17, rr->right, rr->top + 33);			if (texture_item->IsUsedInDocument())				PlotCIcon(&r, gLockIcon);#else // MACINTOSH			texture_item->GetName(str);			VMoveTo(rr->left + 6, rr->bottom - 20);			//JAM MERGE-VPRO-FORWIN			VSetFont(vAppFont,vMediumFont,vBold);     //JAM MERGE-VPRO-FORWIN			VSetRect(&r2, rr->left + 6, rr->top, rr->right - 16, rr->bottom - 20);			VDrawText((StringPtr)str,0,strlen(str),&r2);			texture_item->GetTypeStr(str1);			texture_item->GetInfoStr(str2);			strcat(str1, " ("); 			strcat(str1, str2); 			strcat(str1, ")"); 			VMoveTo(rr->left + 6, rr->bottom - 8);  //JAM MERGE-VPRO-FORWIN			VSetFont(vAppFont,vMediumFont,vNormal);     //JAM MERGE-VPRO-FORWIN			VSetRect(&r2, rr->left + 6, rr->bottom - 20, rr->right - 32, rr->bottom - 10);			VDrawText((StringPtr)str1,0,strlen(str1),&r2);			SetRect(&r, rr->right - 32, rr->top + 11, rr->right - 16, rr->top + 27);			if (texture_item->IsUsedInDocument())			{				//jam added to draw check 1 Sept 94				VSetFont(vButtonFont, vButtonFontSize, vNormal);				VTextMode(vSrcXor);				VDrawChar(TEXTUREMENU_CHECK, &r);    // try this char, Elvis.        		VTextMode(vSrcOr);				// this is done later VSetFont(vSystemFont, vSystemFontSize, vNormal);			   ;// dude	PlotCIcon(&r, gLockIcon);            }#endif // MACINTOSH		}		else		{#if MACINTOSH			sprintf(str, TXT_NOTEXTURE);			C2Pstr(str);//			MoveTo((rr->left + 6) - 32, rr->bottom - 14);			VMoveTo((rr->left + 6) - 32, rr->bottom - 14);	//JAM MERGE-VPRO-FORWIN//			TextFace(bold);				/* !!! MAC !!! */			VSetFont(vAppFont,vMediumFont,vBold);     //JAM MERGE-VPRO-FORWIN			DrawString((StringPtr)str);			/* !!! */#elif WINDOWS//JAM PORTTOWINQQQ			Rect txtR;			sprintf(str, TXT_NOTEXTURE);			#if WINDOWS			VTextMode(vSrcXor);			#endif //WINDOWS			txtR = *rr;//			txtR.left+=12;txtR.bottom=txtR.top+13;txtR.top=txtR.bottom-vSystemFontSize;//			texture_item->GetName(name);			VDrawText(str,0,strlen(str),&txtR);			#if WINDOWS			VTextMode(vSrcOr);			#endif //WINDOWS#endif		}		VSetFont(vSystemFont,vLargeFont,vNormal);     //JAM MERGE-VPRO-FORWIN//		TextFont(systemFont);						/* !!! *///		TextSize(12);							/* !!! */#if MACINTOSH		DisposCIcon(gLockIcon);			//* !!! */#elif WINDOWS//PORTTOWINQQQ#endif	}	/*----------------------------------------------------------------------------------*//* DrawRowTexture() */voidTextureListItem::DrawRowTexture(Rect *rr)	{	Rect r_from, r_to;	RGBColor c;	VCGrafPort tp;	VGrafDevice gd;	V_Offscreen *  thumbnail;			// Temp for thumbnail present test. GJR082392			if (texture_item == NULL) return;//		PenNormal();		VSetPenNormal();	//JAM MERGE-PRO-FORWIN		SetRect(&r_to, rr->left+2,				rr->top+2, 				rr->left+32+2,				rr->top+2+32);			thumbnail = texture_item->GetThumbnail();		// get out if none present GJR082893		if (!thumbnail) return;				r_from = thumbnail->rect;		VGetPort(&tp, &gd);		// memory may move, so reget thumbnail GJR082893#if MACINTOSH		texture_item->GetThumbnail()->MetaTo((V_Display*)list_pane->theWindow, &r_from, &r_to); #elif WINDOWS		texture_item->GetThumbnail()->MetaTo(list_pane, &r_from, &r_to);#endif		VSetPort(tp, gd);	}void TextureListItem::Hilite(int hiliteState)	{	Rect	r;		hilited = hiliteState;		r.left = 0;	r.right = 500; /* dude */	GetRect(&r);//	EraseRect(&r);	VEraseRect(&r);	//JAM MERGE-VPRO-FORWIN	Display();		if(hilited)		{		if (texture_item == NULL)		{        #if MACINTOSH		BitClr ((Ptr)0x938, pHiliteBit);			// ABD do a real hilite (0x938 == HiliteMode)        #endif // MACINTOSH		VInvertRect(&r);								// ABD		}		else 		{		GetRect(&r);		r.left = 0;		r.right = 2;		#if MACINTOSH		BitClr ((Ptr)0x938, pHiliteBit);			// ABD do a real hilite (0x938 == HiliteMode)        #endif // MACINTOSH		VInvertRect(&r);								// ABD		GetRect(&r);		r.left = 2;		r.right = 34;		r.bottom = r.top + 2;		#if MACINTOSH		BitClr ((Ptr)0x938, pHiliteBit);			// ABD do a real hilite (0x938 == HiliteMode)        #endif // MACINTOSH		VInvertRect(&r);								// ABD		GetRect(&r);		r.left = 2;		r.right = 34;		r.top = r.bottom - 2;		#if MACINTOSH		BitClr ((Ptr)0x938, pHiliteBit);			// ABD do a real hilite (0x938 == HiliteMode)        #endif // MACINTOSH		VInvertRect(&r);								// ABD		GetRect(&r);		r.left = 34;		r.right = 500;		#if MACINTOSH		BitClr ((Ptr)0x938, pHiliteBit);			// ABD do a real hilite (0x938 == HiliteMode)        #endif // MACINTOSH		VInvertRect(&r);								// ABD		}		}	}void TextureListItem::Display()	{	Rect	r, rp;	Point	p;	int		i;	char name[256];		VForeColor(&vBlack);#if MACINTOSH	PenPat(&black);#endif	r.left = list_pane->dataRect.left;	r.right = list_pane->dataRect.right;	r.bottom = location;	r.top = location - GetHeight(); 	rp.left = list_pane->paneRect.left;	rp.right = list_pane->paneRect.right;	rp.bottom = location;	rp.top = location - GetHeight(); 	DrawRow(&r);	DrawRowTexture(&rp);	return;#if MACINTOSH	VForeColor(&vBlack);	PenPat(&black);			VSetFont(vAppFont,vMediumFont,vBold);	//	MoveTo(TEXT_START+2,location-2);	VMoveTo(TEXT_START+2,location-2);	//JAM MERGE-VPRO-FORWIN	texture_item->GetName(name);	DrawText(name,0,strlen(name));	VSetFont(vSystemFont,vLargeFont,vNormal);	#else	//it's windows#endif //MACINTOSH	}#if 0void TextureListItem::Display()	{	Rect r;	GetRect(&r);	DrawRow(&r);	//DrawRowTexture(&r);	}	#endifvoid TextureListItem::GetRect(Rect *r)	{	r->top = location - CELL_HEIGHT;	r->bottom = location;	r->left = 0;	}
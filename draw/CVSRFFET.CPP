/*------------------------------------------------------------------------------ * Virtus WalkThrough_ * Version 1.1.1 * * Copyright _ 1990-1992 by Virtus Corporation * All Rights Reserved * Written by   *				 *				Mark J. Uland * Virtus Corporation * Suite 204 * 117 Edinburgh South * Cary, North Carolina    27511 * (919) 467-9700 *------------------------------------------------------------------------------ * Source File:  * Notes:  * History:  *------------------------------------------------------------------------------ */#include "VTypes.h"#include "CVSrfFet.h"#include "CVSrfDef.h"#include "CVSrfObj.h"#include "SrfObj.h"#include "CVPolyh.h"#include "CVDraw.h"#include "CVEnvirn.h"#include "CVLItem.h"#include "VMDL.h"#include "Export.h"#include "WalkBuff.h" /* DWE */#include "Position.h" #include "PolyDefs.h" #include "Misc.h"#include "Geometry.h"#include "VMath.h"#include "VGraphic.h"#include "VGUtil.h"#include <math.h> #include <string.h> voidCAVSurfFeature::cSurfFeature()	{	CAVlist = new(V_Array);	CAVlist->cArray(0,sizeof(CAVSurfObject*),4);	editType = EDIT_POLYGON;	surfFeatureObject = NULL;	localOutline = NULL;	layer = NULL;	picture = NULL;	SurfFeature::cSurfFeature();	}	voidCAVSurfFeature::dSurfFeature()	{	CAVSurfObject 	**cavObject;	int	i,length;		if(length = CAVlist->ALength())		{		cavObject = (CAVSurfObject 	**)CAVlist->Use();				for(i = 0; i < length; i++,cavObject++) 			{ if(NULL != *cavObject) (*cavObject)->dCAVObject(); }		CAVlist->Unuse();		}	CAVlist->dArray();#if MACINTOSH	if(picture)picture->dPicture();#elif WINDOWS	if(picture)picture->dOffscreen();#endif	SurfFeature::dSurfFeature();	}#if MACINTOSHint	CAVSurfFeature::SetPicture(struct V_Picture *pict) { picture = pict; }#elif WINDOWSint	CAVSurfFeature::SetPicture(struct V_Offscreen *pict) { picture = pict; }#endifvoid CAVSurfFeature::SetLayer(LayerItem *nlyr) { layer = nlyr; }voidCAVSurfFeature::Update()	{	CAVSurfObject 	**cavObject;	int	i,length;		SurfFeature::Update();	if(length = CAVlist->ALength())		{		cavObject = (CAVSurfObject 	**)CAVlist->Use();				for(i = 0; i < length; i++,cavObject++) 			{			if(NULL != *cavObject)				(*cavObject)->MakeOutl(posnPoints,FALSE,0,posnPoints->ALength(),1,0,FFZero,FFZero);			}					CAVlist->Unuse();		}	SetDirty(POLY_MUSSED);		}voidCAVSurfFeature::SetDirty(int state)	{	CAVSurfObject 	**cavObject;	Position	*polyPosn,*sdPosn;	Point3DFFixed	pt3D;	PointFFixed		*wPt,*lPt;	int	i,length;			if(length = CAVlist->ALength())		{		cavObject = (CAVSurfObject 	**)CAVlist->Use();		for(i = 0; i < length; i++,cavObject++) 			if(NULL !=(*cavObject)) (*cavObject)->SetDirty(state);					CAVlist->Unuse();		}		if(surfFeatureObject && surfDef)		{polyPosn = new(Position);polyPosn->cPosition();polyPosn->SetScale(surfDef->polyhedron->posn->GetScale());polyPosn->SetShear(surfDef->polyhedron->posn->GetShear());sdPosn = new(Position); /* DWE */sdPosn->cPosition();surfDef->position->Copy(sdPosn);sdPosn->SetOrigin(Pt3DFFZero);				length = outline->ALength();		localOutline->SetLength(length);		wPt = (PointFFixed *)outline->Use();		lPt = (PointFFixed *)localOutline->Use();		for(i = 0; i < length; i++,wPt++,lPt++)			{			*lPt = posn->Transform2D(*wPt);			pt3D.x = lPt->h;			pt3D.y = lPt->v;			pt3D.z = FFZero;			pt3D = sdPosn->Transform3D(pt3D); /* DWE */			pt3D = polyPosn->Transform3D(pt3D);			pt3D = sdPosn->UnTransform3D(pt3D); /* DWE */			lPt->h = pt3D.x;			lPt->v = pt3D.y;			}		localOutline->Unuse();		outline->Unuse();		polyPosn->dPosition();sdPosn->dPosition();				surfFeatureObject->SetDirty(state);		}	}	voidCAVSurfFeature::DisplayFeature(DisplayInfo	*info)	{	DisplayInfo	displayInfo;	CAVSurfObject *cavObject;	int oldEditType;		CAVlist->Get(info->environment->GetID(),&cavObject);		displayInfo.posnPoints = posnPoints;	displayInfo.editType = editType;	displayInfo.direction = info->direction;	displayInfo.picture = picture;	displayInfo.color = color;	if(COLOR_LAYER&info->layer->flagState)displayInfo.color = info->layer->color; 	else displayInfo.color = color;				if(!( EDIT_HIDDEN & editState ) )		{		oldEditType = info->editType;		info->editType = editType;		cavObject->DisplayFeature(&displayInfo);		info->editType = oldEditType;		}	}#if VRML_ANCHOR 	// ABD PRO25 AUG95CAVSurfFeature *CAVSurfFeature::FindNoSideEffects(Point *p,PointFFixed *wPt,long modifiers,int detailed,int findMode) 	{	CAVSurfFeature *val;	PointFFixed	pt1,pt2;	int rejectFlag,mark,nextMark,tempMode,length;		switch(findMode)		{		case FIND_HIDDEN: rejectFlag = (EDIT_HIDDEN&editState) ? FALSE : TRUE;break;		case FIND_LOCKED: rejectFlag = (EDIT_LOCKED&editState) ? FALSE : TRUE; break;		case FIND_NORMAL: rejectFlag = ((EDIT_HIDDEN|EDIT_LOCKED)&editState) ? TRUE : FALSE; break;		case FIND_ALL: rejectFlag = FALSE; break;		}		if(rejectFlag)			val = NULL;	else		{		tempMode = surfFeatureObject->FindNoSideEffects(p,modifiers,detailed);		switch(tempMode)			{			case ON_POLY_DEFLINE:				length = localOutline->ALength();				mark = surfFeatureObject->mark;								nextMark = nextIndex(mark,1,length);				val = this;								localOutline->Get(mark,&pt1);				localOutline->Get(nextMark,&pt2);								pt1= surfFeatureObject->environment->Translate2DFFixed(TO_CAD,pt1);				pt2= surfFeatureObject->environment->Translate2DFFixed(TO_CAD,pt2);								pt1 = FFPtToLine(*wPt,pt1,pt2);				wPt->h = FFAdd(pt1.h,wPt->h);				wPt->v = FFAdd(pt1.v,wPt->v);				break;								case ON_POLY_POINT: 				mark = surfFeatureObject->mark;								val = this;				localOutline->Get(mark,&pt1);				pt1= surfFeatureObject->environment->Translate2DFFixed(TO_CAD,pt1);				*wPt = pt1;				break;				case IN_POLY:				val = this;				break;							case OUT_POLY:			default:				val = NULL;				break;				}		}			return(val);	}#endif // VRML_ANCHORvoid CAVSurfFeature::SetEditState(int value)	{	editState = value;	}int CAVSurfFeature::GetEditState()	{	return editState;	}void CAVSurfFeature::AddCAVCtrl(CAVEnvironment	*environment)	{	CAVSurfObject	*newCAVObject = NULL;	int				id;		id = environment->GetID();	if((id = environment->GetID()) >= CAVlist->ALength()) 		while(id >= CAVlist->ALength()) 			CAVlist->Append(&newCAVObject);	newCAVObject = new(CAVSurfObject);	newCAVObject->cCAVObject();	newCAVObject->SetEnvironment(environment);				CAVlist->Put(id,&newCAVObject);	newCAVObject->MakeOutl(posnPoints,0,0,posnPoints->ALength(),INFL_EXTRUDE,0,FFZero,FFZero);	}void CAVSurfFeature::RemoveCAVCtrl(CAVEnvironment	*environment)	{	CAVSurfObject		*cavObject;			CAVlist->Get(environment->GetID(),&cavObject);	cavObject->dCAVObject();	cavObject = NULL;	CAVlist->Put(environment->GetID(),&cavObject);	}	/*=========================================================================*	void CAVSurfFeature::SetEditType(int newKind)*	void CAVSurfFeature::SetMode(int newMode)*	int  CAVSurfFeature::GetMode()*	int  CAVSurfFeature::GetSide()*	int  CAVSurfFeature::Visible()*	Point  CAVSurfFeature::GetAnchor()*	int  CAVSurfFeature::GetMark()*-------------------------------------------------------------------------*/void CAVSurfFeature::SetEditType(int newKind)   {  editType = newKind;  }void CAVSurfFeature::SetMode(int newMode)   {  surfFeatureObject->SetMode(newMode);  }int  CAVSurfFeature::GetMode()   {  return(surfFeatureObject->GetMode());  }void  CAVSurfFeature::SetSide(int side)   {  surfFeatureObject->SetSide(side);  }int  CAVSurfFeature::GetSide()   {  return(surfFeatureObject->GetSide());  }Point  CAVSurfFeature::GetAnchor(int doScaleThing)   {  return(surfFeatureObject->GetAnchor(doScaleThing));  }int  CAVSurfFeature::GetMark()   {  return(surfFeatureObject->GetMark());  }/*=========================================================================*	void CAVSurfFeature::ScaleSurf(Point anchorPt,PointFFixed mult,long modifiers)  *	void CAVSurfFeature::BreakLine(Point lastPt,long modifiers) *	int  CAVSurfFeature::MoveSurf(Point anchorPt,Point lastPt,FFixed lastAngle,long modifiers)*	int  CAVSurfFeature::RotateSurf(Point anchorPt,FFixed lastAngle,long modifiers) *-------------------------------------------------------------------------*/int CAVSurfFeature::ScaleSurf(Point anchorPt,PointFFixed mult,long modifiers)   	{  return(surfFeatureObject->ScaleSurf(anchorPt,mult,modifiers));  }int CAVSurfFeature::BreakLine(Point lastPt,long modifiers)   	{  return(surfFeatureObject->BreakLine(lastPt));  }int CAVSurfFeature::MoveSurf(Point anchorPt,Point lastPt,FFixed lastAngle,long modifiers)   	{  return(surfFeatureObject->MoveSurf(anchorPt,lastPt,lastAngle,modifiers));  }int CAVSurfFeature::RotateSurf(Point anchorPt,FFixed lastAngle,long modifiers)   	{  return(surfFeatureObject->RotateSurf(anchorPt,lastAngle,modifiers));  }	voidCAVSurfFeature::CalcOutline(PointFFixed *sMin,PointFFixed *sMax)  	{	PointFFixed *defPt,pt;	int			i,length;		*sMin = PtFFMax;	*sMax = PtFFMin;		length = outline->ALength();	defPt = (PointFFixed *)outline->Use();	for(i = 0; i < length; i++,defPt++)		{		pt = posn->Transform2D(*defPt);		FFMin(sMin->h,sMin->h,pt.h);		FFMin(sMin->v,sMin->v,pt.v);		FFMax(sMax->h,sMax->h,pt.h);		FFMax(sMax->v,sMax->v,pt.v);		}		outline->Unuse();	}	voidCAVSurfFeature::Extrema(PointFFixed *sMin,PointFFixed *sMax)  	{	PointFFixed *pt;	int			i,length;		*sMin = PtFFMax;	*sMax = PtFFMin;	length = localOutline->ALength();	pt = (PointFFixed *)localOutline->Use();	for(i = 0; i < length; i++,pt++)		{		FFMin(sMin->h,sMin->h,pt->h);		FFMin(sMin->v,sMin->v,pt->v);		FFMax(sMax->h,sMax->h,pt->h);		FFMax(sMax->v,sMax->v,pt->v);		}		localOutline->Unuse();	}			/* DWE ... */intCAVSurfFeature::Export(ModelExport *export, int where, int connected)	{	RGBAColor clr;	export->Feature(this);	clr.red = color.red;	clr.green = color.green;	clr.blue = color.blue;	clr.alpha = opacity;	export->OpenFeature(posnPoints, clr, where, connected);	export->CloseFeature();	}/* ... DWE */voidCAVSurfFeature::cSurfObject(CAVEnvironment *env)	{	if(!surfFeatureObject)		{		surfFeatureObject = new(SurfObject);		surfFeatureObject->cSurfObject();		surfFeatureObject->SetSurfFeature(this);		surfFeatureObject->SetEnvironment(env);		localOutline = new(V_Array);		localOutline->cArray(0,sizeof(PointFFixed),4);		}	}voidCAVSurfFeature::dSurfObject()	{	surfFeatureObject->dSurfObject();	surfFeatureObject = NULL;	localOutline->dArray();	localOutline = NULL;	}void  CAVSurfFeature::Display()   	{  	if(!( EDIT_HIDDEN & editState ) )		surfFeatureObject->Display((EDIT_LOCKED&editState)?TRUE:FALSE); 	}void  CAVSurfFeature::Select()  {  surfFeatureObject->Select();  }int  CAVSurfFeature::SelectRect(Rect *r,int findMode)   	{  	int result,rejectFlag;	switch(findMode)		{		case FIND_HIDDEN: rejectFlag = (EDIT_HIDDEN&editState) ? FALSE : TRUE;break;		case FIND_LOCKED: rejectFlag = (EDIT_LOCKED&editState) ? FALSE : TRUE; break;		case FIND_NORMAL: rejectFlag = ((EDIT_HIDDEN|EDIT_LOCKED)&editState) ? TRUE : FALSE; break;		case FIND_ALL: rejectFlag = FALSE; break;		}	if(!rejectFlag) result = surfFeatureObject->SelectRect(r);	else result = FALSE;		return(result); 	}CAVSurfFeature *CAVSurfFeature::Find(Point *p,PointFFixed *wPt,long modifiers,int detailed,int findMode) 	{	CAVSurfFeature *val;	PointFFixed	pt1,pt2;	int rejectFlag,mark,nextMark,tempMode,length;		switch(findMode)		{		case FIND_HIDDEN: rejectFlag = (EDIT_HIDDEN&editState) ? FALSE : TRUE;break;		case FIND_LOCKED: rejectFlag = (EDIT_LOCKED&editState) ? FALSE : TRUE; break;		case FIND_NORMAL: rejectFlag = ((EDIT_HIDDEN|EDIT_LOCKED)&editState) ? TRUE : FALSE; break;		case FIND_ALL: rejectFlag = FALSE; break;		}		if(rejectFlag)			val = NULL;	else		{		tempMode = surfFeatureObject->Find(p,modifiers,detailed);		switch(tempMode)			{			case ON_POLY_DEFLINE:				length = localOutline->ALength();				mark = surfFeatureObject->mark;								nextMark = nextIndex(mark,1,length);				val = this;								localOutline->Get(mark,&pt1);				localOutline->Get(nextMark,&pt2);								pt1= surfFeatureObject->environment->Translate2DFFixed(TO_CAD,pt1);				pt2= surfFeatureObject->environment->Translate2DFFixed(TO_CAD,pt2);								pt1 = FFPtToLine(*wPt,pt1,pt2);				wPt->h = FFAdd(pt1.h,wPt->h);				wPt->v = FFAdd(pt1.v,wPt->v);				break;								case ON_POLY_POINT: 				mark = surfFeatureObject->mark;								val = this;				localOutline->Get(mark,&pt1);				pt1= surfFeatureObject->environment->Translate2DFFixed(TO_CAD,pt1);				*wPt = pt1;				break;				case IN_POLY:				val = this;				break;							case OUT_POLY:			default:				val = NULL;				break;				}		}			return(val);	}intCAVSurfFeature::NewMove(PointFFixed offset) 	{	PointFFixed	origin;/*		if(surfDef)		{		offset = posn->UnTransform2D(offset);		origin = posn->UnTransform2D(PtFFZero);		offset = FF2DSub(offset,origin);		}*/			Move(offset);		return(0);	}	intCAVSurfFeature::NewMoveCorner(PointFFixed outlinePt,int whichCorner) 	{	PointFFixed	origin,actualPt,prevPt,nextPt;	FFixed 		angle;	double		ang1,ang2;	int			next,prev,swapLoc;		outlinePt = posn->UnTransform2D(outlinePt);	origin = posn->UnTransform2D(PtFFZero);	outlinePt = FF2DSub(outlinePt,origin);	switch(editType)		{		case EDIT_POLYGON:			MovePoint(whichCorner,outlinePt);			break;		case EDIT_RECTANGLE:			whichCorner = NewOutline(outlinePt,whichCorner);			Update();			break;					case EDIT_REGPOLYGON:			whichCorner = NewOutline(outlinePt,whichCorner);			Update();			break;		}		return(whichCorner);	}intCAVSurfFeature::NewMoveSide(PointFFixed outlinePt,int whichWall) 	{	PointFFixed	coord1,coord2;	PointFFixed	origin;	double	x,y,radius,angl;	int 	newWall,next,length;		outlinePt = posn->UnTransform2D(outlinePt);	origin = posn->UnTransform2D(PtFFZero);	outlinePt = FF2DSub(outlinePt,origin);		switch(editType)		{		case EDIT_POLYGON:			MoveLine(whichWall,outlinePt);			break;		case EDIT_REGPOLYGON:	length = outline->ALength();	next = nextIndex(whichWall,1,length);	outline->Get(whichWall,&coord1);	outline->Get(next,&coord2);	coord2.h = FFAdd(FFShift(FFSub(coord1.h,coord2.h),-1),coord2.h);	coord2.v = FFAdd(FFShift(FFSub(coord1.v,coord2.v),-1),coord2.v);	coord2.h = FFAdd(outlinePt.h,coord2.h);	coord2.v = FFAdd(outlinePt.v,coord2.v);	x = FixedToDbl(coord2.h);	y = FixedToDbl(coord2.v);	radius = sqrt(x*x+y*y);		x = FixedToDbl(coord1.h);	y = FixedToDbl(coord1.v);    if(x==0 && y==0)angl=0;	else	angl = atan2(y,x);	coord1.h=DblToFixed(radius*cos(angl));	coord1.v=DblToFixed(radius*sin(angl));	outline->Put(whichWall,&coord1);	outlinePt = PtFFZero;			whichWall = NewOutline(outlinePt,whichWall);			Update();			break;		case EDIT_RECTANGLE:			if(0 == whichWall || 2 == whichWall)  outlinePt.v = FFZero; 			else  outlinePt.h = FFZero; 			newWall = NewOutline(outlinePt,whichWall);			if(newWall!=whichWall)whichWall = prevIndex(newWall,1,outline->ALength());			Update();			break;		}			return(whichWall);	}intCAVSurfFeature::NewAddPoint(PointFFixed outlinePt,int whichCorner) 	{		outlinePt = posn->UnTransform2D(outlinePt);		switch(editType)		{		case EDIT_RECTANGLE:			SetEditType(EDIT_POLYGON);					case EDIT_POLYGON:			AddPoint(whichCorner + 1,outlinePt);			break;								case EDIT_REGPOLYGON:			AddPoint(whichCorner + 1,outlinePt);			whichCorner = NewOutline(PtFFZero,whichCorner);			Update();			break;		}	return(whichCorner);	}/*=========================================================================*	int CAVSurfFeature::NewRemovePoint(PointFFixed *outlinePt,int whichCorner)*-------------------------------------------------------------------------*/int CAVSurfFeature::NewRemovePoint(PointFFixed *outlinePt,int whichCorner) 	{		if(3 > (outline->ALength()-1)) return(-1);		switch(editType)		{		case EDIT_RECTANGLE:			SetEditType(EDIT_POLYGON);		case EDIT_POLYGON:			outline->Get(whichCorner,outlinePt);			*outlinePt = posn->Transform2D(*outlinePt);			RemovePoint(whichCorner);			break;					case EDIT_REGPOLYGON:			RemovePoint(whichCorner);			whichCorner = NewOutline(PtFFZero,0);			Update();			break;		}			return(whichCorner);	}	int CAVSurfFeature::NewOutline(PointFFixed outlinePt,int whichCorner) 	{	PointFFixed		prevPt,nextPt,coord;	int		next,prev,swapLoc,length;		switch(editType)		{		case EDIT_POLYGON: break;					case EDIT_REGPOLYGON:			outline->Get(whichCorner, &coord);			outlinePt = FF2DAdd(coord,outlinePt);			BuildRegular(&outline,PtFFZero,outlinePt,outline->ALength());			whichCorner = 0;			break;					case EDIT_RECTANGLE: 			length = outline->ALength();			next = nextIndex(whichCorner,1,length);			prev = prevIndex(whichCorner,1,length);			prevPt = nextPt = outlinePt;			nextPt.h = prevPt.v = FFZero;			if(0 == whichCorner || 2 == whichCorner) 				{ swapLoc = prev; prev = next; next = swapLoc; }			outline->Get(whichCorner, &coord);			coord.h = FFAdd(coord.h, outlinePt.h);			coord.v = FFAdd(coord.v, outlinePt.v);			outline->Put(whichCorner, &coord);			outline->Get(next, &coord);			coord.h = FFAdd(coord.h, nextPt.h);			coord.v = FFAdd(coord.v, nextPt.v);			outline->Put(next, &coord);			outline->Get(prev, &coord);			coord.h = FFAdd(coord.h, prevPt.h);			coord.v = FFAdd(coord.v, prevPt.v);			outline->Put(prev, &coord);			break;		}	if (!PolyClockwise(outline))		{		whichCorner = outline->ALength() - whichCorner - 1;		outline->Reverse();		}			return(whichCorner);	}
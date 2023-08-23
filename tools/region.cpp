#include "VTypes.h"#include "WTypes.h"#include "Region.h"#include "VArray.h"#include "VMath.h"voidV_Region::cRegion(struct V_Array *thePoints)	{#if MACINTOSH	PointFFixed	*points;	int		loop, length;		length = thePoints->ALength();	points = (PointFFixed *)thePoints->Use();		maskRegion = NewRgn();	allRegion = NewRgn();		OpenRgn();	MoveTo(FFRound(points[length-1].h), FFRound(points[length-1].v));	for (loop = 0; loop < length; loop ++)		LineTo(FFRound(points[loop].h), FFRound(points[loop].v));	CloseRgn(maskRegion);	CopyRgn(maskRegion, allRegion);		thePoints->Unuse();#elif WINDOWS	V_Array		*shortPoints;	short		length, loop;	POINT		*ourPoints;	PointFFixed	*points;	shortPoints = new(V_Array);	shortPoints->cArray(0, sizeof(POINT), 4);	length = thePoints->ALength();	points = (PointFFixed *)thePoints->Use();	for (loop = 0; loop < length; loop++)		{		POINT	p;		p.x = FFRound(points[loop].h);		p.y = FFRound(points[loop].v);        shortPoints->Append(&p);		}	thePoints->Unuse();	ourPoints = (POINT*)shortPoints->Use();	maskRegion = CreatePolygonRgn(ourPoints, length, WINDING);	allRegion = CreatePolygonRgn(ourPoints, length, WINDING);	shortPoints->Unuse();	shortPoints->dArray();#endif //MACINTOSH	}		intV_Region::AddCompliment(struct V_Array *thePoints)	{#if MACINTOSH	RgnHandle	thisRegion;	PointFFixed	*points;	int		loop, length, ret;		if (EmptyRgn(allRegion)) return (FALSE);	length = thePoints->ALength();	points = (PointFFixed *)thePoints->Use();		thisRegion = NewRgn();	OpenRgn();	MoveTo(FFRound(points[length-1].h), FFRound(points[length-1].v));	for (loop = 0; loop < length; loop ++)		LineTo(FFRound(points[loop].h), FFRound(points[loop].v));	CloseRgn(thisRegion);		thePoints->Unuse();	SectRgn(maskRegion, thisRegion, thisRegion);	ret = !(EmptyRgn(thisRegion));		if (ret)		{		DiffRgn(allRegion, thisRegion, thisRegion);		if (EqualRgn(thisRegion, allRegion))			ret = FALSE;		else			{			CopyRgn(thisRegion, allRegion);			ret = TRUE;			}		}		DisposeRgn(thisRegion);	return (ret);#elif WINDOWS	int			ret;	HRGN		newRgn, resultRgn;	V_Array		*shortPoints;	short		length, loop;	POINT		*ourPoints;	PointFFixed	*points;	shortPoints = new(V_Array);	shortPoints->cArray(0, sizeof(POINT), 4);	length = thePoints->ALength();	points = (PointFFixed *)thePoints->Use();	for (loop = 0; loop < length; loop++)		{		POINT	p;		p.x = FFRound(points[loop].h);		p.y = FFRound(points[loop].v);        shortPoints->Append(&p);		}	thePoints->Unuse();	ourPoints = (POINT*)shortPoints->Use();	newRgn = CreatePolygonRgn(ourPoints, length, WINDING);	shortPoints->Unuse();    shortPoints->dArray();	resultRgn = CreateRectRgn(0,0,0,0);	if (NULLREGION == CombineRgn(resultRgn, maskRegion, newRgn, RGN_AND))		{		DeleteObject(newRgn);		DeleteObject(resultRgn);		ret = FALSE;		}	else		{		DeleteObject(newRgn);		newRgn = resultRgn;		resultRgn = CreateRectRgn(0,0,0,0);		CombineRgn(resultRgn, allRegion, newRgn, RGN_DIFF);		if (EqualRgn(allRegion, resultRgn))			ret = FALSE;		else			ret = TRUE;		DeleteObject(allRegion);		DeleteObject(newRgn);		allRegion = resultRgn;        }    return ret;#endif //MACINTOSH	}		intV_Region::Compliment(void)	{#if MACINTOSH	return (!EmptyRgn(allRegion));#elif WINDOWS	int	ret;	HRGN nullRgn;	nullRgn = CreateRectRgn(0,0,0,0);	if (NULLREGION == CombineRgn(nullRgn,allRegion, NULL, RGN_COPY))		ret = FALSE;	else ret = TRUE;	DeleteObject(nullRgn);    return ret;#endif //MACINTOSH	}voidV_Region::dRegion(void)	{#if MACINTOSH	DisposeRgn(allRegion);	DisposeRgn(maskRegion);#elif WINDOWS	DeleteObject(maskRegion);	DeleteObject(allRegion);#endif //MACINTOSH		delete(this);	}
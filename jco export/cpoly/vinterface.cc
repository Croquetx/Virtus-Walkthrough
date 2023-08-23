#include	"poly.h"#include	"nclip.h"#include "VArray.h"#include "COPLANAR.h"#include "interface.h"#include <stdio.h> // debugvoid PLClear(PolyPList &l);void PLClear(PolyPList &l){	PolyPListIter	i(l);	while(i())		delete i.val();}V_Array *CoPlanarSet::clipPolyPoly(shardPoly *ain, shardPoly *bin, TransfNOAP *to2d, TransfNOAP *to3d){	PolyPList	a_min_b, b_min_a, a_and_b;	Point3Ddouble *pptr,pnt, p2d, p3d;	Poly *cpoly;	clipPoint p;	int i,nverts,nclipped=0;	V_Array *clipped=NULL,*verts=NULL;	shardPoly newpoly;	int vi;	// get 1st point to start polygon	nverts = ain->pgon->ALength();	ain->pgon->Get( 0, &vi );	pptr = (Point3Ddouble *) vertices->GetAddress( vi );	// need to project into 2-d    to2d->TransP3D( pptr, &p2d );	p.x() = p2d.x; p.y() = p2d.y; 	Poly	*a = new Poly (p);// get remaining points	for (pptr++,i=1;i<nverts;i++) {		ain->pgon->Get( i, &vi );		pptr = (Point3Ddouble *) vertices->GetAddress( vi );				to2d->TransP3D( pptr, &p2d );		p.x() = p2d.x; p.y() = p2d.y; 		a->add(p);	}// 1st point for polygon "b"	nverts = bin->pgon->ALength();	bin->pgon->Get( 0, &vi );	pptr = (Point3Ddouble *) vertices->GetAddress( vi );			to2d->TransP3D( pptr, &p2d );	p.x() = p2d.x; p.y() = p2d.y; 	Poly	*b = new Poly (p);// remaining points	for (pptr++,i=1;i<nverts;i++) {		bin->pgon->Get( i, &vi );		pptr = (Point3Ddouble *) vertices->GetAddress( vi );				to2d->TransP3D( pptr, &p2d );		p.x() = p2d.x; p.y() = p2d.y; 		b->add(p);	}	bin->pgon->Unuse();	// some models may come with degenerate polygons	nverts = b->trim_slivers(); 	if (nverts < 3) {		delete a;		delete b;		return NULL;	};	nverts = a->trim_slivers();	if (nverts < 3) {	// a is degenerate, return the trimmed version of b		PolyIter	viter(*b);		delete a;		clipped = new(V_Array);		clipped->cArray(0, sizeof(shardPoly), 1);		if (!clipped) return (NULL);		verts = new(V_Array);		if (!verts) return (NULL);		verts->cArray(0, sizeof(int), 4);				while(viter()) { // need to go back to 3d			p2d.x = viter.node()->point().x();			p2d.y = viter.node()->point().y();			p2d.z = 0.0;			to3d->TransP3D( &p2d, &pnt );			vi = vertIndex( pnt );			verts->Append( &vi );		}		newpoly.pgon = verts;		newpoly.clr = bin->clr;		newpoly.planeIndex = bin->planeIndex;		clipped->Append( &newpoly );		delete b;		return clipped;	};		clip_poly( *a, *b, a_min_b, b_min_a, a_and_b );	delete	a;	delete	b;	// change to v_array of jcopolys	clipped = new(V_Array);	clipped->cArray(0, sizeof(shardPoly), 4);	if (!clipped) return (NULL); // need some error handling		PolyPListIter iter( b_min_a );	while (iter()) {		cpoly = iter.val();		nverts = 0;		if (fabs(cpoly->area()) > .2)  { // don't add degenerate polygons			verts = new(V_Array);			if (!verts) return (NULL);			verts->cArray(0, sizeof(int), 4);			nverts = cpoly->trim_slivers();			if (nverts > 2) {				PolyIter	viter(*cpoly);								while(viter()) { // need to go back to 3d					p2d.x = viter.node()->point().x();					p2d.y = viter.node()->point().y();					p2d.z = 0.0;					to3d->TransP3D( &p2d, &pnt );					vi = vertIndex( pnt );					verts->Append( &vi );				}				newpoly.pgon = verts;				newpoly.clr = bin->clr;				newpoly.planeIndex = bin->planeIndex;				clipped->Append( &newpoly );				nclipped++;			}			else {				verts->dArray();			}		}	};// get rid of sets	PLClear(a_min_b);	PLClear(b_min_a);	PLClear(a_and_b);	if (nclipped > 0) {		return clipped;	}	else {		clipped->dArray();		return NULL;	}}int CoPlanarSet::clumpPolyPoly(shardPoly *ain, shardPoly *bin, TransfNOAP *to2d, TransfNOAP *to3d, shardPoly *pout){	PolyPList	a_or_b;	Point3Ddouble *pptr,pnt, p2d, p3d;	Poly *cpoly;	clipPoint p;	int i,nverts;	V_Array *verts=NULL;	int vi;	    // get 1st point to start polygon	nverts = ain->pgon->ALength();	ain->pgon->Get( 0, &vi );	pptr = (Point3Ddouble *) vertices->GetAddress( vi );	// need to project into 2-d    to2d->TransP3D( pptr, &p2d );	p.x() = p2d.x; p.y() = p2d.y; 	Poly	*a = new Poly (p);// get remaining points	for (pptr++,i=1;i<nverts;i++) {		ain->pgon->Get( i, &vi );		pptr = (Point3Ddouble *) vertices->GetAddress( vi );				to2d->TransP3D( pptr, &p2d );		p.x() = p2d.x; p.y() = p2d.y; 		a->add(p);	}// 1st point for polygon "b"	nverts = bin->pgon->ALength();	bin->pgon->Get( 0, &vi );	pptr = (Point3Ddouble *) vertices->GetAddress( vi );			to2d->TransP3D( pptr, &p2d );	p.x() = p2d.x; p.y() = p2d.y; 	Poly	*b = new Poly (p);// remaining points	for (pptr++,i=1;i<nverts;i++) {		bin->pgon->Get( i, &vi );		pptr = (Point3Ddouble *) vertices->GetAddress( vi );				to2d->TransP3D( pptr, &p2d );		p.x() = p2d.x; p.y() = p2d.y; 		b->add(p);	}	bin->pgon->Unuse();	clump_poly( *a, *b, a_or_b );	delete	a;	delete	b;// count result -- only return a value if the two intersected.	int npolys = 0;	PolyPListIter piter( a_or_b );	while (piter()) npolys++;		if (npolys == 1) {	// change to jcopoly		PolyPListIter iter( a_or_b );		while (iter()) {			cpoly = iter.val();			nverts = 0;			verts = new(V_Array);			if (!verts) return (NULL);			verts->cArray(0, sizeof(int), 4);			PolyIter	viter(*cpoly);			while(viter()) { // need to go back to 3d				p2d.x = viter.node()->point().x();				p2d.y = viter.node()->point().y();				p2d.z = 0.0;				to3d->TransP3D( &p2d, &pnt );				vi = vertIndex( pnt );				verts->Append( &vi );			}			pout->pgon = verts;			pout->clr = bin->clr;			pout->planeIndex = bin->planeIndex;		};	};// get rid of sets	PLClear(a_or_b);	return (npolys == 1);}V_Array *CoPlanarSet::convexConnectivity(shardPoly *ain, TransfNOAP *to2d){	Point3Ddouble *pptr,pnt, p2d, p3d;	int i,nverts,idx;	short vi;	V_Array *rval, *verts;	clipPoint p;	nverts = ain->pgon->ALength();	if (nverts < 3) return NULL;		rval = new(V_Array);	rval->cArray(0, sizeof(V_Array *), 4);		ain->pgon->Get( 0, &idx );	pptr = (Point3Ddouble *) vertices->GetAddress( idx );// need to project into 2-d    to2d->TransP3D( pptr, &p2d );	p.x() = p2d.x; p.y() = p2d.y; 	Poly	*a = new Poly (p);	for (pptr++,i=1;i<nverts;i++) {		ain->pgon->Get( i, &idx );		pptr = (Point3Ddouble *) vertices->GetAddress( idx );		to2d->TransP3D( pptr, &p2d );		p.x() = p2d.x; p.y() = p2d.y; 		a->add(p);	}		ain->pgon->Unuse();	if (a->convex()) {		verts = new(V_Array);		verts->cArray(0, sizeof(short), nverts);		for (vi=0;vi<nverts;vi++) verts->Append( &vi );		rval->Append( &verts );	}	else {		// compose convex connectivity from triangles					double **vertices;		int **triangles;		int ntris,iv,Reverse;						vertices = (double **) malloc( (nverts+1)*sizeof(double *));		PolyIter viter(*a);		// the triangulation algorithm uses "anti-clockwise" order		if (a->orientation() == ClockWise) {			Reverse = TRUE;		}		else {			Reverse = FALSE;		}		for (int i=0;i<nverts+1;i++) {	    	viter();	    	iv = i;	    	if (Reverse) iv = nverts-i;		    vertices[iv] = (double *) malloc(2*sizeof(double));		    if (!vertices[iv]) {		    	// uh-oh		    	fprintf(stderr,"out of memory converting clipped polygons to convex...\n");		    	return (0);		    }		    if (iv>0) {		    	vertices[iv][0] = viter.node()->point().x();		    	vertices[iv][1] = viter.node()->point().y();		    }		}				triangles = (int **) malloc( (nverts-2)*sizeof(int *));		for (i=0;i<nverts-2;i++) triangles[i] = (int *) malloc(3*sizeof(int));						ntris = triangulate_polygon( 1, &nverts, vertices, triangles );		for (i=0;i<ntris;i++) {			verts = new(V_Array);			verts->cArray(0, sizeof(short), 3);			for (int j=0;j<3;j++) {				vi = triangles[i][j] - 1;				if (Reverse) vi = nverts - 1 - vi;				verts->Append( &vi );			}			free(triangles[i]);  //free up triangle memory			rval->Append( &verts );		}		free(triangles);		for (int i=0;i<nverts+1;i++) free(vertices[i]);		free(vertices);	}	delete	a;	return rval;}
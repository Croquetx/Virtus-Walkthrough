#ifndef POLYHEDRON#define POLYHEDRON#include "WTypes.h"#include "VIO.h"#include "VIEWCOLL.H"	// ABD COLLISION DETECTION 8/13/93/* Fields of 'type' */#define POLY_CLASS		0xff00		/* general type class */#define POLY_TYPE		0x00ff		/* specific type within class *//* Classes */#define P_INFLATE		0x0100		/* 2D outline + "inflation" into 3D */#define P_GROUP			0x0200		/* group */#define P_MESH			0x0300		/* described by surface list */#define P_INTERNAL		0xff00		/* special types for internal use only *//* Extrude/converge types */#define POLY_INFLATE	P_INFLATE	/* arbitrary (non-complex) polyhedron */#define POLY_GROUP		P_GROUP		/* group node */#define POLY_MESH		P_MESH		/* described by surface list *//* Internal types */#define P_INVALID		0x0001		/* no valid definition applied */#define P_ROOT			0x0002		/* "world" polyhedron at root of poly tree */#define POLY_INVALID	(P_INTERNAL | P_INVALID)#define POLY_ROOT		(P_INTERNAL | P_ROOT)/* 2D to 3D inflation orientations */#define INFL_X			1#define INFL_Y			2#define INFL_Z			3/* 2D to 3D inflation types */#define INFL_EXTRUDE	1			/* extrusion */#define INFL_CONVERGE	2			/* convergence */#define INFL_DUALCONVERGE 3			/* two-pointed convergence */#define INFL_ELLIPSE	4			/* elliptical convergence */#define INFL_DUALELLIPSE 5			/* two-pointed elliptical convergence *//* Values for 'editType' */#define EDIT_POLYGON	1#define EDIT_RECTANGLE	2#define EDIT_REGPOLYGON	3/* Surface side ID values for color, feature placement *//* Must be numbered 0 to 2 so that 2-ID gets opposite side */#define SURF_INSIDE		0#define SURF_SHARED		1#define SURF_OUTSIDE	2#define SURF_OPPOSITE(s) (2 - (s))/* Definitions related to surface opacity */#define TRANSPARENT_NOM		0x0000		/* Nominal value for transparent */#define TRANSLUCENT_NOM		0x8000		/* Nominal value for translucent */#define OPAQUE_NOM			0xFFFF		/* Nominal value for opaque */#define TRANSL_THRESH		0x1000		/* Threshold between transaparency and translucency */#define OPAQUE_THRESH		0xF000		/* Threshold between translucency and opacity */#define IS_TRANSPARENT(o)		((o) < TRANSL_THRESH)#define IS_TRANSLUCENT(o)		((o) >= TRANSL_THRESH && (o) < OPAQUE_THRESH)#define IS_OPAQUE(o)			((o) >= OPAQUE_THRESH)#define PNT_INSIDE			1#define PNT_SURFACE			2#define PNT_OUTSIDE			3typedef struct ModelInfo	{	long nPolyhedra;		/* Number of polyhedra */	long   nPolyRoot;		/*    number that are root polyhedra */	long   nPolyGroup;		/*    number that are groups */	long   nPolyInflate;	/*    number that are "inflated" */	long   nPolyMesh;		/*    number that mesh objects */	long nSurfaces;			/* Number of surfaces */	long   nSurfCreate;		/*    number from base creation */	long   nSurfSlice;		/*    number from slices */	long nFeatures;			/* Number of surface features */	} ModelInfo;struct Polyhedron:V_IO	{	unsigned short type;	struct WalkDrawing *drawing; /* the drawing */	struct Polyhedron *container; /* polyhedron containing this one */	struct V_Array *contents;	/* (Polyhedron *) polyhedra contained by this one */	struct LightModel *lights;	/* light sources */	struct Unit *unit;		struct V_Array *name;	struct V_Array *data;	short hidden;	/* definition data */	short editType;	char defAxis;				/* orientation from which this poly's defined */	char inflate;				/* 2D to 3D inflation method */	short levels;	FFixed point, base;			/* point end and base end extrusion points */	PointFFixed pointSkew;		/* 2D offset of point end */	PointFFixed baseSkew;		/* 2D offset of base end */	struct V_Array *outline;	/* (PointFFixed) 2D outline */	struct V_Array *meshPoints;	/* (Point3DFFixed) point list for mesh type */	struct V_Array *meshSurfs;	/* (V_Array *) surfaces for mesh type */	struct V_Array *slices;		/* (Slice *) slices applied to poly */	RGBColor insideColor;		/* base color */	RGBColor outsideColor;	unsigned short opacity;	struct V_Array *surfDef;	/* (SurfDef *) definition data for surfaces */	unsigned char insideShadeType;	/* ABD MERGE-GOURAUD flat or gouraud shaded */	unsigned char outsideShadeType;	/* ABD MERGE-GOURAUD flat or gouraud shaded */// ABD MERGE-TEXTURE 9/13/93 [[[	struct TextureItem *insideTexture;		/* ABD TEXTURE base inside texture */	struct TextureItem *outsideTexture;		/* ABD TEXTURE base outside texture */	// note that the texture points (u,v) coords are stored in the surfaces	struct V_Array *NoPerspPoints;	/* JAM TEXTURE (Point3DNoPersp) array same as rendpoints except*/									/* contains the value (.perspecD) that multiplied */									/* the points to obtain perspective projection*/// ABD MERGE-TEXTURE 9/13/93 ]]]	/* positioning information */	struct Position *rPosn;		/* position relative to container */	struct Position *posn;		/* absolute position in world space */	/* 3D data derived from definition */	Point3D boundMin, boundMax;	/* bounding box of polyhedron */	Point3D posnBoundMin, posnBoundMax; /* bounding box of positioned polyhedron */	struct V_Array *points;		/* (Point3DFFixed) vertex points from definition */	struct V_Array *posnPoints;	/* (Point3DFFixed) vertex points after positioning */	struct V_Array *pieces;		/* (PolyConvex *) convex component polyhedra */	struct PolyConvex *piece;	/* convex component polyhedron (iff one piece) */	/* data for 3D rendering */	struct V_Array *rendPoints;	/* (Point3DFast) posnPoints after display calc */		#if VRML_ANCHOR			// VRML-ABD 23JUN95	struct V_Array *vrml_anchor;	#endif // VRML_ANCHOR	/* --- */	virtual void cPolyhedron(struct WalkDrawing *);	virtual void dPolyhedron(void);	void KillContents(void);	void AddPiece(struct PolyConvex *);	void KillPieces(void);	void KillSurfPieces(void);	void Install(struct Polyhedron *);	void Remove(struct Polyhedron *);	void InstallMulti(struct V_Array *);	void RemoveMulti(struct V_Array *);	void TreeInfo(struct ModelInfo *);	void NodeInfo(struct ModelInfo *, int, int);	long TreeSize(int);	void SetName(char *, int);	int GetName(char *, int);	struct V_Array *GetNameArray(void);	void SetData(char *, int);	int GetData(char *, int);	struct V_Array *GetDataArray(void);	void SetHidden(int);	int GetHidden(void);	void CreateRoot(void);	void CreateGroup(void);	void CreateInflate(int, int, struct V_Array *, FFixed, FFixed);	void CreateMesh(struct V_Array *, struct V_Array *);	void ChangeInflate(int, int);	void InflExtrude(int);	void InflDual(int);	void InflLevels(int);// ABD MERGE-TEXTURE 9/13/93 [[[	void SetTexture(struct TextureItem *, int);		// ABD TEXTURE	void SetTextureIO(struct TextureItem *, int);		// ABD TEXTURE	struct TextureItem *GetTexture(int);		// ABD TEXTURE	void GetTextureName(int where, char *name);		// PRO25	void CalcTextureMapping(void);		// ABD TEXTURE	void UpdateBound(void);// ABD MERGE-TEXTURE 9/13/93 ]]]// ABD MERGE-GOURAUD [[[	unsigned char GetShadeType(short);	void SetShadeType(unsigned char,short);// ABD MERGE-GOURAUD ]]]		#if VRML_ANCHOR				// VRML-ABD 23JUN95	void SetVRMLAnchor(char *new_anchor);	struct V_Array * GetVRMLAnchor(void);	int HasVRMLAnchor(void);	int IO_VRAN(struct V_Buffer *buf);	#endif // VRML_ANCHOR	void Connect(int, struct Polyhedron *, int);	void Disconnect(int, struct Polyhedron *, int, int);	void DisconnectNot(int, struct V_Array *, int);	int Connected(struct Polyhedron *, int);	void FixConnections(void);	void SetLightModel(struct LightModel *);	struct LightModel *GetLightModel(void);	struct LightModel *FindLightModel(void);	void SetUnit(struct Unit *);	struct Unit *GetUnit(void);	void RemoveUnit(void);	void ChangeUnit(FFixed);	void SetColor(RGBColor, unsigned short, int);	RGBColor GetColor(int);	unsigned short GetOpacity(void);	void Move(Point3DFFixed);	void MoveTo(Point3DFFixed);	void Rotate(Angle3DFFixed, Point3DFFixed);	void RotateTo(Angle3DFFixed);	void Scale(Point3DFFixed, Point3DFFixed);	void ScaleTo(Point3DFFixed);	void AddSlice(struct Slice *);	void RemoveSlice(struct Slice *);	void MoveEnds(FFixed, FFixed);	void SkewEnds(PointFFixed, PointFFixed);	void Invert(void);	void SetEditType(int);	int GetEditType(void);	void SetLevels(int);	void SetAxis(int);	int GetAxis(void);	void AddPoint(int, PointFFixed);	void RemovePoint(int);	void AddPoints(int);	void RemovePoints(int);	void MovePoint(int, PointFFixed);	void MoveLine(int, PointFFixed);	struct SurfDef *FindSurfDef(int);	struct SurfDef *GetSurfDef(int);	int Inside(Point3D);	struct PolyConvex *InsideContents(Point3D, struct V_Array *);	virtual void Update(void);	virtual void UpdateMove(void);	void CalcShade(void);	void Build(void);	void BuildEPoints(struct BuildInfo *);	void BuildCPoints(struct BuildInfo *);	void BuildLayer(Point3DFFixed *, PointFFixed, FFixed, FFixed);	void BuildPieces(struct BuildInfo *);	void SliceX(void);	void KillNullSurfs(void);	void PositionX(void);	void UpdateSurf(void);		void CalcGroupBound(void);	int SurfCount(void);	int SurfCreateCount(void);	int SurfSliceCount(void);	int SurfEndCount(void);	int SurfSideCount(void);	int LevelCount(void);	int EndSurf(int);	int LineToSurf(int, int);	int SliceToSurf(int);	Point3DFFixed SurfNormal(int);	int PolyValid(void);	void SliceArray(void);	void SurfDefArray(int);	void InstallConvex(struct Polyhedron *);	void RemoveConvex(struct Polyhedron *);	void CompareClose(struct Polyhedron *);	void RemoveClose(struct Polyhedron *);	void TestClose(struct Polyhedron *);	void KillClose(void);	int IsPolySkewed(void); 		// [[ jca 24MAR94 T/F routines to indicate the state of this polyhedron	int IsPolyScaled(void);         	int IsPolyRotated(void);	int GetExtrusionType(void);	int IsPolyPointed(void);	int IsPolyRounded(void);	int IsPolyStraight(void);	int CanLevelsBeChanged(void);	int CanSidesBeChanged(void);    // ]] jca 24MAR94	int IsPolyGrouped(void);	struct Polyhedron *FindContainer(void);	struct Polyhedron *FindGroup(void); // DAS-VR 9/22/93 Find a containing group	struct Surface *FindSurface(int);	Point3DFFixed OutlToWorld(PointFFixed, FFixed);	PointFFixed WorldToOutl(Point3DFFixed);	virtual struct Slice *NewSlice(void);	virtual struct SurfDef *NewSurfDef(void);	int IORoot(struct V_Buffer *);	int IOGroup(struct V_Buffer *);	int IO(struct V_Buffer *);	int ReadState(long, short *, struct V_Buffer *);	int IO_Contents(struct V_Buffer *);	int IO_PRSM(struct V_Buffer *, struct V_Array **, int);	int IO_CONN(struct V_Buffer *, struct V_Array *);	int IO_NAME(struct V_Buffer *);	int IO_DATA(struct V_Buffer *);	int IO_UNIT(struct V_Buffer *);	int IO_LGHT(struct V_Buffer *);	int IO_COLR(struct V_Buffer *);	int IO_PLTX(struct V_Buffer *);	// ABD MERGE-TEXTURE 9/13/93	int IO_PLGR(struct V_Buffer *);	// ABD MERGE-GOURAUD	int IO_POLY(struct V_Buffer *);	int IO_MESH(struct V_Buffer *);	int IO_SLIC(struct V_Buffer *);	int IO_SurfDefs(struct V_Buffer *);	int IO_SURF(struct V_Buffer *);	int IO_POSN(struct V_Buffer *);	virtual int IO_ReadExtra(struct V_Buffer *, long);	virtual int IO_WriteFirst(struct V_Buffer *);	virtual int IO_WriteLast(struct V_Buffer *);	};#endif //POLYHEDRON
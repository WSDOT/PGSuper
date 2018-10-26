///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#ifndef INCLUDED_IFACE_ALIGNMENT_H_
#define INCLUDED_IFACE_ALIGNMENT_H_

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-1999
   Washington State Department Of Transportation
*****************************************************************************/

// SYSTEM INCLUDES
//
#include <WBFLTypes.h>
#include <WBFLCogo.h>


// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IHorzCurve;
interface IVertCurve;
interface IDirection;
interface IPoint2d;

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   IRoadway

   Interface to get alignment information.

DESCRIPTION
   Interface to get alignment information.
*****************************************************************************/
// {AB9CDAA6-022D-11d3-8926-006097C68A9C}
DEFINE_GUID(IID_IRoadway, 
0xab9cdaa6, 0x22d, 0x11d3, 0x89, 0x26, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IRoadway : IUnknown
{
   // Gets the station, elevation, profile grade and coordinate of the start point of the alignment. The start point
   // is assumed to be at lessor station of the 1/n of the first span length before the first pier and
   // the least station of any alignment, profile, or cross section entry. The actually alignment has
   // infinite length, this method gives a reasonable starting point when a finite
   // alignment length is needed.
   virtual void GetStartPoint(Float64 n,Float64* pStartStation,Float64* pStartElevation,Float64* pGrade,IPoint2d** ppPoint) = 0;

   // Gets the station, elevation, profile grade and coordinate of the end point of the alignment. The end point
   // is assumed to be at greater station of the 1/n of the last span length after the last pier and
   // the greatest station of any alignment, profile, or cross section entry. The actually alignment has
   // infinite length, this method gives a reasonable starting point when a finite
   // alignment length is needed.
   virtual void GetEndPoint(Float64 n,Float64* pEndStation,Float64* pEndElevation,Float64* pGrade,IPoint2d** ppPoint) = 0;

   // Returns the cross slope
   virtual Float64 GetSlope(Float64 station,Float64 offset) = 0;

   virtual Float64 GetProfileGrade(Float64 station) = 0;
   virtual Float64 GetElevation(Float64 station,Float64 offset) = 0;
   virtual void GetBearing(Float64 station,IDirection** ppBearing) = 0;
   virtual void GetBearingNormal(Float64 station,IDirection** ppNormal) = 0;
   virtual void GetPoint(Float64 station,Float64 offset,IDirection* pBearing,IPoint2d** ppPoint) = 0;
   virtual void GetStationAndOffset(IPoint2d* point,Float64* pStation,Float64* pOffset) = 0;
   virtual CollectionIndexType GetCurveCount() = 0;
   virtual void GetCurve(CollectionIndexType idx,IHorzCurve** ppCurve) = 0;
   virtual CollectionIndexType GetVertCurveCount() = 0;
   virtual void GetVertCurve(CollectionIndexType idx,IVertCurve** ppCurve) = 0;

   virtual void GetRoadwaySurface(Float64 station,IDirection* pDirection,IPoint2dCollection** ppPoints) = 0;

   virtual Float64 GetCrownPointOffset(Float64 station) = 0;
};


/*****************************************************************************
INTERFACE
   IRoadway

   Interface to get alignment information.

DESCRIPTION
   Interface to get alignment information.
*****************************************************************************/
// {F013DA5F-708F-461b-9905-8BA164A436FA}
DEFINE_GUID(IID_IGeometry, 
0xf013da5f, 0x708f, 0x461b, 0x99, 0x5, 0x8b, 0xa1, 0x64, 0xa4, 0x36, 0xfa);
interface IGeometry : IUnknown
{
   // Measure
   virtual HRESULT Angle(IPoint2d* from,IPoint2d* vertex,IPoint2d* to,IAngle** angle) = 0;
   virtual HRESULT Area(IPoint2dCollection* points,Float64* area) = 0;
   virtual HRESULT Distance(IPoint2d* from,IPoint2d* to,Float64* dist) = 0;
   virtual HRESULT Direction(IPoint2d* from,IPoint2d* to,IDirection** dir) = 0;
   virtual HRESULT Inverse(IPoint2d* from,IPoint2d* to,Float64* dist,IDirection** dir) = 0;

   // Locate
   virtual HRESULT ByDistAngle(IPoint2d* from,IPoint2d* to,Float64 dist,VARIANT varAngle,Float64 offset,IPoint2d** point) = 0;
   virtual HRESULT ByDistDefAngle(IPoint2d* from,IPoint2d* to,Float64 dist,VARIANT varDefAngle,Float64 offset,IPoint2d** point) = 0;
   virtual HRESULT ByDistDir(IPoint2d* from,Float64 dist,VARIANT varDir,Float64 offset,IPoint2d** point) = 0;
   virtual HRESULT PointOnLine(IPoint2d* from,IPoint2d* to,Float64 dist,Float64 offset,IPoint2d** point) = 0;
   virtual HRESULT ParallelLineByPoints(IPoint2d* from,IPoint2d* to,Float64 offset,IPoint2d** p1,IPoint2d** p2) = 0;
   virtual HRESULT ParallelLineSegment(ILineSegment2d* ls,Float64 offset,ILineSegment2d** linesegment) = 0;

   // Intersect
   virtual HRESULT Bearings(IPoint2d* p1,VARIANT varDir1,Float64 offset1,IPoint2d* p2,VARIANT varDir2,Float64 offset2,IPoint2d** point) = 0;
   virtual HRESULT BearingCircle(IPoint2d* p1,VARIANT varDir,Float64 offset,IPoint2d* center,Float64 radius,IPoint2d* nearest,IPoint2d** point) = 0;
   virtual HRESULT Circles(IPoint2d* p1,Float64 r1,IPoint2d* p2,Float64 r2,IPoint2d* nearest,IPoint2d** point) = 0;
   virtual HRESULT LineByPointsCircle(IPoint2d* p1,IPoint2d* p2,Float64 offset,IPoint2d* center,Float64 radius,IPoint2d* nearest,IPoint2d** point) = 0;
   virtual HRESULT LinesByPoints(IPoint2d* p11,IPoint2d* p12,Float64 offset1,IPoint2d* p21,IPoint2d* p22,Float64 offset2,IPoint2d** point) = 0;
   virtual HRESULT Lines(ILineSegment2d* l1,Float64 offset1,ILineSegment2d* l2,Float64 offset2,IPoint2d** point) = 0;
   virtual HRESULT LineSegmentCircle(ILineSegment2d* pSeg,Float64 offset,IPoint2d* center,Float64 radius,IPoint2d* nearest, IPoint2d** point) = 0;

   // Project
   virtual HRESULT PointOnLineByPoints(IPoint2d* pnt,IPoint2d* start,IPoint2d* end,Float64 offset,IPoint2d** point) = 0;
   virtual HRESULT PointOnLineSegment(IPoint2d* from,ILineSegment2d* seg,Float64 offset,IPoint2d** point) = 0;
   virtual HRESULT PointOnCurve(IPoint2d* pnt,IHorzCurve* curve,IPoint2d** point) = 0;

   // Divide
   virtual HRESULT Arc(IPoint2d* from, IPoint2d* vertex, IPoint2d* to,CollectionIndexType nParts,IPoint2dCollection** points) = 0;
   virtual HRESULT BetweenPoints(IPoint2d* from, IPoint2d* to,CollectionIndexType nParts,IPoint2dCollection** points) = 0;
   virtual HRESULT LineSegment(ILineSegment2d* seg,CollectionIndexType nParts,IPoint2dCollection** points) = 0;
	virtual HRESULT HorzCurve(IHorzCurve* curve, CollectionIndexType nParts, IPoint2dCollection** points) = 0;
   virtual HRESULT Path(IPath* pPath,CollectionIndexType nParts,Float64 start,Float64 end,IPoint2dCollection** points) = 0;

   // Tangent
   virtual HRESULT External(IPoint2d* center1, Float64 radius1,IPoint2d* center2,Float64 radius2,TangentSignType sign, IPoint2d** t1,IPoint2d** t2) = 0;
   virtual HRESULT Cross(IPoint2d* center1, Float64 radius1,IPoint2d* center2, Float64 radius2, TangentSignType sign, IPoint2d** t1,IPoint2d** t2) = 0;
   virtual HRESULT Point(IPoint2d* center, Float64 radius,IPoint2d* point, TangentSignType sign, IPoint2d** tangent) = 0;
};

#endif // INCLUDED_IFACE_ALIGNMENT_H_
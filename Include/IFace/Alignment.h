///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
   virtual Float64 GetCrownPointOffset(Float64 station) = 0;
   virtual Float64 GetCrownSlope(Float64 station,Float64 offset) = 0;
   virtual void GetCrownSlope(Float64 station,Float64* pLeftSlope,Float64* pRightSlope) = 0;
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

   virtual void GetCrownPoint(Float64 station,IDirection* dir,IPoint2d** ppPoint) = 0;
   virtual void GetCrownPoint(Float64 station,IDirection* dir,IPoint3d** ppPoint) = 0;
};

#endif // INCLUDED_IFACE_ALIGNMENT_H_


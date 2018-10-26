///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifndef INCLUDED_IFACE_GIRDERHANDLINGPOINTSOFINTEREST_H_
#define INCLUDED_IFACE_GIRDERHANDLINGPOINTSOFINTEREST_H_

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// SYSTEM INCLUDES
//
#if !defined INCLUDED_WBFLTYPES_H_
#include <WbflTypes.h>
#endif

#if !defined INCLUDED_PGSUPERTYPES_H_
#include <PGSuperTypes.h>
#endif

#if !defined INCLUDED_PGSEXT_POINTOFINTEREST_H_
#include <PgsExt\PointOfInterest.h>
#endif

// PROJECT INCLUDES
//
#if !defined INCLUDED_IFACE_BRIDGE_H_
#include <IFace\Bridge.h>
#endif

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//


/*****************************************************************************
INTERFACE
   IGirderLiftingPointsOfInterest

   Interface to points of interest for lifting

DESCRIPTION
   Interface to points of interest for lifting
*****************************************************************************/
// non-COM version
interface IGirderLiftingDesignPointsOfInterest
{
   // locations of points of interest
   virtual std::vector<pgsPointOfInterest> GetLiftingDesignPointsOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 overhang,PoiAttributeType attrib,Uint32 mode = POIFIND_AND) = 0;
};

// {19EA189E-E5F4-11d2-AD3D-00105A9AF985}
DEFINE_GUID(IID_IGirderLiftingPointsOfInterest, 
0x19ea189e, 0xe5f4, 0x11d2, 0xad, 0x3d, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface IGirderLiftingPointsOfInterest : IUnknown, IGirderLiftingDesignPointsOfInterest
{
   virtual std::vector<pgsPointOfInterest> GetLiftingPointsOfInterest(SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,Uint32 mode = POIFIND_AND) = 0;
};

/*****************************************************************************
INTERFACE
   IGirderHaulingPointsOfInterest

   Interface to points of interest for Hauling

DESCRIPTION
   Interface to points of interest for Hauling
*****************************************************************************/
// non-COM version
interface IGirderHaulingDesignPointsOfInterest
{
   // locations of points of interest
   virtual std::vector<pgsPointOfInterest> GetHaulingDesignPointsOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType attrib,Uint32 mode = POIFIND_AND) = 0;
};

// {E6A0E250-E5F4-11d2-AD3D-00105A9AF985}
DEFINE_GUID(IID_IGirderHaulingPointsOfInterest, 
0xe6a0e250, 0xe5f4, 0x11d2, 0xad, 0x3d, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface IGirderHaulingPointsOfInterest : IUnknown, IGirderHaulingDesignPointsOfInterest
{
   // locations of points of interest
   virtual std::vector<pgsPointOfInterest> GetHaulingPointsOfInterest(SpanIndexType span,GirderIndexType gdr,PoiAttributeType attrib,Uint32 mode = POIFIND_AND) = 0;
   virtual Float64 GetMinimumOverhang(SpanIndexType span,GirderIndexType gdr) = 0;
};

#endif // INCLUDED_IFACE_GIRDERHANDLINGPOINTSOFINTEREST_H_


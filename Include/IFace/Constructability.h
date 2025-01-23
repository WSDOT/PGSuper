///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_CONSTRUCTABILITY_H_
#define INCLUDED_IFACE_CONSTRUCTABILITY_H_

/*****************************************************************************
   This group of interfaces provides access to constructability information.
   For this first version only the "A" Dimension will be supported.  Future
   versions will add interfaces to check the extensions of stirrups, girder
   dimensions, tolerances, etc.
*****************************************************************************/

// SYSTEM INCLUDES
//
#if !defined INCLUDED_WBFLTYPES_H_
#include <WbflTypes.h>
#endif

#if !defined INCLUDED_PGSUPERTYPES_H_
#include <PGSuperTypes.h>
#endif

// PROJECT INCLUDES
//
#include <Details.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   IGirderHaunch

DESCRIPTION
   Interface to get the Girder Haunch details
*****************************************************************************/
// {4F3FEB86-88B1-11d2-8882-006097C68A9C}
DEFINE_GUID(IID_IGirderHaunch, 
0x4f3feb86, 0x88b1, 0x11d2, 0x88, 0x82, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IGirderHaunch : IUnknown
{
   virtual Float64 GetRequiredSlabOffset(const CSegmentKey& segmentKey) const = 0;
   virtual const SLABOFFSETDETAILS& GetSlabOffsetDetails(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetSectionGirderOrientationEffect(const pgsPointOfInterest& poi) const = 0;
};


/*****************************************************************************
INTERFACE
   IFabricationOptimization

DESCRIPTION
   Interface to get the Fabrication Optimization details
*****************************************************************************/
// {459F156F-A371-4cfa-B325-9EEA2FA90F61}
DEFINE_GUID(IID_IFabricationOptimization, 
0x459f156f, 0xa371, 0x4cfa, 0xb3, 0x25, 0x9e, 0xea, 0x2f, 0xa9, 0xf, 0x61);
interface IFabricationOptimization : IUnknown
{
   /// Returns true if successful. There are a number of different input types and settings that prevent fabrication optimization analysis
   /// to occur. When these conditions are true, the analysis cannot be performed and this function returns false
   virtual bool GetFabricationOptimizationDetails(const CSegmentKey& segmentKey,FABRICATIONOPTIMIZATIONDETAILS* pDetails) const = 0;
};

#endif // INCLUDED_IFACE_CONSTRUCTABILITY_H_


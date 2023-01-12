///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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


#ifndef INCLUDED_IFACE_GIRDERHANDLING_H_
#define INCLUDED_IFACE_GIRDERHANDLING_H_

// SYSTEM INCLUDES
//
#if !defined INCLUDED_WBFLTYPES_H_
#include <WbflTypes.h>
#endif

#if !defined INCLUDED_PGSUPERTYPES_H_
#include <PGSuperTypes.h>
#endif

#if !defined INCLUDED_DETAILS_H_
#include <Details.h>
#endif

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   ISegmentLifting

   Interface to product-level girder handling information

DESCRIPTION
   Interface to girder handling information
*****************************************************************************/
// {E53A3DB2-DD61-11d2-AD34-00105A9AF985}
DEFINE_GUID(IID_ISegmentLifting, 
0xe53a3db2, 0xdd61, 0x11d2, 0xad, 0x34, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface ISegmentLifting : IUnknown
{
   // location of lifting loop measured from end of girder
   virtual Float64 GetLeftLiftingLoopLocation(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetRightLiftingLoopLocation(const CSegmentKey& segmentKey) const = 0;
   virtual void SetLiftingLoopLocations(const CSegmentKey& segmentKey, Float64 left,Float64 right) = 0;
};


/*****************************************************************************
INTERFACE
   ISegmentHauling

   Interface to product-level girder handling information

DESCRIPTION
   Interface to girder handling information
*****************************************************************************/
// {1D543E66-DD7E-11d2-AD34-00105A9AF985}
DEFINE_GUID(IID_ISegmentHauling, 
0x1d543e66, 0xdd7e, 0x11d2, 0xad, 0x34, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface ISegmentHauling : IUnknown
{
   // location of truck support location measured from end of girder
   virtual Float64 GetLeadingOverhang(const CSegmentKey& segmentKey) const = 0;
   virtual Float64 GetTrailingOverhang(const CSegmentKey& segmentKey) const = 0;
   virtual void SetTruckSupportLocations(const CSegmentKey& segmentKey, Float64 leading,Float64 trailing) = 0;
   virtual LPCTSTR GetHaulTruck(const CSegmentKey& segmentKey) const = 0;
   virtual void SetHaulTruck(const CSegmentKey& segmentKey,LPCTSTR lpszHaulTruck) = 0;
};

#endif // INCLUDED_IFACE_GIRDERHANDLING_H_


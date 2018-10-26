///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_TRANSVERSEREINFORCEMENTSPEC_H_
#define INCLUDED_IFACE_TRANSVERSEREINFORCEMENTSPEC_H_

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

#include <PGSuperTypes.h>

// PROJECT INCLUDES
//
#if !defined INCLUDED_MATERIAL_REBAR_H_
#include <Material\Rebar.h>
#endif
// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   ITransverseReinforcementSpec

   Interface to allowable prestressing strand stresses.

DESCRIPTION
   Interface to allowable prestressing strand stresses.
*****************************************************************************/
// {1AE10C6E-AC04-11d2-ACB7-00105A9AF985}
DEFINE_GUID(IID_ITransverseReinforcementSpec, 
0x1ae10c6e, 0xac04, 0x11d2, 0xac, 0xb7, 0x0, 0x10, 0x5a, 0x9a, 0xf9, 0x85);
interface ITransverseReinforcementSpec : IUnknown
{
   //------------------------------------------------------------------------
   // Calculates the maximum stress in the Splitting zone per 5.10.10.1
   virtual Float64 GetMaxSplittingStress(Float64 fyRebar)=0;

   //------------------------------------------------------------------------
   // Returns the distance from the ends of the girder within which the Splitting
   // stress requirements must be checked. 5.10.10.1
   virtual Float64 GetSplittingZoneLength( Float64 girderHeight )=0;

   //--------------------------------------------------------------
   // Returns N in h/N where the Splitting zone length is computed as h/N
   // See LRFD 5.10.10.1
   virtual Float64 GetSplittingZoneLengthFactor() = 0;

   //------------------------------------------------------------------------
   // Returns the minimum bar size in the confinment zone per 5.10.10.2
   virtual matRebar::Size GetMinConfinmentBarSize()=0;

   //------------------------------------------------------------------------
   // Returns the minimum bar area in the confinment zone per 5.10.10.2
   virtual Float64 GetMaxConfinmentBarSpacing()=0;

   //------------------------------------------------------------------------
   // Returns the minimum Av/S in the confinment zone per 5.10.10.2
   virtual Float64 GetMinConfinmentAvS()=0;

   //------------------------------------------------------------------------
   // Returns max barspacing for Vu over and under limits per 5.8.2.7
   virtual void GetMaxStirrupSpacing(Float64* sUnderLimit, Float64* sOverLimit)=0;

   //------------------------------------------------------------------------
   // Returns min barspacing for stirrups
   virtual Float64 GetMinStirrupSpacing(Float64 maxAggregateSize, Float64 barDiameter)=0;
};

#endif // INCLUDED_IFACE_TRANSVERSEREINFORCEMENTSPEC_H_


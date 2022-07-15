///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include <PGSuperTypes.h>

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
   // Returns the minimum bar size in the confinment zone per 5.9.4.4.2 (pre2017: 5.10.10.2)
   virtual WBFL::Materials::Rebar::Size GetMinConfinmentBarSize() const = 0;

   //------------------------------------------------------------------------
   // Returns the minimum bar area in the confinment zone per 5.9.4.4.2 (pre2017: 5.10.10.2)
   virtual Float64 GetMaxConfinmentBarSpacing() const = 0;

   //------------------------------------------------------------------------
   // Returns the minimum Av/S in the confinment zone per 5.9.4.4.2 (pre2017: 5.10.10.2)
   virtual Float64 GetMinConfinmentAvS() const = 0;

   //------------------------------------------------------------------------
   // Returns max bar spacing for vu over and under limits per 5.7.2.6 (pre2017: 5.8.2.7)
   virtual void GetMaxStirrupSpacing(Float64 dv,Float64* sUnderLimit, Float64* sOverLimit) const = 0;

   //------------------------------------------------------------------------
   // Returns min barspacing for stirrups
   virtual Float64 GetMinStirrupSpacing(Float64 maxAggregateSize, Float64 barDiameter) const = 0;
};

#endif // INCLUDED_IFACE_TRANSVERSEREINFORCEMENTSPEC_H_


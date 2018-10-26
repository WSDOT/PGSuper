///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_MOMENTCAPACITY_H_
#define INCLUDED_IFACE_MOMENTCAPACITY_H_

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

#if !defined INCLUDED_DETAILS_H_
#include <Details.h>
#endif

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsPointOfInterest;

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   IMomentCapacity

   Interface to moment capacity information

DESCRIPTION
   Interface to moment capacity information
*****************************************************************************/
// {B376BD5E-7FF7-11d2-885C-006097C68A9C}
DEFINE_GUID(IID_IMomentCapacity, 
0xb376bd5e, 0x7ff7, 0x11d2, 0x88, 0x5c, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IMomentCapacity : IUnknown
{
   virtual Float64 GetMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment) = 0;
   virtual std::vector<Float64> GetMomentCapacity(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment) = 0;
   virtual void GetMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd) = 0;
   virtual void GetMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd) = 0;

   virtual Float64 GetCrackingMoment(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment) = 0;
   virtual void GetCrackingMomentDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd) = 0;
   virtual void GetCrackingMomentDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd) = 0;

   virtual Float64 GetMinMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment) = 0;
   virtual void GetMinMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd) = 0;
   virtual void GetMinMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd) = 0;

   virtual std::vector<MOMENTCAPACITYDETAILS> GetMomentCapacityDetails(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment) = 0;
   virtual std::vector<MINMOMENTCAPDETAILS> GetMinMomentCapacityDetails(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment) = 0;
   virtual std::vector<CRACKINGMOMENTDETAILS> GetCrackingMomentDetails(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment) = 0;

   virtual std::vector<Float64> GetCrackingMoment(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment) = 0;
   virtual std::vector<Float64> GetMinMomentCapacity(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment) = 0;
};

#endif // INCLUDED_IFACE_MOMENTCAPACITY_H_


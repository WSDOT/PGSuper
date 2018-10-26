///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_SHEARCAPACITY_H_
#define INCLUDED_IFACE_SHEARCAPACITY_H_

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
   IShearCapacity

   Interface to shear capacity information

DESCRIPTION
   Interface to Shear capacity information
*****************************************************************************/
// {D8882B20-9127-11d2-9DA0-00609710E6CE}
DEFINE_GUID(IID_IShearCapacity, 
0xd8882b20, 0x9127, 0x11d2, 0x9d, 0xa0, 0x0, 0x60, 0x97, 0x10, 0xe6, 0xce);
interface IShearCapacity : IUnknown
{
   virtual Float64 GetShearCapacity(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi) = 0;
   virtual std::vector<Float64> GetShearCapacity(pgsTypes::LimitState ls, pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi) = 0;
   virtual void GetShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi,SHEARCAPACITYDETAILS* pmcd) = 0;
   virtual void GetRawShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi,SHEARCAPACITYDETAILS* pmcd) = 0;
   virtual Float64 GetFpc(const pgsPointOfInterest& poi) = 0;
   virtual void GetFpcDetails(const pgsPointOfInterest& poi, FPCDETAILS* pmcd) = 0;

   virtual Float64 GetShearCapacity(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;
   virtual void GetShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,SHEARCAPACITYDETAILS* pmcd) = 0;
   virtual void GetRawShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,SHEARCAPACITYDETAILS* pmcd) = 0;
   virtual Float64 GetFpc(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;
   virtual void GetFpcDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config,FPCDETAILS* pmcd) = 0;

   virtual void GetCriticalSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,Float64* pLeft,Float64* pRight) = 0;
   virtual void GetCriticalSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,Float64* pLeft,Float64* pRight) = 0;
   virtual void GetCriticalSectionDetails(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,CRITSECTDETAILS* pDetails) = 0;
   virtual void GetCriticalSectionDetails(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,CRITSECTDETAILS* pDetails) = 0;

   virtual std::vector<SHEARCAPACITYDETAILS> GetShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi) = 0;
};

#endif // INCLUDED_IFACE_SHEARCAPACITY_H_


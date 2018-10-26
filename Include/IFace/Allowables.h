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

#ifndef INCLUDED_IFACE_ALLOWABLES_H_
#define INCLUDED_IFACE_ALLOWABLES_H_

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

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsPointOfInterest;

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   IAllowableStrandStress

   Interface to allowable prestressing strand stresses.

DESCRIPTION
   Interface to allowable prestressing strand stresses.
*****************************************************************************/
// {82EA97B0-6EB2-11d2-8EEB-006097DF3C68}
DEFINE_GUID(IID_IAllowableStrandStress, 
0x82ea97b0, 0x6eb2, 0x11d2, 0x8e, 0xeb, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface IAllowableStrandStress : IUnknown
{
   virtual bool CheckStressAtJacking() = 0;
   virtual bool CheckStressBeforeXfer() = 0;
   virtual bool CheckStressAfterXfer() = 0;
   virtual bool CheckStressAfterLosses() = 0;

   virtual Float64 GetAllowableAtJacking(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetAllowableBeforeXfer(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetAllowableAfterXfer(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType) = 0;
   virtual Float64 GetAllowableAfterLosses(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType) = 0;
};


/*****************************************************************************
INTERFACE
   IAllowableConcreteStress

   Interface to allowable concrete stresses.

DESCRIPTION
   Interface to allowable concrete stresses.
*****************************************************************************/
// {8D24A46E-7DAD-11d2-8857-006097C68A9C}
DEFINE_GUID(IID_IAllowableConcreteStress, 
0x8d24a46e, 0x7dad, 0x11d2, 0x88, 0x57, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface IAllowableConcreteStress : IUnknown
{   
   virtual Float64 GetAllowableStress(const pgsPointOfInterest& poi, pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType type) = 0;
   virtual std::vector<Float64> GetAllowableStress(const std::vector<pgsPointOfInterest>& vPoi, pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType type) = 0;
   virtual Float64 GetAllowableStress(pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType type,Float64 fc) = 0;
   virtual Float64 GetCastingYardWithMildRebarAllowableStress(SpanIndexType span,GirderIndexType gdr) = 0;

   virtual Float64 GetAllowableCompressiveStressCoefficient(pgsTypes::Stage stage,pgsTypes::LimitState ls) = 0;
   virtual void GetAllowableTensionStressCoefficient(pgsTypes::Stage stage,pgsTypes::LimitState ls,double* pCoeff,bool* pbMax,double* pMaxValue) = 0;

   virtual Float64 GetCastingYardAllowableStress(pgsTypes::LimitState ls,pgsTypes::StressType type,Float64 fc)=0;
   virtual Float64 GetBridgeSiteAllowableStress(pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsTypes::StressType type,Float64 fc)=0;
   virtual Float64 GetInitialAllowableCompressiveStress(Float64 fci)=0;
   virtual Float64 GetInitialAllowableTensileStress(Float64 fci, bool useMinRebar)=0;
   virtual Float64 GetFinalAllowableCompressiveStress(pgsTypes::Stage stage,pgsTypes::LimitState ls,Float64 fc)=0;
   virtual Float64 GetFinalAllowableTensileStress(pgsTypes::Stage stage, Float64 fc)=0;

   virtual Float64 GetCastingYardAllowableTensionStressCoefficientWithRebar() = 0;
};


/*****************************************************************************
INTERFACE
   IDebondLimits

   Interface to access debond limits criteria

DESCRIPTION
   Interface to access debond limits criteria
*****************************************************************************/
// {34C607AB-62D4-43a6-AB8A-6CC66BC8C932}
DEFINE_GUID(IID_IDebondLimits, 
0x34c607ab, 0x62d4, 0x43a6, 0xab, 0x8a, 0x6c, 0xc6, 0x6b, 0xc8, 0xc9, 0x32);
interface IDebondLimits : IUnknown
{
   virtual Float64 GetMaxDebondedStrands(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;  // % of total
   virtual Float64 GetMaxDebondedStrandsPerRow(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0; // % of total in row
   virtual Float64 GetMaxDebondedStrandsPerSection(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0; // % of total debonded
   virtual StrandIndexType GetMaxNumDebondedStrandsPerSection(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0; 
   virtual void    GetMaxDebondLength(SpanIndexType span,GirderIndexType gdr,Float64* pLen, pgsTypes::DebondLengthControl* pControl) = 0; 
   virtual Float64 GetMinDebondSectionDistance(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0; 
};

#endif // INCLUDED_IFACE_ALLOWABLES_H_


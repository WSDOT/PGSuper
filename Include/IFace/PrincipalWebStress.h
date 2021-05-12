///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_PRINCIPALWEBSTRESS_H_
#define INCLUDED_IFACE_PRINCIPALWEBSTRESS_H_

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

#include <IFace\AnalysisResults.h>

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsPointOfInterest;

// MISCELLANEOUS
//
struct PrincipalWebResult
{
   Float64 f_pcx; // web axial stress
   Float64 tau;   // web shear stress

   PrincipalWebResult() :
      f_pcx(0), tau(0)
   {;}
};

struct TimeStepCombinedPrincipalWebStressDetailsAtWebSection
{
   std::_tstring strLocation; // description of web section
   Float64 YwebSection;

   std::array<PrincipalWebResult, lcLoadingCombinationTypeCount> LoadComboResults; // results for each LoadingCombinationType

   // pretensioned & posttensioned prestress
   Float64 PrePrestress_Fpcx;
   Float64 PrePrestress_Tau;
   Float64 PostPrestress_Fpcx;
   Float64 PostPrestress_Tau;

   Float64 LL_Ftop; // results for live load
   Float64 LL_Fbot;
   Float64 LL_Fpcx;
   Float64 LL_Vu;
   Float64 LL_Tau;

   Float64 Service3Fpcx; // results for Service III
   Float64 Service3Tau;
   Float64 Service3PrincipalStress;

   TimeStepCombinedPrincipalWebStressDetailsAtWebSection():
      YwebSection(0), PrePrestress_Fpcx(0), PrePrestress_Tau(0), PostPrestress_Fpcx(0), PostPrestress_Tau(0), LL_Ftop(0), LL_Fbot(0), LL_Fpcx(0), LL_Vu(0), LL_Tau(0), Service3Fpcx(0), Service3Tau(0), Service3PrincipalStress(0)
   {;}
};

/*****************************************************************************
INTERFACE
   IPrincipalWebStress

   Interface to principal web stress information
*****************************************************************************/
// {179D8E05-2C51-4C32-AE11-71D489C4F804}
DEFINE_GUID(IID_IPrincipalWebStress,
   0x179d8e05, 0x2c51, 0x4c32, 0xae, 0x11, 0x71, 0xd4, 0x89, 0xc4, 0xf8, 0x4);
interface IPrincipalWebStress : IUnknown
{
   // Points of interest are in B region
   virtual void GetPrincipalWebStressPointsOfInterest(const CSegmentKey& segmentKey, IntervalIndexType interval, PoiList* pPoiList) const = 0;

   // Principal web stress details: NON-TIME-STEP loss analyses only
   virtual const PRINCIPALSTRESSINWEBDETAILS* GetPrincipalWebStressDetails(const pgsPointOfInterest& poi) const = 0;

   // Principal web stress details: TIME-STEP loss analyses only
   // vector of results at each web section elevation
   virtual const std::vector<TimeStepCombinedPrincipalWebStressDetailsAtWebSection>* GetTimeStepPrincipalWebStressDetails(const pgsPointOfInterest& poi, IntervalIndexType interval) const = 0;
};

#endif // INCLUDED_IFACE_PRINCIPALWEBSTRESS_H_


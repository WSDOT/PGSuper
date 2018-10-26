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

#ifndef INCLUDED_IFACE_DISTRIBUTIONFACTORS_H_
#define INCLUDED_IFACE_DISTRIBUTIONFACTORS_H_

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

#include <Details.h>

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//
class rptChapter;
struct IEAFDisplayUnits;

/*****************************************************************************
INTERFACE
   ILiveLoadDistributionFactors

   Interface to get distribution factors

DESCRIPTION
   Interface to get distribution factors
*****************************************************************************/
// {61DC0CFA-7B2F-11d2-8854-006097C68A9C}
DEFINE_GUID(IID_ILiveLoadDistributionFactors, 
0x61dc0cfa, 0x7b2f, 0x11d2, 0x88, 0x54, 0x0, 0x60, 0x97, 0xc6, 0x8a, 0x9c);
interface ILiveLoadDistributionFactors : IUnknown
{
   // Checks the horizontal curvature requirements and places a message
   // in the status center if they are not satisified
   virtual void VerifyDistributionFactorRequirements(const pgsPointOfInterest& poi) = 0;
   virtual Float64 GetMomentDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls) = 0;
   virtual Float64 GetNegMomentDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls) = 0;
   virtual Float64 GetNegMomentDistFactorAtPier(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace) = 0;
   virtual Float64 GetShearDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls) = 0;
   virtual Float64 GetReactionDistFactor(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls) = 0;

   virtual Float64 GetMomentDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr) = 0;
   virtual Float64 GetNegMomentDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr) = 0;
   virtual Float64 GetNegMomentDistFactorAtPier(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr) = 0;
   virtual Float64 GetShearDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr) = 0;
   virtual Float64 GetReactionDistFactor(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr) = 0;

   virtual void GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState ls,Float64* pM,Float64* nM,Float64* V) = 0;
   virtual void GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState ls,Float64 fcgdr,Float64* pM,Float64* nM,Float64* V) = 0;
   virtual void GetNegMomentDistFactorPoints(SpanIndexType span,GirderIndexType gdr,Float64* dfPoints,Uint32* nPoints) = 0;

   virtual void ReportDistributionFactors(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) = 0;
   virtual bool Run1250Tests(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile) = 0;
   virtual Uint32 GetNumberOfDesignLanes(SpanIndexType span) = 0;
   virtual Uint32 GetNumberOfDesignLanesEx(SpanIndexType span,Float64* pDistToSection,Float64* pCurbToCurb) = 0;
   //---------------------------------------------------------------------
   // Get all types of factors
   virtual bool GetDFResultsEx(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,
                               Float64* gpM, Float64* gpM1, Float64* gpM2,     // pos moment
                               Float64* gnM, Float64* gnM1, Float64* gnM2,     // neg moment
                               Float64* gV,  Float64* gV1,  Float64* gV2,      // shear 
                               Float64* gR,  Float64* gR1,  Float64* gR2 ) = 0;// reaction

   // returns mpf(#lanes/#beams)
   virtual Float64 GetDeflectionDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx) = 0;
};

#endif // INCLUDED_IFACE_DISTRIBUTIONFACTORS_H_


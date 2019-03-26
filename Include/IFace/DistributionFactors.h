///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
   // verifies that certain curvature, stiffness, and parallelnes requires are statisfied
   // must be called before computing LLDF. An unwind exception is thrown if requires are not satisfied
   virtual void VerifyDistributionFactorRequirements(const pgsPointOfInterest& poi) const = 0;

   // Test if ROA is exceeded. A CXUnwind* will be thrown through this interface if not. 
   virtual void TestRangeOfApplicability(const CSpanKey& spanKey) const = 0;

   virtual Float64 GetMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState ls) const = 0;
   virtual Float64 GetNegMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState ls) const = 0;
   virtual Float64 GetNegMomentDistFactorAtPier(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace) const = 0;
   virtual Float64 GetShearDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState ls) const = 0;
   virtual Float64 GetReactionDistFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls) const = 0;

   virtual Float64 GetMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr) const = 0;
   virtual Float64 GetNegMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr) const = 0;
   virtual Float64 GetNegMomentDistFactorAtPier(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr) const = 0;
   virtual Float64 GetShearDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr) const = 0;
   virtual Float64 GetReactionDistFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,Float64 fcgdr) const = 0;

   virtual Float64 GetSkewCorrectionFactorForMoment(const CSpanKey& spanKey,pgsTypes::LimitState ls) const = 0;
   virtual Float64 GetSkewCorrectionFactorForShear(const CSpanKey& spanKey,pgsTypes::LimitState ls) const = 0;

   virtual void GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState ls,Float64* pM,Float64* nM,Float64* V) const = 0;
   virtual void GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState ls,Float64 fcgdr,Float64* pM,Float64* nM,Float64* V) const = 0;
   virtual void GetNegMomentDistFactorPoints(const CSpanKey& spanKey,Float64* dfPoints,IndexType* nPoints) const = 0;

   virtual void ReportDistributionFactors(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const = 0;
   virtual bool Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile) const = 0;
   virtual Uint32 GetNumberOfDesignLanes(SpanIndexType spanIdx) const = 0;
   virtual Uint32 GetNumberOfDesignLanesEx(SpanIndexType spanIdx,Float64* pDistToSection,Float64* pCurbToCurb) const = 0;
   //---------------------------------------------------------------------
   // Get all types of factors
   virtual bool GetDFResultsEx(const CSpanKey& spanKey,pgsTypes::LimitState ls,
                               Float64* gpM, Float64* gpM1, Float64* gpM2,     // pos moment
                               Float64* gnM, Float64* gnM1, Float64* gnM2,     // neg moment
                               Float64* gV,  Float64* gV1,  Float64* gV2,      // shear 
                               Float64* gR,  Float64* gR1,  Float64* gR2 ) const = 0;// reaction

   // returns mpf(#lanes/#beams)
   virtual Float64 GetDeflectionDistFactor(const CSpanKey& spanKey) const = 0;
};

#endif // INCLUDED_IFACE_DISTRIBUTIONFACTORS_H_


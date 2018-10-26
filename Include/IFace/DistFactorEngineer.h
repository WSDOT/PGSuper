///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_DISTFACTORENGINEER_H_
#define INCLUDED_IFACE_DISTFACTORENGINEER_H_

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//
#include <Reporter\Reporter.h>
#include <PGSuperTypes.h>

// FORWARD DECLARATIONS
//
struct IBroker;
struct IEAFDisplayUnits;

// MISCELLANEOUS
//


/*****************************************************************************
INTERFACE
   IDistFactorEngineer

   Interface for computing distribution factors

DESCRIPTION
   Interface for computing distribution factors
*****************************************************************************/
// {884D7805-63AF-4e75-BB3E-27EF68205626}
DEFINE_GUID(IID_IDistFactorEngineer, 
0x884d7805, 0x63af, 0x4e75, 0xbb, 0x3e, 0x27, 0xef, 0x68, 0x20, 0x56, 0x26);
interface IDistFactorEngineer : IUnknown
{
   //---------------------------------------------------------------------
   // Associated a broker object with this object. Call only from
   // IBeamFactory at create time.
   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID) = 0;

   //---------------------------------------------------------------------
   // Returns the distribution factor for moment
   virtual double GetMomentDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls) = 0;
   virtual double GetMomentDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr) = 0;

   //---------------------------------------------------------------------
   // Returns the distribution factor for negative moment over a pier
   virtual double GetNegMomentDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace) = 0;
   virtual double GetNegMomentDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr) = 0;

   //---------------------------------------------------------------------
   // Returns the distribution factor for shear
   virtual double GetShearDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls) = 0;
   virtual double GetShearDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr) = 0;

   //---------------------------------------------------------------------
   // Returns the distribution factor for reaction
   virtual double GetReactionDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls) = 0;
   virtual double GetReactionDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr) = 0;

   //---------------------------------------------------------------------
   // Creates a detailed report of the distribution factor computation
   virtual void BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) = 0;

   //---------------------------------------------------------------------
   // Creates a string that defines how the distribution factors are to be computed
   virtual std::_tstring GetComputationDescription(SpanIndexType span,GirderIndexType gdr,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype,pgsTypes::AdjacentTransverseConnectivity connect) = 0;

   //---------------------------------------------------------------------
   // Runs NCHRP 12-50 Tests for live load distrubtion factors
   virtual bool Run1250Tests(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile) = 0;

   //---------------------------------------------------------------------
   // Get all types of factors
   virtual bool GetDFResultsEx(SpanIndexType span, GirderIndexType gdr,pgsTypes::LimitState ls,
                               Float64* gpM, Float64* gpM1, Float64* gpM2,     // pos moment
                               Float64* gnM, Float64* gnM1, Float64* gnM2,     // neg moment
                               Float64* gV,  Float64* gV1,  Float64* gV2,      // shear
                               Float64* gR,  Float64* gR1,  Float64* gR2 ) = 0;// reaction
};

#endif // INCLUDED_IFACE_DISTFACTORENGINEER_H_


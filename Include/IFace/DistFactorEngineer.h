///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//
#include <Reporter\Reporter.h>
#include <PGSuperTypes.h>
#include <PgsExt\Keys.h>

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
   virtual Float64 GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls, const GDRCONFIG* pConfig = nullptr) = 0;

   //---------------------------------------------------------------------
   // Returns the distribution factor for negative moment over a pier
   virtual Float64 GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace, const GDRCONFIG* pConfig = nullptr) = 0;

   //---------------------------------------------------------------------
   // Returns the distribution factor for shear
   virtual Float64 GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls, const GDRCONFIG* pConfig = nullptr) = 0;

   //---------------------------------------------------------------------
   // Creates a detailed report of the distribution factor computation
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) = 0;

   //---------------------------------------------------------------------
   // Creates a string that defines how the distribution factors are to be computed
   virtual std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype,pgsTypes::AdjacentTransverseConnectivity connect) = 0;

   //---------------------------------------------------------------------
   // Runs NCHRP 12-50 Tests for live load distrubtion factors
   virtual bool Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile) = 0;

   //---------------------------------------------------------------------
   // Get all types of factors
   virtual bool GetDFResultsEx(const CSpanKey& spanKey, pgsTypes::LimitState ls,
      Float64* gpM, Float64* gpM1, Float64* gpM2,     // pos moment
      Float64* gnM, Float64* gnM1, Float64* gnM2,     // neg moment
      Float64* gV, Float64* gV1, Float64* gV2) = 0;      // shear

   virtual Float64 GetSkewCorrectionFactorForMoment(const CSpanKey& spanKey,pgsTypes::LimitState ls) = 0;
   virtual Float64 GetSkewCorrectionFactorForShear(const CSpanKey& spanKey,pgsTypes::LimitState ls) = 0;
};

// {6B0D91AD-A60F-4ce3-8E4A-766E7852E38C}
DEFINE_GUID(IID_IMultiWebDistFactorEngineer, 
0x6b0d91ad, 0xa60f, 0x4ce3, 0x8e, 0x4a, 0x76, 0x6e, 0x78, 0x52, 0xe3, 0x8c);
interface IMultiWebDistFactorEngineer : IUnknown
{
   enum BeamType  {btMultiWebTee, btDeckBulbTee, btDeckedSlabBeam};

   virtual void SetBeamType(BeamType bt) = 0;
   virtual BeamType GetBeamType()  = 0;
};

// {A2636B74-1593-4789-8A80-3C4FBC387F9C}
DEFINE_GUID(IID_IUBeamDistFactorEngineer, 
0xa2636b74, 0x1593, 0x4789, 0x8a, 0x80, 0x3c, 0x4f, 0xbc, 0x38, 0x7f, 0x9c);
interface IUBeamDistFactorEngineer : IUnknown
{
   virtual void Init(bool bTypeB, bool bisSpreadSlab) = 0;
};

// {E77E6143-9E82-4644-AF40-7D073BD9FC2B}
DEFINE_GUID(IID_IBulbTeeDistFactorEngineer,
   0xe77e6143, 0x9e82, 0x4644, 0xaf, 0x40, 0x7d, 0x7, 0x3b, 0xd9, 0xfc, 0x2b);
interface IBulbTeeDistFactorEngineer : IUnknown
{
   virtual void Init() = 0;
};

#endif // INCLUDED_IFACE_DISTFACTORENGINEER_H_


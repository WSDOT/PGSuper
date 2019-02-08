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

// BulbTeeDistFactorEngineer.h : Declaration of the CBulbTeeFactorEngineer

#ifndef __BULBTEEDISTFACTORENGINEER_H_
#define __BULBTEEDISTFACTORENGINEER_H_

#include "resource.h"       // main symbols
#include <IFace\DistFactorEngineer.h>
#include <Plugins\Beams.h>

/////////////////////////////////////////////////////////////////////////////
// CBulbTeeDistFactorEngineer
class ATL_NO_VTABLE CBulbTeeDistFactorEngineer : 
   public CComObjectRootEx<CComSingleThreadModel>,
//   public CComRefCountTracer<CBulbTeeDistFactorEngineer,CComObjectRootEx<CComSingleThreadModel> >,
   public CComCoClass<CBulbTeeDistFactorEngineer, &CLSID_BulbTeeDistFactorEngineer>,
   public IDistFactorEngineer
{
public:
   CBulbTeeDistFactorEngineer():
   m_pBroker(nullptr)
	{
	}

   void Init(/*bool treatAsWsDotI*/);

DECLARE_REGISTRY_RESOURCEID(IDR_BULBTEEDISTFACTORENGINEER)

BEGIN_COM_MAP(CBulbTeeDistFactorEngineer)
   COM_INTERFACE_ENTRY(IDistFactorEngineer)
END_COM_MAP()

public:
   // IDistFactorEngineer
   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID) override;
   virtual Float64 GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls) override;
   virtual Float64 GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr) override;
   virtual Float64 GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace) override;
   virtual Float64 GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr) override;
   virtual Float64 GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls) override;
   virtual Float64 GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr) override;
   virtual Float64 GetReactionDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls) override;
   virtual Float64 GetReactionDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,Float64 fcgdr) override;
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) override;
   virtual std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect) override;
   virtual bool Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile) override;
   virtual bool GetDFResultsEx(const CSpanKey& spanKey,pgsTypes::LimitState ls,
                               Float64* gpM, Float64* gpM1, Float64* gpM2,  // pos moment
                               Float64* gnM, Float64* gnM1, Float64* gnM2,  // neg moment, ahead face
                               Float64* gV,  Float64* gV1,  Float64* gV2,   // shear
                               Float64* gR,  Float64* gR1,  Float64* gR2 ) override; // reaction
   virtual Float64 GetSkewCorrectionFactorForMoment(const CSpanKey& spanKey,pgsTypes::LimitState ls) override;
   virtual Float64 GetSkewCorrectionFactorForShear(const CSpanKey& spanKey,pgsTypes::LimitState ls) override;

private:

   // Farm most of the hard work out to other classes
   CComPtr<IDistFactorEngineer> m_pImpl;

   IBroker* m_pBroker;

};

#endif //__BULBTEEDISTFACTORENGINEER_H_

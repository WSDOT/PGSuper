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

// BulbTeeDistFactorEngineer.h : Declaration of the CBulbTeeFactorEngineer

#ifndef __BULBTEEDISTFACTORENGINEER_H_
#define __BULBTEEDISTFACTORENGINEER_H_

#include "resource.h"       // main symbols
#include <IFace\DistFactorEngineer.h>

// {E02EA7A6-5E4B-4e2d-91C9-20F4E742F1EC}
DEFINE_GUID(CLSID_BulbTeeDistFactorEngineer, 
0xe02ea7a6, 0x5e4b, 0x4e2d, 0x91, 0xc9, 0x20, 0xf4, 0xe7, 0x42, 0xf1, 0xec);

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
   m_pBroker(NULL)
	{
	}

   void Init(bool treatAsWsDotI);


BEGIN_COM_MAP(CBulbTeeDistFactorEngineer)
   COM_INTERFACE_ENTRY(IDistFactorEngineer)
END_COM_MAP()

public:
   // IDistFactorEngineer
   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID);
   virtual Float64 GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls);
   virtual Float64 GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual Float64 GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace);
   virtual Float64 GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr);
   virtual Float64 GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls);
   virtual Float64 GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual Float64 GetReactionDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls);
   virtual Float64 GetReactionDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect);
   virtual bool Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile);
   virtual bool GetDFResultsEx(const CSpanKey& spanKey,pgsTypes::LimitState ls,
                               Float64* gpM, Float64* gpM1, Float64* gpM2,  // pos moment
                               Float64* gnM, Float64* gnM1, Float64* gnM2,  // neg moment, ahead face
                               Float64* gV,  Float64* gV1,  Float64* gV2,   // shear
                               Float64* gR,  Float64* gR1,  Float64* gR2 ); // reaction

private:

   // Farm most of the hard work out to other classes
   CComPtr<IDistFactorEngineer> m_pImpl;

   IBroker* m_pBroker;

};

#endif //__BULBTEEDISTFACTORENGINEER_H_

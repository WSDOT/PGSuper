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

// BoxBeamDistFactorEngineer.h : Declaration of the CBoxBeamFactorEngineer

#ifndef __BOXBEAMDISTFACTORENGINEER_H_
#define __BOXBEAMDISTFACTORENGINEER_H_

#include "resource.h"       // main symbols
#include "DistFactorEngineerImpl.h"


struct BOXBEAM_J_VOID
{
   Float64 Ao;
   typedef std::pair<Float64,Float64> Element; // first = s, second = t
   std::vector<Element> Elements;
   Float64 S_over_T; // Sum of s/t for all the elements
};

struct BOXBEAM_LLDFDETAILS : public BASE_LLDFDETAILS
{
   Float64 L;
   Float64 I;
   Float64 b;
   Float64 d;
   Float64 J;
   Float64 leftDe;
   Float64 rightDe;
   Float64 PossionRatio;
   bool    connectedAsUnit;

   BOXBEAM_J_VOID  Jvoid;
};

// {AF7F923F-3E47-435b-8835-53F68601FE3B}
DEFINE_GUID(CLSID_BoxBeamDistFactorEngineer, 
0xaf7f923f, 0x3e47, 0x435b, 0x88, 0x35, 0x53, 0xf6, 0x86, 0x1, 0xfe, 0x3b);

/////////////////////////////////////////////////////////////////////////////
// CBoxBeamDistFactorEngineer
class ATL_NO_VTABLE CBoxBeamDistFactorEngineer : 
   public CComObjectRootEx<CComSingleThreadModel>,
//   public CComRefCountTracer<CBoxBeamDistFactorEngineer,CComObjectRootEx<CComSingleThreadModel> >,
   public CComCoClass<CBoxBeamDistFactorEngineer, &CLSID_BoxBeamDistFactorEngineer>,
   public CDistFactorEngineerImpl<BOXBEAM_LLDFDETAILS>
{
public:
	CBoxBeamDistFactorEngineer()
	{
	}

   HRESULT FinalConstruct();

BEGIN_COM_MAP(CBoxBeamDistFactorEngineer)
   COM_INTERFACE_ENTRY(IDistFactorEngineer)
END_COM_MAP()

public:
   // IDistFactorEngineer
//   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID);
//   virtual Float64 GetMomentDF(SpanIndexType span,GirderIndexType gdr);
//   virtual Float64 GetNegMomentDF(SpanIndexType pier,GirderIndexType gdr,IDistFactorEngineer::Side side);
//   virtual Float64 GetShearDF(SpanIndexType span,GirderIndexType gdr);
//   virtual Float64 GetReactionDF(SpanIndexType pier,GirderIndexType gdr);
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect);

private:
//   IBroker* m_pBroker;
//   long m_StatusGroupID;

   lrfdLiveLoadDistributionFactorBase* GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,Float64 fcgdr,BOXBEAM_LLDFDETAILS* plldf);

   void ReportMoment(rptParagraph* pPara,BOXBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
   void ReportShear(rptParagraph* pPara,BOXBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
};

#endif //__BOXBEAMDISTFACTORENGINEER_H_

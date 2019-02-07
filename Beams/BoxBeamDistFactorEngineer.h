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

// BoxBeamDistFactorEngineer.h : Declaration of the CBoxBeamFactorEngineer

#ifndef __BOXBEAMDISTFACTORENGINEER_H_
#define __BOXBEAMDISTFACTORENGINEER_H_

#include "resource.h"       // main symbols
#include "DistFactorEngineerImpl.h"
#include <Plugins\Beams.h>


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

DECLARE_REGISTRY_RESOURCEID(IDR_BOXBEAMDISTFACTORENGINEER)

BEGIN_COM_MAP(CBoxBeamDistFactorEngineer)
   COM_INTERFACE_ENTRY(IDistFactorEngineer)
END_COM_MAP()

public:
   // IDistFactorEngineer
//   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID) override;
//   virtual Float64 GetMomentDF(SpanIndexType span,GirderIndexType gdr) override;
//   virtual Float64 GetNegMomentDF(SpanIndexType pier,GirderIndexType gdr,IDistFactorEngineer::Side side) override;
//   virtual Float64 GetShearDF(SpanIndexType span,GirderIndexType gdr) override;
//   virtual Float64 GetReactionDF(SpanIndexType pier,GirderIndexType gdr) override;
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) override;
   virtual std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect) override;

private:
//   IBroker* m_pBroker;
//   long m_StatusGroupID;

   lrfdLiveLoadDistributionFactorBase* GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,Float64 fcgdr,BOXBEAM_LLDFDETAILS* plldf);

   void ReportMoment(rptParagraph* pPara,BOXBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
   void ReportShear(rptParagraph* pPara,BOXBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
};

#endif //__BOXBEAMDISTFACTORENGINEER_H_

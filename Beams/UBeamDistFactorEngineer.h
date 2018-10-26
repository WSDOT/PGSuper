///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// UBeamDistFactorEngineer.h : Declaration of the CUBeamFactorEngineer

#ifndef __UBEAMDISTFACTORENGINEER_H_
#define __UBEAMDISTFACTORENGINEER_H_

#include "resource.h"       // main symbols
#include "DistFactorEngineerImpl.h"
#include <Plugins\Beams.h>

struct UBEAM_LLDFDETAILS : public BASE_LLDFDETAILS
{
   Float64 L;
   Float64 d;

   Float64 leftDe;
   Float64 rightDe;
};

/////////////////////////////////////////////////////////////////////////////
// CUBeamDistFactorEngineer
class ATL_NO_VTABLE CUBeamDistFactorEngineer : 
   public IUBeamDistFactorEngineer,
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CUBeamDistFactorEngineer, &CLSID_UBeamDistFactorEngineer>,
   public CDistFactorEngineerImpl<UBEAM_LLDFDETAILS>
{
public:
	CUBeamDistFactorEngineer()
	{
      m_bTypeB = false;
      m_bIsSpreadSlab = false;
	}

   HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_UBEAMDISTFACTORENGINEER)

BEGIN_COM_MAP(CUBeamDistFactorEngineer)
   COM_INTERFACE_ENTRY(IDistFactorEngineer)
   COM_INTERFACE_ENTRY(IUBeamDistFactorEngineer)
END_COM_MAP()

public:
   // IUBeamDistFactorEngineer
   virtual void Init(bool bTypeB, bool bisSpreadSlab);

   // IDistFactorEngineer
//   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID);
//   virtual Float64 GetMomentDF(SpanIndexType span,GirderIndexType gdr);
//   virtual Float64 GetNegMomentDF(PierIndexType pier,GirderIndexType gdr);
//   virtual Float64 GetShearDF(SpanIndexType span,GirderIndexType gdr);
//   virtual Float64 GetReactionDF(PierIndexType pier,GirderIndexType gdr);
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect);

private:
   lrfdLiveLoadDistributionFactorBase* GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,Float64 fcgdr,UBEAM_LLDFDETAILS* plldf);

   void ReportMoment(IndexType spanOrPierIdx,rptParagraph* pPara,UBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
   void ReportShear(IndexType spanOrPierIdx,rptParagraph* pPara,UBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);

   bool m_bTypeB; // true if type b, otherwise type c section
   bool m_bIsSpreadSlab; // We need to model spread slabs, and aashto has no guidance
};

#endif //__UBEAMDISTFACTORENGINEER_H_

///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// IBeamDistFactorEngineer.h : Declaration of the CIBeamFactorEngineer

#ifndef __IBEAMDISTFACTORENGINEER_H_
#define __IBEAMDISTFACTORENGINEER_H_

#include "resource.h"       // main symbols
#include "DistFactorEngineerImpl.h"
#include <Plugins\Beams.h>

struct IBEAM_LLDFDETAILS : public BASE_LLDFDETAILS
{
   Float64 L;
   Float64 ts;
   Float64 n;
   Float64 I;
   Float64 A;
   Float64 Yt;
   Float64 eg;
   Float64 Kg;
};

/////////////////////////////////////////////////////////////////////////////
// CIBeamFactory
class ATL_NO_VTABLE CIBeamDistFactorEngineer : 
   public CComObjectRootEx<CComSingleThreadModel>,
//   public CComRefCountTracer<CIBeamDistFactorEngineer,CComObjectRootEx<CComSingleThreadModel> >,
   public CComCoClass<CIBeamDistFactorEngineer, &CLSID_IBeamDistFactorEngineer>,
   public CDistFactorEngineerImpl<IBEAM_LLDFDETAILS>
{
public:
	CIBeamDistFactorEngineer()
	{
	}

   HRESULT FinalConstruct();


DECLARE_REGISTRY_RESOURCEID(IDR_IBEAMDISTFACTORENGINEER)

BEGIN_COM_MAP(CIBeamDistFactorEngineer)
   COM_INTERFACE_ENTRY(IDistFactorEngineer)
END_COM_MAP()

public:
   // IDistFactorEngineer
//   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID) override;
//   virtual Float64 GetMomentDF(SpanIndexType span,GirderIndexType gdr) override;
//   virtual Float64 GetNegMomentDF(PierIndexType pier,GirderIndexType gdr) override;
//   virtual Float64 GetShearDF(SpanIndexType span,GirderIndexType gdr) override;
//   virtual Float64 GetReactionDF(PierIndexType pier,GirderIndexType gdr) override;
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) override;
   virtual std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect) override;

private:
   lrfdLiveLoadDistributionFactorBase* GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,Float64 fcgdr,IBEAM_LLDFDETAILS* plldf);

   void ReportMoment(rptParagraph* pPara,IBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
   void ReportShear(rptParagraph* pPara,IBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
};

#endif //__IBEAMDISTFACTORENGINEER_H_

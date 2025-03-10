///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

// VoidedSlab2DistFactorEngineer.h : Declaration of the CVoidedSlabFactorEngineer

#ifndef __VoidedSlab2DistFactorEngineer_H_
#define __VoidedSlab2DistFactorEngineer_H_

#include "resource.h"       // main symbols
#include "DistFactorEngineerImpl.h"
#include "VoidedSlabDistFactorEngineerTypes.h"
#include <Plugins\Beams.h>

/////////////////////////////////////////////////////////////////////////////
// CVoidedSlab2DistFactorEngineer
class ATL_NO_VTABLE CVoidedSlab2DistFactorEngineer : 
   public CComObjectRootEx<CComSingleThreadModel>,
//   public CComRefCountTracer<CVoidedSlab2DistFactorEngineer,CComObjectRootEx<CComSingleThreadModel> >,
   public CComCoClass<CVoidedSlab2DistFactorEngineer, &CLSID_VoidedSlab2DistFactorEngineer>,
   public CDistFactorEngineerImpl<VOIDEDSLAB_LLDFDETAILS>
{
public:
	CVoidedSlab2DistFactorEngineer()
	{
	}

   HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_VOIDEDSLAB2DISTFACTORENGINEER)

BEGIN_COM_MAP(CVoidedSlab2DistFactorEngineer)
   COM_INTERFACE_ENTRY(IDistFactorEngineer)
   COM_INTERFACE_ENTRY(IInitialize)
END_COM_MAP()

// IInitialize
public:
//   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID) override;

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
   
   WBFL::LRFD::LiveLoadDistributionFactorBase* GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,VOIDEDSLAB_LLDFDETAILS* plldf, const GDRCONFIG* pConfig = nullptr);

   void ReportMoment(rptParagraph* pPara,VOIDEDSLAB_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
   void ReportShear(rptParagraph* pPara,VOIDEDSLAB_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
};

#endif //__VoidedSlab2DistFactorEngineer_H_

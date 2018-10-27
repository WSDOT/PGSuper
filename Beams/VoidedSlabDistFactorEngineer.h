///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// VoidedSlabDistFactorEngineer.h : Declaration of the CVoidedSlabFactorEngineer

#ifndef __VOIDEDSLABDISTFACTORENGINEER_H_
#define __VOIDEDSLABDISTFACTORENGINEER_H_

#include "resource.h"       // main symbols
#include "DistFactorEngineerImpl.h"
#include "VoidedSlabDistFactorEngineerTypes.h"
#include <Plugins\Beams.h>

/////////////////////////////////////////////////////////////////////////////
// CVoidedSlabDistFactorEngineer
class ATL_NO_VTABLE CVoidedSlabDistFactorEngineer : 
   public CComObjectRootEx<CComSingleThreadModel>,
//   public CComRefCountTracer<CVoidedSlabDistFactorEngineer,CComObjectRootEx<CComSingleThreadModel> >,
   public CComCoClass<CVoidedSlabDistFactorEngineer, &CLSID_VoidedSlabDistFactorEngineer>,
   public CDistFactorEngineerImpl<VOIDEDSLAB_LLDFDETAILS>
{
public:
	CVoidedSlabDistFactorEngineer()
	{
	}

   HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_VOIDEDSLABDISTFACTORENGINEER)

BEGIN_COM_MAP(CVoidedSlabDistFactorEngineer)
   COM_INTERFACE_ENTRY(IDistFactorEngineer)
END_COM_MAP()

public:
   // IDistFactorEngineer
//   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID);
//   virtual Float64 GetMomentDF(SpanIndexType span,GirderIndexType gdr);
//   virtual Float64 GetNegMomentDF(PierIndexType pier,GirderIndexType gdr);
//   virtual Float64 GetShearDF(SpanIndexType span,GirderIndexType gdr);
//   virtual Float64 GetReactionDF(PierIndexType pier,GirderIndexType gdr);
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect);

private:
   
   lrfdLiveLoadDistributionFactorBase* GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,Float64 fcgdr,VOIDEDSLAB_LLDFDETAILS* plldf);

   void ReportMoment(rptParagraph* pPara,VOIDEDSLAB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
   void ReportShear(rptParagraph* pPara,VOIDEDSLAB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
};

#endif //__VOIDEDSLABDISTFACTORENGINEER_H_

///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// {3AC40380-8764-4087-BD7C-47A50A369AE6}
DEFINE_GUID(CLSID_IBeamDistFactorEngineer, 
0x3ac40380, 0x8764, 0x4087, 0xbd, 0x7c, 0x47, 0xa5, 0xa, 0x36, 0x9a, 0xe6);

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

BEGIN_COM_MAP(CIBeamDistFactorEngineer)
   COM_INTERFACE_ENTRY(IDistFactorEngineer)
END_COM_MAP()

public:
   // IDistFactorEngineer
//   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID);
//   virtual Float64 GetMomentDF(SpanIndexType span,GirderIndexType gdr);
//   virtual Float64 GetNegMomentDF(PierIndexType pier,GirderIndexType gdr);
//   virtual Float64 GetShearDF(SpanIndexType span,GirderIndexType gdr);
//   virtual Float64 GetReactionDF(PierIndexType pier,GirderIndexType gdr);
   virtual void BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual std::_tstring GetComputationDescription(SpanIndexType span,GirderIndexType gdr,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect);

private:
   lrfdLiveLoadDistributionFactorBase* GetLLDFParameters(SpanIndexType spanOrPier,GirderIndexType gdr,DFParam dfType,Float64 fcgdr,IBEAM_LLDFDETAILS* plldf);

   void ReportMoment(rptParagraph* pPara,IBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
   void ReportShear(rptParagraph* pPara,IBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
};

#endif //__IBEAMDISTFACTORENGINEER_H_

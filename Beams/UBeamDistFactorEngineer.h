///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

struct UBEAM_LLDFDETAILS : public BASE_LLDFDETAILS
{
   Float64 L;
   Float64 d;

   Float64 leftDe;
   Float64 rightDe;
};

// {41AB468B-C89F-4153-96FC-A37E18A33C68}
DEFINE_GUID(CLSID_UBeamDistFactorEngineer, 
0x41ab468b, 0xc89f, 0x4153, 0x96, 0xfc, 0xa3, 0x7e, 0x18, 0xa3, 0x3c, 0x68);

/////////////////////////////////////////////////////////////////////////////
// CUBeamDistFactorEngineer
class ATL_NO_VTABLE CUBeamDistFactorEngineer : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CUBeamDistFactorEngineer, &CLSID_UBeamDistFactorEngineer>,
   public CDistFactorEngineerImpl<UBEAM_LLDFDETAILS>
{
public:
	CUBeamDistFactorEngineer()
	{
      m_bTypeB = false;
	}

   HRESULT FinalConstruct();

BEGIN_COM_MAP(CUBeamDistFactorEngineer)
   COM_INTERFACE_ENTRY(IDistFactorEngineer)
END_COM_MAP()

public:
   void Init(bool bTypeB);

   // IDistFactorEngineer
//   virtual void SetBroker(IBroker* pBroker,long statusGroupID);
//   virtual double GetMomentDF(SpanIndexType span,GirderIndexType gdr);
//   virtual double GetNegMomentDF(PierIndexType pier,GirderIndexType gdr);
//   virtual double GetShearDF(SpanIndexType span,GirderIndexType gdr);
//   virtual double GetReactionDF(PierIndexType pier,GirderIndexType gdr);
   virtual void BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits);
   virtual std::_tstring GetComputationDescription(SpanIndexType span,GirderIndexType gdr,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect);

private:
   lrfdLiveLoadDistributionFactorBase* GetLLDFParameters(SpanIndexType spanOrPier,GirderIndexType gdr,DFParam dfType,Float64 fcgdr,UBEAM_LLDFDETAILS* plldf);

   void ReportMoment(Uint32 spanOrPier,rptParagraph* pPara,UBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,double gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
   void ReportShear(Uint32 spanOrPier,rptParagraph* pPara,UBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,double gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);

   bool m_bTypeB; // true if type b, otherwise type c section
};

#endif //__UBEAMDISTFACTORENGINEER_H_

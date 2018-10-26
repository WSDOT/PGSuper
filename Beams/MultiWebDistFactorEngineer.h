///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

// MultiWebDistFactorEngineer.h : Declaration of the CMultiWebFactorEngineer

#ifndef __MULTIWEBDISTFACTORENGINEER_H_
#define __MULTIWEBDISTFACTORENGINEER_H_

#include "resource.h"       // main symbols
#include "DistFactorEngineerImpl.h"

struct MULTIWEB_LLDFDETAILS : public BASE_LLDFDETAILS
{
   bool    connectedAsUnit;
   Float64 b;
   Float64 L;
   Float64 I;
   Float64 A;
   Float64 Ip;
   Float64 J;
   Float64 Ag;
   Float64 Ig;
   Float64 Kg;
   Float64 Yt;
   Float64 ts;
   Float64 eg;
   Float64 n;
   Float64 PossionRatio;

   Float64 CurbOffset;
   Float64 leftDe;
   Float64 rightDe;
};

// {5F9F0F5B-0BCE-4aad-B2A6-47FC36BB331A}
DEFINE_GUID(CLSID_MultiWebDistFactorEngineer, 
0x5f9f0f5b, 0xbce, 0x4aad, 0xb2, 0xa6, 0x47, 0xfc, 0x36, 0xbb, 0x33, 0x1a);


/////////////////////////////////////////////////////////////////////////////
// CMultiWebDistFactorEngineer
class ATL_NO_VTABLE CMultiWebDistFactorEngineer : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CMultiWebDistFactorEngineer, &CLSID_MultiWebDistFactorEngineer>,
   public CDistFactorEngineerImpl<MULTIWEB_LLDFDETAILS>
{
public:
	CMultiWebDistFactorEngineer()
	{
	}

   HRESULT FinalConstruct();

BEGIN_COM_MAP(CMultiWebDistFactorEngineer)
   COM_INTERFACE_ENTRY(IDistFactorEngineer)
END_COM_MAP()

public:
   // IDistFactorEngineer
//   virtual void SetBroker(IBroker* pBroker,long agentID);
//   virtual double GetMomentDF(SpanIndexType span,GirderIndexType gdr);
//   virtual double GetNegMomentDF(PierIndexType pier,GirderIndexType gdr);
//   virtual double GetShearDF(SpanIndexType span,GirderIndexType gdr);
//   virtual double GetReactionDF(PierIndexType pier,GirderIndexType gdr);
   virtual void BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDispUnit);
   virtual std::string GetComputationDescription(SpanIndexType span,GirderIndexType gdr,const std::string& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect);

private:

   lrfdLiveLoadDistributionFactorBase* GetLLDFParameters(SpanIndexType spanOrPier,GirderIndexType gdr,DFParam dfType,Float64 fcgdr,MULTIWEB_LLDFDETAILS* plldf);

   void ReportMoment(rptParagraph* pPara,MULTIWEB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,double gM,bool bSIUnits,IDisplayUnits* pDispUnit);
   void ReportShear(rptParagraph* pPara,MULTIWEB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,double gV,bool bSIUnits,IDisplayUnits* pDispUnit);
};

#endif //__MULTIWEBDISTFACTORENGINEER_H_

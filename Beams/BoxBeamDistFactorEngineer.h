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

// BoxBeamDistFactorEngineer.h : Declaration of the CBoxBeamFactorEngineer

#ifndef __BOXBEAMDISTFACTORENGINEER_H_
#define __BOXBEAMDISTFACTORENGINEER_H_

#include "resource.h"       // main symbols
#include "DistFactorEngineerImpl.h"


struct BOXBEAM_J_VOID
{
   Float64 Ao;
   typedef std::pair<double,double> Element; // first = s, second = t
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

// {DA437468-B32C-4012-8B8E-51BAE278C170}
DEFINE_GUID(CLSID_BoxBeamDistFactorEngineer, 
0xda437468, 0xb32c, 0x4012, 0x8b, 0x8e, 0x51, 0xba, 0xe2, 0x78, 0xc1, 0x70);

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
//   virtual void SetBroker(IBroker* pBroker,long agentID);
//   virtual double GetMomentDF(SpanIndexType span,GirderIndexType gdr);
//   virtual double GetNegMomentDF(SpanIndexType pier,GirderIndexType gdr,IDistFactorEngineer::Side side);
//   virtual double GetShearDF(SpanIndexType span,GirderIndexType gdr);
//   virtual double GetReactionDF(SpanIndexType pier,GirderIndexType gdr);
   virtual void BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IDisplayUnits* pDisplayUnits);
   virtual std::string GetComputationDescription(SpanIndexType span,GirderIndexType gdr,const std::string& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect);

private:
//   IBroker* m_pBroker;
//   long m_AgentID;

   lrfdLiveLoadDistributionFactorBase* GetLLDFParameters(SpanIndexType spanOrPier,GirderIndexType gdr,DFParam dfType,Float64 fcgdr,BOXBEAM_LLDFDETAILS* plldf);

   void ReportMoment(rptParagraph* pPara,BOXBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,double gM,bool bSIUnits,IDisplayUnits* pDisplayUnits);
   void ReportShear(rptParagraph* pPara,BOXBEAM_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,double gV,bool bSIUnits,IDisplayUnits* pDisplayUnits);
};

#endif //__BOXBEAMDISTFACTORENGINEER_H_

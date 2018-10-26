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
      m_BeamType = (BeamType)-1; // set trap to make sure client sets a real value
	}

   HRESULT FinalConstruct();

BEGIN_COM_MAP(CMultiWebDistFactorEngineer)
   COM_INTERFACE_ENTRY(IDistFactorEngineer)
END_COM_MAP()

public: 
   // We need a little help from above to figure beam type
   enum BeamType  {btMultiWebTee, btDeckBulbTee, btDeckedSlabBeam};

   BeamType GetBeamType() const
   {
      return m_BeamType;
   }

   void SetBeamType(BeamType bt)
   {
      m_BeamType = bt;
   }

   Float64 GetTxDOTKfactor() const
   {
      // Refer to txdot manual
      if (m_BeamType==btDeckBulbTee || m_BeamType==btDeckedSlabBeam)
      {
         return 2.0;
      }
      else if (m_BeamType==btMultiWebTee)
      {
         return 2.2;
      }
      else
      {
         ATLASSERT(0); // forgot to set factor?
         return 2.0;
      }
   }

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

   lrfdLiveLoadDistributionFactorBase* GetLLDFParameters(SpanIndexType spanOrPier,GirderIndexType gdr,DFParam dfType,Float64 fcgdr,MULTIWEB_LLDFDETAILS* plldf);

   void ReportMoment(rptParagraph* pPara,MULTIWEB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
   void ReportShear(rptParagraph* pPara,MULTIWEB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);

   BeamType m_BeamType;
};

#endif //__MULTIWEBDISTFACTORENGINEER_H_

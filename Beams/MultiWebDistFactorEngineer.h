///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <Plugins\Beams.h>

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

/////////////////////////////////////////////////////////////////////////////
// CMultiWebDistFactorEngineer
class ATL_NO_VTABLE CMultiWebDistFactorEngineer : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CMultiWebDistFactorEngineer, &CLSID_MultiWebDistFactorEngineer>,
   public CDistFactorEngineerImpl<MULTIWEB_LLDFDETAILS>,
   public IMultiWebDistFactorEngineer
{
public:
	CMultiWebDistFactorEngineer()
	{
      m_BeamType = (BeamType)-1; // set trap to make sure client sets a real value
	}

   HRESULT FinalConstruct();

DECLARE_REGISTRY_RESOURCEID(IDR_MULTIWEBDISTFACTORENGINEER)

BEGIN_COM_MAP(CMultiWebDistFactorEngineer)
   COM_INTERFACE_ENTRY(IMultiWebDistFactorEngineer)
   COM_INTERFACE_ENTRY(IDistFactorEngineer)
   COM_INTERFACE_ENTRY(IInitialize)
END_COM_MAP()

public: 
   Float64 GetTxDOTKfactor() const
   {
      // Refer to txdot manual
      if (m_BeamType==IMultiWebDistFactorEngineer::btDeckBulbTee || m_BeamType==IMultiWebDistFactorEngineer::btDeckedSlabBeam)
      {
         return 2.0;
      }
      else if (m_BeamType==IMultiWebDistFactorEngineer::btMultiWebTee)
      {
         return 2.2;
      }
      else
      {
         ATLASSERT(false); // forgot to set factor?
         return 2.0;
      }
   }

// IDistFactorEngineer
public:
//   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID) override;
//   virtual Float64 GetMomentDF(SpanIndexType span,GirderIndexType gdr) override;
//   virtual Float64 GetNegMomentDF(PierIndexType pier,GirderIndexType gdr) override;
//   virtual Float64 GetShearDF(SpanIndexType span,GirderIndexType gdr) override;
//   virtual Float64 GetReactionDF(PierIndexType pier,GirderIndexType gdr) override;
   virtual void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) override;
   virtual std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect) override;

// IMultiWebDistFactorEngineer
public:
   virtual IMultiWebDistFactorEngineer::BeamType GetBeamType() override;
   virtual void SetBeamType(IMultiWebDistFactorEngineer::BeamType bt) override;

private:

   lrfdLiveLoadDistributionFactorBase* GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,Float64 fcgdr,MULTIWEB_LLDFDETAILS* plldf);

   void ReportMoment(rptParagraph* pPara,MULTIWEB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gM1,lrfdILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);
   void ReportShear(rptParagraph* pPara,MULTIWEB_LLDFDETAILS& lldf,lrfdILiveLoadDistributionFactor::DFResult& gV1,lrfdILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,IEAFDisplayUnits* pDisplayUnits);

   IMultiWebDistFactorEngineer::BeamType m_BeamType;
};

#endif //__MULTIWEBDISTFACTORENGINEER_H_

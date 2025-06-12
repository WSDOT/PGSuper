///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#pragma once

#include <Beams/BeamsExp.h>
#include <Beams/DistFactorEngineerImpl.h>
#include <Plugins\Beams.h>

namespace PGS
{
   namespace Beams 
   {
      struct UBEAM_LLDFDETAILS : public BASE_LLDFDETAILS
      {
         Float64 L;
         Float64 d;

         Float64 leftDe;
         Float64 rightDe;
      };

      class BEAMSCLASS UBeamDistFactorEngineer : public DistFactorEngineerImpl<UBEAM_LLDFDETAILS>
      {
      public:
         UBeamDistFactorEngineer(std::weak_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID, bool bTypeB=false, bool bisSpreadSlab=false);

         // IDistFactorEngineer
         void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) override;
         std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect) override;

      private:
         WBFL::LRFD::LiveLoadDistributionFactorBase* GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,UBEAM_LLDFDETAILS* plldf,const GDRCONFIG* pConfig = nullptr);

         void ReportMoment(IndexType spanOrPierIdx,rptParagraph* pPara,UBEAM_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
         void ReportShear(IndexType spanOrPierIdx,rptParagraph* pPara,UBEAM_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);

         bool m_bTypeB; // true if type b, otherwise type c section
         bool m_bIsSpreadSlab; // We need to model spread slabs, and aashto has no guidance
      };
   };
};
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
      struct BEAMSCLASS IBEAM_LLDFDETAILS : public BASE_LLDFDETAILS
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

      class BEAMSCLASS IBeamDistFactorEngineer : public DistFactorEngineerImpl<IBEAM_LLDFDETAILS>
      {
      public:
         IBeamDistFactorEngineer(std::weak_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID);

      public:
         // DistFactorEngineerBase
         void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) override;
         std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect) override;

      private:
         WBFL::LRFD::LiveLoadDistributionFactorBase* GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,IBEAM_LLDFDETAILS* plldf,const GDRCONFIG* pConfig = nullptr);

         void ReportMoment(rptParagraph* pPara,IBEAM_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
         void ReportShear(rptParagraph* pPara,IBEAM_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
      };
   };
};
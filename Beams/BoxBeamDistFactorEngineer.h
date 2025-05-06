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

#include <EAF/ComponentObject.h>
#include <Beams/DistFactorEngineerImpl.h>
#include <Plugins\Beams.h>

namespace PGS
{
   namespace Beams
   {

      struct BEAMSCLASS BOXBEAM_J_VOID
      {
         Float64 Ao;
         typedef std::pair<Float64,Float64> Element; // first = s, second = t
         std::vector<Element> Elements;
         Float64 S_over_T; // Sum of s/t for all the elements
      };

      struct BEAMSCLASS BOXBEAM_LLDFDETAILS : public BASE_LLDFDETAILS
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

      class BEAMSCLASS BoxBeamDistFactorEngineer : public DistFactorEngineerImpl<BOXBEAM_LLDFDETAILS>
      {
      public:
         BoxBeamDistFactorEngineer(std::weak_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID) :
            DistFactorEngineerImpl<BOXBEAM_LLDFDETAILS>(pBroker, statusGroupID)
         {
         }

      public:
         // DistFactorEngineerBase
         void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) override;
         std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect) override;

      private:
         WBFL::LRFD::LiveLoadDistributionFactorBase* GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,BOXBEAM_LLDFDETAILS* plldf, const GDRCONFIG* pConfig = nullptr);

         void ReportMoment(rptParagraph* pPara,BOXBEAM_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
         void ReportShear(rptParagraph* pPara,BOXBEAM_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
      };
   };
};
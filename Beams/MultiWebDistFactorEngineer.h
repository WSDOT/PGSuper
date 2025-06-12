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

      struct BEAMSCLASS MULTIWEB_LLDFDETAILS : public BASE_LLDFDETAILS
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


      class BEAMSCLASS MultiWebDistFactorEngineer : public DistFactorEngineerImpl<MULTIWEB_LLDFDETAILS>
      {
      public:
         enum class BeamType { MultiWebTee, DeckBulbTee, DeckedSlabBeam };
   
         MultiWebDistFactorEngineer(BeamType beamType, std::weak_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID);

      public: 
         Float64 GetTxDOTKfactor() const
         {
            // Refer to txdot manual
            if (m_BeamType==BeamType::DeckBulbTee || m_BeamType==BeamType::DeckedSlabBeam)
            {
               return 2.0;
            }
            else if (m_BeamType==BeamType::MultiWebTee)
            {
               return 2.2;
            }
            else
            {
               ATLASSERT(false); // forgot to set factor?
               return 2.0;
            }
         }

      // DistFactorEngineerBase
      public:
         void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) override;
         std::_tstring GetComputationDescription(const CGirderKey& girderKey,const std::_tstring& libraryEntryName,pgsTypes::SupportedDeckType decktype, pgsTypes::AdjacentTransverseConnectivity connect) override;

      private:
         WBFL::LRFD::LiveLoadDistributionFactorBase* GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,MULTIWEB_LLDFDETAILS* plldf,const GDRCONFIG* pConfig = nullptr);

         void ReportMoment(rptParagraph* pPara,MULTIWEB_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gM2,Float64 gM,bool bSIUnits,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
         void ReportShear(rptParagraph* pPara,MULTIWEB_LLDFDETAILS& lldf,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV1,WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& gV2,Float64 gV,bool bSIUnits,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);

         BeamType m_BeamType;
      };
   };
};
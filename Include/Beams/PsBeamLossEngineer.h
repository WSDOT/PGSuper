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

#include <IFace\PsLossEngineer.h>
#include <Beams\Interfaces.h>
#include <Beams\PsLossEngineer.h>
#include <Plugins\CLSID.h>

#include <PgsExt\PoiKey.h>

namespace PGS
{
   namespace Beams
   {
      // Class for storing Design Losses
      // Design losses are losses computed during design iterations and need not
      // be recomputed if the design is accepted
      class DesignLosses
      {
         struct Losses
         {
            GDRCONFIG m_Config;
            LOSSDETAILS m_Details;
         };

         std::map<pgsPointOfInterest,Losses> m_Losses;
   
      public:
         DesignLosses();
         void Invalidate();
         const LOSSDETAILS* GetFromCache(const pgsPointOfInterest& poi, const GDRCONFIG& config);
         void SaveToCache(const pgsPointOfInterest& poi, const GDRCONFIG& config, const LOSSDETAILS& losses);
      };

      class BEAMSCLASS PsBeamLossEngineer : public PsLossEngineerBase
      {
      public:
         enum class BeamType { IBeam, UBeam, SolidSlab, BoxBeam, SingleT };

         PsBeamLossEngineer(BeamType beamType, std::weak_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID);

      // IPsLossEngineer
      public:
         const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) override;
         const LOSSDETAILS* GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx) override;
         void ClearDesignLosses() override;
         void BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) override;
         void ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) override;
         const ANCHORSETDETAILS* GetGirderTendonAnchorSetDetails(const CGirderKey& girderKey,DuctIndexType ductIdx) override;
         Float64 GetGirderTendonElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType) override;
         void GetGirderTendonAverageFrictionAndAnchorSetLoss(const CGirderKey& girderKey,DuctIndexType ductIdx,Float64* pfpF,Float64* pfpA) override;
         const ANCHORSETDETAILS* GetSegmentTendonAnchorSetDetails(const CSegmentKey& segmentKey, DuctIndexType ductIdx) override;
         Float64 GetSegmentTendonElongation(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) override;
         void GetSegmentTendonAverageFrictionAndAnchorSetLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx, Float64* pfpF, Float64* pfpA) override;

      private:
         BeamType m_BeamType;
         PsLossEngineer m_Engineer;


         // Losses are cached for two different cases:
         // 1) This data structure caches losses for the current project data
         std::map<PoiIDType,LOSSDETAILS> m_PsLosses;

         // 2) This data structure is for design cases. It caches the most recently
         //    computed losses
         DesignLosses m_DesignLosses;
      };
   };
};

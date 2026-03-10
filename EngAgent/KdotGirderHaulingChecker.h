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

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\KdotHaulingAnalysisArtifact.h>
#include <PgsExt\PoiMap.h>
#include <IFace\PointOfInterest.h>

/*****************************************************************************
CLASS 
   pgsKdotGirderHaulingChecker

   Design Checker for girder hauling

DESCRIPTION
   Design Checker for girder lifting and hauling

LOG
   rdp : 06.25.2013 : Created file
*****************************************************************************/

class pgsKdotGirderHaulingChecker: public pgsGirderHaulingChecker
{
public:
   pgsKdotGirderHaulingChecker(std::weak_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID);
   pgsKdotGirderHaulingChecker() = delete;
   pgsKdotGirderHaulingChecker(const pgsKdotGirderHaulingChecker&) = delete;
   pgsKdotGirderHaulingChecker& operator=(const pgsKdotGirderHaulingChecker&) = delete;
   virtual ~pgsKdotGirderHaulingChecker() = default;

   std::shared_ptr<pgsHaulingAnalysisArtifact> CheckHauling(const CSegmentKey& segmentKey, SHARED_LOGFILE LOGFILE) override;
   std::shared_ptr<pgsHaulingAnalysisArtifact> AnalyzeHauling(const CSegmentKey& segmentKey) override;
   std::shared_ptr<pgsHaulingAnalysisArtifact> AnalyzeHauling(const CSegmentKey& segmentKey,Float64 leftOverhang,Float64 rightOverhang) override;
   std::shared_ptr<pgsHaulingAnalysisArtifact> AnalyzeHauling(const CSegmentKey& segmentKey,const HANDLINGCONFIG& config,std::shared_ptr<ISegmentHaulingDesignPointsOfInterest> pPOId) override;
   std::shared_ptr<pgsHaulingAnalysisArtifact> DesignHauling(const CSegmentKey& segmentKey,HANDLINGCONFIG& config,bool bIgnoreConfigurationLimits,std::shared_ptr<ISegmentHaulingDesignPointsOfInterest> pPOId,bool* bSuccess, SHARED_LOGFILE LOGFILE) override;

private:
   std::weak_ptr<WBFL::EAF::Broker> m_pBroker;
   inline std::shared_ptr<WBFL::EAF::Broker> GetBroker() const { return m_pBroker.lock(); }

   StatusGroupIDType m_StatusGroupID;
   StatusCallbackIDType m_scidBunkPointLocation;

   void AnalyzeHauling(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& config,std::shared_ptr<ISegmentHaulingDesignPointsOfInterest> pPOId,pgsKdotHaulingAnalysisArtifact& pArtifact);
   void PrepareHaulingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 Loh,Float64 Roh,Float64 Fc,Float64 Ec,pgsTypes::ConcreteType concType,pgsKdotHaulingAnalysisArtifact& pArtifact);

   void ComputeHaulingMoments(const CSegmentKey& segmentKey,
                              const pgsKdotHaulingAnalysisArtifact& rArtifact, 
                              const PoiList& vPoi,
                              std::vector<Float64>* pvMoment, Float64* pMidSpanDeflection);

   void ComputeHaulingStresses(const CSegmentKey& segmentKey,bool bUseConfig,
                               const HANDLINGCONFIG& haulConfig,
                               const PoiList& vPoi,
                               const std::vector<Float64>& vMoment,
                               pgsKdotHaulingAnalysisArtifact& pArtifact);
};

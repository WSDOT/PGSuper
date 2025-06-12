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
#include <PgsExt\HaulingAnalysisArtifact.h>
#include <PgsExt\PoiMap.h>
#include <IFace\PointOfInterest.h>

/*****************************************************************************
CLASS 
   pgsWsdotGirderHaulingChecker

   Design Checker for girder hauling

DESCRIPTION
   Design Checker for girder lifting and hauling

LOG
   rdp : 06.25.2013 : Created file
*****************************************************************************/

class pgsWsdotGirderHaulingChecker: public pgsGirderHaulingChecker
{
public:
   pgsWsdotGirderHaulingChecker() = delete;
   pgsWsdotGirderHaulingChecker(const pgsWsdotGirderHaulingChecker&) = delete;
   pgsWsdotGirderHaulingChecker& operator=(const pgsWsdotGirderHaulingChecker&) = delete;
   pgsWsdotGirderHaulingChecker(std::weak_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID);
   virtual ~pgsWsdotGirderHaulingChecker() = default;

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
   StatusCallbackIDType m_scidHaulTruck;

#if defined _DEBUG
   void AnalyzeHauling(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& config,std::shared_ptr<ISegmentHaulingDesignPointsOfInterest> pPOId,WBFL::Stability::HaulingCheckArtifact& pArtifact,const WBFL::Stability::HaulingStabilityProblem** ppStabilityProblem);
#else
   void AnalyzeHauling(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& config,std::shared_ptr<ISegmentHaulingDesignPointsOfInterest> pPOId,WBFL::Stability::HaulingCheckArtifact& pArtifact);
#endif

   void AnalyzeHauling(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& config,std::shared_ptr<ISegmentHaulingDesignPointsOfInterest> pPOId,pgsWsdotHaulingAnalysisArtifact& pArtifact);
};

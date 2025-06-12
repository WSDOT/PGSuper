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
#include <PgsExt\PoiMap.h>
#include <IFace\PointOfInterest.h>

/*****************************************************************************
CLASS 
   pgsGirderLiftingChecker

   Design Checker for girder lifting


DESCRIPTION
   Design Checker for girder lifting

LOG
   rdp : 06.25.2013 : Created file
*****************************************************************************/

class pgsGirderLiftingChecker
{
public:
   pgsGirderLiftingChecker(std::weak_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID);
   pgsGirderLiftingChecker() = delete;
   pgsGirderLiftingChecker(const pgsGirderLiftingChecker&) = delete;
   pgsGirderLiftingChecker& operator=(const pgsGirderLiftingChecker&) = delete;
   ~pgsGirderLiftingChecker() = default;

   std::shared_ptr<WBFL::Stability::LiftingCheckArtifact> CheckLifting(const CSegmentKey& segmentKey);
   std::shared_ptr<WBFL::Stability::LiftingCheckArtifact> AnalyzeLifting(const CSegmentKey& segmentKey,Float64 supportLoc);
   std::shared_ptr<WBFL::Stability::LiftingCheckArtifact> AnalyzeLifting(const CSegmentKey& segmentKey,const HANDLINGCONFIG& config,std::shared_ptr<ISegmentLiftingDesignPointsOfInterest> pPOId, const WBFL::Stability::LiftingStabilityProblem** ppStabilityProblem = nullptr);
   std::pair<pgsDesignCodes::OutcomeType, std::shared_ptr<WBFL::Stability::LiftingCheckArtifact>> DesignLifting(const CSegmentKey& segmentKey,HANDLINGCONFIG& config,std::shared_ptr<ISegmentLiftingDesignPointsOfInterest> pPOId,const WBFL::Stability::LiftingStabilityProblem** ppStabilityProblem,SHARED_LOGFILE LOGFILE);

private:
   std::weak_ptr<WBFL::EAF::Broker> m_pBroker;
   inline std::shared_ptr<WBFL::EAF::Broker> GetBroker() const { return m_pBroker.lock(); }
   StatusGroupIDType m_StatusGroupID;
   StatusCallbackIDType m_scidLiftingSupportLocationError;
   StatusCallbackIDType m_scidLiftingSupportLocationWarning;

   std::shared_ptr<WBFL::Stability::LiftingCheckArtifact> AnalyzeLifting(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& liftConfig,std::shared_ptr<ISegmentLiftingDesignPointsOfInterest> pPoiD,const WBFL::Stability::LiftingStabilityProblem** ppStabilityProblem = nullptr);
};

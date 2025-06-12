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
#include "EngAgent.h"
#include <PgsExt\HaulingAnalysisArtifact.h>
#include <PgsExt\PoiMap.h>
#include <PgsExt\GirderModelFactory.h>

#include <IFace\PointOfInterest.h>

// Virtual members of polymorphic hauling checker
class pgsGirderHaulingChecker
{
public:
   virtual std::shared_ptr<pgsHaulingAnalysisArtifact> CheckHauling(const CSegmentKey& segmentKey, SHARED_LOGFILE LOGFILE) = 0;
   virtual std::shared_ptr<pgsHaulingAnalysisArtifact> AnalyzeHauling(const CSegmentKey& segmentKey) = 0;
   virtual std::shared_ptr<pgsHaulingAnalysisArtifact> AnalyzeHauling(const CSegmentKey& segmentKey,Float64 leftOverhang,Float64 rightOverhang) = 0;
   virtual std::shared_ptr<pgsHaulingAnalysisArtifact> AnalyzeHauling(const CSegmentKey& segmentKey,const HANDLINGCONFIG& config,std::shared_ptr<ISegmentHaulingDesignPointsOfInterest> pPOId) = 0;
   virtual std::shared_ptr<pgsHaulingAnalysisArtifact> DesignHauling(const CSegmentKey& segmentKey,HANDLINGCONFIG& config,bool bIgnoreConfigurationLimits,std::shared_ptr<ISegmentHaulingDesignPointsOfInterest> pPOId, bool* bSuccess, SHARED_LOGFILE LOGFILE) = 0;
};

/*****************************************************************************
CLASS 
   pgsGirderHandlingChecker

   Design Checker Factory for girder lifting and hauling


DESCRIPTION
   Design Checker Factory for girder lifting and hauling

LOG
   rdp : 03.25.1999 : Created file
*****************************************************************************/

class pgsGirderHandlingChecker
{
public:
   pgsGirderHandlingChecker(std::weak_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID);
   pgsGirderHandlingChecker() = delete;
   pgsGirderHandlingChecker(const pgsGirderHandlingChecker&) = delete;
   pgsGirderHandlingChecker& operator=(const pgsGirderHandlingChecker&) = delete;
   ~pgsGirderHandlingChecker() = default;

   // Factory Method to create the appropriate hauling checker
   std::unique_ptr<pgsGirderHaulingChecker> CreateGirderHaulingChecker();

   // Utility functions for the checking classes
   static void ComputeMoments(std::weak_ptr<WBFL::EAF::Broker> pBroker, pgsGirderModelFactory* pGirderModelFactory, const CSegmentKey& segmentKey,
                       IntervalIndexType intervalIdx,
                       Float64 leftOH,Float64 glen,Float64 rightOH,
                       Float64 E, 
                       PoiAttributeType poiReference,
                       const PoiList& rpoiVec,
                       std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection);

private:
   std::weak_ptr<WBFL::EAF::Broker> m_pBroker;
   inline std::shared_ptr<WBFL::EAF::Broker> GetBroker() const { return m_pBroker.lock(); }
   StatusGroupIDType m_StatusGroupID;
};

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

#include "StdAfx.h"
#include "EngAgent.h"
#include <IFace\PointOfInterest.h>
#include <IFace\Bridge.h>
#include <EAF/EAFStatusCenter.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>

#include <PGSuperException.h>

#include <limits>
#include <algorithm>

#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderModelFactory.H>

#include "DesignCodes.h"
#include "AlternativeTensileStressCalculator.h"

#include "GirderHandlingChecker.h"
#include "GirderLiftingChecker.h"
#include "StatusItems.h"

#include "PGSuperUnits.h"

#include <Stability\Stability.h>

pgsGirderLiftingChecker::pgsGirderLiftingChecker(std::weak_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   GET_IFACE2(GetBroker(),IEAFStatusCenter,pStatusCenter);
   m_scidLiftingSupportLocationError   = pStatusCenter->RegisterCallback( std::make_shared<pgsLiftingSupportLocationStatusCallback>(WBFL::EAF::StatusSeverityType::Error) );
   m_scidLiftingSupportLocationWarning = pStatusCenter->RegisterCallback( std::make_shared<pgsLiftingSupportLocationStatusCallback>(WBFL::EAF::StatusSeverityType::Warning) );
}

std::shared_ptr<WBFL::Stability::LiftingCheckArtifact> pgsGirderLiftingChecker::CheckLifting(const CSegmentKey& segmentKey)
{
   GET_IFACE2(GetBroker(),ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);

   if (pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled())
   {
       // Use poi's from global pool
      GET_IFACE2(GetBroker(),ISegmentLiftingPointsOfInterest,pSegmentLiftingPointsOfInterest);
      HANDLINGCONFIG dummy_config;

      // Compute lifting response
      return AnalyzeLifting(segmentKey,false,dummy_config,pSegmentLiftingPointsOfInterest);
   }
   return nullptr;
}

std::shared_ptr<WBFL::Stability::LiftingCheckArtifact> pgsGirderLiftingChecker::AnalyzeLifting(const CSegmentKey& segmentKey,Float64 supportLoc)
{
   GET_IFACE2(GetBroker(),ISegmentLiftingPointsOfInterest,pSegmentLiftingPointsOfInterest);
   HANDLINGCONFIG dummy_config;
   dummy_config.bIgnoreGirderConfig = true;
   dummy_config.LeftOverhang = supportLoc;
   dummy_config.RightOverhang = supportLoc;
   return AnalyzeLifting(segmentKey,true,dummy_config,pSegmentLiftingPointsOfInterest);
}

std::shared_ptr<WBFL::Stability::LiftingCheckArtifact> pgsGirderLiftingChecker::AnalyzeLifting(const CSegmentKey& segmentKey,const HANDLINGCONFIG& liftConfig,std::shared_ptr<ISegmentLiftingDesignPointsOfInterest> pPoiD, const WBFL::Stability::LiftingStabilityProblem** ppStabilityProblem)
{
   return AnalyzeLifting(segmentKey,true,liftConfig,pPoiD,ppStabilityProblem);
}

std::shared_ptr<WBFL::Stability::LiftingCheckArtifact> pgsGirderLiftingChecker::AnalyzeLifting(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& liftConfig,std::shared_ptr<ISegmentLiftingDesignPointsOfInterest> pPoiD,const WBFL::Stability::LiftingStabilityProblem** ppStabilityProblem)
{
   GET_IFACE2(GetBroker(),IGirder,pGirder);
   const WBFL::Stability::Girder* pStabilityModel = pGirder->GetSegmentLiftingStabilityModel(segmentKey);
   const WBFL::Stability::LiftingStabilityProblem* pStabilityProblem = bUseConfig ? pGirder->GetSegmentLiftingStabilityProblem(segmentKey,liftConfig,pPoiD) : pGirder->GetSegmentLiftingStabilityProblem(segmentKey);
   if ( ppStabilityProblem )
   {
      *ppStabilityProblem = pStabilityProblem;
   }

   GET_IFACE2(GetBroker(),ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   WBFL::Stability::LiftingCriteria criteria = pSegmentLiftingSpecCriteria->GetLiftingStabilityCriteria(segmentKey, bUseConfig ? &liftConfig : nullptr);

   WBFL::Stability::StabilityEngineer engineer;
   return std::make_shared<WBFL::Stability::LiftingCheckArtifact>(engineer.CheckLifting(pStabilityModel,pStabilityProblem,criteria));
}

std::pair<pgsDesignCodes::OutcomeType, std::shared_ptr<WBFL::Stability::LiftingCheckArtifact>> pgsGirderLiftingChecker::DesignLifting(const CSegmentKey& segmentKey,HANDLINGCONFIG& config,std::shared_ptr<ISegmentLiftingDesignPointsOfInterest> pPoiD,const WBFL::Stability::LiftingStabilityProblem** ppStabilityProblem,SHARED_LOGFILE LOGFILE)
{
   //
   // Range of lifting loop locations and step increment
   //
   GET_IFACE2(GetBroker(),ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   Float64 min_location = Max(pSegmentLiftingSpecCriteria->GetMinimumLiftingPointLocation(segmentKey,pgsTypes::metStart),pSegmentLiftingSpecCriteria->GetMinimumLiftingPointLocation(segmentKey,pgsTypes::metEnd));
   Float64 location_accuracy = pSegmentLiftingSpecCriteria->GetLiftingPointLocationAccuracy();

   Float64 bigInc = 8*location_accuracy;
   Float64 smallInc = location_accuracy;
   Float64 locInc = bigInc;
   bool bLargeStepSize = true;

   // Max location may be limited by harping point (actually, just before it stopping at the last increment value)
   // But allowing more than 40% of the girder length makes no sense (think rigid-body instability for riggers)
   GET_IFACE2(GetBroker(),IBridge, pBridge);
   Float64 girder_length = pBridge->GetSegmentLength(segmentKey);

   Float64 maxLoc = 0.4*girder_length;

   GET_IFACE2(GetBroker(),IStrandGeometry, pStrandGeom);
   StrandIndexType Nh = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Harped, config.bIgnoreGirderConfig ? nullptr : &config.GdrConfig);

   if (0 < Nh) // only look at harping point if we have harped strands
   {
      Float64 lhp,rhp;
      pStrandGeom->GetHarpingPointLocations(segmentKey,&lhp,&rhp);
      maxLoc = Min(lhp, maxLoc);
   }

   // Find a lifting location that makes the factor of safety against failure
   // equal to 1.5 or better
   Float64 FSf = 0;
   Float64 FSfMin = pSegmentLiftingSpecCriteria->GetLiftingFailureFs();
   Float64 loc = min_location;

   //HANDLINGCONFIG lift_config;
   //lift_config.bIgnoreGirderConfig = false;
   //lift_config.GdrConfig = config;

   std::shared_ptr<WBFL::Stability::LiftingCheckArtifact> artifact;
   while ( loc <= maxLoc )
   {
      LOG(_T(""));
      LOG(_T("Trying location ") << WBFL::Units::ConvertFromSysUnits(loc,WBFL::Units::Measure::Feet) << _T(" ft"));

      config.LeftOverhang = loc;
      config.RightOverhang = loc;

      auto curr_artifact = AnalyzeLifting(segmentKey,config,pPoiD,ppStabilityProblem);
      FSf = curr_artifact->GetLiftingResults().MinAdjFsFailure;

      LOG(_T("FSf = ") << FSf);

      if ( 0.95*FSfMin < FSf && bLargeStepSize)
      {
         // we are getting close... Use a smaller step size
         Float64 oldInc = locInc;
         locInc = smallInc;
         bLargeStepSize = false;
         if ( 1.05*FSfMin <= FSf )
         {
            // We went past the solution... back up
            LOG(_T("Went past the solution... backup"));
            loc -= oldInc;

            if ( loc < min_location )
               loc = min_location - locInc; // increment will be added back with loop continues

            FSf = 0; // don't want to pass the test below
         }
      }

      if ( FSfMin <= FSf )
      {
         artifact = curr_artifact;
         break;
      }

      loc += locInc;
   }

   if ( maxLoc < loc )
   {
      // Temporary strands are required... 
      LOG(_T("Cannot find a pick point to satisfy FSf"));
      LOG(_T("Temporary strands required"));
      LOG(_T("Move on to Shipping Design"));
      return { pgsDesignCodes::LiftingRedesignAfterShipping,artifact };
   }

   return { pgsDesignCodes::LiftingConfigChanged,artifact };
}

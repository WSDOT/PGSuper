///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <PgsExt\PgsExtLib.h>

#include <IFace\PointOfInterest.h>
#include <IFace\Bridge.h>
#include <IFace\StatusCenter.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>

#include "..\PGSuperException.h"

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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsGirderLiftingChecker
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////


//======================== LIFECYCLE  =======================================
pgsGirderLiftingChecker::pgsGirderLiftingChecker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidLiftingSupportLocationError   = pStatusCenter->RegisterCallback( new pgsLiftingSupportLocationStatusCallback(m_pBroker,eafTypes::statusError) );
   m_scidLiftingSupportLocationWarning = pStatusCenter->RegisterCallback( new pgsLiftingSupportLocationStatusCallback(m_pBroker,eafTypes::statusWarning) );
}

pgsGirderLiftingChecker::~pgsGirderLiftingChecker()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

void pgsGirderLiftingChecker::CheckLifting(const CSegmentKey& segmentKey,stbLiftingCheckArtifact* pArtifact)
{
   GET_IFACE(ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);

   if (pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled())
   {
       // Use poi's from global pool
      GET_IFACE(ISegmentLiftingPointsOfInterest,pSegmentLiftingPointsOfInterest);
      HANDLINGCONFIG dummy_config;

      // Compute lifting response
      AnalyzeLifting(segmentKey,false,dummy_config,pSegmentLiftingPointsOfInterest,pArtifact);
   }
}

void pgsGirderLiftingChecker::AnalyzeLifting(const CSegmentKey& segmentKey,Float64 supportLoc,stbLiftingCheckArtifact* pArtifact)
{
   GET_IFACE(ISegmentLiftingPointsOfInterest,pSegmentLiftingPointsOfInterest);
   HANDLINGCONFIG dummy_config;
   dummy_config.bIgnoreGirderConfig = true;
   dummy_config.LeftOverhang = supportLoc;
   dummy_config.RightOverhang = supportLoc;
   AnalyzeLifting(segmentKey,true,dummy_config,pSegmentLiftingPointsOfInterest,pArtifact);
}

void pgsGirderLiftingChecker::AnalyzeLifting(const CSegmentKey& segmentKey,const HANDLINGCONFIG& liftConfig,ISegmentLiftingDesignPointsOfInterest* pPoiD, stbLiftingCheckArtifact* pArtifact, const stbLiftingStabilityProblem** ppStabilityProblem)
{
   AnalyzeLifting(segmentKey,true,liftConfig,pPoiD,pArtifact,ppStabilityProblem);
}

void pgsGirderLiftingChecker::AnalyzeLifting(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& liftConfig,ISegmentLiftingDesignPointsOfInterest* pPoiD,stbLiftingCheckArtifact* pArtifact,const stbLiftingStabilityProblem** ppStabilityProblem)
{
   GET_IFACE(IGirder,pGirder);
   const stbGirder* pStabilityModel = bUseConfig ? pGirder->GetSegmentStabilityModel(segmentKey,liftConfig) : pGirder->GetSegmentStabilityModel(segmentKey);
   const stbLiftingStabilityProblem* pStabilityProblem = bUseConfig ? pGirder->GetSegmentLiftingStabilityProblem(segmentKey,liftConfig,pPoiD) : pGirder->GetSegmentLiftingStabilityProblem(segmentKey);
   if ( ppStabilityProblem )
   {
      *ppStabilityProblem = pStabilityProblem;
   }

   GET_IFACE(ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   stbLiftingCriteria criteria = (bUseConfig ? pSegmentLiftingSpecCriteria->GetLiftingStabilityCriteria(segmentKey,liftConfig) : pSegmentLiftingSpecCriteria->GetLiftingStabilityCriteria(segmentKey));

   GET_IFACE(IDocumentUnitSystem,pDocUnitSystem);
   CComPtr<IUnitServer> unitServer;
   pDocUnitSystem->GetUnitServer(&unitServer);

   CComPtr<IUnitConvert> unitConvert;
   unitServer->get_UnitConvert(&unitConvert);

   stbStabilityEngineer engineer(unitConvert);
   *pArtifact = engineer.CheckLifting(pStabilityModel,pStabilityProblem,criteria);
}

pgsDesignCodes::OutcomeType pgsGirderLiftingChecker::DesignLifting(const CSegmentKey& segmentKey,HANDLINGCONFIG& config,ISegmentLiftingDesignPointsOfInterest* pPoiD,stbLiftingCheckArtifact* pArtifact,const stbLiftingStabilityProblem** ppStabilityProblem,SHARED_LOGFILE LOGFILE)
{
   //
   // Range of lifting loop locations and step increment
   //
   GET_IFACE(ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   Float64 min_location = Max(pSegmentLiftingSpecCriteria->GetMinimumLiftingPointLocation(segmentKey,pgsTypes::metStart),pSegmentLiftingSpecCriteria->GetMinimumLiftingPointLocation(segmentKey,pgsTypes::metEnd));
   Float64 location_accuracy = pSegmentLiftingSpecCriteria->GetLiftingPointLocationAccuracy();

   Float64 bigInc = 8*location_accuracy;
   Float64 smallInc = location_accuracy;
   Float64 locInc = bigInc;
   bool bLargeStepSize = true;

   // Max location may be limited by harping point (actually, just before it stopping at the last increment value)
   // But allowing more than 40% of the girder length makes no sense (think rigid-body instability for riggers)
   GET_IFACE(IBridge, pBridge);
   Float64 girder_length = pBridge->GetSegmentLength(segmentKey);

   Float64 maxLoc = 0.4*girder_length;

   if (0 < config.GdrConfig.PrestressConfig.GetStrandCount(pgsTypes::Harped)) // only look at harping point if we have harped strands
   {
      Float64 lhp,rhp;
      GET_IFACE(IStrandGeometry,pStrandGeom);
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

   stbLiftingCheckArtifact artifact;
   while ( loc <= maxLoc )
   {
      LOG(_T(""));
      LOG(_T("Trying location ") << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << _T(" ft"));

      stbLiftingCheckArtifact curr_artifact;
      config.LeftOverhang = loc;
      config.RightOverhang = loc;

      AnalyzeLifting(segmentKey,config,pPoiD,&curr_artifact,ppStabilityProblem);
      FSf = curr_artifact.GetLiftingResults().MinAdjFsFailure;

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
      LOG(_T("Cannot find a pick point to safisfy FSf"));
      LOG(_T("Temporary strands required"));
      LOG(_T("Move on to Shipping Design"));
      return pgsDesignCodes::LiftingRedesignAfterShipping;
   }

   *pArtifact = artifact;

   return pgsDesignCodes::LiftingConfigChanged;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsGirderLiftingChecker::AssertValid() const
{
   return true;
}

void pgsGirderLiftingChecker::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsGirderLiftingChecker" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsGirderLiftingChecker::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsGirderLiftingChecker");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsGirderLiftingChecker");

   TESTME_EPILOG("GirderHandlingChecker");
}
#endif // _UNITTEST

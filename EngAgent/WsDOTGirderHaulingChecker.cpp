///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#include <IFace\Artifact.h>

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

#include <PGSuperException.h>

#include <limits>
#include <algorithm>

#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderModelFactory.H>

#include "DesignCodes.h"
#include "GirderHandlingChecker.h"
#include "AlternativeTensileStressCalculator.h"

#include "WsdotGirderHaulingChecker.h"
#include "StatusItems.h"

#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool CompareHaulTrucks(const HaulTruckLibraryEntry* pA,const HaulTruckLibraryEntry* pB)
{
   // first compare roll stiffness
   if ( pA->GetRollStiffness() < pB->GetRollStiffness() )
      return true;

   if ( pB->GetRollStiffness() < pA->GetRollStiffness() )
      return false;

   // next compare axle width
   if ( pA->GetAxleWidth() < pB->GetAxleWidth() )
      return true;

   if ( pB->GetAxleWidth() < pA->GetAxleWidth() )
      return false;

   // next compare height of girder above roll center (want the one with the greatest distance... it is more unstable)
   if ( pB->GetBottomOfGirderHeight() - pB->GetRollCenterHeight() < pA->GetBottomOfGirderHeight() - pA->GetRollCenterHeight() )
      return true;

   if ( pA->GetBottomOfGirderHeight() - pA->GetRollCenterHeight() < pB->GetBottomOfGirderHeight() - pB->GetRollCenterHeight() )
      return false;

   // next compare haul capacity
   if ( pA->GetMaxGirderWeight() < pB->GetMaxGirderWeight() )
      return true;

   if ( pB->GetMaxGirderWeight() < pA->GetMaxGirderWeight() )
      return false;

   if ( pA->GetMaximumLeadingOverhang() < pB->GetMaximumLeadingOverhang() )
      return true;

   if ( pB->GetMaximumLeadingOverhang() < pA->GetMaximumLeadingOverhang() )
      return false;

   if ( pA->GetMaxDistanceBetweenBunkPoints() < pB->GetMaxDistanceBetweenBunkPoints() )
      return true;

   if ( pB->GetMaxDistanceBetweenBunkPoints() < pA->GetMaxDistanceBetweenBunkPoints() )
      return false;

   return true;
}

/****************************************************************************
CLASS
   pgsWsdotGirderHaulingChecker
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////


//======================== LIFECYCLE  =======================================
pgsWsdotGirderHaulingChecker::pgsWsdotGirderHaulingChecker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidBunkPointLocation = pStatusCenter->RegisterCallback( new pgsBunkPointLocationStatusCallback(m_pBroker) );
   m_scidHaulTruck         = pStatusCenter->RegisterCallback( new pgsHaulTruckStatusCallback(m_pBroker) );
}

pgsWsdotGirderHaulingChecker::~pgsWsdotGirderHaulingChecker()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

pgsHaulingAnalysisArtifact* pgsWsdotGirderHaulingChecker::CheckHauling(const CSegmentKey& segmentKey, SHARED_LOGFILE LOGFILE)
{
   GET_IFACE(ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);

   if (pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      // AnalyzeHauling should be generic. It will analyze the configuration
      // passed to it, not the current configuration of the girder
      return AnalyzeHauling(segmentKey);
   }
   else
   {
      return nullptr;
   }
}

pgsHaulingAnalysisArtifact*  pgsWsdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey)
{
   std::unique_ptr<pgsWsdotHaulingAnalysisArtifact> pArtifact(std::make_unique<pgsWsdotHaulingAnalysisArtifact>());

   HANDLINGCONFIG dummy_config;
   GET_IFACE(ISegmentHaulingPointsOfInterest,pSegmentHaulingPointsOfInterest); // poi's from global pool
   AnalyzeHauling(segmentKey,false,dummy_config,pSegmentHaulingPointsOfInterest,pArtifact.get());

   return pArtifact.release();
}

pgsHaulingAnalysisArtifact* pgsWsdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,Float64 leftOverhang,Float64 rightOverhang)
{
   std::unique_ptr<pgsWsdotHaulingAnalysisArtifact> pArtifact(std::make_unique<pgsWsdotHaulingAnalysisArtifact>());

   HANDLINGCONFIG dummy_config;
   dummy_config.bIgnoreGirderConfig = true;
   dummy_config.LeftOverhang = leftOverhang;
   dummy_config.RightOverhang = rightOverhang;

   GET_IFACE(ISegmentHaulingPointsOfInterest,pSegmentHaulingPointsOfInterest); // poi's from global pool

   AnalyzeHauling(segmentKey,true,dummy_config,pSegmentHaulingPointsOfInterest,pArtifact.get());
   return pArtifact.release();
}

pgsHaulingAnalysisArtifact* pgsWsdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,const HANDLINGCONFIG& haulConfig,ISegmentHaulingDesignPointsOfInterest* pPOId)
{
   std::unique_ptr<pgsWsdotHaulingAnalysisArtifact> pArtifact(std::make_unique<pgsWsdotHaulingAnalysisArtifact>());
   AnalyzeHauling(segmentKey,true,haulConfig,pPOId,pArtifact.get());

   return pArtifact.release();
}

void pgsWsdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& haulConfig,ISegmentHaulingDesignPointsOfInterest* pPOId,pgsWsdotHaulingAnalysisArtifact* pArtifact)
{
   stbHaulingCheckArtifact artifact;
#if defined _DEBUG
   AnalyzeHauling(segmentKey,bUseConfig,haulConfig,pPOId,&artifact,&pArtifact->m_pStabilityProblem);
#else
   AnalyzeHauling(segmentKey,bUseConfig,haulConfig,pPOId,&artifact);
#endif
   pArtifact->SetHaulingCheckArtifact(artifact);
}

pgsHaulingAnalysisArtifact* pgsWsdotGirderHaulingChecker::DesignHauling(const CSegmentKey& segmentKey,HANDLINGCONFIG& shipping_config,bool bIgnoreConfigurationLimits,ISegmentHaulingDesignPointsOfInterest* pPOId,bool* bSuccess, SHARED_LOGFILE LOGFILE)
{
   LOG(_T("Entering pgsWsdotGirderHaulingChecker::DesignHauling"));
   std::unique_ptr<pgsWsdotHaulingAnalysisArtifact> artifact(std::make_unique<pgsWsdotHaulingAnalysisArtifact>());

   // Get all of the haul trucks that have sufficient capacity to carry the girder
   GET_IFACE(ISectionProperties,pSectProps);
   Float64 Wgt = pSectProps->GetSegmentWeight(segmentKey);
   GET_IFACE(ILibraryNames,pLibNames);
   std::vector<std::_tstring> names;
   pLibNames->EnumHaulTruckNames( &names );
   if (names.size() == 0)
   {
      // the haul truck library is empty... this is a problem.
      GET_IFACE(IEAFStatusCenter, pStatusCenter);

      CString strMsg = _T("The haul truck library is empty. Please define haul trucks in the Haul Truck library.");
      pgsHaulTruckStatusItem* pStatusItem = new pgsHaulTruckStatusItem(m_StatusGroupID, m_scidHaulTruck, strMsg);
      pStatusCenter->Add(pStatusItem);

      strMsg += _T("\nSee Status Center for details");
      THROW_UNWIND(strMsg, -1);
   }
   std::vector<const HaulTruckLibraryEntry*> vHaulTrucks;
   GET_IFACE(ILibrary,pLib);
   const HaulTruckLibraryEntry* pMaxCapacityTruck = pLib->GetHaulTruckEntry(names.front().c_str()); // keep track of the truck with the max capacity
   for (const auto& strHaulTruckName : names)
   {
      const HaulTruckLibraryEntry* pHaulTruck = pLib->GetHaulTruckEntry(strHaulTruckName.c_str());
      if ( CompareHaulTrucks(pMaxCapacityTruck,pHaulTruck) )
      {
         pMaxCapacityTruck = pHaulTruck;
      }

      if ( ::IsGE(Wgt,pHaulTruck->GetMaxGirderWeight()) )
      {
         vHaulTrucks.push_back(pHaulTruck);
      }
   }

   if ( vHaulTrucks.size() == 0 )
   {
      LOG(_T("There aren't any trucks capable of hauling this girder - using the truck with the maximum haul capacity"));
      vHaulTrucks.push_back(pMaxCapacityTruck);
   }

   // sort the haul trucks from least to most capable
   std::sort(vHaulTrucks.begin(),vHaulTrucks.end(),CompareHaulTrucks);


   // Get range of values for truck support locations
   GET_IFACE(IBridge,pBridge);
   Float64 Lg = pBridge->GetSegmentLength(segmentKey);

   GET_IFACE(ISegmentHaulingSpecCriteria,pCriteria);
   Float64 FScrMin = pCriteria->GetHaulingCrackingFs();
   Float64 FSrMin = pCriteria->GetHaulingRolloverFs();
   LOG(_T("Allowable FS cracking FScrMin = ")<<FScrMin);
   LOG(_T("Allowable FS rollover FSrMin = ")<<FSrMin);


   Float64 location_accuracy = pCriteria->GetHaulingSupportLocationAccuracy();
   const Float64 bigInc = Max(10*location_accuracy,ConvertToSysUnits(5.0,unitMeasure::Feet));
   const Float64 mediumInc = bigInc / 2;
   const Float64 smallInc = location_accuracy;
   const int bigStep = 1;
   const int mediumStep = 2;
   const int smallStep = 3;

   Float64 min_overhang_start = pCriteria->GetMinimumHaulingSupportLocation(segmentKey,pgsTypes::metStart);
   Float64 min_overhang_end   = pCriteria->GetMinimumHaulingSupportLocation(segmentKey,pgsTypes::metEnd);

   for (const auto& pHaulTruck : vHaulTrucks)
   {
      LOG(_T("Attempting design for haul truck ") << pHaulTruck->GetName().c_str());
      shipping_config.pHaulTruckEntry = pHaulTruck;

      Float64 maxDistanceBetweenSupports = pHaulTruck->GetMaxDistanceBetweenBunkPoints();
      if ( bIgnoreConfigurationLimits )
      {
         // if we are ignoring the shipping configuration limits the max distance between supports
         // is the girder length less the min support location
         maxDistanceBetweenSupports = Lg - min_overhang_start - min_overhang_end;
      }

      Float64 min_location = Max(min_overhang_start,min_overhang_end);

      Float64 minOverhang = (Lg - maxDistanceBetweenSupports)/2.;
      if ( minOverhang < 0 || bIgnoreConfigurationLimits)
      {
         minOverhang = min_location;
      }

      Float64 maxOverhang = Lg/2;
      if ( maxOverhang < minOverhang )
      {
         Float64 temp = maxOverhang;
         maxOverhang = minOverhang;
         minOverhang = temp;
      }

      Float64 maxLeadingOverhang = pHaulTruck->GetMaximumLeadingOverhang();

      if ( bIgnoreConfigurationLimits )
      {
         maxLeadingOverhang = Lg/2;
      }

      Float64 inc = bigInc;
      int stepSize = bigStep;

      Float64 loc = minOverhang;

      Float64 lastFScr = 2*FScrMin;

      while ( loc < maxOverhang )
      {
         LOG(_T(""));

         pgsWsdotHaulingAnalysisArtifact curr_artifact;

         if ( maxLeadingOverhang < loc && !bIgnoreConfigurationLimits)
         {
            shipping_config.RightOverhang = maxLeadingOverhang;
            shipping_config.LeftOverhang = (loc - maxLeadingOverhang) + loc;
         }
         else
         {
            shipping_config.LeftOverhang  = loc;
            shipping_config.RightOverhang = loc;
         }

         // make sure the geometry is correct
#if defined _DEBUG
         if ( !bIgnoreConfigurationLimits)
         {
            ATLASSERT( IsLE((Lg - shipping_config.LeftOverhang - shipping_config.RightOverhang),maxDistanceBetweenSupports) );
         }
#endif

         LOG(_T("Trying Trailing Overhang = ") << ::ConvertFromSysUnits(shipping_config.LeftOverhang,unitMeasure::Feet) << _T(" ft") << _T("      Leading Overhang = ") << ::ConvertFromSysUnits(shipping_config.RightOverhang,unitMeasure::Feet) << _T(" ft"));

         LOG_EXECUTION_TIME(AnalyzeHauling(segmentKey,true,shipping_config,pPOId,&curr_artifact));

         Float64 FScr = Min(curr_artifact.GetMinFsForCracking(pgsTypes::CrownSlope),curr_artifact.GetMinFsForCracking(pgsTypes::Superelevation));

         LOG(_T("FScr = ") << FScr);
         if ( FScr < FScrMin && ((FScr < lastFScr && lastFScr < FScrMin) || maxOverhang/4 < loc) )
         {
            // Moving the supports closer isn't going to help
            LOG(_T("Could not satisfy FScr... Try next haul truck"));
            break; // next haul truck
         }
         lastFScr = FScr;

         Float64 FSr = Min(curr_artifact.GetFsRollover(pgsTypes::CrownSlope),curr_artifact.GetFsRollover(pgsTypes::Superelevation));
         LOG(_T("FSr = ") << FSr);

         Float64 fra = (stepSize == bigStep ? 0.990 : 0.995);
         if ((stepSize == bigStep || stepSize == mediumStep) && fra*FScrMin < FScr && fra*FSrMin < FSr)
         {
            // we are getting close... Use a smaller step size
            Float64 oldInc = inc;
            if (stepSize == bigStep)
            {
               LOG(_T("Swithcing to medium step size"));
               inc = mediumInc;
               stepSize = mediumStep;
            }
            else
            {
               ATLASSERT(stepSize == mediumStep);
               LOG(_T("Swithcing to small step size"));
               inc = smallInc;
               stepSize = smallStep;
            }

            if ( 1.05*FSrMin <= FSr && !IsEqual(loc,minOverhang) )
            {
               // We went past the solution and we aren't at the first location... 
               // back up
               LOG(_T("Went past the solution... backup"));
               loc -= oldInc;

               if ( loc < minOverhang )
               {
                  loc = minOverhang-inc; // inc will be added back when the loop continues
               }

               FSr = 0; // don't want to pass the test below
               lastFScr = 2*FScrMin; // reset because we've changed loc
            }

         }

         if ( FScrMin <= FScr && FSrMin <= FSr )
         {
            LOG(_T("Found a stable hauling configuration, now check stresses"));
            std::unique_ptr<pgsHaulingAnalysisArtifact> pAnalysisArtifact(AnalyzeHauling(segmentKey, shipping_config, pPOId));

            *bSuccess = pAnalysisArtifact->Passed(bIgnoreConfigurationLimits);
            *artifact = curr_artifact;
            LOG(_T("Stress check was ") << (*bSuccess ? _T("success") : _T("unsuccessful")));

            if (*bSuccess || IsEqual(loc,minOverhang))
            {
               // obviously, we are done if the stress check passed
               // if the stress check failed, but we have a stable configuration and the bunks are
               // at the minimum location, going to a stiffer truck won't help so just exit
               // and let the designer take corrective actions.
               LOG(_T("Exiting pgsWsdotGirderHaulingChecker::DesignHauling - ") << (*bSuccess ? _T("with successful design") : _T("stability satisfied with minimum overhangs - stiffer haul truck wont help stresses")));
               return artifact.release();
            }
            else
            {
               break; // go to next truck
            }
         }

         loc += inc;
      } // next bunk point location

      LOG(_T("")); // blank line before starting next truc
   } // next haul truck

   LOG(_T("A successful design could not be found with any of the haul trucks. Add temporary strands and try again"));
   *bSuccess = false;
   return artifact.release();
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
bool pgsWsdotGirderHaulingChecker::AssertValid() const
{
   return true;
}

void pgsWsdotGirderHaulingChecker::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsWsdotGirderHaulingChecker" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsWsdotGirderHaulingChecker::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsWsdotGirderHaulingChecker");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsWsdotGirderHaulingChecker");

   TESTME_EPILOG("GirderHandlingChecker");
}
#endif // _UNITTEST


////////////////////////////////////////////////////////
// hauling
////////////////////////////////////////////////////////
#if defined _DEBUG
void pgsWsdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& config,ISegmentHaulingDesignPointsOfInterest* pPOId,stbHaulingCheckArtifact* pArtifact,const stbHaulingStabilityProblem** ppStabilityProblem)
#else
void pgsWsdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& config,ISegmentHaulingDesignPointsOfInterest* pPOId,stbHaulingCheckArtifact* pArtifact)
#endif
{
   GET_IFACE(IGirder,pGirder);
   const stbGirder* pStabilityModel = pGirder->GetSegmentHaulingStabilityModel(segmentKey);
   const stbHaulingStabilityProblem* pStabilityProblem = (bUseConfig ? pGirder->GetSegmentHaulingStabilityProblem(segmentKey,config,pPOId) : pGirder->GetSegmentHaulingStabilityProblem(segmentKey));

#if defined _DEBUG
   *ppStabilityProblem = pStabilityProblem;
#endif

   GET_IFACE(ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   stbHaulingCriteria criteria = (bUseConfig ? pSegmentHaulingSpecCriteria->GetHaulingStabilityCriteria(segmentKey,config) : pSegmentHaulingSpecCriteria->GetHaulingStabilityCriteria(segmentKey));

   stbStabilityEngineer engineer;
   *pArtifact = engineer.CheckHauling(pStabilityModel,pStabilityProblem,criteria);
}

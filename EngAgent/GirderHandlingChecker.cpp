///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <EAF\EAFDisplayUnits.h>

#include "..\PGSuperException.h"

#include <limits>
#include <algorithm>

#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderModelFactory.H>

#include "DesignCodes.h"

#include "GirderHandlingChecker.h"
#include "StatusItems.h"

#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


inline void ComputeReqdFcTens(Float64 ft, // stress demand
                              Float64 rcsT, bool rcsBfmax, Float64 rcsFmax, Float64 rcsTalt, // allowable stress coeff's
                              Float64* pFcNo,Float64* pFcWithRebar)
{
   if ( 0 < ft )
   {
      // Without rebar
      if ( rcsBfmax &&  ft>rcsFmax)
      {
         // allowable stress is limited and we hit the limit
         *pFcNo = -1;
      }
      else
      {
         *pFcNo = pow(ft/rcsT,2);
      }

      // With rebar
      *pFcWithRebar = pow(ft/rcsTalt,2);

   }
   else
   {
      // Compression
      *pFcNo = 0.0;
      *pFcWithRebar = 0.0;
   }
}

/****************************************************************************
CLASS
   pgsGirderHandlingChecker
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////


//======================== LIFECYCLE  =======================================
pgsGirderHandlingChecker::pgsGirderHandlingChecker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidLiftingSupportLocationError   = pStatusCenter->RegisterCallback( new pgsLiftingSupportLocationStatusCallback(m_pBroker,eafTypes::statusError) );
   m_scidLiftingSupportLocationWarning = pStatusCenter->RegisterCallback( new pgsLiftingSupportLocationStatusCallback(m_pBroker,eafTypes::statusWarning) );
   m_scidBunkPointLocation             = pStatusCenter->RegisterCallback( new pgsBunkPointLocationStatusCallback(m_pBroker) );
   m_scidTruckStiffness                = pStatusCenter->RegisterCallback( new pgsTruckStiffnessStatusCallback(m_pBroker) );

   m_Model.CoCreateInstance(CLSID_Fem2dModel);
}

pgsGirderHandlingChecker::~pgsGirderHandlingChecker()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

void pgsGirderHandlingChecker::CheckHauling(const CSegmentKey& segmentKey,pgsHaulingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);

   if (pGirderHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      // AnalyzeHauling should be generic. It will analyze the configuration
      // passed to it, not the current configuration of the girder
      AnalyzeHauling(segmentKey,pArtifact);
   }
}

void pgsGirderHandlingChecker::CheckLifting(const CSegmentKey& segmentKey,pgsLiftingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);

   if (pGirderLiftingSpecCriteria->IsLiftingAnalysisEnabled())
   {
//      // calc some initial information
//      PrepareLiftingCheckArtifact(segmentKey,pArtifact);
//
//
//      // AnalyzeLifting should be generic. It will analyze the configuration
//      // passed to it, not the current configuration of the girder
//      AnalyzeLifting(segmentKey,pArtifact);

      // Use poi's from global pool
      GET_IFACE(IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
      HANDLINGCONFIG dummy_config;

      // Compute lifting response
      AnalyzeLifting(segmentKey,false,dummy_config,pGirderLiftingPointsOfInterest,pArtifact);
   }
}

void pgsGirderHandlingChecker::AnalyzeLifting(const CSegmentKey& segmentKey,const HANDLINGCONFIG& liftConfig,IGirderLiftingDesignPointsOfInterest* pPoiD, pgsLiftingAnalysisArtifact* pArtifact)
{
   AnalyzeLifting(segmentKey,true,liftConfig,pPoiD,pArtifact);
}

void pgsGirderHandlingChecker::AnalyzeLifting(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& liftConfig,IGirderLiftingDesignPointsOfInterest* pPoiD,pgsLiftingAnalysisArtifact* pArtifact)
{
   // calc some initial information
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);

   Float64 Loh, Roh, Eci, Fci;
   pgsTypes::ConcreteType concType;

   std::vector<pgsPointOfInterest> poi_vec;
   if ( bUseConfig )
   {
      Loh = liftConfig.LeftOverhang;
      Roh = liftConfig.RightOverhang;

      if ( liftConfig.GdrConfig.bUserEci )
         Eci = liftConfig.GdrConfig.Eci;
      else
         Eci = pMaterial->GetEconc(liftConfig.GdrConfig.Fci, pMaterial->GetSegmentStrengthDensity(segmentKey),
                                                             pMaterial->GetSegmentEccK1(segmentKey),
                                                             pMaterial->GetSegmentEccK2(segmentKey));

      Fci = liftConfig.GdrConfig.Fci;
      concType = liftConfig.GdrConfig.ConcType;

      poi_vec = pPoiD->GetLiftingDesignPointsOfInterest(segmentKey,Loh);
   }
   else
   {
      GET_IFACE(IGirderLifting,pGirderLifting);
      Loh = pGirderLifting->GetLeftLiftingLoopLocation(segmentKey);
      Roh = pGirderLifting->GetRightLiftingLoopLocation(segmentKey);

      GET_IFACE(IMaterials,pMaterial);
      Eci = pMaterial->GetSegmentEc(segmentKey,liftSegmentIntervalIdx);
      Fci = pMaterial->GetSegmentFc(segmentKey,liftSegmentIntervalIdx);
      concType = pMaterial->GetSegmentConcreteType(segmentKey);

      GET_IFACE(IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
      poi_vec = pGirderLiftingPointsOfInterest->GetLiftingPointsOfInterest(segmentKey,0,POIFIND_OR);
   }
   
   PrepareLiftingAnalysisArtifact(segmentKey,Loh,Roh,Fci,Eci,concType,pArtifact);

   pArtifact->SetLiftingPointsOfInterest(poi_vec);

   // get moments at pois  and mid-span deflection due to dead vertical lifting
   std::vector<Float64> moment_vec;
   Float64 mid_span_deflection;
   ComputeLiftingMoments(segmentKey, *pArtifact, poi_vec, &moment_vec,&mid_span_deflection);

   ComputeLiftingStresses(segmentKey, bUseConfig, liftConfig, poi_vec, moment_vec, pArtifact);

   if (ComputeLiftingFsAgainstCracking(segmentKey, bUseConfig, liftConfig, poi_vec, moment_vec, mid_span_deflection, pArtifact))
   {
      ComputeLiftingFsAgainstFailure(segmentKey, pArtifact);
   }
   else
   {
      pArtifact->SetThetaFailureMax(0.0);
      pArtifact->SetFsFailure(0.0);
   }
}

void pgsGirderHandlingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,pgsHaulingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest); // poi's from global pool

   HANDLINGCONFIG dummy_config;
   AnalyzeHauling(segmentKey,false,dummy_config,pGirderHaulingPointsOfInterest,pArtifact);
}

void pgsGirderHandlingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,const HANDLINGCONFIG& haulConfig,IGirderHaulingDesignPointsOfInterest* pPOId,pgsHaulingAnalysisArtifact* pArtifact)
{
   AnalyzeHauling(segmentKey,true,haulConfig,pPOId,pArtifact);
}

void pgsGirderHandlingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& haulConfig,IGirderHaulingDesignPointsOfInterest* pPOId,pgsHaulingAnalysisArtifact* pArtifact)
{
   // calc some initial information
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);

   Float64 Loh, Roh, Ec, Fc;
   pgsTypes::ConcreteType concType;

   std::vector<pgsPointOfInterest> poi_vec;
   if ( bUseConfig )
   {
      Loh = haulConfig.LeftOverhang;
      Roh = haulConfig.RightOverhang;

      if ( haulConfig.GdrConfig.bUserEc )
         Ec = haulConfig.GdrConfig.Ec;
      else
         Ec = pMaterial->GetEconc(haulConfig.GdrConfig.Fc, pMaterial->GetSegmentStrengthDensity(segmentKey),
                                                           pMaterial->GetSegmentEccK1(segmentKey),
                                                           pMaterial->GetSegmentEccK2(segmentKey));

      Fc = haulConfig.GdrConfig.Fc;
      concType = haulConfig.GdrConfig.ConcType;

      poi_vec = pPOId->GetHaulingDesignPointsOfInterest(segmentKey,Loh,Roh);
   }
   else
   {
      GET_IFACE(IGirderHauling,pGirderHauling);
      Loh = pGirderHauling->GetTrailingOverhang(segmentKey);
      Roh = pGirderHauling->GetLeadingOverhang(segmentKey);

      Fc = pMaterial->GetSegmentFc(segmentKey,haulSegmentIntervalIdx);
      Ec = pMaterial->GetSegmentEc(segmentKey,haulSegmentIntervalIdx);
      concType = pMaterial->GetSegmentConcreteType(segmentKey);

      GET_IFACE(IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);
      poi_vec = pGirderHaulingPointsOfInterest->GetHaulingPointsOfInterest(segmentKey,0,POIFIND_OR);
   }

   PrepareHaulingAnalysisArtifact(segmentKey,Loh,Roh,Fc,Ec,concType,pArtifact);


   pArtifact->SetHaulingPointsOfInterest(poi_vec);

   // get moments at pois  and mid-span deflection due to dead vertical hauling
   std::vector<Float64> moment_vec;
   Float64 mid_span_deflection;
   ComputeHaulingMoments(segmentKey, *pArtifact, poi_vec, &moment_vec,&mid_span_deflection);

   ComputeHaulingRollAngle(segmentKey, pArtifact, poi_vec, &moment_vec,&mid_span_deflection);

   ComputeHaulingStresses(segmentKey, bUseConfig, haulConfig, poi_vec, moment_vec, pArtifact);

   ComputeHaulingFsForCracking(segmentKey, poi_vec, moment_vec, pArtifact);

   ComputeHaulingFsForRollover(segmentKey, pArtifact);
}

bool pgsGirderHandlingChecker::DesignShipping(const CSegmentKey& segmentKey,const GDRCONFIG& config,bool bDesignForEqualOverhangs,bool bIgnoreConfigurationLimits,IGirderHaulingDesignPointsOfInterest* pPOId,pgsHaulingAnalysisArtifact* pArtifact,SHARED_LOGFILE LOGFILE)
{
   LOG(_T("Entering pgsGirderHandlingChecker::DesignShipping"));
   // Get range of values for truck support locations
   GET_IFACE(IBridge,pBridge);
   Float64 Lg = pBridge->GetSegmentLength(segmentKey);

   GET_IFACE(IGirderHaulingSpecCriteria,pCriteria);
   Float64 maxDistanceBetweenSupports = pCriteria->GetAllowableDistanceBetweenSupports();
   Float64 min_overhang_start = pCriteria->GetMinimumHaulingSupportLocation(segmentKey,pgsTypes::metStart);
   Float64 min_overhang_end   = pCriteria->GetMinimumHaulingSupportLocation(segmentKey,pgsTypes::metEnd);
   if ( bIgnoreConfigurationLimits )
   {
      // if we are ignoring the shipping configuration limits the max distance between supports
      // is the girder length less the min support location
      maxDistanceBetweenSupports = Lg - min_overhang_start - min_overhang_end;
   }

   Float64 min_location      = max(min_overhang_start,min_overhang_end);
   Float64 location_accuracy = pCriteria->GetHaulingSupportLocationAccuracy();

   Float64 minOverhang = (Lg - maxDistanceBetweenSupports)/2.;
   if ( minOverhang < 0 || bDesignForEqualOverhangs)
      minOverhang = min_location;

   Float64 maxOverhang = Lg/2;
   if ( maxOverhang < minOverhang )
   {
      Float64 temp = maxOverhang;
      maxOverhang = minOverhang;
      minOverhang = temp;
   }

   Float64 maxLeadingOverhang = pCriteria->GetAllowableLeadingOverhang();

   if ( bIgnoreConfigurationLimits )
   {
      maxLeadingOverhang = Lg/2;
   }

   Float64 bigInc = 8*location_accuracy;
   Float64 smallInc = location_accuracy;
   Float64 inc = bigInc;
   bool bLargeStepSize = true;

   Float64 FScr = 0;
   Float64 FSr = 0;
   Float64 FScrMin = pCriteria->GetHaulingCrackingFs();
   Float64 FSrMin = pCriteria->GetHaulingRolloverFs();

   LOG(_T("Allowable FS cracking FScrMin = ")<<FScrMin);
   LOG(_T("Allowable FS rollover FSrMin = ")<<FSrMin);

   Float64 loc = minOverhang;
   pgsHaulingAnalysisArtifact artifact;

   HANDLINGCONFIG shipping_config;
   shipping_config.GdrConfig = config;

   while ( loc < maxOverhang )
   {
      LOG(_T(""));

      //
      pgsGirderHandlingChecker checker(m_pBroker,m_StatusGroupID);

      pgsHaulingAnalysisArtifact curr_artifact;

      if ( maxLeadingOverhang < loc && !bDesignForEqualOverhangs)
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
      if ( !bDesignForEqualOverhangs )
         ATLASSERT( IsLE((Lg - shipping_config.LeftOverhang - shipping_config.RightOverhang),maxDistanceBetweenSupports) );
#endif

      LOG(_T("Trying Trailing Overhang = ") << ::ConvertFromSysUnits(shipping_config.LeftOverhang,unitMeasure::Feet) << _T(" ft") << _T("      Leading Overhang = ") << ::ConvertFromSysUnits(shipping_config.RightOverhang,unitMeasure::Feet) << _T(" ft"));

      checker.AnalyzeHauling(segmentKey,shipping_config,pPOId,&curr_artifact);
      FScr = curr_artifact.GetMinFsForCracking();

      LOG(_T("FScr = ") << FScr);
      if ( FScr < FScrMin && maxOverhang/4 < loc )
      {
         LOG(_T("Could not satisfy FScr... Need to add temporary strands"));
         // Moving the supports closer isn't going to help
         *pArtifact = curr_artifact;
         return false; // design failed
      }

      FSr  = curr_artifact.GetFsRollover();

      LOG(_T("FSr = ") << FSr);

      if ( 0.95*FScrMin < FScr && 0.95*FSrMin < FSr && bLargeStepSize)
      {
         // we are getting close... Use a smaller step size
         Float64 oldInc = inc;
         inc = smallInc;
         bLargeStepSize = false;
         if ( 1.05*FSrMin <= FSr )
         {
            // We went past the solution... back up
            LOG(_T("Went past the solution... backup"));
            loc -= oldInc;

            if ( loc < minOverhang )
               loc = minOverhang-inc; // inc will be added back when the loop continues

            FSr = 0; // don't want to pass the test below
         }

      }

      if ( FScrMin <= FScr && FSrMin <= FSr )
      {
         artifact = curr_artifact;
         break;
      }

      loc += inc;
   }

   *pArtifact = artifact;

   LOG(_T("Exiting pgsGirderHandlingChecker::DesignShipping - success"));
   return true;
}

pgsDesignCodes::OutcomeType pgsGirderHandlingChecker::DesignLifting(const CSegmentKey& segmentKey,const GDRCONFIG& config,IGirderLiftingDesignPointsOfInterest* pPoiD,pgsLiftingAnalysisArtifact* pArtifact,SHARED_LOGFILE LOGFILE)
{
   //
   // Range of lifting loop locations and step increment
   //
   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   Float64 min_location = max(pGirderLiftingSpecCriteria->GetMinimumLiftingPointLocation(segmentKey,pgsTypes::metStart),pGirderLiftingSpecCriteria->GetMinimumLiftingPointLocation(segmentKey,pgsTypes::metEnd));
   Float64 location_accuracy = pGirderLiftingSpecCriteria->GetLiftingPointLocationAccuracy();

   Float64 bigInc = 8*location_accuracy;
   Float64 smallInc = location_accuracy;
   Float64 locInc = bigInc;
   bool bLargeStepSize = true;

   // Max location may be limited by harping point (actually, just before it stopping at the last increment value)
   // But allowing more than 40% of the girder length makes no sense (think rigid-body instability for riggers)
   GET_IFACE(IBridge, pBridge);
   Float64 girder_length = pBridge->GetSegmentLength(segmentKey);

   Float64 maxLoc = 0.4*girder_length;

   if (0 < config.PrestressConfig.GetNStrands(pgsTypes::Harped)) // only look at harping point if we have harped strands
   {
      Float64 lhp,rhp;
      GET_IFACE(IStrandGeometry,pStrandGeom);
      pStrandGeom->GetHarpingPointLocations(segmentKey,&lhp,&rhp);

      maxLoc = min(lhp, maxLoc);
   }


   // Find a lifting location that makes the factor of safety against failure
   // equal to 1.5 or better
   Float64 FSf = 0;
   Float64 FSfMin = pGirderLiftingSpecCriteria->GetLiftingFailureFs();
   Float64 loc = min_location;

   HANDLINGCONFIG lift_config;
   lift_config.GdrConfig = config;

   pgsLiftingAnalysisArtifact artifact;
   GET_IFACE(ILosses,pLosses);
   while ( loc <= maxLoc )
   {
      LOG(_T(""));
      LOG(_T("Trying location ") << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << _T(" ft"));

      pgsLiftingAnalysisArtifact curr_artifact;
      lift_config.LeftOverhang = loc;
      lift_config.RightOverhang = loc;

      AnalyzeLifting(segmentKey,lift_config,pPoiD,&curr_artifact);
      FSf = curr_artifact.GetFsFailure();

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
bool pgsGirderHandlingChecker::AssertValid() const
{
//#pragma Reminder("TODO: Implement the AssertValid method for pgsGirderHandlingChecker")
   return true;
}

void pgsGirderHandlingChecker::Dump(dbgDumpContext& os) const
{
//#pragma Reminder("TODO: Implement the Dump method for pgsGirderHandlingChecker")
   os << "Dump for pgsGirderHandlingChecker" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsGirderHandlingChecker::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsGirderHandlingChecker");

//#pragma Reminder("TODO: Implement the TestMe method for pgsGirderHandlingChecker")
   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsGirderHandlingChecker");

   TESTME_EPILOG("GirderHandlingChecker");
}
#endif // _UNITTEST

// lifting
void pgsGirderHandlingChecker::PrepareLiftingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 Loh,Float64 Roh,Float64 Fci,Float64 Eci,pgsTypes::ConcreteType concType,pgsLiftingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IBridge, pBridge);
   Float64 girder_length = pBridge->GetSegmentLength(segmentKey);
   pArtifact->SetGirderLength(girder_length);

   GET_IFACE(ISectionProperties,pSectProp);
   Float64 volume = pSectProp->GetVolume(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 density = pMaterial->GetSegmentWeightDensity(segmentKey,liftSegmentIntervalIdx);
   Float64 total_weight = volume * density * unitSysUnitsMgr::GetGravitationalAcceleration();
   pArtifact->SetGirderWeight(total_weight);

   GET_IFACE(IPretensionForce,pPrestressForce);
   Float64 XFerLength =  pPrestressForce->GetXferLength(segmentKey,pgsTypes::Permanent);
   pArtifact->SetXFerLength(XFerLength);

   Float64 span_len = girder_length - Loh - Roh;

   if (span_len <= 0.0)
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);

      LPCTSTR msg = _T("Lifting support overhang cannot exceed one-half of the span length");
      pgsLiftingSupportLocationStatusItem* pStatusItem = new pgsLiftingSupportLocationStatusItem(segmentKey,m_StatusGroupID,m_scidLiftingSupportLocationError,msg);
      pStatusCenter->Add(pStatusItem);

      std::_tstring str(msg);
      str += std::_tstring(_T("\nSee Status Center for details"));
      THROW_UNWIND(str.c_str(),-1);
   }

   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   pArtifact->SetAllowableFsForCracking(pGirderLiftingSpecCriteria->GetLiftingCrackingFs());
   pArtifact->SetAllowableFsForFailure(pGirderLiftingSpecCriteria->GetLiftingFailureFs());

   Float64 min_lift_point_start = pGirderLiftingSpecCriteria->GetMinimumLiftingPointLocation(segmentKey,pgsTypes::metStart);
   Float64 min_lift_point_end   = pGirderLiftingSpecCriteria->GetMinimumLiftingPointLocation(segmentKey,pgsTypes::metEnd);
   if ( Loh < min_lift_point_start )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format(_T("Left lift point is less than the minimum value of %s"),::FormatDimension(min_lift_point_start,pDisplayUnits->GetSpanLengthUnit()));
      pgsLiftingSupportLocationStatusItem* pStatusItem = new pgsLiftingSupportLocationStatusItem(segmentKey,m_StatusGroupID,m_scidLiftingSupportLocationWarning,strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   if ( Roh < min_lift_point_end )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format(_T("Right lift point is less than the minimum value of %s"),::FormatDimension(min_lift_point_end,pDisplayUnits->GetSpanLengthUnit()));
      pgsLiftingSupportLocationStatusItem* pStatusItem = new pgsLiftingSupportLocationStatusItem(segmentKey,m_StatusGroupID,m_scidLiftingSupportLocationWarning,strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   pArtifact->SetClearSpanBetweenPickPoints(span_len);
   pArtifact->SetOverhangs(Loh,Roh);

   Float64 bracket_hgt = pGirderLiftingSpecCriteria->GetHeightOfPickPointAboveGirderTop();
   Float64 ycgt = pSectProp->GetYtGirder(liftSegmentIntervalIdx,pgsPointOfInterest(segmentKey,0.00));
   Float64 e_hgt = bracket_hgt+ycgt;
   pArtifact->SetVerticalDistanceFromPickPointToGirderCg(e_hgt);

   Float64 upwardi, downwardi;
   pGirderLiftingSpecCriteria->GetLiftingImpact(&downwardi, &upwardi);
   pArtifact->SetUpwardImpact(upwardi);
   pArtifact->SetDownwardImpact(downwardi);

   pArtifact->SetSweepTolerance(pGirderLiftingSpecCriteria->GetLiftingSweepTolerance());
   pArtifact->SetLiftingDeviceTolerance(pGirderLiftingSpecCriteria->GetLiftingLoopPlacementTolerance());

   pArtifact->SetConcreteStrength(Fci);
   pArtifact->SetModRupture( pGirderLiftingSpecCriteria->GetLiftingModulusOfRupture(Fci,concType) );
   pArtifact->SetModRuptureCoefficient( pGirderLiftingSpecCriteria->GetLiftingModulusOfRuptureCoefficient(concType) );

   pArtifact->SetElasticModulusOfGirderConcrete(Eci);

   Float64 gird_weight = pArtifact->GetGirderWeight();
   Float64 ang = pGirderLiftingSpecCriteria->GetLiftingCableMinInclination();

   // Use the complementary angle = 180 - 90 - ang
   // This way we don't divide by zero if tan(theta) = 0
   Float64 axial = -(gird_weight/2) * tan(PI_OVER_2 - ang);
   pArtifact->SetAxialCompressiveForceDueToInclinationOfLiftingCables(axial);
   Float64 incl_moment = -axial * e_hgt;
   pArtifact->SetMomentInGirderDueToInclinationOfLiftingCables(incl_moment);
   pArtifact->SetInclinationOfLiftingCables(ang);

   Float64 offset_factor = span_len/girder_length;
   offset_factor = offset_factor*offset_factor - 1/3.;
   pArtifact->SetOffsetFactor(offset_factor);

   Float64 sweep = pGirderLiftingSpecCriteria->GetLiftingSweepTolerance();
   Float64 e_sweep = sweep * girder_length;
   pArtifact->SetEccentricityDueToSweep(e_sweep);

   Float64 e_placement = pGirderLiftingSpecCriteria->GetLiftingLoopPlacementTolerance();
   pArtifact->SetEccentricityDueToPlacementTolerance(e_placement);

   pArtifact->SetTotalInitialEccentricity(e_sweep*offset_factor + e_placement);
}

void pgsGirderHandlingChecker::ComputeLiftingMoments(const CSegmentKey& segmentKey,
                                                     const pgsLiftingAnalysisArtifact& rArtifact, 
                                                     const std::vector<pgsPointOfInterest>& rpoiVec,
                                                     std::vector<Float64>* pmomVec, 
                                                     Float64* pMidSpanDeflection)
{
   // build a model
   Float64 glen    = rArtifact.GetGirderLength();
   Float64 leftOH  = rArtifact.GetLeftOverhang();
   Float64 rightOH = rArtifact.GetRightOverhang();
   Float64 E = rArtifact.GetElasticModulusOfGirderConcrete();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   ComputeMoments(segmentKey,
                  liftSegmentIntervalIdx,
                  leftOH, glen, rightOH,
                  E,
                  POI_LIFT_SEGMENT,
                  rpoiVec,
                  pmomVec, pMidSpanDeflection);
}

void pgsGirderHandlingChecker::ComputeLiftingStresses(const CSegmentKey& segmentKey,bool bUseConfig,
                                                      const HANDLINGCONFIG& liftConfig,
                                                      const std::vector<pgsPointOfInterest>& rpoiVec,
                                                      const std::vector<Float64>& momVec,
                                                      pgsLiftingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IPretensionForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeometry);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(ISectionProperties,pSectProps);
   GET_IFACE(IShapes,pShapes);
   GET_IFACE(IMaterials,pMaterials);
   GET_IFACE(ILongRebarGeometry, pRebarGeom);
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);

   // get poi-independent values used for stress calc
   // factor in forces from inclined lifting cables
   Float64 p_lift = pArtifact->GetAxialCompressiveForceDueToInclinationOfLiftingCables();
   Float64 m_lift = pArtifact->GetMomentInGirderDueToInclinationOfLiftingCables();
   Float64 impact_up   = 1.0 - pArtifact->GetUpwardImpact();
   Float64 impact_down = 1.0 + pArtifact->GetDownwardImpact();

   IndexType psiz = rpoiVec.size();
   IndexType msiz = momVec.size();
   CHECK(psiz==msiz);

   Float64 glen    = pArtifact->GetGirderLength();
   Float64 leftOH  = pArtifact->GetLeftOverhang();
   Float64 rightOH = pArtifact->GetRightOverhang();

#if defined _DEBUG
   if ( bUseConfig )
   {
      ATLASSERT(IsEqual(leftOH,liftConfig.LeftOverhang));
      ATLASSERT(IsEqual(rightOH,liftConfig.RightOverhang));
   }
#endif

   // Get allowable tension for with and without rebar cases
   Float64 fLowTensAllowable  = pGirderLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStress(segmentKey);
   Float64 fHighTensAllowable = pGirderLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStress(segmentKey);
   Float64 fCompAllowable     = pGirderLiftingSpecCriteria->GetLiftingAllowableCompressiveConcreteStress(segmentKey);

   // Parameters for computing required concrete strengths
   Float64 rcsC = pGirderLiftingSpecCriteria->GetLiftingAllowableCompressionFactor();
   Float64 rcsT;
   bool    rcsBfmax;
   Float64 rcsFmax;
   pGirderLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStressParameters(&rcsT,&rcsBfmax,&rcsFmax);
   Float64 rcsTalt = pGirderLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStressFactor();

   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   // Use calculator object to deal with casting yard higher allowable stress
   pgsAlternativeTensileStressCalculator altCalc(segmentKey, liftSegmentIntervalIdx, pGirder, pShapes,pSectProps, pRebarGeom, pMaterials, bUnitsSI);

   Float64 AsMax = 0;
   Float64 As = 0;
   for(IndexType i=0; i<psiz; i++)
   {
      const pgsPointOfInterest& poi = rpoiVec[i];

      Float64 ag,stg,sbg;
      if ( bUseConfig )
      {
         ag  = pSectProps->GetAg(liftSegmentIntervalIdx,poi,liftConfig.GdrConfig.Fci);
         stg = pSectProps->GetSt(liftSegmentIntervalIdx,poi,liftConfig.GdrConfig.Fci);
         sbg = pSectProps->GetSb(liftSegmentIntervalIdx,poi,liftConfig.GdrConfig.Fci);
      }
      else
      {
         ag  = pSectProps->GetAg(liftSegmentIntervalIdx,poi);
         stg = pSectProps->GetSt(liftSegmentIntervalIdx,poi);
         sbg = pSectProps->GetSb(liftSegmentIntervalIdx,poi);
      }

      Float64 dist_from_start = poi.GetDistFromStart();

      Float64 Plift = p_lift;
      Float64 Mlift = m_lift;

      // if this poi is in the cantilevers, the effect of the inclined cable is 0
      if ( dist_from_start < leftOH ||
           glen-rightOH < dist_from_start )
      {
         Plift = 0;
         Mlift = 0;
      }

      // calc total prestress force and eccentricity
      Float64 nfs, nfh, nft;
      Float64 hps_force, he;
      Float64 sps_force, se;
      Float64 tps_force, te;       

      if ( bUseConfig )
      {
         hps_force = pPrestressForce->GetPrestressForce(poi,liftConfig.GdrConfig,pgsTypes::Harped,liftSegmentIntervalIdx,pgsTypes::Middle);
         he        = pStrandGeometry->GetHsEccentricity(liftSegmentIntervalIdx,poi,liftConfig.GdrConfig, &nfh);
         sps_force = pPrestressForce->GetPrestressForce(poi,liftConfig.GdrConfig,pgsTypes::Straight,liftSegmentIntervalIdx,pgsTypes::Middle);
         se        = pStrandGeometry->GetSsEccentricity(liftSegmentIntervalIdx,poi,liftConfig.GdrConfig, &nfs);
         tps_force = pPrestressForce->GetPrestressForce(poi,liftConfig.GdrConfig,pgsTypes::Temporary,liftSegmentIntervalIdx,pgsTypes::Middle);
         te        = pStrandGeometry->GetTempEccentricity(liftSegmentIntervalIdx,poi,liftConfig.GdrConfig, &nft);
      }
      else
      {
         hps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Harped,liftSegmentIntervalIdx,pgsTypes::Middle);
         he        = pStrandGeometry->GetHsEccentricity(liftSegmentIntervalIdx,poi,&nfh);
         sps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Straight,liftSegmentIntervalIdx,pgsTypes::Middle);
         se        = pStrandGeometry->GetSsEccentricity(liftSegmentIntervalIdx,poi,&nfs);
         tps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,liftSegmentIntervalIdx,pgsTypes::Middle);
         te        = pStrandGeometry->GetTempEccentricity(liftSegmentIntervalIdx,poi,&nft);
      }

      Float64 total_ps_force = hps_force + sps_force + tps_force;
      Float64 total_e = 0.0;
      if (0.0 < total_ps_force)
         total_e = (hps_force*he + sps_force*se + tps_force*te) / total_ps_force;
      else if ( 0 < (nfh + nfs + nft) )
         total_e = (he*nfh + se*nfs + te*nft) / (nfh + nfs + nft);

      // start building artifact
      pgsLiftingStressAnalysisArtifact lift_artifact;

      lift_artifact.SetEffectiveHorizPsForce(total_ps_force);
      lift_artifact.SetEccentricityPsForce(total_e);

      // impacted moments
      Float64 mom_no   = momVec[i] + Mlift;
      Float64 mom_up   = impact_up   * mom_no;
      Float64 mom_down = impact_down * mom_no;
      lift_artifact.SetMomentImpact(mom_up,mom_no, mom_down);

      Float64 mom_ps = -total_ps_force * total_e;

      // stresses
      Float64 ft_ps   = (-total_ps_force)   / ag + (mom_ps)/stg;
      Float64 ft_up   = (Plift*impact_up)   / ag + (mom_up)  /stg + ft_ps;
      Float64 ft_no   = (Plift)             / ag + (mom_no)  /stg + ft_ps;
      Float64 ft_down = (Plift*impact_down) / ag + (mom_down)/stg + ft_ps;

      Float64 fb_ps   = (-total_ps_force)  / ag + (mom_ps)/sbg;
      Float64 fb_up   = (Plift*impact_up)  / ag + (mom_up)/sbg   + fb_ps;
      Float64 fb_no   = (Plift)            / ag + (mom_no)/sbg   + fb_ps;
      Float64 fb_down = (Plift*impact_down)/ ag + (mom_down)/sbg + fb_ps;

      lift_artifact.SetTopFiberStress(ft_ps, ft_up, ft_no, ft_down);
      lift_artifact.SetBottomFiberStress(fb_ps, fb_up, fb_no, fb_down);
      
      // Now look at tensile capacity side of equations
      Float64 YnaUp,   AtUp,   TUp,   AsReqdUp,   AsProvdUp;
      Float64 YnaNone, AtNone, TNone, AsReqdNone, AsProvdNone;
      Float64 YnaDown, AtDown, TDown, AsReqdDown, AsProvdDown;
      bool isAdequateBarUp, isAdequateBarNone, isAdequateBarDown;

      const GDRCONFIG* pConfig = bUseConfig ? &(liftConfig.GdrConfig) : NULL;

      Float64 fAllowUp = altCalc.ComputeAlternativeStressRequirements(poi, pConfig, ft_up, fb_up, fLowTensAllowable, fHighTensAllowable,
                                                                      &YnaUp, &AtUp, &TUp, &AsProvdUp, &AsReqdUp, &isAdequateBarUp);

      lift_artifact.SetAlternativeTensileStressParameters(idUp, YnaUp,   AtUp,   TUp,   AsProvdUp,
                                                          AsReqdUp, fAllowUp);

      Float64 fAllowNone = altCalc.ComputeAlternativeStressRequirements(poi, pConfig, ft_no, fb_no, fLowTensAllowable, fHighTensAllowable,
                                                                      &YnaNone, &AtNone, &TNone, &AsProvdNone, &AsReqdNone, &isAdequateBarNone);

      lift_artifact.SetAlternativeTensileStressParameters(idNone, YnaNone,   AtNone,   TNone,   AsProvdNone,
                                                          AsReqdNone, fAllowNone);

      Float64 fAllowDown = altCalc.ComputeAlternativeStressRequirements(poi, pConfig, ft_down, fb_down, fLowTensAllowable, fHighTensAllowable,
                                                                      &YnaDown, &AtDown, &TDown, &AsProvdDown, &AsReqdDown, &isAdequateBarDown);

      lift_artifact.SetAlternativeTensileStressParameters(idDown, YnaDown,   AtDown,   TDown,   AsProvdDown,
                                                          AsReqdDown, fAllowDown);

      lift_artifact.SetCompressiveCapacity(fCompAllowable);

      // Compute required concrete strengths
      // Compression
      Float64 min_stress = lift_artifact.GetMaximumConcreteCompressiveStress();

      Float64 fc_compression = 0.0;
      if ( min_stress < 0 )
      {
         fc_compression = min_stress/rcsC;
      }

      // Tension is more challenging. Use inline function to compute
      Float64 max_stress = lift_artifact.GetMaximumConcreteTensileStress();

      Float64 fc_tens_norebar, fc_tens_withrebar;
      ComputeReqdFcTens(max_stress, rcsT, rcsBfmax, rcsFmax, rcsTalt, &fc_tens_norebar, &fc_tens_withrebar);

      lift_artifact.SetRequiredConcreteStrength(fc_compression,fc_tens_norebar,fc_tens_withrebar);

      pArtifact->AddLiftingStressAnalysisArtifact(poi.GetDistFromStart(),lift_artifact);
   }

}

bool pgsGirderHandlingChecker::ComputeLiftingFsAgainstCracking(const CSegmentKey& segmentKey,bool bUseConfig,
                                                               const HANDLINGCONFIG& liftConfig,
                                                               const std::vector<pgsPointOfInterest>& rpoiVec,
                                                               const std::vector<Float64>& momVec,
                                                               Float64 midSpanDeflection,
                                                               pgsLiftingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentInterval = pIntervals->GetLiftSegmentInterval(segmentKey);

   Float64 fo = pArtifact->GetOffsetFactor();

   pArtifact->SetCamberDueToSelfWeight(midSpanDeflection);

   // get mid-span poi so we can calc camber due to ps
   pgsPointOfInterest poi_ms;
   bool found=false;
   std::vector<pgsPointOfInterest>::const_iterator iter(rpoiVec.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(rpoiVec.end());
   while( iter != end )
   {
      const pgsPointOfInterest& rpoi = *iter;

   if (rpoi.IsMidSpan(POI_LIFT_SEGMENT))
      {
         poi_ms = rpoi;
         found = true;
         break;
      }

      iter++;
   }
   ATLASSERT(found);

   Float64 ps_camber = 0.0;
   Float64 temp_ps_camber = 0.0;

   GET_IFACE(ICamber,pCamber);
   if ( bUseConfig )
   {
      ps_camber = pCamber->GetPrestressDeflection(poi_ms,liftConfig.GdrConfig,false);
      temp_ps_camber = pCamber->GetInitialTempPrestressDeflection(poi_ms,liftConfig.GdrConfig,false);
   }
   else
   {
      ps_camber = pCamber->GetPrestressDeflection(poi_ms,false);
      temp_ps_camber = pCamber->GetInitialTempPrestressDeflection(poi_ms,false);
   }

   ps_camber += temp_ps_camber;
   pArtifact->SetCamberDueToPrestress(ps_camber);

   // total adjusted camber
   Float64 total_camber = ps_camber + midSpanDeflection;
   pArtifact->SetTotalCamberAtLifting(total_camber);

   // adjusted yr = distance between cg and lifting point
   Float64 yr = pArtifact->GetVerticalDistanceFromPickPointToGirderCg();
   Float64 ayr = yr - fo*total_camber;
   pArtifact->SetAdjustedYr(ayr);

   // Zo (based on mid-span properties)
   GET_IFACE(ISectionProperties,pSectProp);
   Float64 Ix = pSectProp->GetIx(liftSegmentInterval,poi_ms);
   Float64 Iy = pSectProp->GetIy(liftSegmentInterval,poi_ms);
   Float64 span_len = pArtifact->GetClearSpanBetweenPickPoints();
   Float64 gird_len = pArtifact->GetGirderLength();
   Float64 Eci = pArtifact->GetElasticModulusOfGirderConcrete();
   Float64 w = pArtifact->GetAvgGirderWeightPerLength();
   Float64 a = (gird_len-span_len)/2.;
   Float64 l = span_len;

   Float64 zo = (w/(12*Eci*Iy*gird_len))*
                (l*l*l*l*l/10. - a*a*l*l*l + 3.*a*a*a*a*l + 6.*a*a*a*a*a/5.);

   pArtifact->SetZo(zo);
   pArtifact->SetIy(Iy);
   pArtifact->SetIx(Ix);

   // intial tilt angle
   Float64 ei = pArtifact->GetTotalInitialEccentricity();
   Float64 theta_i = ei/ayr;
   pArtifact->SetInitialTiltAngle(theta_i);

   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   Float64 fs_all_cr = pGirderLiftingSpecCriteria->GetLiftingCrackingFs();

   // If at this point, yr is negative, then the girder is unstable for
   // lifting and we can do no more calculations. So return false.
   if (0.0 < ayr)
   {
      Float64 fr = pArtifact->GetModRupture();
      GET_IFACE(IGirder,pGdr);
      Float64 bt_bot = pGdr->GetBottomWidth(poi_ms);
      Float64 bt_top = pGdr->GetTopWidth(poi_ms);

      // loop over all pois and calculate cracking fs
      IndexType psiz = rpoiVec.size();
      IndexType msiz = momVec.size();
      ATLASSERT(psiz==msiz);
      for(IndexType i = 0; i < psiz; i++)
      {
         const pgsPointOfInterest& poi( rpoiVec[i] );
         Float64 mom_vert = momVec[i];

         pgsLiftingCrackingAnalysisArtifact crack_artifact;
         crack_artifact.SetVerticalMoment(mom_vert);

         crack_artifact.SetAllowableFsForCracking(fs_all_cr);

         // determine lateral moment that will cause cracking in section
         // upward impact will crack top flange and downward impact will crack bottom
         const pgsLiftingStressAnalysisArtifact* pstr = pArtifact->GetLiftingStressAnalysisArtifact(poi.GetDistFromStart());
         CHECK(pstr!=0);

         // top flange cracking
         Float64 ftop_ps, ftop_up, ftop_no, ftop_down;
         pstr->GetTopFiberStress(&ftop_ps, &ftop_up, &ftop_no, &ftop_down);

         Float64 m_lat_top = 2 * (fr - ftop_up) * Iy / bt_top;

         // bottom flange cracking
         Float64 fbot_ps, fbot_up, fbot_no, fbot_down;
         pstr->GetBottomFiberStress(&fbot_ps, &fbot_up, &fbot_no, &fbot_down);

         Float64 m_lat_bot = 2 * (fr - fbot_down) * Iy / bt_bot;

         Float64 m_lat;
         if (m_lat_top < m_lat_bot)
         {
            m_lat = m_lat_top;
            crack_artifact.SetCrackedFlange(TopFlange);
            crack_artifact.SetLateralMomentStress(ftop_up);
         }
         else
         {
            m_lat = m_lat_bot;
            crack_artifact.SetCrackedFlange(BottomFlange);
            crack_artifact.SetLateralMomentStress(fbot_down);
         }

         Float64 theta_max;
         // if m_lat is negative, this means that beam cracks even if it's not tilted
         if (m_lat > 0.0)
         {
            crack_artifact.SetLateralMoment(m_lat);

            // roll angle
            theta_max = fabs(m_lat / mom_vert);
            theta_max = ForceIntoRange(0.,theta_max,PI_OVER_2);
            crack_artifact.SetThetaCrackingMax(theta_max);

            // FS
            Float64 fs = 1/(zo/ayr + theta_i/theta_max);
            crack_artifact.SetFsCracking(fs);
         }
         else
         {
            // we have no capacity for cracking
            crack_artifact.SetLateralMoment(0.0);
            crack_artifact.SetThetaCrackingMax(0.0);
            crack_artifact.SetFsCracking(0.0);
         }

         pArtifact->AddLiftingCrackingAnalysisArtifact(poi.GetDistFromStart(),crack_artifact);
      }
   }
   else
   {
      // yr is negative, lift is unstable
      pArtifact->SetIsGirderStable(false);
      return false;
   }

   // if we made it to here, girder is stable
   pArtifact->SetIsGirderStable(true);
   return true;
}

void pgsGirderHandlingChecker::ComputeLiftingFsAgainstFailure(const CSegmentKey& segmentKey,
                                                              pgsLiftingAnalysisArtifact* pArtifact)
{

   Float64 zo = pArtifact->GetZo();
   ATLASSERT(0.0 < zo);
   Float64 ei = pArtifact->GetTotalInitialEccentricity();

   Float64 theta_max = sqrt(ei/(2.5*zo));
   pArtifact->SetThetaFailureMax(theta_max);

   Float64 zo_prime = zo*(1+2.5*theta_max);
   pArtifact->SetZoPrime(zo_prime);

   Float64 yr = pArtifact->GetAdjustedYr();

   Float64 fs = yr*theta_max / (zo_prime*theta_max+ei);
   pArtifact->SetBasicFsFailure(fs);

   // if fs is less then the factor of safety against cracking,
   // take fs = fscr.
   Float64 fscr = pArtifact->GetMinFsForCracking();
   fs = (fs < fscr) ? fscr : fs;
   pArtifact->SetFsFailure(fs);
}

////////////////////////////////////////////////////////
// hauling
////////////////////////////////////////////////////////
void pgsGirderHandlingChecker::PrepareHaulingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 Loh,Float64 Roh,Float64 Fc,Float64 Ec,pgsTypes::ConcreteType concType,pgsHaulingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IBridge, pBridge);
   Float64 girder_length = pBridge->GetSegmentLength(segmentKey);
   pArtifact->SetGirderLength(girder_length);

   GET_IFACE(ISectionProperties,pSectProp);
   Float64 volume = pSectProp->GetVolume(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 density = pMaterial->GetSegmentWeightDensity(segmentKey,haulSegmentIntervalIdx);
   Float64 total_weight = volume * density * unitSysUnitsMgr::GetGravitationalAcceleration();
   pArtifact->SetGirderWeight(total_weight);

   Float64 span_len = girder_length - Loh - Roh;

   if (span_len <= 0.0)
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);

      LPCTSTR msg = _T("Hauling support overhang cannot exceed one-half of the span length");
      pgsLiftingSupportLocationStatusItem* pStatusItem = new pgsLiftingSupportLocationStatusItem(segmentKey,m_StatusGroupID,m_scidLiftingSupportLocationError,msg);
      pStatusCenter->Add(pStatusItem);

      std::_tstring str(msg);
      str += std::_tstring(_T("\nSee Status Center for details"));
      THROW_UNWIND(str.c_str(),-1);
   }

   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   Float64 min_bunk_point_start = pGirderHaulingSpecCriteria->GetMinimumHaulingSupportLocation(segmentKey,pgsTypes::metStart);
   Float64 min_bunk_point_end   = pGirderHaulingSpecCriteria->GetMinimumHaulingSupportLocation(segmentKey,pgsTypes::metEnd);
   if ( Loh < min_bunk_point_start )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format(_T("Left bunk point is less than the minimum value of %s"),::FormatDimension(min_bunk_point_start,pDisplayUnits->GetSpanLengthUnit()));
      pgsBunkPointLocationStatusItem* pStatusItem = new pgsBunkPointLocationStatusItem(segmentKey,m_StatusGroupID,m_scidBunkPointLocation,strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   if ( Roh < min_bunk_point_end )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format(_T("Right bunk point is less than the minimum value of %s"),::FormatDimension(min_bunk_point_end,pDisplayUnits->GetSpanLengthUnit()));
      pgsBunkPointLocationStatusItem* pStatusItem = new pgsBunkPointLocationStatusItem(segmentKey,m_StatusGroupID,m_scidBunkPointLocation,strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   pArtifact->SetClearSpanBetweenSupportLocations(span_len);
   pArtifact->SetOverhangs(Loh,Roh);

   Float64 roll_hgt = pGirderHaulingSpecCriteria->GetHeightOfTruckRollCenterAboveRoadway();
   Float64 gb_hgt =  pGirderHaulingSpecCriteria->GetHeightOfGirderBottomAboveRoadway();
   Float64 camber_factor = 1.0 + pGirderHaulingSpecCriteria->GetIncreaseInCgForCamber();
   Float64 ycgb = pSectProp->GetYb(haulSegmentIntervalIdx,pgsPointOfInterest(segmentKey,0.0));
   Float64 e_hgt = (gb_hgt+ycgb-roll_hgt) * camber_factor;
   pArtifact->SetHeightOfRollCenterAboveRoadway(roll_hgt);
   pArtifact->SetHeightOfCgOfGirderAboveRollCenter(e_hgt);

   Float64 roll_stiff;
   if ( pGirderHaulingSpecCriteria->GetRollStiffnessMethod() == IGirderHaulingSpecCriteria::LumpSum )
      roll_stiff = pGirderHaulingSpecCriteria->GetLumpSumRollStiffness();
   else
   {
      int nAxles = int(pArtifact->GetGirderWeight() / pGirderHaulingSpecCriteria->GetAxleWeightLimit()) + 1;
      Float64 axle_stiff = pGirderHaulingSpecCriteria->GetAxleStiffness();
      roll_stiff = axle_stiff * nAxles;
      Float64 min_roll_stiffness = pGirderHaulingSpecCriteria->GetMinimumRollStiffness();
      if ( roll_stiff < min_roll_stiffness )
         roll_stiff = min_roll_stiffness;
   }

   pArtifact->SetRollStiffnessOfvehicle(roll_stiff);

   Float64 axle_space = pGirderHaulingSpecCriteria->GetAxleWidth();
   pArtifact->SetAxleWidth(axle_space);

   Float64 superelev = pGirderHaulingSpecCriteria->GetMaxSuperelevation();
   pArtifact->SetRoadwaySuperelevation(superelev);

   Float64 upwardi, downwardi;
   pGirderHaulingSpecCriteria->GetHaulingImpact( &downwardi, &upwardi);
   pArtifact->SetUpwardImpact(upwardi);
   pArtifact->SetDownwardImpact(downwardi);

   Float64 sweep = pGirderHaulingSpecCriteria->GetHaulingSweepTolerance();
   pArtifact->SetSweepTolerance(sweep);
   pArtifact->SetSupportPlacementTolerance(pGirderHaulingSpecCriteria->GetHaulingSupportPlacementTolerance());


   pArtifact->SetConcreteStrength(Fc);
   pArtifact->SetModRupture( pGirderHaulingSpecCriteria->GetHaulingModulusOfRupture(Fc,concType) );
   pArtifact->SetModRuptureCoefficient( pGirderHaulingSpecCriteria->GetHaulingModulusOfRuptureCoefficient(concType) );

   pArtifact->SetElasticModulusOfGirderConcrete(Ec);

   Float64 offset_factor = span_len/girder_length;
   offset_factor = offset_factor*offset_factor - 1/3.;
   pArtifact->SetOffsetFactor(offset_factor);

   Float64 e_sweep = sweep * girder_length;
   pArtifact->SetEccentricityDueToSweep(e_sweep);

   Float64 e_placement = pGirderHaulingSpecCriteria->GetHaulingSupportPlacementTolerance();
   pArtifact->SetEccentricityDueToPlacementTolerance(e_placement);

   pArtifact->SetTotalInitialEccentricity(e_sweep*offset_factor + e_placement);

   Float64 f,fmax;
   bool bMax;
   pGirderHaulingSpecCriteria->GetHaulingAllowableTensileConcreteStressParameters(&f,&bMax,&fmax);
   pArtifact->SetAllowableTensileConcreteStressParameters(f,bMax,fmax);
   pArtifact->SetAllowableCompressionFactor(pGirderHaulingSpecCriteria->GetHaulingAllowableCompressionFactor());
   pArtifact->SetAlternativeTensileConcreteStressFactor(pGirderHaulingSpecCriteria->GetHaulingWithMildRebarAllowableStressFactor()  );

   // Allowables
   pArtifact->SetAllowableSpanBetweenSupportLocations(pGirderHaulingSpecCriteria->GetAllowableDistanceBetweenSupports());
   pArtifact->SetAllowableLeadingOverhang(pGirderHaulingSpecCriteria->GetAllowableLeadingOverhang());
   pArtifact->SetAllowableFsForCracking(pGirderHaulingSpecCriteria->GetHaulingCrackingFs());
   pArtifact->SetAllowableFsForRollover(pGirderHaulingSpecCriteria->GetHaulingRolloverFs());
   pArtifact->SetMaxGirderWgt(pGirderHaulingSpecCriteria->GetMaxGirderWgt());
}

void pgsGirderHandlingChecker::ComputeHaulingStresses(const CSegmentKey& segmentKey,bool bUseConfig,
                                                      const HANDLINGCONFIG& haulConfig,
                                                      const std::vector<pgsPointOfInterest>& rpoiVec,
                                                      const std::vector<Float64>& momVec,
                                                      pgsHaulingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IPretensionForce,pPrestressForce);
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IStrandGeometry,pStrandGeometry);
   GET_IFACE(ISectionProperties,pSectProps);
   GET_IFACE(IShapes,pShapes);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(IMaterials,pMaterials);
   GET_IFACE(ILongRebarGeometry, pRebarGeom);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   Float64 roll_angle = pArtifact->GetEqualibriumAngle();

   // get poi-independent values used for stress calc
   Float64 impact_up   = 1.0 - pArtifact->GetUpwardImpact();
   Float64 impact_down = 1.0 + pArtifact->GetDownwardImpact();

   // Get allowable tension for with and without rebar cases
   Float64 fLowTensAllowable  = pGirderHaulingSpecCriteria->GetHaulingAllowableTensileConcreteStress(segmentKey);
   Float64 fHighTensAllowable = pGirderHaulingSpecCriteria->GetHaulingWithMildRebarAllowableStress(segmentKey);
   Float64 fCompAllowable     = pGirderHaulingSpecCriteria->GetHaulingAllowableCompressiveConcreteStress(segmentKey);

   // Inclined girder tension is limited to modulus of rupture
   Float64 modRuptureCoeff = pArtifact->GetModRuptureCoefficient();
   Float64 modRupture      = pArtifact->GetModRupture();

   // Parameters for computing required concrete strengths
   Float64 rcsC = pGirderHaulingSpecCriteria->GetHaulingAllowableCompressionFactor();
   Float64 rcsT;
   bool    rcsBfmax;
   Float64 rcsFmax;
   pGirderHaulingSpecCriteria->GetHaulingAllowableTensileConcreteStressParameters(&rcsT,&rcsBfmax,&rcsFmax);
   Float64 rcsTalt = pGirderHaulingSpecCriteria->GetHaulingWithMildRebarAllowableStressFactor();

   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);
   // Use calculator object to deal with casting yard higher allowable stress
   pgsAlternativeTensileStressCalculator altCalc(segmentKey, haulSegmentIntervalIdx, pGdr, pShapes, pSectProps, pRebarGeom, pMaterials, bUnitsSI);

   IndexType psiz = rpoiVec.size();
   IndexType msiz = momVec.size();
   CHECK(psiz==msiz);

   Float64 AsMax = 0;

   for(IndexType i=0; i<psiz; i++)
   {
      const pgsPointOfInterest& poi( rpoiVec[i] );

      Float64 bt_bot = pGdr->GetBottomWidth(poi);
      Float64 bt_top = pGdr->GetTopWidth(poi);
      Float64 ag,iy,stg,sbg;
      if ( bUseConfig )
      {
         ag     = pSectProps->GetAg(haulSegmentIntervalIdx,poi,haulConfig.GdrConfig.Fc);
         iy     = pSectProps->GetIy(haulSegmentIntervalIdx,poi,haulConfig.GdrConfig.Fc);
         stg    = pSectProps->GetSt(haulSegmentIntervalIdx,poi,haulConfig.GdrConfig.Fc);
         sbg    = pSectProps->GetSb(haulSegmentIntervalIdx,poi,haulConfig.GdrConfig.Fc);
      }
      else
      {
         ag     = pSectProps->GetAg(haulSegmentIntervalIdx,poi);
         iy     = pSectProps->GetIy(haulSegmentIntervalIdx,poi);
         stg    = pSectProps->GetSt(haulSegmentIntervalIdx,poi);
         sbg    = pSectProps->GetSb(haulSegmentIntervalIdx,poi);
      }


      pgsHaulingStressAnalysisArtifact haul_artifact;

      // calc total prestress force and eccentricity
      Float64 nfs, nfh, nft;
      Float64 hps_force, he;
      Float64 sps_force, se;
      Float64 tps_force, te;

      if ( bUseConfig )
      {
         hps_force = pPrestressForce->GetPrestressForce(poi,haulConfig.GdrConfig,pgsTypes::Harped,haulSegmentIntervalIdx,pgsTypes::Middle);
         he = pStrandGeometry->GetHsEccentricity(haulSegmentIntervalIdx,poi,haulConfig.GdrConfig, &nfh);
         sps_force = pPrestressForce->GetPrestressForce(poi,haulConfig.GdrConfig,pgsTypes::Straight,haulSegmentIntervalIdx,pgsTypes::Middle);
         se = pStrandGeometry->GetSsEccentricity(haulSegmentIntervalIdx,poi,haulConfig.GdrConfig,&nfs);
         tps_force = pPrestressForce->GetPrestressForce(poi,haulConfig.GdrConfig,pgsTypes::Temporary,haulSegmentIntervalIdx,pgsTypes::Middle);
         te = pStrandGeometry->GetTempEccentricity(haulSegmentIntervalIdx,poi,haulConfig.GdrConfig,&nft);
      }
      else
      {
         hps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Harped,haulSegmentIntervalIdx,pgsTypes::Middle);
         he = pStrandGeometry->GetHsEccentricity(haulSegmentIntervalIdx,poi,&nfh);
         sps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Straight,haulSegmentIntervalIdx,pgsTypes::Middle);
         se = pStrandGeometry->GetSsEccentricity(haulSegmentIntervalIdx,poi,&nfs);
         tps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,haulSegmentIntervalIdx,pgsTypes::Middle);
         te = pStrandGeometry->GetTempEccentricity(haulSegmentIntervalIdx,poi,&nft);
      }

      Float64 total_ps_force = hps_force + sps_force + tps_force;
      Float64 total_e=0.0;
      if (0 < total_ps_force)
      {
         total_e = (hps_force*he + sps_force*se + tps_force*te) / total_ps_force;
      }
      else if (0 < (nfh + nfs + nft))
      {
         Float64 Aps[3];
         Aps[pgsTypes::Straight] = nfs*pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Straight)->GetNominalArea();
         Aps[pgsTypes::Harped]   = nfh*pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Harped)->GetNominalArea();
         Aps[pgsTypes::Temporary]= nft*pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetNominalArea();

         Float64 aps = Aps[pgsTypes::Straight] + Aps[pgsTypes::Harped] + Aps[pgsTypes::Temporary];

         total_e = (he*Aps[pgsTypes::Harped] + se*Aps[pgsTypes::Straight] + te*Aps[pgsTypes::Temporary]) / aps;
      }

      haul_artifact.SetEffectiveHorizPsForce(total_ps_force);
      haul_artifact.SetEccentricityPsForce(total_e);

      // moments
      Float64 vert_mom   = momVec[i];
      Float64 moment_impact_up,moment_impact_down;
      moment_impact_up = impact_up*vert_mom;
      moment_impact_down = impact_down*vert_mom;
      haul_artifact.SetMomentImpact(moment_impact_up,vert_mom,moment_impact_down);

      Float64 lat_mom = vert_mom * roll_angle;
      haul_artifact.SetLateralMoment(lat_mom);

      Float64 mom_ps = -total_ps_force * total_e;


      // compute stresses
      // In accordance with the PCI Journal article that these
      // calculations are based on, don't included impact when checking
      // stresses for the lateral moment effects. That is, check
      // "impact and no tilt" and "no impact with tilt".

      Float64 ft_ps  = -total_ps_force/ag + (mom_ps)/stg;
      Float64 ft_up   = moment_impact_up/stg   + ft_ps;
      Float64 ft_no   = vert_mom/stg           + ft_ps;
      Float64 ft_down = moment_impact_down/stg + ft_ps;

      Float64 fb_ps = -total_ps_force/ag + (mom_ps)/sbg;
      Float64 fb_up   = moment_impact_up/sbg   + fb_ps;
      Float64 fb_no   = vert_mom/sbg           + fb_ps;
      Float64 fb_down = moment_impact_down/sbg + fb_ps;

      Float64 ftu = ft_no - lat_mom*bt_top/(iy*2);
      Float64 ftd = ft_no + lat_mom*bt_top/(iy*2);
      Float64 fbu = fb_no - lat_mom*bt_bot/(iy*2);
      Float64 fbd = fb_no + lat_mom*bt_bot/(iy*2);

      haul_artifact.SetTopFiberStress(ft_ps,ft_up,ft_no,ft_down);
      haul_artifact.SetBottomFiberStress(fb_ps,fb_up,fb_no,fb_down);
      haul_artifact.SetInclinedGirderStresses(ftu,ftd,fbu,fbd);

      // Now look at tensile capacity side of equations
      Float64 YnaUp,   AtUp,   TUp,   AsReqdUp,   AsProvdUp;
      Float64 YnaNone, AtNone, TNone, AsReqdNone, AsProvdNone;
      Float64 YnaDown, AtDown, TDown, AsReqdDown, AsProvdDown;
      bool isAdequateBarUp, isAdequateBarNone, isAdequateBarDown;

      const GDRCONFIG* pConfig = bUseConfig ? &(haulConfig.GdrConfig) : NULL;

      Float64 fAllowUp = altCalc.ComputeAlternativeStressRequirements(poi, pConfig, ft_up, fb_up, fLowTensAllowable, fHighTensAllowable,
                                                                      &YnaUp, &AtUp, &TUp, &AsProvdUp, &AsReqdUp, &isAdequateBarUp);

      haul_artifact.SetAlternativeTensileStressParameters(idUp, YnaUp,   AtUp,   TUp,   AsProvdUp,
                                                          AsReqdUp, fAllowUp);

      Float64 fAllowNone = altCalc.ComputeAlternativeStressRequirements(poi, pConfig, ft_no, fb_no, fLowTensAllowable, fHighTensAllowable,
                                                                      &YnaNone, &AtNone, &TNone, &AsProvdNone, &AsReqdNone, &isAdequateBarNone);

      haul_artifact.SetAlternativeTensileStressParameters(idNone, YnaNone,   AtNone,   TNone,   AsProvdNone,
                                                          AsReqdNone, fAllowNone);

      Float64 fAllowDown = altCalc.ComputeAlternativeStressRequirements(poi, pConfig, ft_down, fb_down, fLowTensAllowable, fHighTensAllowable,
                                                                      &YnaDown, &AtDown, &TDown, &AsProvdDown, &AsReqdDown, &isAdequateBarDown);

      haul_artifact.SetAlternativeTensileStressParameters(idDown, YnaDown,   AtDown,   TDown,   AsProvdDown,
                                                          AsReqdDown, fAllowDown);

      haul_artifact.SetCompressiveCapacity(fCompAllowable);
      haul_artifact.SetInclinedTensileCapacity(modRupture);

      // Compute required concrete strengths
      // Compression: Lump in inclined stress.
      Float64 min_stress = haul_artifact.GetMaximumConcreteCompressiveStress();
      Float64 min_stress_inc = haul_artifact.GetMaximumInclinedConcreteCompressiveStress();
      min_stress = min(min_stress, min_stress_inc);

      Float64 fc_compression = 0.0;
      if ( min_stress < 0 )
      {
         fc_compression = min_stress/rcsC;
      }

      // Tension is more challenging. Use inline function to compute
      Float64 max_stress = haul_artifact.GetMaximumConcreteTensileStress();

      // Impacted plumb girder stress is treated differently than inclined girder
      Float64 fc_tens_norebar, fc_tens_withrebar;
      ComputeReqdFcTens(max_stress, rcsT, rcsBfmax, rcsFmax, rcsTalt, &fc_tens_norebar, &fc_tens_withrebar);

      // For inclined girder, tension is limited by modulus of rupture. Rebar is not considered, so it must be treated differently.
      Float64 max_stress_inclined = haul_artifact.GetMaximumInclinedConcreteTensileStress();

      Float64 fc_tension_inclined = -1;
      if ( 0 < max_stress_inclined )
      {
         fc_tension_inclined = pow(max_stress_inclined/modRuptureCoeff,2);
      }
      else
      {
         fc_tension_inclined = 0;
      }

      // Now, get max required strength
      if (fc_tens_norebar > 0.0)
      {
         fc_tens_norebar = max(fc_tens_norebar, fc_tension_inclined);
      }

      if (fc_tens_withrebar > 0.0)
      {
         fc_tens_withrebar = max(fc_tens_withrebar, fc_tension_inclined);
      }

      haul_artifact.SetRequiredConcreteStrength(fc_compression,fc_tens_norebar,fc_tens_withrebar);

      pArtifact->AddHaulingStressAnalysisArtifact(poi.GetDistFromStart(),haul_artifact);
   }
}

void pgsGirderHandlingChecker::ComputeHaulingFsForCracking(const CSegmentKey& segmentKey,
                                                           const std::vector<pgsPointOfInterest>& rpoiVec,
                                                           const std::vector<Float64>& momVec,
                                                           pgsHaulingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   Float64 fo = pArtifact->GetOffsetFactor();
   Float64 y = pArtifact->GetHeightOfCgOfGirderAboveRollCenter();
   
   Float64 kt = pArtifact->GetRollStiffnessOfvehicle();
   Float64 gird_len = pArtifact->GetGirderLength();
   Float64 w = pArtifact->GetAvgGirderWeightPerLength();
   Float64 gird_wt = w * gird_len;
   PRECONDITION(gird_wt>0.0);

   Float64 r = pArtifact->GetRadiusOfStability();
   Float64 zo = pArtifact->GetZo();
   Float64 ei = pArtifact->GetTotalInitialEccentricity();
   Float64 superelevation_angle = ::ConvertFromSysUnits(pArtifact->GetRoadwaySuperelevation(), unitMeasure::Radian);

   Float64 fs_all = pArtifact->GetAllowableFsForCracking();

   Float64 fr = pArtifact->GetModRupture();

   GET_IFACE(IGirder,pGdr);
   GET_IFACE(ISectionProperties,pSectProp);

   // loop over all pois and calculate cracking fs
   IndexType psiz = rpoiVec.size();
   IndexType msiz = momVec.size();
   CHECK(psiz==msiz);
   for(IndexType i=0; i<psiz; i++)
   {
      const pgsPointOfInterest& poi( rpoiVec[i] );
      Float64 bt_bot = pGdr->GetBottomWidth(poi);
      Float64 bt_top = pGdr->GetTopWidth(poi);

      Float64 iy = pSectProp->GetIy(haulSegmentIntervalIdx,poi);

      Float64 mom_vert = momVec[i];

      pgsHaulingCrackingAnalysisArtifact crack_artifact;
      crack_artifact.SetVerticalMoment(mom_vert);

      // determine lateral moment that will cause cracking in section
      const pgsHaulingStressAnalysisArtifact* pstr = pArtifact->GetHaulingStressAnalysisArtifact(poi.GetDistFromStart());
      CHECK(pstr!=0);

      // upward impact will crack top flange and downward impact will crack bottom
      // average uphill and downhill stresses to get vertical stress at center of flange
      Float64 ftps, ftu,ft,ftd;
      pstr->GetTopFiberStress(&ftps,&ftu,&ft,&ftd);
      Float64 fbps,fbu,fb,fbd;
      pstr->GetBottomFiberStress(&fbps,&fbu,&fb,&fbd);

      // top flange cracking
      Float64 m_lat_top = 2 * (fr - ft) * iy / bt_top;

      // bottom flange cracking
      Float64 m_lat_bot = 2 * (fr - fb) * iy / bt_bot;

      Float64 m_lat;
      if (m_lat_top < m_lat_bot)
      {
         m_lat = m_lat_top;
         crack_artifact.SetCrackedFlange(TopFlange);
         crack_artifact.SetLateralMomentStress(ft);
      }
      else
      {
         m_lat = m_lat_bot;
         crack_artifact.SetCrackedFlange(BottomFlange);
         crack_artifact.SetLateralMomentStress(fb);
      }

      Float64 theta_max;
      // if m_lat is negative, this means that beam cracks even if it's not tilted
      if (m_lat >= 0.0)
      {
         crack_artifact.SetLateralMoment(m_lat);

         // roll angle
         theta_max = fabs(m_lat / mom_vert);

         theta_max = ForceIntoRange(0.,theta_max,PI_OVER_2);

         crack_artifact.SetThetaCrackingMax(theta_max);

         // FS
         if (theta_max>superelevation_angle)
         {
            Float64 fs;
            if (mom_vert!=0.0)
               fs = r*(theta_max-superelevation_angle)/(zo*theta_max + ei + y*theta_max);
            else
               fs = Float64_Inf;

            crack_artifact.SetFsCracking(fs);
         }
         else
         {
            // superelevation angle cracks girder
            crack_artifact.SetFsCracking(0.0);
         }
      }
      else
      {
         crack_artifact.SetThetaCrackingMax(0.0);
         crack_artifact.SetFsCracking(0.0);
      }

      crack_artifact.SetAllowableFsForCracking(fs_all);

      pArtifact->AddHaulingCrackingAnalysisArtifact(poi.GetDistFromStart(),crack_artifact);
   }
}

void pgsGirderHandlingChecker::ComputeHaulingFsForRollover(const CSegmentKey& segmentKey,
                                                           pgsHaulingAnalysisArtifact* pArtifact)
{
   Float64 zo = pArtifact->GetZo();
   CHECK(zo>0.0);
   Float64 zmax = pArtifact->GetAxleWidth()/2.0;
   Float64 y= pArtifact->GetHeightOfCgOfGirderAboveRollCenter();
   Float64 hr = pArtifact->GetHeightOfRollCenterAboveRoadway();
   Float64 r = pArtifact->GetRadiusOfStability();
   Float64 alpha = pArtifact->GetRoadwaySuperelevation();
   Float64 ei = pArtifact->GetTotalInitialEccentricity();
   PRECONDITION(r>0.0);

   Float64 theta_max = (zmax - hr*alpha)/r + alpha;
   pArtifact->SetThetaRolloverMax(theta_max);

   Float64 zo_prime = zo*(1+2.5*theta_max);
   pArtifact->SetZoPrime(zo_prime);

   Float64 fs = r*(theta_max-alpha) / (zo_prime*theta_max + ei + y*theta_max);
   pArtifact->SetFsRollover(fs);
}

void pgsGirderHandlingChecker::ComputeHaulingMoments(const CSegmentKey& segmentKey,
                                                     const pgsHaulingAnalysisArtifact& rArtifact, 
                                                     const std::vector<pgsPointOfInterest>& rpoiVec,
                                                     std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection)
{
   // build a model
   Float64 glen = rArtifact.GetGirderLength();
   Float64 leftOH = rArtifact.GetTrailingOverhang();
   Float64 rightOH = rArtifact.GetLeadingOverhang();
   Float64 E = rArtifact.GetElasticModulusOfGirderConcrete();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   ComputeMoments(segmentKey,
                  haulSegmentIntervalIdx,
                  leftOH, glen, rightOH,
                  E,
                  POI_HAUL_SEGMENT,
                  rpoiVec,
                  pmomVec, pMidSpanDeflection);
}

void pgsGirderHandlingChecker::ComputeHaulingRollAngle(const CSegmentKey& segmentKey,
                                                       pgsHaulingAnalysisArtifact* pArtifact, 
                                                       const std::vector<pgsPointOfInterest> rpoiVec,
                                                       std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   Float64 fo = pArtifact->GetOffsetFactor();
   Float64 y = pArtifact->GetHeightOfCgOfGirderAboveRollCenter();
   
   Float64 kt = pArtifact->GetRollStiffnessOfvehicle();
   Float64 gird_len = pArtifact->GetGirderLength();
   Float64 gird_wt = pArtifact->GetGirderWeight();

   Float64 w = pArtifact->GetAvgGirderWeightPerLength();

   Float64 r = kt/gird_wt;
   pArtifact->SetRadiusOfStability(r);

   // Zo (based on mid-span section properties)
   // we want to use the mid-span of the actual girder, not the girder in the hauling configuration
   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(segmentKey,POI_RELEASED_SEGMENT | POI_MIDSPAN) );
   ATLASSERT(vPOI.size() == 1);
   pgsPointOfInterest poi( vPOI[0] );
   GET_IFACE(ISectionProperties,pSectProp);
   Float64 Ix = pSectProp->GetIx(releaseIntervalIdx,poi);
   Float64 Iy = pSectProp->GetIy(releaseIntervalIdx,poi);
   Float64 span_len = pArtifact->GetClearSpanBetweenSupportLocations();
   Float64 Ec = pArtifact->GetElasticModulusOfGirderConcrete();
   Float64 a = (gird_len-span_len)/2.;
   Float64 l = span_len;

   Float64 zo = (w/(12*Ec*Iy*gird_len))*
                (l*l*l*l*l/10. - a*a*l*l*l + 3.*a*a*a*a*l + 6.*a*a*a*a*a/5.);

   pArtifact->SetZo(zo);
   pArtifact->SetIy(Iy);
   pArtifact->SetIx(Ix);

   // roll angle
   Float64 ei = pArtifact->GetTotalInitialEccentricity();
   Float64 superelevation_angle = pArtifact->GetRoadwaySuperelevation();

   // if denominator for roll angle equation is negative, this means that the spring
   // stiffness cannot overcome the girder weight - unstable.
   Float64 denom = r-y-zo;
   if (denom<=0.0)
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);

      LPCTSTR msg = _T("Truck spring stiffness is inadequate - girder/trailer is unstable");
      pgsTruckStiffnessStatusItem* pStatusItem = new pgsTruckStiffnessStatusItem(m_StatusGroupID,m_scidTruckStiffness,msg);
      pStatusCenter->Add(pStatusItem);

      std::_tstring str(msg);
      str += std::_tstring(_T("\nSee Status Center for details"));
      THROW_UNWIND(str.c_str(),-1);
   }

   Float64 roll_angle = (superelevation_angle * r + ei) / (r-y-zo);
   pArtifact->SetEqualibriumAngle(roll_angle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The function below computes alternate tensile stress for the biaxial case. This is no longer used because inclined
// girders do not use the alternate increased tensile stress. They only use the fracture modulus.
// If we change our mind some day, this function will be useful.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
void pgsGirderHandlingChecker::GetRequirementsForAlternativeTensileStress(const pgsPointOfInterest& poi,Float64 ftu,Float64 ftd,Float64 fbu,Float64 fbd,Float64* pY,Float64* pA,Float64* pT,Float64* pAs)
{
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IShapes,pShapes);
   GET_IFACE(IMaterials,pMaterial);

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   Float64 Es, fs, fu;
   pMaterial->GetSegmentLongitudinalRebarProperties(segmentKey,&Es,&fs,&fu);
   fs *= 0.5;

   Float64 fsMax = (bUnitsSI ? ::ConvertToSysUnits(206.0,unitMeasure::MPa) : ::ConvertToSysUnits(30.0,unitMeasure::KSI) );
   if ( fsMax < fs )
    fs = fsMax;

   // Determine As requirement for alternative allowable tensile stress
   Float64 H = pGirder->GetHeight(poi);

   Float64 fTop = (ftu + ftd)/2;
   Float64 fBot = (fbu + fbd)/2;

   Float64 T = 0;
   Float64 At = 0;
   Float64 As = 0;
   Float64 Yna = -1;  // < 0 means it is not on the cross section
   if ( IsLE(fTop,0.) && IsLE(fBot,0.) )
   {
      // compression over entire cross section
      T = 0;
   }
   else if ( IsLE(0.,fTop) && IsLE(0.,fBot) )
   {
      // tension over entire cross section
      At = pSectProp->GetAg(releaseIntervalIdx,poi);
      Float64 fAvg = (fTop + fBot)/2;
      T = fAvg * At;
   }
   else
   {
      // Location of neutral axis from Bottom of Girder
      //       Yna = (IsZero(fBot) ? 0 : H - (fTop*H/(fTop-fBot)) );

      CComPtr<IShape> shape;
      pShapes->GetSegmentShape(releaseIntervalIdx,poi,false,pgsTypes::scGirder,&shape);

      CComQIPtr<IXYPosition> position(shape);
      CComPtr<IPoint2d> tc,bc;
      position->get_LocatorPoint(lpTopCenter,   &tc);
      position->get_LocatorPoint(lpBottomCenter,&bc);

      Float64 Xtop,Ytop;
      Float64 Xbot,Ybot;
      tc->get_X(&Xtop);
      tc->get_Y(&Ytop);

      bc->get_X(&Xbot);
      bc->get_Y(&Ybot);

      Float64 Wt, Wb;
      Wt = pGirder->GetTopWidth(poi);
      Wb = pGirder->GetBottomWidth(poi);

      // create a 3D plane to represent the stress plane
      CComPtr<IPlane3d> plane;
      plane.CoCreateInstance(CLSID_Plane3d);
      CComPtr<IPoint3d> p1,p2,p3;
      p1.CoCreateInstance(CLSID_Point3d);
      p2.CoCreateInstance(CLSID_Point3d);
      p3.CoCreateInstance(CLSID_Point3d);

      p1->Move( Xtop+Wt/2,Ytop,ftu);
      p2->Move( Xtop-Wt/2,Ytop,ftd);
      p3->Move( Xbot+Wb/2,Ybot,fbu);

      plane->ThroughPoints(p1,p2,p3);

      // Determine neutral axis line by finding two points where z(stress) is zero
      Float64 ya,yb;
      plane->GetY(-10000,0.00,&ya);
      plane->GetY( 10000,0.00,&yb);

      CComPtr<IPoint2d> pa,pb;
      pa.CoCreateInstance(CLSID_Point2d);
      pb.CoCreateInstance(CLSID_Point2d);
      pa->Move(-10000,ya);
      pb->Move( 10000,yb);

      CComPtr<ILine2d> line;
      line.CoCreateInstance(CLSID_Line2d);

      Float64 fAvg;

      // line clips away left hand side
      if ( 0 <= fTop && fBot <= 0 )
      {
         // Tension top, compression bottom
         // line needs to go right to left
         line->ThroughPoints(pb,pa);

         fAvg = fTop / 2;
      }
      else if ( fTop <= 0 && 0 <= fBot )
      {
         // Tension bottom
         // line needs to go left to right
         line->ThroughPoints(pa,pb);

         fAvg = fBot / 2;
      }

      CComPtr<IShape> clipped_shape;
      shape->ClipWithLine(line,&clipped_shape);

      CComPtr<IShapeProperties> props;
      clipped_shape->get_ShapeProperties(&props);

      props->get_Area(&At);

      T = fAvg * At;
   }

   As = T/fs;

   *pY = Yna;
   *pA = At;
   *pT = T;
   *pAs = As;
}
*/
void pgsGirderHandlingChecker::ComputeMoments(const CSegmentKey& segmentKey,
                                              IntervalIndexType intervalIdx,
                                              Float64 leftOH,Float64 segmentLength,Float64 rightOH,
                                              Float64 E,
                                              PoiAttributeType poiReference,
                                              const std::vector<pgsPointOfInterest>& rpoiVec,
                                              std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection)
{
   m_Model.Release();

   // need left and right support locations measured from the left end of the girder
   Float64 leftSupportLocation = leftOH;
   Float64 rightSupportLocation = segmentLength - rightOH;
   LoadCaseIDType lcid = 0;
   pgsGirderModelFactory::CreateGirderModel(m_pBroker,intervalIdx,segmentKey,leftSupportLocation,rightSupportLocation,E,lcid,true,rpoiVec,&m_Model,&m_PoiMap);

   // Get results
   CComQIPtr<IFem2dModelResults> results(m_Model);
   pmomVec->clear();
   *pMidSpanDeflection = 0.0;
   bool found_mid = false;

   Float64 dx,dy,rz;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(rpoiVec.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(rpoiVec.end());
   for ( ; poiIter != poiIterEnd; poiIter++)
   {
      const pgsPointOfInterest& poi = *poiIter;
      Float64 fx,fy,mz;
      PoiIDType femPoiID = m_PoiMap.GetModelPoi(poi);
      HRESULT hr = results->ComputePOIForces(lcid,femPoiID,mftLeft,lotMember,&fx,&fy,&mz);
      ATLASSERT(SUCCEEDED(hr));
      pmomVec->push_back(mz);


      if (poi.IsMidSpan(poiReference))
      {
         ATLASSERT(found_mid == false);
         ATLASSERT( IsEqual(poi.GetDistFromStart(),(segmentLength-leftOH-rightOH)/2.0 + leftOH) );

         hr = results->ComputePOIDisplacements(lcid,femPoiID,lotMember,&dx,&dy,&rz);
         ATLASSERT(SUCCEEDED(hr));

         *pMidSpanDeflection = dy;
         found_mid = true;
      }
   }

   ATLASSERT(found_mid); // must have a point at mid-span for calc to work right
}

/****************************************************************************
CLASS
   pgsAlternativeTensileStressCalculator
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

pgsAlternativeTensileStressCalculator::pgsAlternativeTensileStressCalculator(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx, IGirder* pGirder,
                                         IShapes* pShapes,ISectionProperties* pSectProps, ILongRebarGeometry* pRebarGeom,
                                         IMaterials* pMaterials, bool bSIUnits):
m_pGirder(pGirder),
m_pShapes(pShapes),
m_pSectProps(pSectProps),
m_pRebarGeom(pRebarGeom),
m_pMaterials(pMaterials)
{
   m_IntervalIdx = intervalIdx;

   // Precompute some values
   Float64 Es, fy, fu;
   pMaterials->GetSegmentLongitudinalRebarProperties(segmentKey,&Es,&fy,&fu);

   // Max bar stress for computing higher allowable temporary tensile (5.9.4.1.2)
   Float64 fs = 0.5*fy;
   Float64 fsMax = (bSIUnits ? ::ConvertToSysUnits(206.0,unitMeasure::MPa) : ::ConvertToSysUnits(30.0,unitMeasure::KSI) );
   if ( fsMax < fs )
       fs = fsMax;

   m_AllowableFs = fs; 
}

Float64 pgsAlternativeTensileStressCalculator::ComputeAlternativeStressRequirements(
                                        const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,
                                        Float64 fTop, Float64 fBot, 
                                        Float64 lowAllowTens, Float64 highAllowTens,
                                        Float64 *pYna, Float64 *pAreaTens, Float64 *pT, 
                                        Float64 *pAsProvd, Float64 *pAsReqd, bool* pIsAdequateRebar)
{
   // Determine neutral axis location and mild steel requirement for alternative tensile stress
   typedef enum {slAllTens, slAllComp, slTopTens, slBotTens} StressLocation;
   StressLocation stressLoc;
   Float64 Yna = -1;
   Float64 H = m_pGirder->GetHeight(poi);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   if ( fTop <= TOLERANCE && fBot <= TOLERANCE )
   {
      // compression over entire cross section
      stressLoc = slAllComp;
   }
   else if ( 0 <= fTop && 0 <= fBot )
   {
       // tension over entire cross section
      stressLoc = slAllTens;
   }
   else
   {
      ATLASSERT( BinarySign(fBot) != BinarySign(fTop) );

      stressLoc = fBot>0.0 ? slBotTens : slTopTens;

      // Location of neutral axis from Bottom of Girder
      Yna = (IsZero(fBot) ? 0 : H - (fTop*H/(fTop-fBot)) );

      ATLASSERT( 0 <= Yna );
   }

   // Compute area on concrete in tension and total tension
   Float64 AreaTens; // area of concrete in tension
   Float64 T;        // tension
   if ( stressLoc == slAllComp )
   {
       // Compression over entire cross section
      AreaTens = 0.0;
      T = 0.0;
   }
   else if ( stressLoc == slAllTens )
   {
       // Tension over entire cross section
       AreaTens = m_pSectProps->GetAg(m_IntervalIdx,poi);
       Float64 fAvg = (fTop + fBot)/2;
       T = fAvg * AreaTens;

       ATLASSERT( T != 0 );
   }
   else
   {
      // Clip shape to determine concrete tension area
      CComPtr<IShape> shape;
      m_pShapes->GetSegmentShape(m_IntervalIdx,poi,false,pgsTypes::scGirder,&shape);

      CComQIPtr<IXYPosition> position(shape);
      CComPtr<IPoint2d> bc;
      position->get_LocatorPoint(lpBottomCenter,&bc);
      Float64 Y;
      bc->get_Y(&Y);

      CComPtr<ILine2d> line;
      line.CoCreateInstance(CLSID_Line2d);
      CComPtr<IPoint2d> p1, p2;
      p1.CoCreateInstance(CLSID_Point2d);
      p2.CoCreateInstance(CLSID_Point2d);
      p1->Move(-10000,Y+Yna);
      p2->Move( 10000,Y+Yna);

      Float64 fAvg;

      if ( stressLoc == slTopTens )
      {
          // Tension top, compression bottom
          // line needs to go right to left
         line->ThroughPoints(p2,p1);

         fAvg = fTop / 2;
      }
      else
      {
         // Compression Top, Tension Bottom
         // line needs to go left to right
         ATLASSERT(stressLoc==slBotTens);
         line->ThroughPoints(p1,p2);

         fAvg = fBot / 2;
      }

      CComPtr<IShape> clipped_shape;
      shape->ClipWithLine(line,&clipped_shape);

      if ( clipped_shape )
      {
         CComPtr<IShapeProperties> props;
         clipped_shape->get_ShapeProperties(&props);

         props->get_Area(&AreaTens);
      }
      else
      {
         AreaTens = 0.0;
      }

      T = fAvg * AreaTens;

      ATLASSERT( T != 0 );
   }

   // Area of steel required to meet higher tensile stress requirement
   Float64 AsReqd = T/m_AllowableFs;
   ATLASSERT( 0 <= AsReqd );

// This will need to be revisited if we start designing longitudinal rebar
#pragma Reminder("This function assumes that longitudinal rebar does not change during design")

   // Compute area of rebar actually provided in tension zone. Reduce values for development
   Float64 AsProvd = 0.0; // As provided
   if ( stressLoc != slAllComp )
   {
      CComPtr<IRebarSection> rebar_section;
      m_pRebarGeom->GetRebars(poi,&rebar_section);

      pgsTypes::ConcreteType conc_type;
      Float64 fci, fct;
      bool isfct;
      if (pConfig!=NULL)
      {
         fci       = pConfig->Fci;
         conc_type = pConfig->ConcType;
         isfct     = pConfig->bHasFct;
         fct       = pConfig->Fct;
      }
      else
      {
         fci       = m_pMaterials->GetSegmentFc(segmentKey,m_IntervalIdx);
         conc_type = m_pMaterials->GetSegmentConcreteType(segmentKey);
         isfct     = m_pMaterials->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
         fct       = isfct ? m_pMaterials->GetSegmentConcreteAggSplittingStrength(segmentKey) : 0.0;
      }

      CComPtr<IEnumRebarSectionItem> enumItems;
      rebar_section->get__EnumRebarSectionItem(&enumItems);

      CComPtr<IRebarSectionItem> item;
      while ( enumItems->Next(1,&item,NULL) != S_FALSE )
      {
         CComPtr<IRebar> rebar;
         item->get_Rebar(&rebar);
         Float64 as;
         rebar->get_NominalArea(&as);

         Float64 dev_length_factor = m_pRebarGeom->GetDevLengthFactor(item, conc_type, fci, isfct, fct);

         if (dev_length_factor >= 1.0) // Bars must be fully developed before higher 
                                       // allowable stress can be used
         {
            ATLASSERT(dev_length_factor==1.0);

            if (stressLoc == slAllTens)
            {
               // all bars in tension - just add
               AsProvd += as;
            }
            else
            {
               CComPtr<IPoint2d> location;
               item->get_Location(&location);

               Float64 x,y;
               location->get_X(&x);
               location->get_Y(&y);
               // Add bar if it's on right side of NA
               if ( stressLoc == slTopTens && y > Yna)
               {
                  AsProvd += as;
               }
               else if ( stressLoc == slBotTens && y < Yna)
               {
                  AsProvd += as;
               }
            }
         }

         item.Release();
      }
   }

   // Now we can determine which allowable we can use
   Float64 fAllowable;
   if (AsProvd > AsReqd)
   {
      fAllowable = highAllowTens;
      *pIsAdequateRebar = true;
   }
   else
   {
      fAllowable = lowAllowTens;
      *pIsAdequateRebar = false;
   }

   *pYna = Yna;
   *pAreaTens = AreaTens;
   *pT = T;
   *pAsProvd = AsProvd;
   *pAsReqd = AsReqd;

   return fAllowable;
}

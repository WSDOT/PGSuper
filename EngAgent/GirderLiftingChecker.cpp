///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include "AlternativeTensileStressCalculator.h"

#include "GirderHandlingChecker.h"
#include "GirderLiftingChecker.h"
#include "StatusItems.h"

#include "PGSuperUnits.h"

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

void pgsGirderLiftingChecker::CheckLifting(const CSegmentKey& segmentKey,pgsLiftingAnalysisArtifact* pArtifact)
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

void pgsGirderLiftingChecker::AnalyzeLifting(const CSegmentKey& segmentKey,Float64 supportLoc,pgsLiftingAnalysisArtifact* pArtifact)
{
   GET_IFACE(ISegmentLiftingPointsOfInterest,pSegmentLiftingPointsOfInterest);
   HANDLINGCONFIG dummy_config;
   dummy_config.bIgnoreGirderConfig = true;
   dummy_config.LeftOverhang = supportLoc;
   dummy_config.RightOverhang = supportLoc;
   AnalyzeLifting(segmentKey,true,dummy_config,pSegmentLiftingPointsOfInterest,pArtifact);
}

void pgsGirderLiftingChecker::AnalyzeLifting(const CSegmentKey& segmentKey,const HANDLINGCONFIG& liftConfig,ISegmentLiftingDesignPointsOfInterest* pPoiD, pgsLiftingAnalysisArtifact* pArtifact)
{
   AnalyzeLifting(segmentKey,true,liftConfig,pPoiD,pArtifact);
}

void pgsGirderLiftingChecker::AnalyzeLifting(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& liftConfig,ISegmentLiftingDesignPointsOfInterest* pPoiD,pgsLiftingAnalysisArtifact* pArtifact)
{
   // calc some initial information
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);

   Float64 Loh, Roh, Eci, Fci;
   pgsTypes::ConcreteType concType;

   std::vector<pgsPointOfInterest> vPoi;
   if ( bUseConfig )
   {
      Loh = liftConfig.LeftOverhang;
      Roh = liftConfig.RightOverhang;

      vPoi = pPoiD->GetLiftingDesignPointsOfInterest(segmentKey,Loh);
   }
   else
   {
      GET_IFACE(ISegmentLifting,pSegmentLifting);
      Loh = pSegmentLifting->GetLeftLiftingLoopLocation(segmentKey);
      Roh = pSegmentLifting->GetRightLiftingLoopLocation(segmentKey);

      GET_IFACE(ISegmentLiftingPointsOfInterest,pSegmentLiftingPointsOfInterest);
      vPoi = pSegmentLiftingPointsOfInterest->GetLiftingPointsOfInterest(segmentKey,0);
   }
   ATLASSERT(0 < vPoi.size());

   if ( !bUseConfig || liftConfig.bIgnoreGirderConfig )
   {
      // Not using the configuration, or the configuration applies only for the overhang

      Eci = pMaterial->GetSegmentEc(segmentKey,liftSegmentIntervalIdx);
      Fci = pMaterial->GetSegmentFc(segmentKey,liftSegmentIntervalIdx);
      concType = pMaterial->GetSegmentConcreteType(segmentKey);
   }
   else
   {
      // Using the config
      ATLASSERT(liftConfig.bIgnoreGirderConfig == false);

      if ( liftConfig.GdrConfig.bUserEci )
      {
         Eci = liftConfig.GdrConfig.Eci;
      }
      else
      {
         Eci = pMaterial->GetEconc(liftConfig.GdrConfig.Fci, pMaterial->GetSegmentStrengthDensity(segmentKey),
                                                             pMaterial->GetSegmentEccK1(segmentKey),
                                                             pMaterial->GetSegmentEccK2(segmentKey));
      }

      Fci = liftConfig.GdrConfig.Fci;
      concType = liftConfig.GdrConfig.ConcType;
   }

   
   PrepareLiftingAnalysisArtifact(segmentKey,Loh,Roh,Fci,Eci,concType,pArtifact);

   // get moments at pois  and mid-span deflection due to dead vertical lifting
   std::vector<Float64> vMoment;
   Float64 mid_span_deflection;
   ComputeLiftingMoments(segmentKey, *pArtifact, vPoi, &vMoment,&mid_span_deflection);

   ComputeLiftingStresses(segmentKey, bUseConfig, liftConfig, vPoi, vMoment, pArtifact);

   if (ComputeLiftingFsAgainstCracking(segmentKey, bUseConfig, liftConfig, vPoi, vMoment, mid_span_deflection, pArtifact))
   {
      ComputeLiftingFsAgainstFailure(segmentKey, pArtifact);
   }
   else
   {
      pArtifact->SetThetaFailureMax(0.0);
      pArtifact->SetFsFailure(0.0);
   }
}

pgsDesignCodes::OutcomeType pgsGirderLiftingChecker::DesignLifting(const CSegmentKey& segmentKey,const GDRCONFIG& config,ISegmentLiftingDesignPointsOfInterest* pPoiD,pgsLiftingAnalysisArtifact* pArtifact,SHARED_LOGFILE LOGFILE)
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

   if (0 < config.PrestressConfig.GetStrandCount(pgsTypes::Harped)) // only look at harping point if we have harped strands
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

   HANDLINGCONFIG lift_config;
   lift_config.bIgnoreGirderConfig = false;
   lift_config.GdrConfig = config;

   pgsLiftingAnalysisArtifact artifact;
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
bool pgsGirderLiftingChecker::AssertValid() const
{
//#pragma Reminder("TODO: Implement the AssertValid method for pgsGirderLiftingChecker")
   return true;
}

void pgsGirderLiftingChecker::Dump(dbgDumpContext& os) const
{
//#pragma Reminder("TODO: Implement the Dump method for pgsGirderLiftingChecker")
   os << "Dump for pgsGirderLiftingChecker" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsGirderLiftingChecker::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsGirderLiftingChecker");

//#pragma Reminder("TODO: Implement the TestMe method for pgsGirderLiftingChecker")
   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsGirderLiftingChecker");

   TESTME_EPILOG("GirderHandlingChecker");
}
#endif // _UNITTEST

// lifting
void pgsGirderLiftingChecker::PrepareLiftingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 Loh,Float64 Roh,Float64 Fci,Float64 Eci,pgsTypes::ConcreteType concType,pgsLiftingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IBridge, pBridge);
   Float64 girder_length = pBridge->GetSegmentLength(segmentKey);
   pArtifact->SetGirderLength(girder_length);

   GET_IFACE(ISectionProperties,pSectProp);
   Float64 volume = pSectProp->GetSegmentVolume(segmentKey);

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
      pgsLiftingSupportLocationStatusItem* pStatusItem = new pgsLiftingSupportLocationStatusItem(segmentKey,pgsTypes::metStart,m_StatusGroupID,m_scidLiftingSupportLocationError,msg);
      pStatusCenter->Add(pStatusItem);

      std::_tstring str(msg);
      str += std::_tstring(_T("\nSee Status Center for details"));
      THROW_UNWIND(str.c_str(),-1);
   }

   GET_IFACE(ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   pArtifact->SetAllowableFsForCracking(pSegmentLiftingSpecCriteria->GetLiftingCrackingFs());
   pArtifact->SetAllowableFsForFailure(pSegmentLiftingSpecCriteria->GetLiftingFailureFs());

   Float64 min_lift_point_start = pSegmentLiftingSpecCriteria->GetMinimumLiftingPointLocation(segmentKey,pgsTypes::metStart);
   Float64 min_lift_point_end   = pSegmentLiftingSpecCriteria->GetMinimumLiftingPointLocation(segmentKey,pgsTypes::metEnd);
   if ( Loh < min_lift_point_start )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format(_T("Group %d Girder %s Segment %d: Left lift point is less than the minimum value of %s"),LABEL_GROUP(segmentKey.groupIndex),LABEL_GIRDER(segmentKey.girderIndex),LABEL_SEGMENT(segmentKey.segmentIndex),::FormatDimension(min_lift_point_start,pDisplayUnits->GetSpanLengthUnit()));
      pgsLiftingSupportLocationStatusItem* pStatusItem = new pgsLiftingSupportLocationStatusItem(segmentKey,pgsTypes::metStart,m_StatusGroupID,m_scidLiftingSupportLocationWarning,strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   if ( Roh < min_lift_point_end )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format(_T("Group %d Girder %s Segment %d: Right lift point is less than the minimum value of %s"),LABEL_GROUP(segmentKey.groupIndex),LABEL_GIRDER(segmentKey.girderIndex),LABEL_SEGMENT(segmentKey.segmentIndex),::FormatDimension(min_lift_point_end,pDisplayUnits->GetSpanLengthUnit()));
      pgsLiftingSupportLocationStatusItem* pStatusItem = new pgsLiftingSupportLocationStatusItem(segmentKey,pgsTypes::metEnd,m_StatusGroupID,m_scidLiftingSupportLocationWarning,strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   pArtifact->SetClearSpanBetweenPickPoints(span_len);
   pArtifact->SetOverhangs(Loh,Roh);

   Float64 bracket_hgt = pSegmentLiftingSpecCriteria->GetHeightOfPickPointAboveGirderTop();
   Float64 ycgt = pSectProp->GetY(liftSegmentIntervalIdx,pgsPointOfInterest(segmentKey,0.00),pgsTypes::TopGirder);
   Float64 e_hgt = bracket_hgt+ycgt;
   pArtifact->SetVerticalDistanceFromPickPointToGirderCg(e_hgt);

   Float64 upwardi, downwardi;
   pSegmentLiftingSpecCriteria->GetLiftingImpact(&downwardi, &upwardi);
   pArtifact->SetUpwardImpact(upwardi);
   pArtifact->SetDownwardImpact(downwardi);

   pArtifact->SetSweepTolerance(pSegmentLiftingSpecCriteria->GetLiftingSweepTolerance());
   pArtifact->SetLiftingDeviceTolerance(pSegmentLiftingSpecCriteria->GetLiftingLoopPlacementTolerance());

   pArtifact->SetConcreteStrength(Fci);
   pArtifact->SetModRupture( pSegmentLiftingSpecCriteria->GetLiftingModulusOfRupture(Fci,concType) );
   pArtifact->SetModRuptureCoefficient( pSegmentLiftingSpecCriteria->GetLiftingModulusOfRuptureFactor(concType) );

   pArtifact->SetElasticModulusOfGirderConcrete(Eci);

   Float64 gird_weight = pArtifact->GetGirderWeight();
   Float64 ang = pSegmentLiftingSpecCriteria->GetLiftingCableMinInclination();

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

   Float64 sweep = pSegmentLiftingSpecCriteria->GetLiftingSweepTolerance();
   Float64 e_sweep = sweep * girder_length;
   pArtifact->SetEccentricityDueToSweep(e_sweep);

   Float64 e_placement = pSegmentLiftingSpecCriteria->GetLiftingLoopPlacementTolerance();
   pArtifact->SetEccentricityDueToPlacementTolerance(e_placement);

   pArtifact->SetTotalInitialEccentricity(e_sweep*offset_factor + e_placement);
}

void pgsGirderLiftingChecker::ComputeLiftingMoments(const CSegmentKey& segmentKey,
                                                     const pgsLiftingAnalysisArtifact& rArtifact, 
                                                     const std::vector<pgsPointOfInterest>& vPoi,
                                                     std::vector<Float64>* pvMoment, 
                                                     Float64* pMidSpanDeflection)
{
   // build a model
   Float64 glen    = rArtifact.GetGirderLength();
   Float64 leftOH  = rArtifact.GetLeftOverhang();
   Float64 rightOH = rArtifact.GetRightOverhang();
   Float64 E = rArtifact.GetElasticModulusOfGirderConcrete();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   pgsGirderModelFactory girderModelFactory;

   pgsGirderHandlingChecker::ComputeMoments(m_pBroker,&girderModelFactory,segmentKey,
                                            liftSegmentIntervalIdx,
                                            leftOH, glen, rightOH,
                                            E,
                                            POI_LIFT_SEGMENT,
                                            vPoi,
                                            pvMoment, 
                                            pMidSpanDeflection);
}

void pgsGirderLiftingChecker::ComputeLiftingStresses(const CSegmentKey& segmentKey,bool bUseConfig,
                                                      const HANDLINGCONFIG& liftConfig,
                                                      const std::vector<pgsPointOfInterest>& vPoi,
                                                      const std::vector<Float64>& vMoment,
                                                      pgsLiftingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IPretensionForce,           pPrestressForce);
   GET_IFACE(IStrandGeometry,            pStrandGeometry);
   GET_IFACE(ISegmentLiftingSpecCriteria, pSegmentLiftingSpecCriteria);
   GET_IFACE(IBridge,                    pBridge);
   GET_IFACE(IGirder,                    pGirder);
   GET_IFACE(ISectionProperties,         pSectProps);
   GET_IFACE(IShapes,                    pShapes);
   GET_IFACE(IMaterials,                 pMaterials);
   GET_IFACE(ILongRebarGeometry,         pRebarGeom);
   GET_IFACE(IPointOfInterest,           pPoi);
   GET_IFACE(IIntervals,                 pIntervals);
   
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);


   // get poi-independent values used for stress calc
   // factor in forces from inclined lifting cables
   Float64 p_lift = pArtifact->GetAxialCompressiveForceDueToInclinationOfLiftingCables();
   Float64 m_lift = pArtifact->GetMomentInGirderDueToInclinationOfLiftingCables();
   Float64 impact_up   = 1.0 - pArtifact->GetUpwardImpact();
   Float64 impact_down = 1.0 + pArtifact->GetDownwardImpact();

   IndexType psiz = vPoi.size();
   IndexType msiz = vMoment.size();
   ATLASSERT(psiz==msiz);

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
   Float64 fLowTensAllowable  = pSegmentLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStress(segmentKey);
   Float64 fHighTensAllowable = pSegmentLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStress(segmentKey);
   Float64 fCompAllowable     = pSegmentLiftingSpecCriteria->GetLiftingAllowableCompressiveConcreteStress(segmentKey);

   // Parameters for computing required concrete strengths
   Float64 rcsC = pSegmentLiftingSpecCriteria->GetLiftingAllowableCompressionFactor();
   Float64 rcsT;
   bool    rcsBfmax;
   Float64 rcsFmax;
   pSegmentLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStressParameters(&rcsT,&rcsBfmax,&rcsFmax);
   Float64 rcsTalt = pSegmentLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStressFactor();

   bool bSISpec = lrfdVersionMgr::GetVersion() == lrfdVersionMgr::SI ? true : false;

   // Use calculator object to deal with casting yard higher allowable stress
   pgsAlternativeTensileStressCalculator altCalc(segmentKey, liftSegmentIntervalIdx, pBridge, pGirder, pShapes,pSectProps, pRebarGeom, pMaterials, pPoi, true/*limit bar stress*/, bSISpec, true /*girder stresses*/);

   Float64 AsMax = 0;
   Float64 As = 0;
   for(IndexType i=0; i<psiz; i++)
   {
      const pgsPointOfInterest& poi = vPoi[i];

      Float64 ag,stg,sbg;
      if ( bUseConfig && !liftConfig.bIgnoreGirderConfig )
      {
         ag  = pSectProps->GetAg(liftSegmentIntervalIdx,poi,liftConfig.GdrConfig.Fci);
         stg = pSectProps->GetS(liftSegmentIntervalIdx,poi,pgsTypes::TopGirder,   liftConfig.GdrConfig.Fci);
         sbg = pSectProps->GetS(liftSegmentIntervalIdx,poi,pgsTypes::BottomGirder,liftConfig.GdrConfig.Fci);
      }
      else
      {
         ag  = pSectProps->GetAg(liftSegmentIntervalIdx,poi);
         stg = pSectProps->GetS(liftSegmentIntervalIdx,poi,pgsTypes::TopGirder);
         sbg = pSectProps->GetS(liftSegmentIntervalIdx,poi,pgsTypes::BottomGirder);
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

      if ( bUseConfig && !liftConfig.bIgnoreGirderConfig )
      {
         hps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Harped,liftSegmentIntervalIdx,pgsTypes::Middle,pgsTypes::ServiceI,liftConfig.GdrConfig);
         he        = pStrandGeometry->GetHsEccentricity(liftSegmentIntervalIdx,poi,liftConfig.GdrConfig, &nfh);
         sps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Straight,liftSegmentIntervalIdx,pgsTypes::Middle,pgsTypes::ServiceI,liftConfig.GdrConfig);
         se        = pStrandGeometry->GetSsEccentricity(liftSegmentIntervalIdx,poi,liftConfig.GdrConfig, &nfs);
         tps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,liftSegmentIntervalIdx,pgsTypes::Middle,pgsTypes::ServiceI,liftConfig.GdrConfig);
         te        = pStrandGeometry->GetTempEccentricity(liftSegmentIntervalIdx,poi,liftConfig.GdrConfig, &nft);
      }
      else
      {
         hps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Harped,liftSegmentIntervalIdx,pgsTypes::Middle,pgsTypes::ServiceI);
         he        = pStrandGeometry->GetHsEccentricity(liftSegmentIntervalIdx,poi,&nfh);
         sps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Straight,liftSegmentIntervalIdx,pgsTypes::Middle,pgsTypes::ServiceI);
         se        = pStrandGeometry->GetSsEccentricity(liftSegmentIntervalIdx,poi,&nfs);
         tps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,liftSegmentIntervalIdx,pgsTypes::Middle,pgsTypes::ServiceI);
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
      Float64 mom_no   = vMoment[i] + Mlift;
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

      const GDRCONFIG* pConfig = (bUseConfig && !liftConfig.bIgnoreGirderConfig ) ? &(liftConfig.GdrConfig) : NULL;

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
      pgsAlternativeTensileStressCalculator::ComputeReqdFcTens(max_stress, rcsT, rcsBfmax, rcsFmax, rcsTalt, &fc_tens_norebar, &fc_tens_withrebar);

      lift_artifact.SetRequiredConcreteStrength(fc_compression,fc_tens_norebar,fc_tens_withrebar);

      pArtifact->AddLiftingStressAnalysisArtifact(poi,lift_artifact);
   }
}

bool pgsGirderLiftingChecker::ComputeLiftingFsAgainstCracking(const CSegmentKey& segmentKey,bool bUseConfig,
                                                               const HANDLINGCONFIG& liftConfig,
                                                               const std::vector<pgsPointOfInterest>& vPoi,
                                                               const std::vector<Float64>& vMoment,
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
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
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
   if ( bUseConfig && !liftConfig.bIgnoreGirderConfig )
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

   GET_IFACE(ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   Float64 fs_all_cr = pSegmentLiftingSpecCriteria->GetLiftingCrackingFs();

   // If at this point, yr is negative, then the girder is unstable for
   // lifting and we can do no more calculations. So return false.
   if (0.0 < ayr)
   {
      Float64 fr = pArtifact->GetModRupture();
      GET_IFACE(IGirder,pGdr);
      Float64 bt_bot = pGdr->GetBottomWidth(poi_ms);
      Float64 bt_top = pGdr->GetTopWidth(poi_ms);

      // loop over all pois and calculate cracking fs
      ATLASSERT(vPoi.size() == vMoment.size());
      std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
      std::vector<Float64>::const_iterator momentIter(vMoment.begin());
      for ( ; poiIter != poiIterEnd; poiIter++, momentIter++ )
      {
         const pgsPointOfInterest& poi( *poiIter );
         Float64 mom_vert = *momentIter;

         pgsLiftingCrackingAnalysisArtifact crack_artifact;
         crack_artifact.SetVerticalMoment(mom_vert);

         crack_artifact.SetAllowableFsForCracking(fs_all_cr);

         // determine lateral moment that will cause cracking in section
         // upward impact will crack top flange and downward impact will crack bottom
         const pgsLiftingStressAnalysisArtifact* pstr = pArtifact->GetLiftingStressAnalysisArtifact(poi);
         ATLASSERT(pstr!=0);

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
         if (0.0 < m_lat)
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

         pArtifact->AddLiftingCrackingAnalysisArtifact(poi,crack_artifact);
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

void pgsGirderLiftingChecker::ComputeLiftingFsAgainstFailure(const CSegmentKey& segmentKey,
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


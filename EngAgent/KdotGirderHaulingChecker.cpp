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
#include "GirderHandlingChecker.h"
#include "AlternativeTensileStressCalculator.h"

#include "KdotGirderHaulingChecker.h"
#include "StatusItems.h"

#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsKdotGirderHaulingChecker
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////


//======================== LIFECYCLE  =======================================
pgsKdotGirderHaulingChecker::pgsKdotGirderHaulingChecker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidBunkPointLocation             = pStatusCenter->RegisterCallback( new pgsBunkPointLocationStatusCallback(m_pBroker) );
}

pgsKdotGirderHaulingChecker::~pgsKdotGirderHaulingChecker()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

pgsHaulingAnalysisArtifact* pgsKdotGirderHaulingChecker::CheckHauling(const CSegmentKey& segmentKey, SHARED_LOGFILE LOGFILE)
{
   GET_IFACE(ISegmentHaulingSpecCriteria,pHaulingSpecCriteria);

   if (pHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      ATLASSERT(pHaulingSpecCriteria->GetHaulingAnalysisMethod()==pgsTypes::hmKDOT);

      // Run analysis to get check results
      std::unique_ptr<pgsHaulingAnalysisArtifact> artifact( AnalyzeHauling(segmentKey) );

      pgsKdotHaulingAnalysisArtifact* kart = dynamic_cast<pgsKdotHaulingAnalysisArtifact*>(artifact.get()); // eating our own dog food here

      bool didpass = kart->Passed(pgsTypes::CrownSlope);

      // Next run design to get overhang limits
      GET_IFACE(IBridge,pBridge);
      GET_IFACE(ISegmentHaulingPointsOfInterest, pPOId);

      HANDLINGCONFIG config;
      config.bIgnoreGirderConfig = false;
      config.GdrConfig = pBridge->GetSegmentConfiguration(segmentKey);
      bool success;

      // Design
      std::unique_ptr<pgsHaulingAnalysisArtifact> dartifact( this->DesignHauling( segmentKey, config, true, false, pPOId,  &success, LOGFILE) );

      pgsKdotHaulingAnalysisArtifact* dkart = dynamic_cast<pgsKdotHaulingAnalysisArtifact*>(dartifact.get());

      pgsKdotHaulingAnalysisArtifact::DesignOutcome outcome = dkart->GetDesignOutcome();
      Float64 design_overhang = dkart->GetDesignOverhang();

      // Transfer design results to check artifact
      kart->SetDesignOutcome(outcome);
      kart->SetDesignOverhang(design_overhang);

      return artifact.release();
   }
   else
   {
      return nullptr;
   }
}

pgsHaulingAnalysisArtifact*  pgsKdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey)
{
   GET_IFACE(ISegmentHaulingPointsOfInterest,pSegmentHaulingPointsOfInterest); // poi's from global pool

   std::unique_ptr<pgsKdotHaulingAnalysisArtifact> pArtifact(std::make_unique<pgsKdotHaulingAnalysisArtifact>());

   HANDLINGCONFIG dummy_config;
   AnalyzeHauling(segmentKey,false,dummy_config,pSegmentHaulingPointsOfInterest,pArtifact.get());

   return pArtifact.release();
}

pgsHaulingAnalysisArtifact* pgsKdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,Float64 leftOverhang,Float64 rightOverhang)
{
   std::unique_ptr<pgsKdotHaulingAnalysisArtifact> pArtifact(std::make_unique<pgsKdotHaulingAnalysisArtifact>());
   GET_IFACE(ISegmentHaulingPointsOfInterest,pSegmentHaulingPointsOfInterest); // poi's from global pool
   HANDLINGCONFIG dummy_config;
   dummy_config.bIgnoreGirderConfig = true;
   dummy_config.LeftOverhang = leftOverhang;
   dummy_config.RightOverhang = rightOverhang;
   AnalyzeHauling(segmentKey,true,dummy_config,pSegmentHaulingPointsOfInterest,pArtifact.get());
   return pArtifact.release();
}

pgsHaulingAnalysisArtifact* pgsKdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,const HANDLINGCONFIG& haulConfig,ISegmentHaulingDesignPointsOfInterest* pPOId)
{
   std::unique_ptr<pgsKdotHaulingAnalysisArtifact> pArtifact(std::make_unique<pgsKdotHaulingAnalysisArtifact>());
   AnalyzeHauling(segmentKey,true,haulConfig,pPOId,pArtifact.get());

   return pArtifact.release();
}

void pgsKdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& haulConfig,
                                                 ISegmentHaulingDesignPointsOfInterest* pPOId,pgsKdotHaulingAnalysisArtifact* pArtifact)
{
   // calc some initial information
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);

   Float64 Loh, Roh, Ec, Fc;
   pgsTypes::ConcreteType concType;

   PoiList vPoi;
   std::vector<pgsPointOfInterest> vLocalPois; // must be scoped to match vPoi
   if ( bUseConfig )
   {
      Loh = haulConfig.LeftOverhang;
      Roh = haulConfig.RightOverhang;

      pPOId->GetHaulingDesignPointsOfInterest(segmentKey,2,Loh,Roh,0,&vLocalPois);
      MakePoiList(vLocalPois, &vPoi);
   }
   else
   {
      GET_IFACE(ISegmentHauling,pSegmentHauling);
      Loh = pSegmentHauling->GetTrailingOverhang(segmentKey);
      Roh = pSegmentHauling->GetLeadingOverhang(segmentKey);

      GET_IFACE(ISegmentHaulingPointsOfInterest,pSegmentHaulingPointsOfInterest);
      pSegmentHaulingPointsOfInterest->GetHaulingPointsOfInterest(segmentKey,0,&vPoi);
   }

   if ( !bUseConfig || haulConfig.bIgnoreGirderConfig )
   {
      // Not using the configuration, or the configuration applies only for the overhang

      Fc = pMaterial->GetSegmentDesignFc(segmentKey,haulSegmentIntervalIdx);
      Ec = pMaterial->GetSegmentEc(segmentKey,haulSegmentIntervalIdx);
      concType = pMaterial->GetSegmentConcreteType(segmentKey);
   }
   else
   {
      // Using the config
      ATLASSERT(haulConfig.bIgnoreGirderConfig == false);

      if ( haulConfig.GdrConfig.bUserEc )
      {
         Ec = haulConfig.GdrConfig.Ec;
      }
      else
      {
         Ec = pMaterial->GetEconc(haulConfig.GdrConfig.Fc, pMaterial->GetSegmentStrengthDensity(segmentKey),
                                                           pMaterial->GetSegmentEccK1(segmentKey),
                                                           pMaterial->GetSegmentEccK2(segmentKey));
      }

      Fc = haulConfig.GdrConfig.Fc;
      concType = haulConfig.GdrConfig.ConcType;
   }

   PrepareHaulingAnalysisArtifact(segmentKey,Loh,Roh,Fc,Ec,concType,pArtifact);

   // get moments at pois  and mid-span deflection due to dead vertical hauling
   std::vector<Float64> vMoment;
   Float64 mid_span_deflection;
   ComputeHaulingMoments(segmentKey, *pArtifact, vPoi, &vMoment,&mid_span_deflection);

   ComputeHaulingStresses(segmentKey, bUseConfig, haulConfig, vPoi, vMoment, pArtifact);
}

pgsHaulingAnalysisArtifact* pgsKdotGirderHaulingChecker::DesignHauling(const CSegmentKey& segmentKey,HANDLINGCONFIG& shipping_config,bool bDesignForEqualOverhangs,bool bIgnoreConfigurationLimits,ISegmentHaulingDesignPointsOfInterest* pPOId,bool* bSuccess, SHARED_LOGFILE LOGFILE)
{
   LOG(_T("Entering pgsKdotGirderHaulingChecker::DesignHauling"));

   // Assume the best
   *bSuccess = true;

   std::unique_ptr<pgsKdotHaulingAnalysisArtifact> pArtifact(std::make_unique<pgsKdotHaulingAnalysisArtifact>());

   // Get range of values for truck support locations
   GET_IFACE(IBridge,pBridge);
   Float64 Ls = pBridge->GetSegmentLength(segmentKey);

   GET_IFACE(IKdotGirderHaulingSpecCriteria,pCriteria);
   Float64 hardMinHaulingDistance, haulingDistanceFactor;
   bool bUseHaulingDistanceFactor;
   pCriteria->GetMinimumHaulingSupportLocation(&hardMinHaulingDistance,&bUseHaulingDistanceFactor,&haulingDistanceFactor);

   Float64 softMinHaulingDistance(hardMinHaulingDistance);
   if(bUseHaulingDistanceFactor)
   {
      softMinHaulingDistance = Max(Ls*haulingDistanceFactor, hardMinHaulingDistance);
   }

   const Float64 max_overhang = 0.4 * Ls;

   Float64 location_accuracy = pCriteria->GetHaulingDesignLocationAccuracy();

   Float64 bigInc = 4*location_accuracy;
   Float64 smallInc = location_accuracy;
   Float64 inc = bigInc;
   bool bLargeStepSize = true;

   LOG(_T("softMinHaulingDistance = ") << ::ConvertFromSysUnits(softMinHaulingDistance, unitMeasure::Feet)<<_T(" ft"));
   LOG(_T("hardMinHaulingDistance = ") << ::ConvertFromSysUnits(hardMinHaulingDistance, unitMeasure::Feet)<<_T(" ft"));

   Float64 loc = hardMinHaulingDistance;

   bool did_pass(false);
   while ( loc < max_overhang )
   {
      pgsKdotHaulingAnalysisArtifact temp_artifact;
      shipping_config.LeftOverhang  = loc;
      shipping_config.RightOverhang = loc;

      this->AnalyzeHauling(segmentKey, true, shipping_config, pPOId, &temp_artifact);

      bool passed = temp_artifact.Passed(pgsTypes::CrownSlope);

      if(passed)
      {
         *pArtifact = temp_artifact; // last artifact that passed
      }

      LOG(_T("Iteration; overhang = ")<<loc<<_T(" passed = ")<<passed);

      if (!did_pass)
      {
         if(passed)
         {
            did_pass = true; // passed in first iteration
         }
         else
         {
            // Failed at first iteration - we will never pass
            *pArtifact = temp_artifact; // save last so we can dump later
            break;
         }
      }
      else
      {
         // We have passed at least once
         if (!passed)
         {
            if (inc == smallInc)
            {
               // we have gone as far as we can go
               loc -= smallInc;
               break;
            }
            else
            {
               // We're still taking big steps. Revert to small steps and see if we can sneak up
               loc -= inc;
               inc = smallInc;
            }
         }
      }

      loc += inc;
   }

   if (!did_pass)
   {
      *bSuccess = false;
      LOG(_T("Design failed at first iteration - there is no hope"));
      pArtifact->SetDesignOutcome(pgsKdotHaulingAnalysisArtifact::doFailed);
   }
   else
   {
      if (loc < softMinHaulingDistance)
      {
         LOG(_T("Design succeeded, but overhang is within soft limits"));
         pArtifact->SetDesignOutcome(pgsKdotHaulingAnalysisArtifact::doSuccessInSoftZone);
      }
      else
      {
         LOG(_T("Design succeeded within limits"));
         pArtifact->SetDesignOutcome(pgsKdotHaulingAnalysisArtifact::doSuccessInSoftZone);
      }
   }

   shipping_config.LeftOverhang = loc;
   shipping_config.RightOverhang = loc;
   pArtifact->SetOverhangs(loc, loc);
   pArtifact->SetDesignOverhang(loc);

   LOG(_T("Exiting pgsKdotGirderHaulingChecker::DesignHauling. Overhang location is ")<<::ConvertFromSysUnits(loc, unitMeasure::Feet)<<_T(" ft"));

   return pArtifact.release();
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
bool pgsKdotGirderHaulingChecker::AssertValid() const
{
   return true;
}

void pgsKdotGirderHaulingChecker::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsKdotGirderHaulingChecker" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsKdotGirderHaulingChecker::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsKdotGirderHaulingChecker");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsKdotGirderHaulingChecker");

   TESTME_EPILOG("GirderHandlingChecker");
}
#endif // _UNITTEST


////////////////////////////////////////////////////////
// hauling
////////////////////////////////////////////////////////
void pgsKdotGirderHaulingChecker::PrepareHaulingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 Loh,Float64 Roh,Float64 Fc,Float64 Ec,pgsTypes::ConcreteType concType,pgsKdotHaulingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IBridge, pBridge);
   Float64 girder_length = pBridge->GetSegmentLength(segmentKey);
   pArtifact->SetGirderLength(girder_length);

   GET_IFACE(ISectionProperties,pSectProp);
   Float64 volume = pSectProp->GetSegmentVolume(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 density = pMaterial->GetSegmentWeightDensity(segmentKey,haulSegmentIntervalIdx);
   Float64 total_weight = volume * density * unitSysUnitsMgr::GetGravitationalAcceleration();
   pArtifact->SetGirderWeight(total_weight);

   Float64 span_len = girder_length - Loh - Roh;

   if (span_len <= 0.0)
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);

      LPCTSTR msg = _T("Hauling support overhang cannot exceed one-half of the span length");
      pgsBunkPointLocationStatusItem* pStatusItem = new pgsBunkPointLocationStatusItem(segmentKey,pgsTypes::metStart,m_StatusGroupID,m_scidBunkPointLocation,msg);
      pStatusCenter->Add(pStatusItem);

      std::_tstring str(msg);
      str += std::_tstring(_T("\nSee Status Center for details"));
      THROW_UNWIND(str.c_str(),-1);
   }

   GET_IFACE(IKdotGirderHaulingSpecCriteria,pSegmentHaulingSpecCriteria);

   Float64 gOverhang, gInterior;
   pSegmentHaulingSpecCriteria->GetHaulingGFactors(&gOverhang, &gInterior);

   pArtifact->SetGFactors(gOverhang, gInterior);

   Float64 min_bunk_point_loc, length_factor;
   bool use_length_factor;
   pSegmentHaulingSpecCriteria->GetMinimumHaulingSupportLocation(&min_bunk_point_loc, &use_length_factor, &length_factor);

   // Limits of overhangs
   Float64 soft_limit; // hard limit is min_bunk_point_loc
   if (use_length_factor)
   {
      soft_limit = girder_length * length_factor;
      soft_limit = Max(soft_limit, min_bunk_point_loc);
   }
   else
   {
      soft_limit = min_bunk_point_loc;
   }

   pArtifact->SetOverhangLimits(min_bunk_point_loc, soft_limit);
   pArtifact->SetClearSpanBetweenSupportLocations(span_len);
   pArtifact->SetOverhangs(Loh,Roh);

   if ( Loh < min_bunk_point_loc )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format(_T("%s: Left bunk point is less than the absolute minimum value of %s"),SEGMENT_LABEL(segmentKey),::FormatDimension(min_bunk_point_loc,pDisplayUnits->GetSpanLengthUnit()));
      pgsBunkPointLocationStatusItem* pStatusItem = new pgsBunkPointLocationStatusItem(segmentKey,pgsTypes::metStart,m_StatusGroupID,m_scidBunkPointLocation,strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   if ( Roh < min_bunk_point_loc )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format(_T("%s: Right bunk point is less than the absolute minimum value of %s"),SEGMENT_LABEL(segmentKey),::FormatDimension(min_bunk_point_loc,pDisplayUnits->GetSpanLengthUnit()));
      pgsBunkPointLocationStatusItem* pStatusItem = new pgsBunkPointLocationStatusItem(segmentKey,pgsTypes::metEnd,m_StatusGroupID,m_scidBunkPointLocation,strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   pArtifact->SetElasticModulusOfGirderConcrete(Ec);
}

void pgsKdotGirderHaulingChecker::ComputeHaulingStresses(const CSegmentKey& segmentKey,bool bUseConfig,
                                                      const HANDLINGCONFIG& haulConfig,
                                                      const PoiList& vPoi,
                                                      const std::vector<Float64>& vMoment,
                                                      pgsKdotHaulingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IBridge,                        pBridge);
   GET_IFACE(IPretensionForce,               pPrestressForce);
   GET_IFACE(IMaterials,                     pMaterials);
   GET_IFACE(IStrandGeometry,                pStrandGeometry);
   GET_IFACE(ISectionProperties,             pSectProps);
   GET_IFACE(IShapes,                        pShapes);
   GET_IFACE(IGirder,                        pGdr);
   GET_IFACE(IKdotGirderHaulingSpecCriteria, pHaulingSpecCriteria);
   GET_IFACE(ILongRebarGeometry,             pRebarGeom);
   GET_IFACE(IPointOfInterest,               pPoi);
   GET_IFACE(IIntervals,                     pIntervals);

   IntervalIndexType releaseIntervalIdx     = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   // Get allowable tension for with and without rebar cases
   Float64 fLowTensAllowable, fHighTensAllowable , fCompAllowable;
   if (!bUseConfig)
   {
      fLowTensAllowable  = pHaulingSpecCriteria->GetKdotHaulingAllowableTensileConcreteStress(segmentKey);
      fHighTensAllowable = pHaulingSpecCriteria->GetKdotHaulingWithMildRebarAllowableStress(segmentKey);
      fCompAllowable     = pHaulingSpecCriteria->GetKdotHaulingAllowableCompressiveConcreteStress(segmentKey);
   }
   else
   {
      Float64 fc = haulConfig.GdrConfig.Fc;

      fLowTensAllowable  = pHaulingSpecCriteria->GetKdotHaulingAllowableTensileConcreteStressEx(segmentKey, fc, false);
      fHighTensAllowable = pHaulingSpecCriteria->GetKdotHaulingAllowableTensileConcreteStressEx(segmentKey, fc, true);
      fCompAllowable     = pHaulingSpecCriteria->GetKdotHaulingAllowableCompressiveConcreteStressEx(segmentKey, fc);
   }

   // Parameters for computing required concrete strengths
   Float64 rcsC = pHaulingSpecCriteria->GetKdotHaulingAllowableCompressionFactor();
   Float64 rcsT;
   bool    rcsBfmax;
   Float64 rcsFmax;
   pHaulingSpecCriteria->GetKdotHaulingAllowableTensileConcreteStressParameters(&rcsT,&rcsBfmax,&rcsFmax);
   Float64 rcsTalt = pHaulingSpecCriteria->GetKdotHaulingWithMildRebarAllowableStressFactor();

   bool bSISpec = lrfdVersionMgr::GetVersion() == lrfdVersionMgr::SI ? true : false;
   // Use calculator object to deal with casting yard higher allowable stress
   Float64 fsMax = (bSISpec ? ::ConvertToSysUnits(206.0,unitMeasure::MPa) : ::ConvertToSysUnits(30.0,unitMeasure::KSI) );
   pgsAlternativeTensileStressCalculator altCalc(segmentKey, haulSegmentIntervalIdx, pBridge, pGdr, pShapes, pSectProps, pRebarGeom, pMaterials, pPoi, true/*limit bar stress*/, fsMax, true/*girder stresses*/);

   Float64 AsMax = 0;

   const GDRCONFIG* pConfig = (bUseConfig && !haulConfig.bIgnoreGirderConfig) ? &(haulConfig.GdrConfig) : nullptr;

   ATLASSERT(vPoi.size() == vMoment.size());
   auto poiIter(vPoi.begin());
   auto poiIterEnd(vPoi.end());
   auto momentIter(vMoment.begin());
   for ( ; poiIter != poiIterEnd; poiIter++, momentIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);

      Float64 Cat, Cbt;
      pSectProps->GetStressCoefficients(haulSegmentIntervalIdx, poi, pgsTypes::TopGirder, pConfig, &Cat, &Cbt);

      Float64 Cab, Cbb;
      pSectProps->GetStressCoefficients(haulSegmentIntervalIdx, poi, pgsTypes::BottomGirder, pConfig, &Cab, &Cbb);

      pgsKdotHaulingStressAnalysisArtifact haul_artifact;

      // calc total prestress force and eccentricity
      Float64 nfs, nfh, nft;
      Float64 hps_force, he;
      Float64 sps_force, se;
      Float64 tps_force, te;

      if ( pConfig )
      {
         hps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Harped,haulSegmentIntervalIdx,pgsTypes::Middle, pConfig);
         he = pStrandGeometry->GetEccentricity(releaseIntervalIdx,poi,pgsTypes::Harped,pConfig, &nfh);
         sps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Straight,haulSegmentIntervalIdx,pgsTypes::Middle,pConfig);
         se = pStrandGeometry->GetEccentricity(releaseIntervalIdx,poi,pgsTypes::Straight,pConfig,&nfs);
         tps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,haulSegmentIntervalIdx,pgsTypes::Middle,pConfig);
         te = pStrandGeometry->GetEccentricity(releaseIntervalIdx,poi,pgsTypes::Temporary,pConfig,&nft);
      }
      else
      {
         hps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Harped,haulSegmentIntervalIdx,pgsTypes::Middle);
         he = pStrandGeometry->GetEccentricity(releaseIntervalIdx,poi,pgsTypes::Harped,&nfh);
         sps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Straight,haulSegmentIntervalIdx,pgsTypes::Middle);
         se = pStrandGeometry->GetEccentricity(releaseIntervalIdx,poi,pgsTypes::Straight,&nfs);
         tps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,haulSegmentIntervalIdx,pgsTypes::Middle);
         te = pStrandGeometry->GetEccentricity(releaseIntervalIdx,poi,pgsTypes::Temporary,&nft);
      }

      Float64 total_ps_force = hps_force + sps_force + tps_force;
      Float64 total_e=0.0;
      if (0 < total_ps_force)
      {
         total_e = (hps_force*he + sps_force*se + tps_force*te) / total_ps_force;
      }
      else if (0 < nfh + nfs + nft)
      {
         Float64 Aps[3];
         Aps[pgsTypes::Straight] = nfs*pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Straight)->GetNominalArea();
         Aps[pgsTypes::Harped]   = nfh*pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Harped)->GetNominalArea();
         Aps[pgsTypes::Temporary]= nft*pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Temporary)->GetNominalArea();

         Float64 aps = Aps[pgsTypes::Straight] + Aps[pgsTypes::Harped] + Aps[pgsTypes::Temporary];

         total_e = (he*Aps[pgsTypes::Harped] + se*Aps[pgsTypes::Straight] + te*Aps[pgsTypes::Temporary]) / aps;
      }

      haul_artifact.SetEffectiveHorizPsForce(total_ps_force);
      haul_artifact.SetEccentricityPsForce(total_e);

      // moments
      Float64 vert_mom = *momentIter;

      haul_artifact.SetMoment(vert_mom);

      Float64 mom_ps = -total_ps_force * total_e;

      // compute stresses
      // In accordance with the PCI Journal article that these
      // calculations are based on, don't included impact when checking
      // stresses for the lateral moment effects. That is, check
      // "impact and no tilt" and "no impact with tilt".

      Float64 ft_ps  = -Cat*total_ps_force + Cbt*mom_ps;
      Float64 ft_mo  = Cbt*vert_mom        + ft_ps;

      Float64 fb_ps = -Cab*total_ps_force + Cbb*mom_ps;
      Float64 fb_mo = Cbb*vert_mom        + fb_ps;

      haul_artifact.SetTopFiberStress(ft_ps,ft_mo);
      haul_artifact.SetBottomFiberStress(fb_ps,fb_mo);

      // Now look at tensile capacity side of equations
      Float64 Yna, At, T, AsReqd, AsProvd;
      bool isAdequateBar;
 
      Float64 fAllow = altCalc.ComputeAlternativeStressRequirements(poi, pConfig, ft_mo, fb_mo, fLowTensAllowable, fHighTensAllowable,
                                                                      &Yna, &At, &T, &AsProvd, &AsReqd, &isAdequateBar);

      haul_artifact.SetAlternativeTensileStressParameters(Yna, At, T, AsProvd, AsReqd, fAllow);

      haul_artifact.SetCompressiveCapacity(fCompAllowable);

      // Compute required concrete strengths
      // Compression
      Float64 min_stress = haul_artifact.GetMaximumConcreteCompressiveStress();

      Float64 fc_compression = 0.0;
      if ( min_stress < 0 )
      {
         fc_compression = min_stress/rcsC;
      }

      // Tension is more challenging. Use inline function to compute
      Float64 max_stress = haul_artifact.GetMaximumConcreteTensileStress();

      Float64 fc_tens_norebar, fc_tens_withrebar;
      pgsAlternativeTensileStressCalculator::ComputeReqdFcTens(segmentKey,max_stress, rcsT, rcsBfmax, rcsFmax, rcsTalt, &fc_tens_norebar, &fc_tens_withrebar);

      haul_artifact.SetRequiredConcreteStrength(fc_compression,fc_tens_norebar,fc_tens_withrebar);

      pArtifact->AddHaulingStressAnalysisArtifact(poi,haul_artifact);
   }
}

void pgsKdotGirderHaulingChecker::ComputeHaulingMoments(const CSegmentKey& segmentKey,
                                                     const pgsKdotHaulingAnalysisArtifact& rArtifact, 
                                                     const PoiList& vPoi,
                                                     std::vector<Float64>* pvMoment, Float64* pMidSpanDeflection)
{
   // build a model
   Float64 glen = rArtifact.GetGirderLength();
   Float64 leftOH = rArtifact.GetTrailingOverhang();
   Float64 rightOH = rArtifact.GetLeadingOverhang();
   Float64 E = rArtifact.GetElasticModulusOfGirderConcrete();

   Float64 gOverhang, gInterior;
   rArtifact.GetGFactors(&gOverhang, &gInterior);

   pgsKdotHaulingGirderModelFactory girderModelFactory(gOverhang, gInterior);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   pgsGirderHandlingChecker::ComputeMoments(m_pBroker, &girderModelFactory, segmentKey,
                                            haulSegmentIntervalIdx,
                                            leftOH, glen, rightOH,
                                            E,
                                            POI_HAUL_SEGMENT,
                                            vPoi,
                                            pvMoment, pMidSpanDeflection);
}
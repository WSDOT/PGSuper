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

#include "WsdotGirderHaulingChecker.h"
#include "StatusItems.h"

#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
   m_scidBunkPointLocation             = pStatusCenter->RegisterCallback( new pgsBunkPointLocationStatusCallback(m_pBroker) );
   m_scidTruckStiffness                = pStatusCenter->RegisterCallback( new pgsTruckStiffnessStatusCallback(m_pBroker) );
}

pgsWsdotGirderHaulingChecker::~pgsWsdotGirderHaulingChecker()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

pgsHaulingAnalysisArtifact* pgsWsdotGirderHaulingChecker::CheckHauling(const CSegmentKey& segmentKey, SHARED_LOGFILE LOGFILE)
{
   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);

   if (pGirderHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      // AnalyzeHauling should be generic. It will analyze the configuration
      // passed to it, not the current configuration of the girder
      return AnalyzeHauling(segmentKey);
   }
   else
   {
      return NULL;
   }
}

pgsHaulingAnalysisArtifact*  pgsWsdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey)
{
   GET_IFACE(IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest); // poi's from global pool

   std::auto_ptr<pgsWsdotHaulingAnalysisArtifact> pArtifact(new pgsWsdotHaulingAnalysisArtifact());

   HANDLINGCONFIG dummy_config;
   AnalyzeHauling(segmentKey,false,dummy_config,pGirderHaulingPointsOfInterest,pArtifact.get());

   return pArtifact.release();
}

pgsHaulingAnalysisArtifact* pgsWsdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,Float64 leftOverhang,Float64 rightOverhang)
{
   std::auto_ptr<pgsWsdotHaulingAnalysisArtifact> pArtifact(new pgsWsdotHaulingAnalysisArtifact());
   GET_IFACE(IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest); // poi's from global pool
   HANDLINGCONFIG dummy_config;
   dummy_config.bIgnoreGirderConfig = true;
   dummy_config.LeftOverhang = leftOverhang;
   dummy_config.RightOverhang = rightOverhang;
   AnalyzeHauling(segmentKey,true,dummy_config,pGirderHaulingPointsOfInterest,pArtifact.get());
   return pArtifact.release();
}

pgsHaulingAnalysisArtifact* pgsWsdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,const HANDLINGCONFIG& haulConfig,IGirderHaulingDesignPointsOfInterest* pPOId)
{
   std::auto_ptr<pgsWsdotHaulingAnalysisArtifact> pArtifact(new pgsWsdotHaulingAnalysisArtifact());
   AnalyzeHauling(segmentKey,true,haulConfig,pPOId,pArtifact.get());

   return pArtifact.release();
}

void pgsWsdotGirderHaulingChecker::AnalyzeHauling(const CSegmentKey& segmentKey,bool bUseConfig,const HANDLINGCONFIG& haulConfig,IGirderHaulingDesignPointsOfInterest* pPOId,pgsWsdotHaulingAnalysisArtifact* pArtifact)
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

      poi_vec = pPOId->GetHaulingDesignPointsOfInterest(segmentKey,10,Loh,Roh);
   }
   else
   {
      GET_IFACE(IGirderHauling,pGirderHauling);
      Loh = pGirderHauling->GetTrailingOverhang(segmentKey);
      Roh = pGirderHauling->GetLeadingOverhang(segmentKey);

      GET_IFACE(IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);
      poi_vec = pGirderHaulingPointsOfInterest->GetHaulingPointsOfInterest(segmentKey,0,POIFIND_OR);
   }

   if ( !bUseConfig || haulConfig.bIgnoreGirderConfig )
   {
      // Not using the configuration, or the configuration applies only for the overhang

      Fc = pMaterial->GetSegmentFc(segmentKey,haulSegmentIntervalIdx);
      Ec = pMaterial->GetSegmentEc(segmentKey,haulSegmentIntervalIdx);
      concType = pMaterial->GetSegmentConcreteType(segmentKey);
   }
   else
   {
      // Using the config
      ATLASSERT(haulConfig.bIgnoreGirderConfig == false);

      if ( haulConfig.GdrConfig.bUserEc )
         Ec = haulConfig.GdrConfig.Ec;
      else
         Ec = pMaterial->GetEconc(haulConfig.GdrConfig.Fc, pMaterial->GetSegmentStrengthDensity(segmentKey),
                                                           pMaterial->GetSegmentEccK1(segmentKey),
                                                           pMaterial->GetSegmentEccK2(segmentKey));

      Fc = haulConfig.GdrConfig.Fc;
      concType = haulConfig.GdrConfig.ConcType;
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

pgsHaulingAnalysisArtifact* pgsWsdotGirderHaulingChecker::DesignHauling(const CSegmentKey& segmentKey,const GDRCONFIG& config,bool bDesignForEqualOverhangs,bool bIgnoreConfigurationLimits,IGirderHaulingDesignPointsOfInterest* pPOId, bool* bSuccess, SHARED_LOGFILE LOGFILE)
{
   LOG(_T("Entering pgsWsdotGirderHaulingChecker::DesignHauling"));
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

   Float64 min_location      = Max(min_overhang_start,min_overhang_end);
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
   std::auto_ptr<pgsWsdotHaulingAnalysisArtifact> artifact(new pgsWsdotHaulingAnalysisArtifact);

   HANDLINGCONFIG shipping_config;
   shipping_config.bIgnoreGirderConfig = false;
   shipping_config.GdrConfig = config;

   while ( loc < maxOverhang )
   {
      LOG(_T(""));

      //
      pgsWsdotGirderHaulingChecker checker(m_pBroker,m_StatusGroupID);

      pgsWsdotHaulingAnalysisArtifact curr_artifact;

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

      checker.AnalyzeHauling(segmentKey,true,shipping_config,pPOId,&curr_artifact);
      FScr = curr_artifact.GetMinFsForCracking();

      LOG(_T("FScr = ") << FScr);
      if ( FScr < FScrMin && maxOverhang/4 < loc )
      {
         LOG(_T("Could not satisfy FScr... Need to add temporary strands"));
         // Moving the supports closer isn't going to help
         *bSuccess = false; // design failed
         *artifact = curr_artifact;
         return artifact.release();
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
         *artifact = curr_artifact;
         break;
      }

      loc += inc;
   }

   LOG(_T("Exiting pgsWsdotGirderHaulingChecker::DesignHauling - success"));
   *bSuccess = true;
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
//#pragma Reminder("TODO: Implement the AssertValid method for pgsWsdotGirderHaulingChecker")
   return true;
}

void pgsWsdotGirderHaulingChecker::Dump(dbgDumpContext& os) const
{
//#pragma Reminder("TODO: Implement the Dump method for pgsWsdotGirderHaulingChecker")
   os << "Dump for pgsWsdotGirderHaulingChecker" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsWsdotGirderHaulingChecker::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsWsdotGirderHaulingChecker");

//#pragma Reminder("TODO: Implement the TestMe method for pgsWsdotGirderHaulingChecker")
   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsWsdotGirderHaulingChecker");

   TESTME_EPILOG("GirderHandlingChecker");
}
#endif // _UNITTEST


////////////////////////////////////////////////////////
// hauling
////////////////////////////////////////////////////////
void pgsWsdotGirderHaulingChecker::PrepareHaulingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 Loh,Float64 Roh,Float64 Fc,Float64 Ec,pgsTypes::ConcreteType concType,pgsWsdotHaulingAnalysisArtifact* pArtifact)
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

   if (span_len<=0.0)
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);

      LPCTSTR msg = _T("Hauling support overhang cannot exceed one-half of the span length");
      pgsBunkPointLocationStatusItem* pStatusItem = new pgsBunkPointLocationStatusItem(segmentKey,m_StatusGroupID,m_scidBunkPointLocation,msg);
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
   pArtifact->SetModRuptureCoefficient( pGirderHaulingSpecCriteria->GetHaulingModulusOfRuptureFactor(concType) );

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

void pgsWsdotGirderHaulingChecker::ComputeHaulingStresses(const CSegmentKey& segmentKey,bool bUseConfig,
                                                      const HANDLINGCONFIG& haulConfig,
                                                      const std::vector<pgsPointOfInterest>& rpoiVec,
                                                      const std::vector<Float64>& momVec,
                                                      pgsWsdotHaulingAnalysisArtifact* pArtifact)
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

   bool bSISpec = lrfdVersionMgr::GetVersion() == lrfdVersionMgr::SI ? true : false;
   // Use calculator object to deal with casting yard higher allowable stress
   pgsAlternativeTensileStressCalculator altCalc(segmentKey, haulSegmentIntervalIdx, pGdr, pShapes, pSectProps, pRebarGeom, pMaterials, true/*limit bar stress*/, bSISpec);

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
      if ( bUseConfig && !haulConfig.bIgnoreGirderConfig )
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

      pgsWsdotHaulingStressAnalysisArtifact haul_artifact;

      // calc total prestress force and eccentricity
      Float64 nfs, nfh, nft;
      Float64 hps_force, he;
      Float64 sps_force, se;
      Float64 tps_force, te;

      if ( bUseConfig && !haulConfig.bIgnoreGirderConfig )
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

      const GDRCONFIG* pConfig = (bUseConfig && !haulConfig.bIgnoreGirderConfig ) ? &(haulConfig.GdrConfig) : NULL;

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
      min_stress = Min(min_stress, min_stress_inc);

      Float64 fc_compression = 0.0;
      if ( min_stress < 0 )
      {
         fc_compression = min_stress/rcsC;
      }

      // Tension is more challenging. Use inline function to compute
      Float64 max_stress = haul_artifact.GetMaximumConcreteTensileStress();

      // Impacted plumb girder stress is treated differently than inclined girder
      Float64 fc_tens_norebar, fc_tens_withrebar;
      pgsAlternativeTensileStressCalculator::ComputeReqdFcTens(max_stress, rcsT, rcsBfmax, rcsFmax, rcsTalt, &fc_tens_norebar, &fc_tens_withrebar);

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
         fc_tens_norebar = Max(fc_tens_norebar, fc_tension_inclined);
      }

      if (fc_tens_withrebar > 0.0)
      {
         fc_tens_withrebar = Max(fc_tens_withrebar, fc_tension_inclined);
      }

      haul_artifact.SetRequiredConcreteStrength(fc_compression,fc_tens_norebar,fc_tens_withrebar);

      pArtifact->AddHaulingStressAnalysisArtifact(poi.GetDistFromStart(),haul_artifact);
   }
}

void pgsWsdotGirderHaulingChecker::ComputeHaulingFsForCracking(const CSegmentKey& segmentKey,
                                                           const std::vector<pgsPointOfInterest>& rpoiVec,
                                                           const std::vector<Float64>& momVec,
                                                           pgsWsdotHaulingAnalysisArtifact* pArtifact)
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
      const pgsPointOfInterest& poi = rpoiVec[i];
      Float64 bt_bot = pGdr->GetBottomWidth(poi);
      Float64 bt_top = pGdr->GetTopWidth(poi);

      Float64 iy = pSectProp->GetIy(haulSegmentIntervalIdx,poi);

      Float64 mom_vert = momVec[i];

      pgsWsdotHaulingCrackingAnalysisArtifact crack_artifact;
      crack_artifact.SetVerticalMoment(mom_vert);

      // determine lateral moment that will cause cracking in section
      const pgsWsdotHaulingStressAnalysisArtifact* pstr = pArtifact->GetHaulingStressAnalysisArtifact(poi.GetDistFromStart());
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

void pgsWsdotGirderHaulingChecker::ComputeHaulingFsForRollover(const CSegmentKey& segmentKey,
                                                           pgsWsdotHaulingAnalysisArtifact* pArtifact)
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

void pgsWsdotGirderHaulingChecker::ComputeHaulingMoments(const CSegmentKey& segmentKey,
                                                     const pgsWsdotHaulingAnalysisArtifact& rArtifact, 
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

   pgsGirderModelFactory factory;

   pgsGirderHandlingChecker::ComputeMoments(m_pBroker, &factory, segmentKey,
                                            haulSegmentIntervalIdx,
                                            leftOH, glen, rightOH,
                                            E,
                                            POI_HAUL_SEGMENT,
                                            rpoiVec,
                                            pmomVec, pMidSpanDeflection);
}

void pgsWsdotGirderHaulingChecker::ComputeHaulingRollAngle(const CSegmentKey& segmentKey,
                                                       pgsWsdotHaulingAnalysisArtifact* pArtifact, 
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

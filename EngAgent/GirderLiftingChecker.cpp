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

#include <IFace\GirderHandlingPointOfInterest.h>
#include <IFace\Bridge.h>
#include <IFace\StatusCenter.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\PrestressForce.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>

#include "..\PGSuperException.h"

#include <limits>
#include <algorithm>

#include <PgsExt\StatusItem.h>
#include <PgsExt\GirderModelFactory.H>

#include "DesignCodes.h"

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

void pgsGirderLiftingChecker::CheckLifting(SpanIndexType span,GirderIndexType gdr,pgsLiftingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);

   if (pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
       // Use poi's from global pool
      GET_IFACE(IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
      HANDLINGCONFIG dummy_config;

      // Compute lifting response
      AnalyzeLifting(span,gdr,false,dummy_config,pGirderLiftingPointsOfInterest,pArtifact);
   }
}

void pgsGirderLiftingChecker::AnalyzeLifting(SpanIndexType span,GirderIndexType gdr,const HANDLINGCONFIG& liftConfig,IGirderLiftingDesignPointsOfInterest* pPoiD, pgsLiftingAnalysisArtifact* pArtifact)
{
   AnalyzeLifting(span,gdr,true,liftConfig,pPoiD,pArtifact);
}

void pgsGirderLiftingChecker::AnalyzeLifting(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,const HANDLINGCONFIG& liftConfig,IGirderLiftingDesignPointsOfInterest* pPoiD,pgsLiftingAnalysisArtifact* pArtifact)
{
   // calc some initial information
   GET_IFACE(IBridgeMaterialEx,pMaterial);

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
         Eci = pMaterial->GetEconc(liftConfig.GdrConfig.Fci, pMaterial->GetStrDensityGdr(span,gdr),pMaterial->GetEccK1Gdr(span,gdr),pMaterial->GetEccK2Gdr(span,gdr));

      Fci = liftConfig.GdrConfig.Fci;
      concType = liftConfig.GdrConfig.ConcType;

      poi_vec = pPoiD->GetLiftingDesignPointsOfInterest(span,gdr,Loh,POI_FLEXURESTRESS);
   }
   else
   {
      GET_IFACE(IGirderLifting,pGirderLifting);
      Loh = pGirderLifting->GetLeftLiftingLoopLocation(span,gdr);
      Roh = pGirderLifting->GetRightLiftingLoopLocation(span,gdr);

      GET_IFACE(IBridgeMaterialEx,pMaterial);
      Eci = pMaterial->GetEciGdr(span,gdr);
      Fci = pMaterial->GetFciGdr(span,gdr);
      concType = pMaterial->GetGdrConcreteType(span,gdr);

      GET_IFACE(IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
      poi_vec = pGirderLiftingPointsOfInterest->GetLiftingPointsOfInterest(span,gdr,POI_FLEXURESTRESS);
   }
   
   PrepareLiftingAnalysisArtifact(span,gdr,Loh,Roh,Fci,Eci,concType,pArtifact);

   pArtifact->SetLiftingPointsOfInterest(poi_vec);

   // get moments at pois  and mid-span deflection due to dead vertical lifting
   std::vector<Float64> moment_vec;
   Float64 mid_span_deflection;
   ComputeLiftingMoments(span, gdr, *pArtifact, poi_vec, &moment_vec,&mid_span_deflection);

   ComputeLiftingStresses(span, gdr, bUseConfig, liftConfig, poi_vec, moment_vec, pArtifact);

   if (ComputeLiftingFsAgainstCracking(span, gdr, bUseConfig, liftConfig, poi_vec, moment_vec, mid_span_deflection, pArtifact))
   {
      ComputeLiftingFsAgainstFailure(span, gdr, pArtifact);
   }
   else
   {
      pArtifact->SetThetaFailureMax(0.0);
      pArtifact->SetFsFailure(0.0);
   }
}

pgsDesignCodes::OutcomeType pgsGirderLiftingChecker::DesignLifting(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,IGirderLiftingDesignPointsOfInterest* pPoiD,pgsLiftingAnalysisArtifact* pArtifact,SHARED_LOGFILE LOGFILE)
{
   //
   // Range of lifting loop locations and step increment
   //
   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   Float64 min_location = max(pGirderLiftingSpecCriteria->GetMinimumLiftingPointLocation(span,gdr,pgsTypes::metStart),pGirderLiftingSpecCriteria->GetMinimumLiftingPointLocation(span,gdr,pgsTypes::metEnd));
   Float64 location_accuracy = pGirderLiftingSpecCriteria->GetLiftingPointLocationAccuracy();

   Float64 bigInc = 8*location_accuracy;
   Float64 smallInc = location_accuracy;
   Float64 locInc = bigInc;
   bool bLargeStepSize = true;

   // Max location may be limited by harping point (actually, just before it stopping at the last increment value)
   // But allowing more than 40% of the girder length makes no sense (think rigid-body instability for riggers)
   GET_IFACE(IBridge, pBridge);
   Float64 girder_length = pBridge->GetGirderLength(span,gdr);

   Float64 maxLoc = 0.4*girder_length;

   if (config.PrestressConfig.GetNStrands(pgsTypes::Harped) > 0) // only look at harping point if we have harped strands
   {
      Float64 lhp,rhp;
      GET_IFACE(IStrandGeometry,pStrandGeom);
      pStrandGeom->GetHarpingPointLocations(span,gdr,&lhp,&rhp);

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

      AnalyzeLifting(span,gdr,lift_config,pPoiD,&curr_artifact);
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
void pgsGirderLiftingChecker::PrepareLiftingAnalysisArtifact(SpanIndexType span,GirderIndexType gdr,Float64 Loh,Float64 Roh,Float64 Fci,Float64 Eci,pgsTypes::ConcreteType concType,pgsLiftingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IBridge, pBridge);
   Float64 girder_length = pBridge->GetGirderLength(span,gdr);
   pArtifact->SetGirderLength(girder_length);

   GET_IFACE(ISectProp2,pSectProp2);
   Float64 volume = pSectProp2->GetVolume(span,gdr);

   GET_IFACE(IBridgeMaterial,pMaterial);
   Float64 density = pMaterial->GetWgtDensityGdr(span,gdr);
   Float64 total_weight = volume * density * unitSysUnitsMgr::GetGravitationalAcceleration();
   pArtifact->SetGirderWeight(total_weight);

   GET_IFACE(IPrestressForce,pPrestressForce);
   Float64 XFerLength =  pPrestressForce->GetXferLength(span,gdr,pgsTypes::Permanent);
   pArtifact->SetXFerLength(XFerLength);

   Float64 span_len = girder_length - Loh - Roh;

   if (span_len <= 0.0)
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);

      LPCTSTR msg = _T("Lifting support overhang cannot exceed one-half of the span length");
      pgsLiftingSupportLocationStatusItem* pStatusItem = new pgsLiftingSupportLocationStatusItem(span,gdr,m_StatusGroupID,m_scidLiftingSupportLocationError,msg);
      pStatusCenter->Add(pStatusItem);

      std::_tstring str(msg);
      str += std::_tstring(_T("\nSee Status Center for details"));
      THROW_UNWIND(str.c_str(),-1);
   }

   GET_IFACE(IGirderLiftingSpecCriteriaEx,pGirderLiftingSpecCriteria);
   pArtifact->SetAllowableFsForCracking(pGirderLiftingSpecCriteria->GetLiftingCrackingFs());
   pArtifact->SetAllowableFsForFailure(pGirderLiftingSpecCriteria->GetLiftingFailureFs());

   Float64 min_lift_point_start = pGirderLiftingSpecCriteria->GetMinimumLiftingPointLocation(span,gdr,pgsTypes::metStart);
   Float64 min_lift_point_end   = pGirderLiftingSpecCriteria->GetMinimumLiftingPointLocation(span,gdr,pgsTypes::metEnd);
   if ( Loh < min_lift_point_start )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format(_T("Left lift point is less than the minimum value of %s"),::FormatDimension(min_lift_point_start,pDisplayUnits->GetSpanLengthUnit()));
      pgsLiftingSupportLocationStatusItem* pStatusItem = new pgsLiftingSupportLocationStatusItem(span,gdr,m_StatusGroupID,m_scidLiftingSupportLocationWarning,strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   if ( Roh < min_lift_point_end )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format(_T("Right lift point is less than the minimum value of %s"),::FormatDimension(min_lift_point_end,pDisplayUnits->GetSpanLengthUnit()));
      pgsLiftingSupportLocationStatusItem* pStatusItem = new pgsLiftingSupportLocationStatusItem(span,gdr,m_StatusGroupID,m_scidLiftingSupportLocationWarning,strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   pArtifact->SetClearSpanBetweenPickPoints(span_len);
   pArtifact->SetOverhangs(Loh,Roh);

   Float64 bracket_hgt = pGirderLiftingSpecCriteria->GetHeightOfPickPointAboveGirderTop();
   Float64 ycgt = pSectProp2->GetYtGirder(pgsTypes::CastingYard,pgsPointOfInterest(span,gdr,0.00));
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

void pgsGirderLiftingChecker::ComputeLiftingMoments(SpanIndexType span,GirderIndexType gdr,
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

   pgsGirderModelFactory girderModelFactory;

   pgsGirderHandlingChecker::ComputeMoments(m_pBroker, &girderModelFactory, span, gdr,
                                            pgsTypes::Lifting,
                                            leftOH, glen, rightOH,
                                            E,
                                            rpoiVec,
                                            pmomVec, pMidSpanDeflection);
}

void pgsGirderLiftingChecker::ComputeLiftingStresses(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,
                                                      const HANDLINGCONFIG& liftConfig,
                                                      const std::vector<pgsPointOfInterest>& rpoiVec,
                                                      const std::vector<Float64>& momVec,
                                                      pgsLiftingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IPrestressForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeometry);
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(ISectProp2,pSectProp2);
   GET_IFACE(IBridgeMaterial,pMaterial);
   GET_IFACE(IBridgeMaterialEx,pMaterialEx);
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
   Float64 fLowTensAllowable  = pGirderLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStress(span,gdr);
   Float64 fHighTensAllowable = pGirderLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStress(span,gdr);
   Float64 fCompAllowable     = pGirderLiftingSpecCriteria->GetLiftingAllowableCompressiveConcreteStress(span,gdr);
   if (!bUseConfig)
   {
      fLowTensAllowable  = pGirderLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStress(span,gdr);
      fHighTensAllowable = pGirderLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStress(span,gdr);
      fCompAllowable     = pGirderLiftingSpecCriteria->GetLiftingAllowableCompressiveConcreteStress(span,gdr);
   }
   else
   {
      Float64 fci = liftConfig.GdrConfig.Fci;
      fLowTensAllowable  = pGirderLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStressEx(fci, false);
      fHighTensAllowable = pGirderLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStressEx(fci, true);
      fCompAllowable     = pGirderLiftingSpecCriteria->GetLiftingAllowableCompressiveConcreteStressEx(fci);
   }

   // Parameters for computing required concrete strengths
   Float64 rcsC = pGirderLiftingSpecCriteria->GetLiftingAllowableCompressionFactor();
   Float64 rcsT;
   bool    rcsBfmax;
   Float64 rcsFmax;
   pGirderLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStressParameters(&rcsT,&rcsBfmax,&rcsFmax);
   Float64 rcsTalt = pGirderLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStressFactor();

   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   // Use calculator object to deal with casting yard higher allowable stress
   pgsAlternativeTensileStressCalculator altCalc(span,gdr, pGirder, pSectProp2, pRebarGeom, pMaterial, pMaterialEx, bUnitsSI);

   Float64 AsMax = 0;
   Float64 As = 0;
   for(IndexType i=0; i<psiz; i++)
   {
      const pgsPointOfInterest& poi = rpoiVec[i];

      Float64 ag,stg,sbg;
      if ( bUseConfig )
      {
         ag  = pSectProp2->GetAg(pgsTypes::CastingYard,poi,liftConfig.GdrConfig.Fci);
         stg = pSectProp2->GetSt(pgsTypes::CastingYard,poi,liftConfig.GdrConfig.Fci);
         sbg = pSectProp2->GetSb(pgsTypes::CastingYard,poi,liftConfig.GdrConfig.Fci);
      }
      else
      {
         ag  = pSectProp2->GetAg(pgsTypes::CastingYard,poi);
         stg = pSectProp2->GetSt(pgsTypes::CastingYard,poi);
         sbg = pSectProp2->GetSb(pgsTypes::CastingYard,poi);
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
         hps_force = pPrestressForce->GetPrestressForce(poi,liftConfig.GdrConfig,pgsTypes::Harped,pgsTypes::AtLifting);
         he        = pStrandGeometry->GetHsEccentricity(poi,liftConfig.GdrConfig.PrestressConfig, &nfh);
         sps_force = pPrestressForce->GetPrestressForce(poi,liftConfig.GdrConfig,pgsTypes::Straight,pgsTypes::AtLifting);
         se        = pStrandGeometry->GetSsEccentricity(poi,liftConfig.GdrConfig.PrestressConfig, &nfs);
         tps_force = pPrestressForce->GetPrestressForce(poi,liftConfig.GdrConfig,pgsTypes::Temporary,pgsTypes::AtLifting);
         te        = pStrandGeometry->GetTempEccentricity(poi,liftConfig.GdrConfig.PrestressConfig, &nft);
      }
      else
      {
         hps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Harped,pgsTypes::AtLifting);
         he        = pStrandGeometry->GetHsEccentricity(poi,&nfh);
         sps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Straight,pgsTypes::AtLifting);
         se        = pStrandGeometry->GetSsEccentricity(poi,&nfs);
         tps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AtLifting);
         te        = pStrandGeometry->GetTempEccentricity(poi,&nft);
      }

      Float64 total_ps_force = hps_force + sps_force + tps_force;
      Float64 total_e = 0.0;
      if (total_ps_force>0)
         total_e = (hps_force*he + sps_force*se + tps_force*te) / total_ps_force;
      else if ( nfh + nfs + nft > 0 )
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
      pgsAlternativeTensileStressCalculator::ComputeReqdFcTens(max_stress, rcsT, rcsBfmax, rcsFmax, rcsTalt, &fc_tens_norebar, &fc_tens_withrebar);

      lift_artifact.SetRequiredConcreteStrength(fc_compression,fc_tens_norebar,fc_tens_withrebar);

      pArtifact->AddLiftingStressAnalysisArtifact(poi.GetDistFromStart(),lift_artifact);
   }

}

bool pgsGirderLiftingChecker::ComputeLiftingFsAgainstCracking(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,
                                                               const HANDLINGCONFIG& liftConfig,
                                                               const std::vector<pgsPointOfInterest>& rpoiVec,
                                                               const std::vector<Float64>& momVec,
                                                               Float64 midSpanDeflection,
                                                               pgsLiftingAnalysisArtifact* pArtifact)
{
   Float64 fo = pArtifact->GetOffsetFactor();

   pArtifact->SetCamberDueToSelfWeight(midSpanDeflection);

   // get mid-span poi so we can calc camber due to ps
   pgsPointOfInterest poi_ms;
   bool found=false;
   std::vector<pgsPointOfInterest>::const_iterator iter=rpoiVec.begin();
   while(iter!=rpoiVec.end())
   {
      const pgsPointOfInterest& rpoi = *iter;
      if (rpoi.IsMidSpan(pgsTypes::Lifting))
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
   GET_IFACE(ISectProp2,pSectProp2);
   Float64 Ix = pSectProp2->GetIx(pgsTypes::CastingYard,poi_ms);
   Float64 Iy = pSectProp2->GetIy(pgsTypes::CastingYard,poi_ms);
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
   if (ayr>0.0)
   {
      Float64 fr = pArtifact->GetModRupture();
      GET_IFACE(IGirder,pGdr);
      Float64 bt_bot = pGdr->GetBottomWidth(poi_ms);
      Float64 bt_top = pGdr->GetTopWidth(poi_ms);

      // loop over all pois and calculate cracking fs
      IndexType psiz = rpoiVec.size();
      IndexType msiz = momVec.size();
      CHECK(psiz==msiz);
      for(IndexType i=0; i<psiz; i++)
      {
         const pgsPointOfInterest poi = rpoiVec[i];
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

void pgsGirderLiftingChecker::ComputeLiftingFsAgainstFailure(SpanIndexType span,GirderIndexType gdr,
                                                              pgsLiftingAnalysisArtifact* pArtifact)
{

   Float64 zo = pArtifact->GetZo();
   CHECK(zo>0.0);
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


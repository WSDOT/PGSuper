///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include "StatusItems.h"

#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

void pgsGirderHandlingChecker::CheckHauling(SpanIndexType span,GirderIndexType gdr,pgsHaulingCheckArtifact* pArtifact)
{
   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);

   if (pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      // calc some initial information
      PrepareHaulingCheckArtifact(span,gdr,pArtifact);

      // AnalyzeHauling should be generic. It will analyze the configuration
      // passed to it, not the current configuration of the girder
      AnalyzeHauling(span,gdr,pArtifact);
   }
}

void pgsGirderHandlingChecker::CheckLifting(SpanIndexType span,GirderIndexType gdr,pgsLiftingCheckArtifact* pArtifact)
{
   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);

   if (pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
      // calc some initial information
      PrepareLiftingCheckArtifact(span,gdr,pArtifact);

      // AnalyzeLifting should be generic. It will analyze the configuration
      // passed to it, not the current configuration of the girder
      AnalyzeLifting(span,gdr,pArtifact);
   }
}

void pgsGirderHandlingChecker::AnalyzeLifting(SpanIndexType span,GirderIndexType gdr,pgsLiftingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest); // poi's from global pool
   HANDLINGCONFIG dummy_config;
   AnalyzeLifting(span,gdr,false,dummy_config,pGirderLiftingPointsOfInterest,pArtifact);
}

void pgsGirderHandlingChecker::AnalyzeLifting(SpanIndexType span,GirderIndexType gdr,const HANDLINGCONFIG& liftConfig,IGirderLiftingDesignPointsOfInterest* pPoiD, pgsLiftingAnalysisArtifact* pArtifact)
{
   AnalyzeLifting(span,gdr,true,liftConfig,pPoiD,pArtifact);
}

void pgsGirderHandlingChecker::AnalyzeLifting(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,const HANDLINGCONFIG& liftConfig,IGirderLiftingDesignPointsOfInterest* pPoiD,pgsLiftingAnalysisArtifact* pArtifact)
{
   // calc some initial information
   GET_IFACE(IBridgeMaterial,pMaterial);

   Float64 Loh, Roh, Eci, Fci;

   std::vector<pgsPointOfInterest> poi_vec;
   if ( bUseConfig )
   {
      Loh = liftConfig.LeftOverhang;
      Roh = liftConfig.RightOverhang;

      if ( liftConfig.GdrConfig.bUserEci )
         Eci = liftConfig.GdrConfig.Eci;
      else
         Eci = pMaterial->GetEconc(liftConfig.GdrConfig.Fci, pMaterial->GetStrDensityGdr(span,gdr),pMaterial->GetK1Gdr(span,gdr));

      Fci = liftConfig.GdrConfig.Fci;

      poi_vec = pPoiD->GetLiftingDesignPointsOfInterest(span,gdr,Loh,POI_FLEXURESTRESS);
   }
   else
   {
      GET_IFACE(IGirderLifting,pGirderLifting);
      Loh = pGirderLifting->GetLeftLiftingLoopLocation(span,gdr);
      Roh = pGirderLifting->GetRightLiftingLoopLocation(span,gdr);

      GET_IFACE(IBridgeMaterial,pMaterial);
      Eci = pMaterial->GetEciGdr(span,gdr);
      Fci = pMaterial->GetFciGdr(span,gdr);

      GET_IFACE(IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
      poi_vec = pGirderLiftingPointsOfInterest->GetLiftingPointsOfInterest(span,gdr,POI_FLEXURESTRESS);
   }
   
   PrepareLiftingAnalysisArtifact(span,gdr,Loh,Roh,Fci,Eci,pArtifact);

   pArtifact->SetLiftingPointsOfInterest(poi_vec);

   // get moments at pois  and mid-span deflection due to dead vertical lifting
   std::vector<Float64> moment_vec;
   Float64 mid_span_deflection, overhang_deflection;
   ComputeLiftingMoments(span, gdr, *pArtifact, poi_vec, &moment_vec,&mid_span_deflection,&overhang_deflection);

   ComputeLiftingStresses(span, gdr, bUseConfig, liftConfig, poi_vec, moment_vec, pArtifact);

   if (ComputeLiftingFsAgainstCracking(span, gdr, bUseConfig, liftConfig, poi_vec, moment_vec, mid_span_deflection, overhang_deflection, pArtifact))
   {
      ComputeLiftingFsAgainstFailure(span, gdr, pArtifact);
   }
   else
   {
      pArtifact->SetThetaFailureMax(0.0);
      pArtifact->SetFsFailure(0.0);
   }
}

void pgsGirderHandlingChecker::AnalyzeHauling(SpanIndexType span,GirderIndexType gdr,pgsHaulingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest); // poi's from global pool

   HANDLINGCONFIG dummy_config;
   AnalyzeHauling(span,gdr,false,dummy_config,pGirderHaulingPointsOfInterest,pArtifact);
}

void pgsGirderHandlingChecker::AnalyzeHauling(SpanIndexType span,GirderIndexType gdr,const HANDLINGCONFIG& haulConfig,IGirderHaulingDesignPointsOfInterest* pPOId,pgsHaulingAnalysisArtifact* pArtifact)
{
   AnalyzeHauling(span,gdr,true,haulConfig,pPOId,pArtifact);
}

void pgsGirderHandlingChecker::AnalyzeHauling(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,const HANDLINGCONFIG& haulConfig,IGirderHaulingDesignPointsOfInterest* pPOId,pgsHaulingAnalysisArtifact* pArtifact)
{
   // calc some initial information
   GET_IFACE(IBridgeMaterial,pMaterial);

   Float64 Loh, Roh, Ec, Fc;

   std::vector<pgsPointOfInterest> poi_vec;
   if ( bUseConfig )
   {
      Loh = haulConfig.LeftOverhang;
      Roh = haulConfig.RightOverhang;

      if ( haulConfig.GdrConfig.bUserEc )
         Ec = haulConfig.GdrConfig.Ec;
      else
         Ec = pMaterial->GetEconc(haulConfig.GdrConfig.Fc, pMaterial->GetStrDensityGdr(span,gdr),pMaterial->GetK1Gdr(span,gdr));

      Fc = haulConfig.GdrConfig.Fc;

      poi_vec = pPOId->GetHaulingDesignPointsOfInterest(span,gdr,Loh,Roh,POI_FLEXURESTRESS);
   }
   else
   {
      GET_IFACE(IGirderHauling,pGirderHauling);
      Loh = pGirderHauling->GetTrailingOverhang(span,gdr);
      Roh = pGirderHauling->GetLeadingOverhang(span,gdr);

      Fc = pMaterial->GetFcGdr(span,gdr);
      Ec = pMaterial->GetEcGdr(span,gdr);

      GET_IFACE(IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);
      poi_vec = pGirderHaulingPointsOfInterest->GetHaulingPointsOfInterest(span,gdr,POI_FLEXURESTRESS);
   }

   PrepareHaulingAnalysisArtifact(span,gdr,Loh,Roh,Fc,Ec,pArtifact);


   pArtifact->SetHaulingPointsOfInterest(poi_vec);

   // get moments at pois  and mid-span deflection due to dead vertical hauling
   std::vector<Float64> moment_vec;
   Float64 mid_span_deflection,overhang_deflection;
   ComputeHaulingMoments(span, gdr, *pArtifact, poi_vec, &moment_vec,&mid_span_deflection,&overhang_deflection);

   ComputeHaulingRollAngle(span, gdr, pArtifact, poi_vec, &moment_vec,&mid_span_deflection,&overhang_deflection);

   ComputeHaulingStresses(span, gdr, bUseConfig, haulConfig, poi_vec, moment_vec, pArtifact);

   ComputeHaulingFsForCracking(span, gdr, poi_vec, moment_vec, mid_span_deflection, overhang_deflection, pArtifact);

   ComputeHaulingFsForRollover(span, gdr, pArtifact);
}

bool pgsGirderHandlingChecker::DesignShipping(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,bool bDesignForEqualOverhangs,bool bIgnoreConfigurationLimits,IGirderHaulingDesignPointsOfInterest* pPOId,pgsHaulingAnalysisArtifact* pArtifact,SHARED_LOGFILE LOGFILE)
{
   LOG("Entering pgsGirderHandlingChecker::DesignShipping");
   // Get range of values for truck support locations
   GET_IFACE(IBridge,pBridge);
   Float64 Lg = pBridge->GetGirderLength(span,gdr);

   GET_IFACE(IGirderHaulingSpecCriteria,pCriteria);
   Float64 maxDistanceBetweenSupports = pCriteria->GetAllowableDistanceBetweenSupports();
   Float64 min_overhang_start = pCriteria->GetMinimumHaulingSupportLocation(span,gdr,pgsTypes::metStart);
   Float64 min_overhang_end   = pCriteria->GetMinimumHaulingSupportLocation(span,gdr,pgsTypes::metEnd);
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

   double bigInc = 8*location_accuracy;
   double smallInc = location_accuracy;
   double inc = bigInc;
   bool bLargeStepSize = true;

   Float64 FScr = 0;
   Float64 FSr = 0;
   Float64 FScrMin = pCriteria->GetHaulingCrackingFs();
   Float64 FSrMin = pCriteria->GetHaulingRolloverFs();

   LOG("Allowable FS cracking FScrMin = "<<FScrMin);
   LOG("Allowable FS rollover FSrMin = "<<FSrMin);

   Float64 loc = minOverhang;
   pgsHaulingAnalysisArtifact artifact;

   HANDLINGCONFIG shipping_config;
   shipping_config.GdrConfig = config;

   while ( loc < maxOverhang )
   {
      LOG("");

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

      LOG("Trying Trailing Overhang = " << ::ConvertFromSysUnits(shipping_config.LeftOverhang,unitMeasure::Feet) << " ft" << "      Leading Overhang = " << ::ConvertFromSysUnits(shipping_config.RightOverhang,unitMeasure::Feet) << " ft");

      checker.AnalyzeHauling(span,gdr,shipping_config,pPOId,&curr_artifact);
      FScr = curr_artifact.GetMinFsForCracking();

      LOG("FScr = " << FScr);
      if ( FScr < FScrMin && maxOverhang/4 < loc )
      {
         LOG("Could not satisfy FScr... Need to add temporary strands");
         // Moving the supports closer isn't going to help
         *pArtifact = curr_artifact;
         return false; // design failed
      }

      FSr  = curr_artifact.GetFsRollover();

      LOG("FSr = " << FSr);

      if ( 0.95*FScrMin < FScr && 0.95*FSrMin < FSr && bLargeStepSize)
      {
         // we are getting close... Use a smaller step size
         double oldInc = inc;
         inc = smallInc;
         bLargeStepSize = false;
         if ( 1.05*FSrMin <= FSr )
         {
            // We went past the solution... back up
            LOG("Went past the solution... backup");
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

   LOG("Exiting pgsGirderHandlingChecker::DesignShipping - success");
   return true;
}

pgsDesignCodes::OutcomeType pgsGirderHandlingChecker::DesignLifting(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,IGirderLiftingDesignPointsOfInterest* pPoiD,pgsLiftingAnalysisArtifact* pArtifact,SHARED_LOGFILE LOGFILE)
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

   if (config.Nstrands[pgsTypes::Harped] > 0) // only look at harping point if we have harped strands
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
      LOG("");
      LOG("Trying location " << ::ConvertFromSysUnits(loc,unitMeasure::Feet) << " ft");

      pgsLiftingAnalysisArtifact curr_artifact;
      lift_config.LeftOverhang = loc;
      lift_config.RightOverhang = loc;

      AnalyzeLifting(span,gdr,lift_config,pPoiD,&curr_artifact);
      FSf = curr_artifact.GetFsFailure();

      LOG("FSf = " << FSf);

      if ( 0.95*FSfMin < FSf && bLargeStepSize)
      {
         // we are getting close... Use a smaller step size
         Float64 oldInc = locInc;
         locInc = smallInc;
         bLargeStepSize = false;
         if ( 1.05*FSfMin <= FSf )
         {
            // We went past the solution... back up
            LOG("Went past the solution... backup");
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
      LOG("Cannot find a pick point to safisfy FSf");
      LOG("Temporary strands required");
      LOG("Move on to Shipping Design");
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
void pgsGirderHandlingChecker::PrepareLiftingCheckArtifact(SpanIndexType span,GirderIndexType gdr,pgsLiftingCheckArtifact* pArtifact)
{
   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   pArtifact->SetAllowableTensileStress(pGirderLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStress(span,gdr));
   pArtifact->SetAllowableCompressionStress(pGirderLiftingSpecCriteria->GetLiftingAllowableCompressiveConcreteStress(span,gdr));
   pArtifact->SetAllowableFsForCracking(pGirderLiftingSpecCriteria->GetLiftingCrackingFs());
   pArtifact->SetAllowableFsForFailure(pGirderLiftingSpecCriteria->GetLiftingFailureFs());
   pArtifact->SetAlternativeTensionAllowableStress(pGirderLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStress(span,gdr));
}

void pgsGirderHandlingChecker::PrepareLiftingAnalysisArtifact(SpanIndexType span,GirderIndexType gdr,Float64 Loh,Float64 Roh,Float64 Fci,Float64 Eci,pgsLiftingAnalysisArtifact* pArtifact)
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
   Float64 XFerLength =  pPrestressForce->GetXferLength(span,gdr);
   pArtifact->SetXFerLength(XFerLength);

   Float64 span_len = girder_length - Loh - Roh;

   if (span_len <= 0.0)
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);

      const char* msg = "Lifting support overhang cannot exceed one-half of the span length";
      pgsLiftingSupportLocationStatusItem* pStatusItem = new pgsLiftingSupportLocationStatusItem(span,gdr,m_StatusGroupID,m_scidLiftingSupportLocationError,msg);
      pStatusCenter->Add(pStatusItem);

      std::string str(msg);
      str += std::string("\nSee Status Center for details");
      THROW_UNWIND(str.c_str(),-1);
   }

   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   Float64 min_lift_point_start = pGirderLiftingSpecCriteria->GetMinimumLiftingPointLocation(span,gdr,pgsTypes::metStart);
   Float64 min_lift_point_end   = pGirderLiftingSpecCriteria->GetMinimumLiftingPointLocation(span,gdr,pgsTypes::metEnd);
   if ( Loh < min_lift_point_start )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format("Left lift point is less than the minimum value of %s",::FormatDimension(min_lift_point_start,pDisplayUnits->GetSpanLengthUnit()));
      pgsLiftingSupportLocationStatusItem* pStatusItem = new pgsLiftingSupportLocationStatusItem(span,gdr,m_StatusGroupID,m_scidLiftingSupportLocationWarning,strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   if ( Roh < min_lift_point_end )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format("Right lift point is less than the minimum value of %s",::FormatDimension(min_lift_point_end,pDisplayUnits->GetSpanLengthUnit()));
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
   pArtifact->SetModRupture( pGirderLiftingSpecCriteria->GetLiftingModulusOfRupture(Fci) );
   pArtifact->SetModRuptureCoefficient( pGirderLiftingSpecCriteria->GetLiftingModulusOfRuptureCoefficient() );

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


   double f,fmax;
   bool bMax;
   pGirderLiftingSpecCriteria->GetLiftingAllowableTensileConcreteStressParameters(&f,&bMax,&fmax);
   pArtifact->SetAllowableTensileConcreteStressParameters(f,bMax,fmax);
   pArtifact->SetAllowableCompressionFactor(pGirderLiftingSpecCriteria->GetLiftingAllowableCompressionFactor());
   pArtifact->SetAlternativeTensileConcreteStressFactor(pGirderLiftingSpecCriteria->GetLiftingWithMildRebarAllowableStressFactor()  );
}

void pgsGirderHandlingChecker::ComputeLiftingMoments(SpanIndexType span,GirderIndexType gdr,
                                                     const pgsLiftingAnalysisArtifact& rArtifact, 
                                                     const std::vector<pgsPointOfInterest>& rpoiVec,
                                                     std::vector<Float64>* pmomVec, 
                                                     Float64* pMidSpanDeflection, Float64* pOverhangDeflection)
{
   // build a model
   Float64 glen    = rArtifact.GetGirderLength();
   Float64 leftOH  = rArtifact.GetLeftOverhang();
   Float64 rightOH = rArtifact.GetRightOverhang();
   Float64 E = rArtifact.GetElasticModulusOfGirderConcrete();

   ComputeMoments(span, gdr,
                  pgsTypes::Lifting,
                  leftOH, glen, rightOH,
                  E,
                  rpoiVec,
                  pmomVec, pMidSpanDeflection, pOverhangDeflection);
}

void pgsGirderHandlingChecker::ComputeLiftingStresses(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,
                                                      const HANDLINGCONFIG& liftConfig,
                                                      const std::vector<pgsPointOfInterest>& rpoiVec,
                                                      const std::vector<Float64>& momVec,
                                                      pgsLiftingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IPrestressForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeometry);

   // get poi-independent values used for stress calc
   // factor in forces from inclined lifting cables
   Float64 p_lift = pArtifact->GetAxialCompressiveForceDueToInclinationOfLiftingCables();
   Float64 m_lift = pArtifact->GetMomentInGirderDueToInclinationOfLiftingCables();
   Float64 impact_up   = 1.0 - pArtifact->GetUpwardImpact();
   Float64 impact_down = 1.0 + pArtifact->GetDownwardImpact();

   Uint32 psiz = rpoiVec.size();
   Uint32 msiz = momVec.size();
   CHECK(psiz==msiz);

   Float64 glen    = pArtifact->GetGirderLength();
   Float64 leftOH  = pArtifact->GetLeftOverhang();
   Float64 rightOH = pArtifact->GetRightOverhang();

   Float64 AsMax = 0;
   Float64 As = 0;

   GET_IFACE(ISectProp2,pSectProp2);

   for(Uint32 i=0; i<psiz; i++)
   {
      const pgsPointOfInterest& poi = rpoiVec[i];

      Float64 ag = pSectProp2->GetAg(pgsTypes::CastingYard,poi,liftConfig.GdrConfig.Fci);
      Float64 stg = pSectProp2->GetSt(pgsTypes::CastingYard,poi,liftConfig.GdrConfig.Fci);
      Float64 sbg = pSectProp2->GetSb(pgsTypes::CastingYard,poi,liftConfig.GdrConfig.Fci);

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


      pgsLiftingStressAnalysisArtifact lift_artifact;

      // calc total prestress force and eccentricity
      Float64 nfs, nfh, nft;
      Float64 hps_force, he;
      Float64 sps_force, se;
      Float64 tps_force, te;       

      if ( bUseConfig )
      {
         hps_force = pPrestressForce->GetPrestressForce(poi,liftConfig.GdrConfig,pgsTypes::Harped,pgsTypes::AtLifting);
         he        = pStrandGeometry->GetHsEccentricity(poi,liftConfig.GdrConfig, &nfh);
         sps_force = pPrestressForce->GetPrestressForce(poi,liftConfig.GdrConfig,pgsTypes::Straight,pgsTypes::AtLifting);
         se        = pStrandGeometry->GetSsEccentricity(poi,liftConfig.GdrConfig, &nfs);
         tps_force = pPrestressForce->GetPrestressForce(poi,liftConfig.GdrConfig,pgsTypes::Temporary,pgsTypes::AtLifting);
         te        = pStrandGeometry->GetTempEccentricity(poi,liftConfig.GdrConfig, &nft);
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

      Float64 Yna1, At1, T1, As1;
      Float64 Yna2, At2, T2, As2;
      Float64 Yna3, At3, T3, As3;
      GetRequirementsForAlternativeTensileStress(poi,ft_up,ft_up,  fb_up,fb_up,  &Yna1,&At1,&T1,&As1);
      GetRequirementsForAlternativeTensileStress(poi,ft_no,ft_no,  fb_no,fb_no,  &Yna2,&At2,&T2,&As2);
      GetRequirementsForAlternativeTensileStress(poi,ft_down,ft_down,fb_down,fb_down,&Yna3,&At3,&T3,&As3);

      Float64 As = Max3(As1,As2,As3);
      double At, T;
      if ( IsEqual(As,As1) )
      {
         At = At1;
         T  = T1;
      }
      else if ( IsEqual(As,As2) )
      {
         At = At2;
         T  = T2;
      }
      else if ( IsEqual(As,As3) )
      {
         At = At3;
         T  = T3;
      }
      lift_artifact.SetAlternativeTensileStressParameters(Yna1,Yna2,Yna3,At,T,As);
      AsMax = _cpp_max(As,AsMax);

      pArtifact->AddLiftingStressAnalysisArtifact(poi.GetDistFromStart(),lift_artifact);
   }

   pArtifact->SetAlterantiveTensileStressAsMax(AsMax);
}

bool pgsGirderHandlingChecker::ComputeLiftingFsAgainstCracking(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,
                                                               const HANDLINGCONFIG& liftConfig,
                                                               const std::vector<pgsPointOfInterest>& rpoiVec,
                                                               const std::vector<Float64>& momVec,
                                                               Float64 midSpanDeflection,
                                                               Float64 overhangDeflection,
                                                               pgsLiftingAnalysisArtifact* pArtifact)
{
   Float64 fo = pArtifact->GetOffsetFactor();

   pArtifact->SetCamberDueToSelfWeight(midSpanDeflection);
   pArtifact->SetCamberDueToSelfWeightOverhang(overhangDeflection);

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
   Float64 total_camber = ps_camber + midSpanDeflection - overhangDeflection;
   pArtifact->SetTotalCamberAtLifting(total_camber);

   // adjusted yr = distance between cg and lifting point
   Float64 yr = pArtifact->GetVerticalDistanceFromPickPointToGirderCg();
   Float64 ayr = yr - fo*total_camber;
   pArtifact->SetAdjustedYr(ayr);

   // Zo (based on mid-span properties)
   GET_IFACE(ISectProp2,pSectProp2);
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

   // intial tilt angle
   Float64 ei = pArtifact->GetTotalInitialEccentricity();
   Float64 theta_i = ei/ayr;
   pArtifact->SetInitialTiltAngle(theta_i);

   // If at this point, yr is negative, then the girder is unstable for
   // lifting and we can do no more calculations. So return false.
   if (ayr>0.0)
   {
      Float64 fr = pArtifact->GetModRupture();
      GET_IFACE(IGirder,pGdr);
      Float64 bt_bot = pGdr->GetBottomWidth(poi_ms);
      Float64 bt_top = pGdr->GetTopWidth(poi_ms);

      // loop over all pois and calculate cracking fs
      Uint32 psiz = rpoiVec.size();
      Uint32 msiz = momVec.size();
      CHECK(psiz==msiz);
      for(Uint32 i=0; i<psiz; i++)
      {
         const pgsPointOfInterest poi = rpoiVec[i];
         Float64 mom_vert = momVec[i];

         pgsLiftingCrackingAnalysisArtifact crack_artifact;
         crack_artifact.SetVerticalMoment(mom_vert);

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

void pgsGirderHandlingChecker::ComputeLiftingFsAgainstFailure(SpanIndexType span,GirderIndexType gdr,
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

////////////////////////////////////////////////////////
// hauling
////////////////////////////////////////////////////////
void pgsGirderHandlingChecker::PrepareHaulingAnalysisArtifact(SpanIndexType span,GirderIndexType gdr,Float64 Loh,Float64 Roh,Float64 Fc,Float64 Ec,pgsHaulingAnalysisArtifact* pArtifact)
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

   Float64 span_len = girder_length - Loh - Roh;

   if (span_len<=0.0)
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);

      const char* msg = "Hauling support overhang cannot exceed one-half of the span length";
      pgsLiftingSupportLocationStatusItem* pStatusItem = new pgsLiftingSupportLocationStatusItem(span,gdr,m_StatusGroupID,m_scidLiftingSupportLocationError,msg);
      pStatusCenter->Add(pStatusItem);

      std::string str(msg);
      str += std::string("\nSee Status Center for details");
      THROW_UNWIND(str.c_str(),-1);
   }

   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   Float64 min_bunk_point_start = pGirderHaulingSpecCriteria->GetMinimumHaulingSupportLocation(span,gdr,pgsTypes::metStart);
   Float64 min_bunk_point_end   = pGirderHaulingSpecCriteria->GetMinimumHaulingSupportLocation(span,gdr,pgsTypes::metEnd);
   if ( Loh < min_bunk_point_start )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format("Left bunk point is less than the minimum value of %s",::FormatDimension(min_bunk_point_start,pDisplayUnits->GetSpanLengthUnit()));
      pgsBunkPointLocationStatusItem* pStatusItem = new pgsBunkPointLocationStatusItem(span,gdr,m_StatusGroupID,m_scidBunkPointLocation,strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   if ( Roh < min_bunk_point_end )
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      CString strMsg;
      strMsg.Format("Right bunk point is less than the minimum value of %s",::FormatDimension(min_bunk_point_end,pDisplayUnits->GetSpanLengthUnit()));
      pgsBunkPointLocationStatusItem* pStatusItem = new pgsBunkPointLocationStatusItem(span,gdr,m_StatusGroupID,m_scidBunkPointLocation,strMsg);
      pStatusCenter->Add(pStatusItem);
   }

   pArtifact->SetClearSpanBetweenSupportLocations(span_len);
   pArtifact->SetOverhangs(Loh,Roh);

   Float64 roll_hgt = pGirderHaulingSpecCriteria->GetHeightOfTruckRollCenterAboveRoadway();
   Float64 gb_hgt =  pGirderHaulingSpecCriteria->GetHeightOfGirderBottomAboveRoadway();
   Float64 camber_factor = 1.0 + pGirderHaulingSpecCriteria->GetIncreaseInCgForCamber();
   Float64 ycgb = pSectProp2->GetYb(pgsTypes::CastingYard,pgsPointOfInterest(span,gdr,0.0));
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
   pArtifact->SetModRupture( pGirderHaulingSpecCriteria->GetHaulingModulusOfRupture(Fc) );
   pArtifact->SetModRuptureCoefficient( pGirderHaulingSpecCriteria->GetHaulingModulusOfRuptureCoefficient() );

   pArtifact->SetElasticModulusOfGirderConcrete(Ec);

   Float64 offset_factor = span_len/girder_length;
   offset_factor = offset_factor*offset_factor - 1/3.;
   pArtifact->SetOffsetFactor(offset_factor);

   Float64 e_sweep = sweep * girder_length;
   pArtifact->SetEccentricityDueToSweep(e_sweep);

   Float64 e_placement = pGirderHaulingSpecCriteria->GetHaulingSupportPlacementTolerance();
   pArtifact->SetEccentricityDueToPlacementTolerance(e_placement);

   pArtifact->SetTotalInitialEccentricity(e_sweep*offset_factor + e_placement);

   double f,fmax;
   bool bMax;
   pGirderHaulingSpecCriteria->GetHaulingAllowableTensileConcreteStressParameters(&f,&bMax,&fmax);
   pArtifact->SetAllowableTensileConcreteStressParameters(f,bMax,fmax);
   pArtifact->SetAllowableCompressionFactor(pGirderHaulingSpecCriteria->GetHaulingAllowableCompressionFactor());
   pArtifact->SetAlternativeTensileConcreteStressFactor(pGirderHaulingSpecCriteria->GetHaulingWithMildRebarAllowableStressFactor()  );
}

void pgsGirderHandlingChecker::PrepareHaulingCheckArtifact(SpanIndexType span,GirderIndexType gdr,pgsHaulingCheckArtifact* pArtifact)
{
   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   pArtifact->SetAllowableSpanBetweenSupportLocations(pGirderHaulingSpecCriteria->GetAllowableDistanceBetweenSupports());
   pArtifact->SetAllowableLeadingOverhang(pGirderHaulingSpecCriteria->GetAllowableLeadingOverhang());
   pArtifact->SetAllowableTensileStress(pGirderHaulingSpecCriteria->GetHaulingAllowableTensileConcreteStress(span,gdr));
   pArtifact->SetAllowableCompressionStress(pGirderHaulingSpecCriteria->GetHaulingAllowableCompressiveConcreteStress(span,gdr));
   pArtifact->SetAllowableFsForCracking(pGirderHaulingSpecCriteria->GetHaulingCrackingFs());
   pArtifact->SetAllowableFsForRollover(pGirderHaulingSpecCriteria->GetHaulingRolloverFs());
   pArtifact->SetMaxGirderWgt(pGirderHaulingSpecCriteria->GetMaxGirderWgt());
   pArtifact->SetAlternativeTensionAllowableStress(pGirderHaulingSpecCriteria->GetHaulingWithMildRebarAllowableStress(span,gdr));
}

void pgsGirderHandlingChecker::ComputeHaulingStresses(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,
                                                      const HANDLINGCONFIG& haulConfig,
                                                      const std::vector<pgsPointOfInterest>& rpoiVec,
                                                      const std::vector<Float64>& momVec,
                                                      pgsHaulingAnalysisArtifact* pArtifact)
{
   GET_IFACE(IPrestressForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeometry);
   GET_IFACE(ISectProp2,pSectProp2);
   GET_IFACE(IGirder,pGdr);

   Float64 roll_angle = pArtifact->GetEqualibriumAngle();

   // get poi-independent values used for stress calc
   Float64 impact_up   = 1.0 - pArtifact->GetUpwardImpact();
   Float64 impact_down = 1.0 + pArtifact->GetDownwardImpact();

   Uint32 psiz = rpoiVec.size();
   Uint32 msiz = momVec.size();
   CHECK(psiz==msiz);

   Float64 AsMax = 0;

   for(Uint32 i=0; i<psiz; i++)
   {
      const pgsPointOfInterest poi = rpoiVec[i];

      Float64 bt_bot = pGdr->GetBottomWidth(poi);
      Float64 bt_top = pGdr->GetTopWidth(poi);
      Float64 ag     = pSectProp2->GetAg(pgsTypes::CastingYard,poi,haulConfig.GdrConfig.Fc);
      Float64 iy     = pSectProp2->GetIy(pgsTypes::CastingYard,poi,haulConfig.GdrConfig.Fc);
      Float64 stg    = pSectProp2->GetSt(pgsTypes::CastingYard,poi,haulConfig.GdrConfig.Fc);
      Float64 sbg    = pSectProp2->GetSb(pgsTypes::CastingYard,poi,haulConfig.GdrConfig.Fc);


      pgsHaulingStressAnalysisArtifact haul_artifact;

      // calc total prestress force and eccentricity
      Float64 nfs, nfh, nft;
      Float64 hps_force, he;
      Float64 sps_force, se;
      Float64 tps_force, te;

      if ( bUseConfig )
      {
         hps_force = pPrestressForce->GetPrestressForce(poi,haulConfig.GdrConfig,pgsTypes::Harped,pgsTypes::AtShipping);
         he = pStrandGeometry->GetHsEccentricity(poi,haulConfig.GdrConfig, &nfh);
         sps_force = pPrestressForce->GetPrestressForce(poi,haulConfig.GdrConfig,pgsTypes::Straight,pgsTypes::AtShipping);
         se = pStrandGeometry->GetSsEccentricity(poi,haulConfig.GdrConfig,&nfs);
         tps_force = pPrestressForce->GetPrestressForce(poi,haulConfig.GdrConfig,pgsTypes::Temporary,pgsTypes::AtShipping);
         te = pStrandGeometry->GetTempEccentricity(poi,haulConfig.GdrConfig,&nft);
      }
      else
      {
         hps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Harped,pgsTypes::AtShipping);
         he = pStrandGeometry->GetHsEccentricity(poi,&nfh);
         sps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Straight,pgsTypes::AtShipping);
         se = pStrandGeometry->GetSsEccentricity(poi,&nfs);
         tps_force = pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,pgsTypes::AtShipping);
         te = pStrandGeometry->GetTempEccentricity(poi,&nft);
      }

      Float64 total_ps_force = hps_force + sps_force + tps_force;
      Float64 total_e=0.0;
      if (total_ps_force>0)
         total_e = (hps_force*he + sps_force*se + tps_force*te) / total_ps_force;
      else if (nfh + nfs + nft > 0)
         total_e = (he*nfh + se*nfs + te*nft) / (nfh + nfs + nft);

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
      Float64 ft_impact_up   = moment_impact_up/stg   + ft_ps;
      Float64 ft             = vert_mom/stg           + ft_ps;
      Float64 ft_impact_down = moment_impact_down/stg + ft_ps;

      Float64 fb_ps = -total_ps_force/ag + (mom_ps)/sbg;
      Float64 fb_impact_up   = moment_impact_up/sbg   + fb_ps;
      Float64 fb             = vert_mom/sbg           + fb_ps;
      Float64 fb_impact_down = moment_impact_down/sbg + fb_ps;

      Float64 ftu = ft - lat_mom*bt_top/(iy*2);
      Float64 ftd = ft + lat_mom*bt_top/(iy*2);
      Float64 fbu = fb - lat_mom*bt_bot/(iy*2);
      Float64 fbd = fb + lat_mom*bt_bot/(iy*2);

      haul_artifact.SetTopFiberStress(ft_ps,ft_impact_up,ft,ft_impact_down);
      haul_artifact.SetBottomFiberStress(fb_ps,fb_impact_up,fb,fb_impact_down);
      haul_artifact.SetIncludedGirderStresses(ftu,ftd,fbu,fbd);


      Float64 Yna1, At1, T1, As1;
      Float64 Yna2, At2, T2, As2;
      Float64 Yna3, At3, T3, As3;
      GetRequirementsForAlternativeTensileStress(poi, ft_impact_up,   ft_impact_up,   fb_impact_up,   fb_impact_up,    &Yna1, &At1, &T1, &As1);
      GetRequirementsForAlternativeTensileStress(poi, ft_impact_down, ft_impact_down, fb_impact_down, fb_impact_down,  &Yna2, &At2, &T2, &As2);
      GetRequirementsForAlternativeTensileStress(poi, ftu,            ftd,            fbu,            fbd,             &Yna3, &At3, &T3, &As3);

      Float64 As = Max3(As1,As2,As3);
      ImpactDir controlling_impact;
      GirderOrientation controlling_orientation;
      double At, T;
      if ( IsEqual(As,As1) )
      {
         At = At1;
         T  = T1;
         controlling_impact = Up;
         controlling_orientation = Plumb;
      }
      else if ( IsEqual(As,As2) )
      {
         At = At2;
         T  = T2;
         controlling_impact = Down;
         controlling_orientation = Plumb;
      }
      else if ( IsEqual(As,As3) )
      {
         At = At3;
         T  = T3;
         controlling_impact = None;
         controlling_orientation = Inclined;
      }

      haul_artifact.SetAlternativeTensileStressParameters(Yna1,Yna2,Yna3,controlling_impact,controlling_orientation,At,T,As);
      AsMax = _cpp_max(As,AsMax);

      pArtifact->AddHaulingStressAnalysisArtifact(poi.GetDistFromStart(),haul_artifact);
   }

   pArtifact->SetAlterantiveTensileStressAsMax(AsMax);
}

void pgsGirderHandlingChecker::ComputeHaulingFsForCracking(SpanIndexType span,GirderIndexType gdr,
                                                           const std::vector<pgsPointOfInterest>& rpoiVec,
                                                           const std::vector<Float64>& momVec,
                                                           Float64 midSpanDeflection,
                                                           Float64 overhangDeflection,
                                                           pgsHaulingAnalysisArtifact* pArtifact)
{
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

   Float64 fr = pArtifact->GetModRupture();

   GET_IFACE(IGirder,pGdr);
   GET_IFACE(ISectProp2,pSectProp2);

   // loop over all pois and calculate cracking fs
   Uint32 psiz = rpoiVec.size();
   Uint32 msiz = momVec.size();
   CHECK(psiz==msiz);
   for(Uint32 i=0; i<psiz; i++)
   {
      const pgsPointOfInterest& poi = rpoiVec[i];
      Float64 bt_bot = pGdr->GetBottomWidth(poi);
      Float64 bt_top = pGdr->GetTopWidth(poi);

      Float64 iy = pSectProp2->GetIy(pgsTypes::CastingYard,poi);

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

      pArtifact->AddHaulingCrackingAnalysisArtifact(poi.GetDistFromStart(),crack_artifact);
   }
}

void pgsGirderHandlingChecker::ComputeHaulingFsForRollover(SpanIndexType span,GirderIndexType gdr,
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

void pgsGirderHandlingChecker::ComputeHaulingMoments(SpanIndexType span,GirderIndexType gdr,
                                                     const pgsHaulingAnalysisArtifact& rArtifact, 
                                                     const std::vector<pgsPointOfInterest>& rpoiVec,
                                                     std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection,Float64* pOverhangDeflection)
{
   // build a model
   Float64 glen = rArtifact.GetGirderLength();
   Float64 leftOH = rArtifact.GetTrailingOverhang();
   Float64 rightOH = rArtifact.GetLeadingOverhang();
   Float64 E = rArtifact.GetElasticModulusOfGirderConcrete();

   ComputeMoments(span, gdr,
                  pgsTypes::Hauling,
                  leftOH, glen, rightOH,
                  E,
                  rpoiVec,
                  pmomVec, pMidSpanDeflection, pOverhangDeflection);
}

void pgsGirderHandlingChecker::ComputeHaulingRollAngle(SpanIndexType span,GirderIndexType gdr,
                                                       pgsHaulingAnalysisArtifact* pArtifact, 
                                                       const std::vector<pgsPointOfInterest> rpoiVec,
                                                       std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection,Float64* pOverhangDeflection)
{
   Float64 fo = pArtifact->GetOffsetFactor();
   Float64 y = pArtifact->GetHeightOfCgOfGirderAboveRollCenter();
   
   Float64 kt = pArtifact->GetRollStiffnessOfvehicle();
   Float64 gird_len = pArtifact->GetGirderLength();
   Float64 gird_wt = pArtifact->GetGirderWeight();

   Float64 w = pArtifact->GetAvgGirderWeightPerLength();

   Float64 r = kt/gird_wt;
   pArtifact->SetRadiusOfStability(r);

   // Zo (based on mid-span section properties)
   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(span,gdr,pgsTypes::CastingYard,POI_MIDSPAN);
   pgsPointOfInterest poi = vPOI[0];
   GET_IFACE(ISectProp2,pSectProp2);
   Float64 Iy = pSectProp2->GetIy(pgsTypes::CastingYard,poi);
   Float64 span_len = pArtifact->GetClearSpanBetweenSupportLocations();
   Float64 Ec = pArtifact->GetElasticModulusOfGirderConcrete();
   Float64 a = (gird_len-span_len)/2.;
   Float64 l = span_len;

   Float64 zo = (w/(12*Ec*Iy*gird_len))*
                (l*l*l*l*l/10. - a*a*l*l*l + 3.*a*a*a*a*l + 6.*a*a*a*a*a/5.);

   pArtifact->SetZo(zo);
   pArtifact->SetIy(Iy);

   // roll angle
   Float64 ei = pArtifact->GetTotalInitialEccentricity();
   Float64 superelevation_angle = pArtifact->GetRoadwaySuperelevation();

   // if denominator for roll angle equation is negative, this means that the spring
   // stiffness cannot overcome the girder weight - unstable.
   Float64 denom = r-y-zo;
   if (denom<=0.0)
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);

      const char* msg = "Truck spring stiffness is inadequate - girder/trailer is unstable";
      pgsTruckStiffnessStatusItem* pStatusItem = new pgsTruckStiffnessStatusItem(m_StatusGroupID,m_scidTruckStiffness,msg);
      pStatusCenter->Add(pStatusItem);

      std::string str(msg);
      str += std::string("\nSee Status Center for details");
      THROW_UNWIND(str.c_str(),-1);
   }

   Float64 roll_angle = (superelevation_angle * r + ei) / (r-y-zo);
   pArtifact->SetEqualibriumAngle(roll_angle);
}


void pgsGirderHandlingChecker::GetRequirementsForAlternativeTensileStress(const pgsPointOfInterest& poi,Float64 ftu,Float64 ftd,Float64 fbu,Float64 fbd,Float64* pY,Float64* pA,Float64* pT,Float64* pAs)
{
    GET_IFACE(IGirder,pGirder);
    GET_IFACE(ISectProp2,pSectProp2);
    GET_IFACE(IBridgeMaterial,pMaterial);

    GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
    bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

    Float64 Es, fs;
    pMaterial->GetLongitudinalRebarProperties(span,gdr,&Es,&fs);
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
       At = pSectProp2->GetAg(pgsTypes::CastingYard,poi);
       Float64 fAvg = (fTop + fBot)/2;
       T = fAvg * At;
    }
    else
    {
       // Location of neutral axis from Bottom of Girder
//       Yna = (IsZero(fBot) ? 0 : H - (fTop*H/(fTop-fBot)) );

       CComPtr<IShape> shape;
       pSectProp2->GetGirderShape(poi,false,&shape);

       CComQIPtr<IXYPosition> position(shape);
       CComPtr<IPoint2d> tc,bc;
       position->get_LocatorPoint(lpTopCenter,   &tc);
       position->get_LocatorPoint(lpBottomCenter,&bc);

       double Xtop,Ytop;
       double Xbot,Ybot;
       tc->get_X(&Xtop);
       tc->get_Y(&Ytop);

       bc->get_X(&Xbot);
       bc->get_Y(&Ybot);

       double Wt, Wb;
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
       double ya,yb;
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

void pgsGirderHandlingChecker::ComputeMoments(SpanIndexType span,GirderIndexType gdr,
                                              pgsTypes::Stage stage,
                                              Float64 leftOH,Float64 glen,Float64 rightOH,
                                              Float64 E,
                                              const std::vector<pgsPointOfInterest>& rpoiVec,
                                              std::vector<Float64>* pmomVec, Float64* pMidSpanDeflection, Float64* pOverhangDeflection)
{
   m_Model.Release();

   // need left and right support locations measured from the left end of the girder
   Float64 leftSupportLocation = leftOH;
   Float64 rightSupportLocation = glen - rightOH;
   LoadCaseIDType lcid = 0;
   pgsGirderModelFactory::CreateGirderModel(m_pBroker,span,gdr,leftSupportLocation,rightSupportLocation,E,lcid,true,rpoiVec,&m_Model,&m_PoiMap);

   // Get results
   CComQIPtr<IFem2dModelResults> results(m_Model);
   pmomVec->clear();
   *pMidSpanDeflection = 0.0;
   bool found_mid = false;

   Float64 dx,dy,rz;
   if ( !IsZero(leftOH) )
   {
      HRESULT hr = results->ComputeJointDisplacements(lcid,0,&dx,&dy,&rz);
      ATLASSERT(SUCCEEDED(hr));
      *pOverhangDeflection = dy;
   }
   else
   {
      *pOverhangDeflection = 0.00;
   }

   for (std::vector<pgsPointOfInterest>::const_iterator poiIter = rpoiVec.begin(); poiIter != rpoiVec.end(); poiIter++)
   {
      const pgsPointOfInterest& poi = *poiIter;
      Float64 fx,fy,mz;
      PoiIDType femPoiID = m_PoiMap.GetModelPoi(poi);
      HRESULT hr = results->ComputePOIForces(0,femPoiID,mftLeft,lotMember,&fx,&fy,&mz);
      ATLASSERT(SUCCEEDED(hr));
      pmomVec->push_back(mz);


      if (poi.IsMidSpan(stage))
      {
         hr = results->ComputePOIDisplacements(0,femPoiID,lotMember,&dx,&dy,&rz);
         ATLASSERT(SUCCEEDED(hr));

         *pMidSpanDeflection = dy;
         found_mid=true;
      }
   }

   CHECK(found_mid); // must have a point at mid-span for calc to work right
}

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

#include "stdafx.h"
#include "LoadRater.h"
#include "Designer2.h"

#include <IFace\Project.h> // for ISpecification
#include <IFace\RatingSpecification.h>
#include <IFace\Bridge.h>
#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <IFace\CrackedSection.h>
#include <IFace\PrestressForce.h>
#include <IFace\DistributionFactors.h>
#include <IFace\Intervals.h>

#pragma Reminder("BUG: Load Rater doesn't work correctly")
// The load rater is supposed to do a load rating along an entire girder line (first to last pier)
// When the girder line index was passed, it did that. Now that we are using GirderIDType for
// a single girder that can be made of 1 (precast girder bridge) or more (spliced girder bridge)
// segments, this function only details with a single superstructure member and it is assumed
// it has only one segment
//
// GetPointsOfInterest is using a dummy parameter (ALL_SEGMENTS) just so I can get
// this thing to compile.

#define RF_MAX 9999999
Float64 sum_values(Float64 a,Float64 b) { return a + b; }
void special_transform(IBridge* pBridge,IPointOfInterest* pPoi,IIntervals* pIntervals,
                       std::vector<pgsPointOfInterest>::const_iterator poiBeginIter,
                       std::vector<pgsPointOfInterest>::const_iterator poiEndIter,
                       std::vector<Float64>::iterator forceBeginIter,
                       std::vector<Float64>::iterator resultBeginIter,
                       std::vector<Float64>::iterator outputBeginIter);

bool AxleHasWeight(AxlePlacement& placement)
{
   return !IsZero(placement.Weight);
}

pgsLoadRater::pgsLoadRater(void)
{
   CREATE_LOGFILE("LoadRating");
}

pgsLoadRater::~pgsLoadRater(void)
{
   CLOSE_LOGFILE;
}

void pgsLoadRater::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;
}

pgsRatingArtifact pgsLoadRater::Rate(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx)
{
   GET_IFACE(IRatingSpecification,pRatingSpec);

   pgsRatingArtifact ratingArtifact;

   // Rate for positive moment - flexure
   MomentRating(girderKey,true,ratingType,vehicleIdx,ratingArtifact);

   // Rate for negative moment - flexure, if applicable
   GET_IFACE(IBridge,pBridge);
   if ( pBridge->ProcessNegativeMoments(ALL_SPANS) )
      MomentRating(girderKey,false,ratingType,vehicleIdx,ratingArtifact);

   // Rate for shear if applicable
   if ( pRatingSpec->RateForShear(ratingType) )
      ShearRating(girderKey,ratingType,vehicleIdx,ratingArtifact);

   // Rate for stress if applicable
   if ( pRatingSpec->RateForStress(ratingType) )
   {
      if ( ratingType == pgsTypes::lrPermit_Routine || ratingType == pgsTypes::lrPermit_Special )
      {
         // Service I reinforcement yield check if permit rating
         CheckReinforcementYielding(girderKey,ratingType,vehicleIdx,true,ratingArtifact);

         if ( pBridge->ProcessNegativeMoments(ALL_SPANS) )
            CheckReinforcementYielding(girderKey,ratingType,vehicleIdx,false,ratingArtifact);
      }
      else
      {
         // Service III flexure if other rating type
         StressRating(girderKey,ratingType,vehicleIdx,ratingArtifact);
      }
   }

   return ratingArtifact;
}

void pgsLoadRater::MomentRating(const CGirderKey& girderKey,bool bPositiveMoment,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,pgsRatingArtifact& ratingArtifact)
{
   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)) );

   std::vector<Float64> vDCmin, vDCmax;
   std::vector<Float64> vDWmin, vDWmax;
   std::vector<Float64> vLLIMmin,vLLIMmax;
   std::vector<Float64> vPLmin,vPLmax;
   std::vector<VehicleIndexType> vMinTruckIndex, vMaxTruckIndex;
   GetMoments(girderKey,bPositiveMoment,ratingType, vehicleIdx, vPOI, vDCmin, vDCmax, vDWmin, vDWmax, vLLIMmin, vMinTruckIndex, vLLIMmax, vMaxTruckIndex, vPLmin, vPLmax);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE(IMomentCapacity,pMomentCapacity);
   std::vector<MOMENTCAPACITYDETAILS> vM = pMomentCapacity->GetMomentCapacityDetails(liveLoadIntervalIdx,vPOI,bPositiveMoment);
   std::vector<MINMOMENTCAPDETAILS> vMmin = pMomentCapacity->GetMinMomentCapacityDetails(liveLoadIntervalIdx,vPOI,bPositiveMoment);

   ATLASSERT(vPOI.size()     == vDCmax.size());
   ATLASSERT(vPOI.size()     == vDWmax.size());
   ATLASSERT(vPOI.size()     == vLLIMmax.size());
   ATLASSERT(vPOI.size()     == vM.size());
   ATLASSERT(vDCmin.size()   == vDCmax.size());
   ATLASSERT(vDWmin.size()   == vDWmax.size());
   ATLASSERT(vLLIMmin.size() == vLLIMmax.size());

   GET_IFACE(IRatingSpecification,pRatingSpec);
   Float64 system_factor    = pRatingSpec->GetSystemFactorFlexure();
   bool bIncludePL = pRatingSpec->IncludePedestrianLiveLoad();

   pgsTypes::LimitState ls = GetStrengthLimitStateType(ratingType);

   Float64 gDC = pRatingSpec->GetDeadLoadFactor(ls);
   Float64 gDW = pRatingSpec->GetWearingSurfaceFactor(ls);

   GET_IFACE(IProductLoads,pProductLoads);
   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);
   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

   GET_IFACE(IProductForces,pProductForces);
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   CollectionIndexType nPOI = vPOI.size();
   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      pgsPointOfInterest& poi = vPOI[i];

      Float64 condition_factor = (bPositiveMoment ? pRatingSpec->GetGirderConditionFactor(poi.GetSegmentKey()) 
                                                  : pRatingSpec->GetDeckConditionFactor() );

      Float64 DC   = (bPositiveMoment ? vDCmax[i]   : vDCmin[i]);
      Float64 DW   = (bPositiveMoment ? vDWmax[i]   : vDWmin[i]);
      Float64 LLIM = (bPositiveMoment ? vLLIMmax[i] : vLLIMmin[i]);
      Float64 PL   = (bIncludePL ? (bPositiveMoment ? vPLmax[i]   : vPLmin[i]) : 0.0);

      VehicleIndexType truck_index = vehicleIdx;
      if ( vehicleIdx == INVALID_INDEX )
         truck_index = (bPositiveMoment ? vMaxTruckIndex[i] : vMinTruckIndex[i]);

      Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
      if ( gLL < 0 )
      {
         if ( ::IsStrengthLimitState(ls) )
         {
            Float64 Mmin, Mmax, Dummy;
            AxleConfiguration MinAxleConfig, MaxAxleConfig, DummyAxleConfig;
            if ( analysisType == pgsTypes::Envelope )
            {
               pProductForces->GetVehicularLiveLoadMoment(llType,truck_index,liveLoadIntervalIdx,poi,pgsTypes::MinSimpleContinuousEnvelope,true,true,&Mmin,&Dummy,&MinAxleConfig,&DummyAxleConfig);
               pProductForces->GetVehicularLiveLoadMoment(llType,truck_index,liveLoadIntervalIdx,poi,pgsTypes::MaxSimpleContinuousEnvelope,true,true,&Dummy,&Mmax,&DummyAxleConfig,&MaxAxleConfig);
            }
            else
            {
               pProductForces->GetVehicularLiveLoadMoment(llType,truck_index,liveLoadIntervalIdx,poi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,&Mmin,&Mmax,&MinAxleConfig,&MaxAxleConfig);
            }
            gLL = GetStrengthLiveLoadFactor(ratingType,bPositiveMoment ? MaxAxleConfig : MinAxleConfig);
         }
         else
         {
            gLL = GetServiceLiveLoadFactor(ratingType);
         }
      }

      Float64 phi_moment = vM[i].Phi; 
      Float64 Mn = vM[i].Mn;

      // NOTE: K can be less than zero when we are rating for negative moment and the minumum moment demand (Mu)
      // is positive. This happens near the simple ends of spans. For example Mr < 0 because we are rating for
      // negative moment and Mmin = min (1.2Mcr and 1.33Mu)... Mcr < 0 because we are looking at negative moment
      // and Mu > 0.... Since we are looking at the negative end of things, Mmin = 1.33Mu. +/- = -... it doesn't
      // make since for K to be negative... K < 0 indicates that the section is most definate NOT under reinforced.
      // No adjustment needs to be made for underreinforcement so take K = 1.0
      Float64 K = (IsZero(vMmin[i].MrMin) ? 1.0 : vMmin[i].Mr/vMmin[i].MrMin);
      if ( K < 0.0 || 1.0 < K )
         K = 1.0;


      std::_tstring strVehicleName = strLLNames[truck_index];
      Float64 W = pProductLoads->GetVehicleWeight(llType,truck_index);

      pgsMomentRatingArtifact momentArtifact;
      momentArtifact.SetRatingType(ratingType);
      momentArtifact.SetPointOfInterest(poi);
      momentArtifact.SetVehicleIndex(truck_index);
      momentArtifact.SetVehicleWeight(W);
      momentArtifact.SetVehicleName(strVehicleName.c_str());
      momentArtifact.SetSystemFactor(system_factor);
      momentArtifact.SetConditionFactor(condition_factor);
      momentArtifact.SetCapacityReductionFactor(phi_moment);
      momentArtifact.SetMinimumReinforcementFactor(K);
      momentArtifact.SetNominalMomentCapacity(Mn);
      momentArtifact.SetDeadLoadFactor(gDC);
      momentArtifact.SetDeadLoadMoment(DC);
      momentArtifact.SetWearingSurfaceFactor(gDW);
      momentArtifact.SetWearingSurfaceMoment(DW);
      momentArtifact.SetLiveLoadFactor(gLL);
      momentArtifact.SetLiveLoadMoment(LLIM+PL);

      ratingArtifact.AddArtifact(poi,momentArtifact,bPositiveMoment);
   }
}

void pgsLoadRater::ShearRating(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,pgsRatingArtifact& ratingArtifact)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)) );

   std::vector<sysSectionValue> vDCmin, vDCmax;
   std::vector<sysSectionValue> vDWmin, vDWmax;
   std::vector<sysSectionValue> vLLIMmin,vLLIMmax;
   std::vector<sysSectionValue> vUnused;
   std::vector<VehicleIndexType> vMinTruckIndex, vMaxTruckIndex, vUnusedIndex;
   std::vector<sysSectionValue> vPLmin, vPLmax;

   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);

   GET_IFACE(ICombinedForces2,pCombinedForces);
   GET_IFACE(IProductForces2,pProductForces);
   if ( analysisType == pgsTypes::Envelope )
   {
      vDCmin = pCombinedForces->GetShear(lcDC,liveLoadIntervalIdx,vPOI,ctCummulative,pgsTypes::MinSimpleContinuousEnvelope);
      vDCmax = pCombinedForces->GetShear(lcDC,liveLoadIntervalIdx,vPOI,ctCummulative,pgsTypes::MaxSimpleContinuousEnvelope);
      vDWmin = pCombinedForces->GetShear(lcDW,liveLoadIntervalIdx,vPOI,ctCummulative,pgsTypes::MinSimpleContinuousEnvelope);
      vDWmax = pCombinedForces->GetShear(lcDW,liveLoadIntervalIdx,vPOI,ctCummulative,pgsTypes::MaxSimpleContinuousEnvelope);

      if ( vehicleIdx == INVALID_INDEX )
      {
         pProductForces->GetLiveLoadShear( llType, liveLoadIntervalIdx, vPOI, pgsTypes::MinSimpleContinuousEnvelope, true, true, &vLLIMmin, &vUnused, &vMinTruckIndex, &vUnusedIndex );
         pProductForces->GetLiveLoadShear( llType, liveLoadIntervalIdx, vPOI, pgsTypes::MaxSimpleContinuousEnvelope, true, true, &vUnused, &vLLIMmax, &vUnusedIndex, &vMaxTruckIndex );
      }
      else
      {
         pProductForces->GetVehicularLiveLoadShear( llType, vehicleIdx, liveLoadIntervalIdx, vPOI, pgsTypes::MinSimpleContinuousEnvelope, true, true, &vLLIMmin, &vUnused, NULL,NULL,NULL,NULL);
         pProductForces->GetVehicularLiveLoadShear( llType, vehicleIdx, liveLoadIntervalIdx, vPOI, pgsTypes::MaxSimpleContinuousEnvelope, true, true, &vUnused, &vLLIMmax, NULL,NULL,NULL,NULL);
      }

      pCombinedForces->GetCombinedLiveLoadShear( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPOI, pgsTypes::MaxSimpleContinuousEnvelope, false, &vUnused, &vPLmax );
      pCombinedForces->GetCombinedLiveLoadShear( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPOI, pgsTypes::MinSimpleContinuousEnvelope, false, &vPLmin, &vUnused );
   }
   else
   {
      vDCmax = pCombinedForces->GetShear(lcDC,liveLoadIntervalIdx,vPOI,ctCummulative,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
      vDCmin = vDCmax;
      vDWmax = pCombinedForces->GetShear(lcDW,liveLoadIntervalIdx,vPOI,ctCummulative,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
      vDWmin = vDWmax;

      if ( vehicleIdx == INVALID_INDEX )
         pProductForces->GetLiveLoadShear( llType, liveLoadIntervalIdx, vPOI, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, true, &vLLIMmin, &vLLIMmax, &vMinTruckIndex, &vMaxTruckIndex );
      else
         pProductForces->GetVehicularLiveLoadShear( llType, vehicleIdx, liveLoadIntervalIdx, vPOI, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, true, &vLLIMmin, &vLLIMmax, NULL, NULL, NULL, NULL);

      pCombinedForces->GetCombinedLiveLoadShear( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPOI, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, false, &vPLmin, &vPLmax );
   }

   pgsTypes::LimitState ls = GetStrengthLimitStateType(ratingType);

   GET_IFACE(IShearCapacity,pShearCapacity);
   std::vector<SHEARCAPACITYDETAILS> vVn = pShearCapacity->GetShearCapacityDetails(ls,liveLoadIntervalIdx,vPOI);

   ATLASSERT(vPOI.size()     == vDCmax.size());
   ATLASSERT(vPOI.size()     == vDWmax.size());
   ATLASSERT(vPOI.size()     == vLLIMmax.size());
   ATLASSERT(vPOI.size()     == vVn.size());
   ATLASSERT(vDCmin.size()   == vDCmax.size());
   ATLASSERT(vDWmin.size()   == vDWmax.size());
   ATLASSERT(vLLIMmin.size() == vLLIMmax.size());
   ATLASSERT(vPLmin.size()   == vPLmax.size());

   GET_IFACE(IRatingSpecification,pRatingSpec);
   Float64 system_factor    = pRatingSpec->GetSystemFactorShear();
   bool bIncludePL = pRatingSpec->IncludePedestrianLiveLoad();

   Float64 gDC = pRatingSpec->GetDeadLoadFactor(ls);
   Float64 gDW = pRatingSpec->GetWearingSurfaceFactor(ls);

   GET_IFACE(IProductLoads,pProductLoads);
   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

   GET_IFACE(IProductForces,pProductForce);

   CollectionIndexType nPOI = vPOI.size();
   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      pgsPointOfInterest& poi = vPOI[i];

      Float64 condition_factor = pRatingSpec->GetGirderConditionFactor(poi.GetSegmentKey());

      Float64 DCmin   = Min(vDCmin[i].Left(),  vDCmin[i].Right());
      Float64 DCmax   = Max(vDCmax[i].Left(),  vDCmax[i].Right());
      Float64 DWmin   = Min(vDWmin[i].Left(),  vDWmin[i].Right());
      Float64 DWmax   = Max(vDWmax[i].Left(),  vDWmax[i].Right());
      Float64 LLIMmin = Min(vLLIMmin[i].Left(),vLLIMmin[i].Right());
      Float64 LLIMmax = Max(vLLIMmax[i].Left(),vLLIMmax[i].Right());
      Float64 PLmin   = Min(vPLmin[i].Left(),  vPLmin[i].Right());
      Float64 PLmax   = Max(vPLmax[i].Left(),  vPLmax[i].Right());

      Float64 DC   = Max(fabs(DCmin),fabs(DCmax));
      Float64 DW   = Max(fabs(DWmin),fabs(DWmax));
      Float64 LLIM = Max(fabs(LLIMmin),fabs(LLIMmax));
      Float64 PL   = (bIncludePL ? Max(fabs(PLmin),fabs(PLmax)) : 0);
      VehicleIndexType truck_index = vehicleIdx;
      if ( vehicleIdx == INVALID_INDEX )
         truck_index = (fabs(LLIMmin) < fabs(LLIMmax) ? vMaxTruckIndex[i] : vMinTruckIndex[i]);

      Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
      if ( gLL < 0 )
      {
         // need to compute gLL based on axle weights
         if ( ::IsStrengthLimitState(ls) )
         {
            sysSectionValue Vmin, Vmax, Dummy;
            AxleConfiguration MinLeftAxleConfig, MaxLeftAxleConfig, MinRightAxleConfig, MaxRightAxleConfig, DummyLeftAxleConfig, DummyRightAxleConfig;
            if ( analysisType == pgsTypes::Envelope )
            {
               pProductForce->GetVehicularLiveLoadShear(llType,truck_index,liveLoadIntervalIdx,poi,pgsTypes::MinSimpleContinuousEnvelope,true,true,&Vmin,&Dummy,&MinLeftAxleConfig,&MinRightAxleConfig,&DummyLeftAxleConfig,&DummyRightAxleConfig);
               pProductForce->GetVehicularLiveLoadShear(llType,truck_index,liveLoadIntervalIdx,poi,pgsTypes::MaxSimpleContinuousEnvelope,true,true,&Dummy,&Vmax,&DummyLeftAxleConfig,&DummyRightAxleConfig,&MaxLeftAxleConfig,&MaxRightAxleConfig);
            }
            else
            {
               pProductForce->GetVehicularLiveLoadShear(llType,truck_index,liveLoadIntervalIdx,poi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,&Vmin,&Vmax,&MinLeftAxleConfig,&MinRightAxleConfig,&MaxLeftAxleConfig,&MaxRightAxleConfig);
            }

            if ( fabs(LLIMmin) < fabs(LLIMmax) )
            {
               if (IsEqual(fabs(vLLIMmax[i].Left()),fabs(LLIMmax)))
                  gLL = GetStrengthLiveLoadFactor(ratingType,MaxLeftAxleConfig);
               else
                  gLL = GetStrengthLiveLoadFactor(ratingType,MaxRightAxleConfig);
            }
            else
            {
               if (IsEqual(fabs(vLLIMmin[i].Left()),fabs(LLIMmin)))
                  gLL = GetStrengthLiveLoadFactor(ratingType,MinLeftAxleConfig);
               else
                  gLL = GetStrengthLiveLoadFactor(ratingType,MinRightAxleConfig);
            }
         }
         else
         {
            gLL = GetServiceLiveLoadFactor(ratingType);
         }
      }

      Float64 phi_shear = vVn[i].Phi; 
      Float64 Vn = vVn[i].Vn;

      std::_tstring strVehicleName = strLLNames[truck_index];
      Float64 W = pProductLoads->GetVehicleWeight(llType,truck_index);

      pgsShearRatingArtifact shearArtifact;
      shearArtifact.SetRatingType(ratingType);
      shearArtifact.SetPointOfInterest(poi);
      shearArtifact.SetVehicleIndex(truck_index);
      shearArtifact.SetVehicleWeight(W);
      shearArtifact.SetVehicleName(strVehicleName.c_str());
      shearArtifact.SetSystemFactor(system_factor);
      shearArtifact.SetConditionFactor(condition_factor);
      shearArtifact.SetCapacityReductionFactor(phi_shear);
      shearArtifact.SetNominalShearCapacity(Vn);
      shearArtifact.SetDeadLoadFactor(gDC);
      shearArtifact.SetDeadLoadShear(DC);
      shearArtifact.SetWearingSurfaceFactor(gDW);
      shearArtifact.SetWearingSurfaceShear(DW);
      shearArtifact.SetLiveLoadFactor(gLL);
      shearArtifact.SetLiveLoadShear(LLIM+PL);

      // longitudinal steel check
      pgsLongReinfShearArtifact l_artifact;
      SHEARCAPACITYDETAILS scd;
      pgsDesigner2 designer;
      designer.SetBroker(m_pBroker);
      std::vector<CRITSECTDETAILS> vCSDetails = pShearCapacity->GetCriticalSectionDetails(ls,poi.GetSegmentKey());
      pShearCapacity->GetShearCapacityDetails(ls,liveLoadIntervalIdx,poi,&scd);
      designer.InitShearCheck(poi.GetSegmentKey(),liveLoadIntervalIdx,ls,vCSDetails,NULL);
      designer.CheckLongReinfShear(poi,liveLoadIntervalIdx,ls,scd,NULL,&l_artifact);
      shearArtifact.SetLongReinfShearArtifact(l_artifact);

      ratingArtifact.AddArtifact(poi,shearArtifact);
   }
}

void pgsLoadRater::StressRating(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,pgsRatingArtifact& ratingArtifact)
{
   ATLASSERT(ratingType == pgsTypes::lrDesign_Inventory || 
             ratingType == pgsTypes::lrLegal_Routine    ||
             ratingType == pgsTypes::lrLegal_Special ); // see MBE C6A.5.4.1


   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)) );

   std::vector<Float64> vDCTopMin, vDCBotMin, vDCTopMax, vDCBotMax;
   std::vector<Float64> vDWTopMin, vDWBotMin, vDWTopMax, vDWBotMax;
   std::vector<Float64> vLLIMTopMin, vLLIMBotMin, vLLIMTopMax, vLLIMBotMax;
   std::vector<Float64> vUnused1,vUnused2;
   std::vector<VehicleIndexType> vTruckIndexTopMin, vTruckIndexTopMax, vTruckIndexBotMin, vTruckIndexBotMax;
   std::vector<VehicleIndexType> vUnusedIndex1, vUnusedIndex2;
   std::vector<Float64> vPLTopMin, vPLBotMin, vPLTopMax, vPLBotMax;

   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);

   pgsTypes::StressLocation topLocation = pgsTypes::TopGirder;
   pgsTypes::StressLocation botLocation = pgsTypes::BottomGirder;

   GET_IFACE(ICombinedForces2,pCombinedForces);
   GET_IFACE(IProductForces2,pProductForces);
   if ( analysisType == pgsTypes::Envelope )
   {
      pCombinedForces->GetStress(lcDC,liveLoadIntervalIdx,vPOI,ctCummulative,pgsTypes::MinSimpleContinuousEnvelope,&vDCTopMin,&vDCBotMin);
      pCombinedForces->GetStress(lcDC,liveLoadIntervalIdx,vPOI,ctCummulative,pgsTypes::MaxSimpleContinuousEnvelope,&vDCTopMax,&vDCBotMax);
      pCombinedForces->GetStress(lcDW,liveLoadIntervalIdx,vPOI,ctCummulative,pgsTypes::MinSimpleContinuousEnvelope,&vDWTopMin,&vDWBotMin);
      pCombinedForces->GetStress(lcDW,liveLoadIntervalIdx,vPOI,ctCummulative,pgsTypes::MaxSimpleContinuousEnvelope,&vDWTopMax,&vDWBotMax);

      if ( vehicleIdx == INVALID_INDEX )
      {
         pProductForces->GetLiveLoadStress(llType,liveLoadIntervalIdx,vPOI,pgsTypes::MinSimpleContinuousEnvelope,true,true,topLocation,botLocation,&vLLIMTopMin,&vUnused1,&vLLIMBotMin,&vUnused2,&vTruckIndexTopMin, &vUnusedIndex1, &vTruckIndexBotMin, &vUnusedIndex2);
         pProductForces->GetLiveLoadStress(llType,liveLoadIntervalIdx,vPOI,pgsTypes::MaxSimpleContinuousEnvelope,true,true,topLocation,botLocation,&vUnused1,&vLLIMTopMax,&vUnused2,&vLLIMBotMax,&vUnusedIndex1, &vTruckIndexTopMax, &vUnusedIndex2, &vTruckIndexBotMax);
      }
      else
      {
         pProductForces->GetVehicularLiveLoadStress(llType,vehicleIdx,liveLoadIntervalIdx,vPOI,pgsTypes::MinSimpleContinuousEnvelope,true,true,topLocation,botLocation,&vLLIMTopMin,&vUnused1,&vLLIMBotMin,&vUnused2,NULL,NULL,NULL,NULL);
         pProductForces->GetVehicularLiveLoadStress(llType,vehicleIdx,liveLoadIntervalIdx,vPOI,pgsTypes::MaxSimpleContinuousEnvelope,true,true,topLocation,botLocation,&vUnused1,&vLLIMTopMax,&vUnused2,&vLLIMBotMax,NULL,NULL,NULL,NULL);
      }

      pCombinedForces->GetCombinedLiveLoadStress( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPOI, pgsTypes::MaxSimpleContinuousEnvelope, &vUnused1,  &vPLTopMax, &vUnused2, &vPLBotMax );
      pCombinedForces->GetCombinedLiveLoadStress( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPOI, pgsTypes::MinSimpleContinuousEnvelope, &vPLTopMin, &vUnused2,  &vPLBotMin, &vUnused2 );
   }
   else
   {
      pCombinedForces->GetStress(lcDC,liveLoadIntervalIdx,vPOI,ctCummulative,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,&vDCTopMin,&vDCBotMin);
      vDCTopMax = vDCTopMin;
      vDCBotMax = vDCBotMin;

      pCombinedForces->GetStress(lcDW,liveLoadIntervalIdx,vPOI,ctCummulative,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,&vDWTopMin,&vDWBotMin);
      vDWTopMax = vDWTopMin;
      vDWBotMax = vDWBotMin;

      if ( vehicleIdx == INVALID_INDEX )
         pProductForces->GetLiveLoadStress(llType,liveLoadIntervalIdx,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,topLocation,botLocation,&vLLIMTopMin,&vLLIMTopMax,&vLLIMBotMin,&vLLIMBotMax,&vTruckIndexTopMin, &vTruckIndexTopMax, &vTruckIndexBotMin, &vTruckIndexBotMax);
      else
         pProductForces->GetVehicularLiveLoadStress(llType,vehicleIdx,liveLoadIntervalIdx,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,topLocation,botLocation,&vLLIMTopMin,&vLLIMTopMax,&vLLIMBotMin,&vLLIMBotMax,NULL,NULL,NULL,NULL);

      pCombinedForces->GetCombinedLiveLoadStress( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPOI, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &vPLTopMin, &vPLTopMax,  &vPLBotMin, &vPLBotMax );
   }

   ATLASSERT(vPOI.size()     == vDCTopMin.size());
   ATLASSERT(vPOI.size()     == vDCTopMax.size());
   ATLASSERT(vPOI.size()     == vDCBotMin.size());
   ATLASSERT(vPOI.size()     == vDCBotMax.size());
   ATLASSERT(vPOI.size()     == vDWTopMin.size());
   ATLASSERT(vPOI.size()     == vDWTopMax.size());
   ATLASSERT(vPOI.size()     == vDWBotMin.size());
   ATLASSERT(vPOI.size()     == vDWBotMax.size());
   ATLASSERT(vPOI.size()     == vLLIMTopMin.size());
   ATLASSERT(vPOI.size()     == vLLIMTopMax.size());
   ATLASSERT(vPOI.size()     == vLLIMBotMin.size());
   ATLASSERT(vPOI.size()     == vLLIMBotMax.size());
   ATLASSERT(vPOI.size()     == vPLTopMin.size());
   ATLASSERT(vPOI.size()     == vPLTopMax.size());
   ATLASSERT(vPOI.size()     == vPLBotMin.size());
   ATLASSERT(vPOI.size()     == vPLBotMax.size());

   GET_IFACE(IPretensionStresses,pPrestress);
   std::vector<Float64> vPS = pPrestress->GetStress(liveLoadIntervalIdx,vPOI,pgsTypes::BottomGirder);

   GET_IFACE(IRatingSpecification,pRatingSpec);

   Float64 system_factor    = pRatingSpec->GetSystemFactorFlexure();
   bool bIncludePL = pRatingSpec->IncludePedestrianLiveLoad();

   pgsTypes::LimitState ls = GetServiceLimitStateType(ratingType);

   Float64 gDC = pRatingSpec->GetDeadLoadFactor(ls);
   Float64 gDW = pRatingSpec->GetWearingSurfaceFactor(ls);

   GET_IFACE(IProductLoads,pProductLoads);
   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

   GET_IFACE(IProductForces,pProductForce);

   CollectionIndexType nPOI = vPOI.size();
   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      pgsPointOfInterest& poi = vPOI[i];

      Float64 condition_factor = pRatingSpec->GetGirderConditionFactor(poi.GetSegmentKey());
      Float64 fr               = pRatingSpec->GetAllowableTension(ratingType,poi.GetSegmentKey());

      Float64 DC   = vDCBotMax[i];
      Float64 DW   = vDWBotMax[i];
      Float64 LLIM = vLLIMBotMax[i];
      Float64 PL   = (bIncludePL ? vPLBotMax[i] : 0);
      Float64 PS   = vPS[i];

      VehicleIndexType truck_index = vehicleIdx;
      if ( vehicleIdx == INVALID_INDEX )
         truck_index = vTruckIndexBotMax[i];

      Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
      if ( gLL < 0 )
      {
         if ( ::IsStrengthLimitState(ls) )
         {
            // need to compute gLL based on axle weights
            Float64 fMinTop, fMinBot, fMaxTop, fMaxBot, fDummyTop, fDummyBot;
            AxleConfiguration MinAxleConfigTop, MinAxleConfigBot, MaxAxleConfigTop, MaxAxleConfigBot, DummyAxleConfigTop, DummyAxleConfigBot;
            if ( analysisType == pgsTypes::Envelope )
            {
               pProductForce->GetVehicularLiveLoadStress(llType,truck_index,liveLoadIntervalIdx,poi,pgsTypes::MinSimpleContinuousEnvelope,true,true,topLocation,botLocation,&fMinTop,&fDummyTop,&fMinBot,&fDummyBot,&MinAxleConfigTop,&DummyAxleConfigTop,&MinAxleConfigBot,&DummyAxleConfigBot);
               pProductForce->GetVehicularLiveLoadStress(llType,truck_index,liveLoadIntervalIdx,poi,pgsTypes::MaxSimpleContinuousEnvelope,true,true,topLocation,botLocation,&fDummyTop,&fMaxTop,&fDummyBot,&fMaxBot,&DummyAxleConfigTop,&MaxAxleConfigTop,&DummyAxleConfigBot,&MaxAxleConfigBot);
            }
            else
            {
               pProductForce->GetVehicularLiveLoadStress(llType,truck_index,liveLoadIntervalIdx,poi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,topLocation,botLocation,&fMinTop,&fMaxTop,&fMaxTop,&fMaxBot,&MinAxleConfigTop,&MaxAxleConfigTop,&MinAxleConfigBot,&MaxAxleConfigBot);
            }

            gLL = GetStrengthLiveLoadFactor(ratingType,MaxAxleConfigBot);
         }
         else
         {
            gLL = GetServiceLiveLoadFactor(ratingType);
         }
      }


      std::_tstring strVehicleName = strLLNames[truck_index];
      Float64 W = pProductLoads->GetVehicleWeight(llType,truck_index);

      pgsStressRatingArtifact stressArtifact;
      stressArtifact.SetRatingType(ratingType);
      stressArtifact.SetPointOfInterest(poi);
      stressArtifact.SetVehicleIndex(truck_index);
      stressArtifact.SetVehicleWeight(W);
      stressArtifact.SetVehicleName(strVehicleName.c_str());
      stressArtifact.SetAllowableStress(fr);
      stressArtifact.SetDeadLoadFactor(gDC);
      stressArtifact.SetDeadLoadStress(DC);
      stressArtifact.SetPrestressStress(PS);
      stressArtifact.SetWearingSurfaceFactor(gDW);
      stressArtifact.SetWearingSurfaceStress(DW);
      stressArtifact.SetLiveLoadFactor(gLL);
      stressArtifact.SetLiveLoadStress(LLIM+PL);
   
      ratingArtifact.AddArtifact(poi,stressArtifact);
   }
}

void pgsLoadRater::CheckReinforcementYielding(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,bool bPositiveMoment,pgsRatingArtifact& ratingArtifact)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   ATLASSERT(ratingType == pgsTypes::lrPermit_Routine || ratingType == pgsTypes::lrPermit_Special);
   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)) );

   std::vector<Float64> vDCmin, vDCmax;
   std::vector<Float64> vDWmin, vDWmax;
   std::vector<Float64> vLLIMmin,vLLIMmax;
   std::vector<VehicleIndexType> vMinTruckIndex, vMaxTruckIndex;
   std::vector<Float64> vPLmin,vPLmax;
   GetMoments(girderKey,bPositiveMoment,ratingType, vehicleIdx, vPOI, vDCmin, vDCmax, vDWmin, vDWmax, vLLIMmin, vMinTruckIndex, vLLIMmax, vMaxTruckIndex, vPLmin, vPLmax);

   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);
   pgsTypes::LimitState ls = GetServiceLimitStateType(ratingType);

   GET_IFACE(IRatingSpecification,pRatingSpec);
   bool bIncludePL = pRatingSpec->IncludePedestrianLiveLoad();

   GET_IFACE(IMomentCapacity,pMomentCapacity);
   std::vector<CRACKINGMOMENTDETAILS> vMcr = pMomentCapacity->GetCrackingMomentDetails(liveLoadIntervalIdx,vPOI,bPositiveMoment);
   std::vector<MOMENTCAPACITYDETAILS> vM = pMomentCapacity->GetMomentCapacityDetails(liveLoadIntervalIdx,vPOI,bPositiveMoment);

   GET_IFACE(ICrackedSection,pCrackedSection);
   std::vector<CRACKEDSECTIONDETAILS> vCrackedSection = pCrackedSection->GetCrackedSectionDetails(vPOI,bPositiveMoment);
   
   ATLASSERT(vPOI.size()     == vDCmax.size());
   ATLASSERT(vPOI.size()     == vDWmax.size());
   ATLASSERT(vPOI.size()     == vLLIMmax.size());
   ATLASSERT(vPOI.size()     == vMcr.size());
   ATLASSERT(vPOI.size()     == vM.size());
   ATLASSERT(vPOI.size()     == vCrackedSection.size());
   ATLASSERT(vDCmin.size()   == vDCmax.size());
   ATLASSERT(vDWmin.size()   == vDWmax.size());
   ATLASSERT(vLLIMmin.size() == vLLIMmax.size());
   ATLASSERT(vPLmin.size()   == vPLmax.size());


   // Get load factors
   Float64 gDC = pRatingSpec->GetDeadLoadFactor(ls);
   Float64 gDW = pRatingSpec->GetWearingSurfaceFactor(ls);

   // parameter needed for negative moment evalation
   // (get it outside the loop so we don't have to get it over and over)
   GET_IFACE(ILongRebarGeometry,pLongRebar);
   Float64 top_slab_cover = pLongRebar->GetCoverTopMat();

   GET_IFACE(IProductLoads,pProductLoads);
   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

   GET_IFACE(IProductForces,pProductForces);
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   pgsTypes::StressLocation topLocation = pgsTypes::TopGirder;
   pgsTypes::StressLocation botLocation = pgsTypes::BottomGirder;

   // Create artifacts
   GET_IFACE(IPretensionForce,pPrestressForce);
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
   CollectionIndexType nPOI = vPOI.size();
   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      pgsPointOfInterest& poi = vPOI[i];

      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      // Get material properties
      GET_IFACE(IMaterials,pMaterials);
      Float64 Eg = pMaterials->GetSegmentEc(segmentKey,liveLoadIntervalIdx);
      Float64 Es, fy, fu;
      if ( bPositiveMoment )
      {
         Es = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Permanent)->GetE();
         fy = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Permanent)->GetYieldStrength();
      }
      else
      {
         pMaterials->GetDeckRebarProperties(&Es,&fy,&fu);
      }

      // Get allowable
      Float64 K = pRatingSpec->GetYieldStressLimitCoefficient();
      Float64 fr = K*fy; // 6A.5.4.2.2b 

      if ( ratingType == pgsTypes::lrPermit_Special ) // if it is any of the special permit types
      {
         // The live load distribution factor used for special permits is one loaded lane without multiple presense factor.
         // However, when evaluating the reinforcement yielding in the Service I limit state (the thing this function does)
         // the controlling of one loaded lane and two or more loaded lanes live load distribution factors is to be used.
         // (See MBE 6A.4.5.4.2b and C6A.5.4.2.2b).
         //
         // vLLIMmin and vLLIMmax includes the one lane LLDF... divide out this LLDF and multiply by the correct LLDF

         Float64 gpM_Old, gnM_Old, gV;
         Float64 gpM_New, gnM_New;
         pLLDF->GetDistributionFactors(poi,pgsTypes::FatigueI,              &gpM_Old,&gnM_Old,&gV);
         pLLDF->GetDistributionFactors(poi,pgsTypes::ServiceI_PermitSpecial,&gpM_New,&gnM_New,&gV);

         if ( bPositiveMoment )
         {
            Float64 g = gpM_New/gpM_Old;
            vLLIMmax[i] *= g;
         }
         else
         {
            Float64 g = gnM_New/gnM_Old;
            vLLIMmin[i] *= g;
         }
      }

      Float64 DC   = (bPositiveMoment ? vDCmax[i]   : vDCmin[i]);
      Float64 DW   = (bPositiveMoment ? vDWmax[i]   : vDWmin[i]);
      Float64 LLIM = (bPositiveMoment ? vLLIMmax[i] : vLLIMmin[i]);
      Float64 PL   = (bIncludePL ? (bPositiveMoment ? vPLmax[i]   : vPLmin[i]) : 0.0);
      VehicleIndexType truck_index = vehicleIdx;
      if ( vehicleIdx == INVALID_INDEX )
         truck_index = (bPositiveMoment ? vMaxTruckIndex[i] : vMinTruckIndex[i]);


      Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
      if ( gLL < 0 )
      {
         if ( ::IsStrengthLimitState(ls) )
         {
            // need to compute gLL based on axle weights
            Float64 fMinTop, fMinBot, fMaxTop, fMaxBot, fDummyTop, fDummyBot;
            AxleConfiguration MinAxleConfigTop, MinAxleConfigBot, MaxAxleConfigTop, MaxAxleConfigBot, DummyAxleConfigTop, DummyAxleConfigBot;
            if ( analysisType == pgsTypes::Envelope )
            {
               pProductForces->GetVehicularLiveLoadStress(llType,truck_index,liveLoadIntervalIdx,poi,pgsTypes::MinSimpleContinuousEnvelope,true,true,topLocation,botLocation,&fMinTop,&fDummyTop,&fMinBot,&fDummyBot,&MinAxleConfigTop,&DummyAxleConfigTop,&MinAxleConfigBot,&DummyAxleConfigBot);
               pProductForces->GetVehicularLiveLoadStress(llType,truck_index,liveLoadIntervalIdx,poi,pgsTypes::MaxSimpleContinuousEnvelope,true,true,topLocation,botLocation,&fDummyTop,&fMaxTop,&fDummyBot,&fMaxBot,&DummyAxleConfigTop,&MaxAxleConfigTop,&DummyAxleConfigBot,&MaxAxleConfigBot);
            }
            else
            {
               pProductForces->GetVehicularLiveLoadStress(llType,truck_index,liveLoadIntervalIdx,poi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,topLocation,botLocation,&fMinTop,&fMaxTop,&fMaxTop,&fMaxBot,&MinAxleConfigTop,&MaxAxleConfigTop,&MinAxleConfigBot,&MaxAxleConfigBot);
            }

            gLL = GetStrengthLiveLoadFactor(ratingType,MaxAxleConfigBot);
         }
         else
         {
            gLL = GetServiceLiveLoadFactor(ratingType);
         }
      }


      Float64 Mcr = vMcr[i].Mcr;

      Float64 Icr = vCrackedSection[i].Icr;
      Float64 c   = vCrackedSection[i].c;

      Float64 dps = vM[i].dt;

      Float64 fpe; // stress in reinforcement before cracking
      if ( bPositiveMoment )
      {
         // positive moment - use fpe (effective prestress)
         fpe = pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,liveLoadIntervalIdx,pgsTypes::Middle);
      }
      else
      {
         // negative moment - compute stress in deck rebar for uncracked section
         Float64 I = pSectProp->GetIx(liveLoadIntervalIdx,poi);
         Float64 y = pSectProp->GetY(liveLoadIntervalIdx,poi,pgsTypes::TopDeck);
         
         y -= top_slab_cover; 

         fpe = -(Es/Eg)*Mcr*y/I; // - sign is to make the stress come out positive (tension) because Mcr is < 0
      }

      std::_tstring strVehicleName = strLLNames[truck_index];
      Float64 W = pProductLoads->GetVehicleWeight(llType,truck_index);

      pgsYieldStressRatioArtifact stressRatioArtifact;
      stressRatioArtifact.SetRatingType(ratingType);
      stressRatioArtifact.SetPointOfInterest(poi);
      stressRatioArtifact.SetVehicleIndex(truck_index);
      stressRatioArtifact.SetVehicleWeight(W);
      stressRatioArtifact.SetVehicleName(strVehicleName.c_str());
      stressRatioArtifact.SetAllowableStress(fr);
      stressRatioArtifact.SetDeadLoadFactor(gDC);
      stressRatioArtifact.SetDeadLoadMoment(DC);
      stressRatioArtifact.SetWearingSurfaceFactor(gDW);
      stressRatioArtifact.SetWearingSurfaceMoment(DW);
      stressRatioArtifact.SetLiveLoadFactor(gLL);
      stressRatioArtifact.SetLiveLoadMoment(LLIM+PL);
      stressRatioArtifact.SetCrackingMoment(Mcr);
      stressRatioArtifact.SetIcr(Icr);
      stressRatioArtifact.SetCrackDepth(c);
      stressRatioArtifact.SetReinforcementDepth(dps);
      stressRatioArtifact.SetEffectivePrestress(fpe);
      stressRatioArtifact.SetEs(Es);
      stressRatioArtifact.SetEg(Eg);

      ratingArtifact.AddArtifact(poi,stressRatioArtifact,bPositiveMoment);
   }
}

pgsTypes::LimitState pgsLoadRater::GetStrengthLimitStateType(pgsTypes::LoadRatingType ratingType)
{
   pgsTypes::LimitState ls;
   switch(ratingType)
   {
   case pgsTypes::lrDesign_Inventory:
      ls = pgsTypes::StrengthI_Inventory;
      break;

   case pgsTypes::lrDesign_Operating:
      ls = pgsTypes::StrengthI_Operating;
      break;

   case pgsTypes::lrLegal_Routine:
      ls = pgsTypes::StrengthI_LegalRoutine;
      break;

   case pgsTypes::lrLegal_Special:
      ls = pgsTypes::StrengthI_LegalSpecial;
      break;

   case pgsTypes::lrPermit_Routine:
      ls = pgsTypes::StrengthII_PermitRoutine;
      break;

   case pgsTypes::lrPermit_Special:
      ls = pgsTypes::StrengthII_PermitSpecial;
      break;

   default:
      ATLASSERT(false); // SHOULD NEVER GET HERE (unless there is a new rating type)
   }

   return ls;
}

pgsTypes::LimitState pgsLoadRater::GetServiceLimitStateType(pgsTypes::LoadRatingType ratingType)
{
   pgsTypes::LimitState ls;
   switch(ratingType)
   {
   case pgsTypes::lrDesign_Inventory:
      ls = pgsTypes::ServiceIII_Inventory;
      break;

   case pgsTypes::lrDesign_Operating:
      ls = pgsTypes::ServiceIII_Operating;
      break;

   case pgsTypes::lrLegal_Routine:
      ls = pgsTypes::ServiceIII_LegalRoutine;
      break;

   case pgsTypes::lrLegal_Special:
      ls = pgsTypes::ServiceIII_LegalSpecial;
      break;

   case pgsTypes::lrPermit_Routine:
      ls = pgsTypes::ServiceI_PermitRoutine;
      break;

   case pgsTypes::lrPermit_Special:
      ls = pgsTypes::ServiceI_PermitSpecial;
      break;

   default:
      ATLASSERT(false); // SHOULD NEVER GET HERE (unless there is a new rating type)
   }

   return ls;
}

void pgsLoadRater::GetMoments(const CGirderKey& girderKey,bool bPositiveMoment,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx, const std::vector<pgsPointOfInterest>& vPOI, std::vector<Float64>& vDCmin, std::vector<Float64>& vDCmax,std::vector<Float64>& vDWmin, std::vector<Float64>& vDWmax, std::vector<Float64>& vLLIMmin, std::vector<VehicleIndexType>& vMinTruckIndex,std::vector<Float64>& vLLIMmax,std::vector<VehicleIndexType>& vMaxTruckIndex,std::vector<Float64>& vPLmin,std::vector<Float64>& vPLmax)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bIncludeNoncompositeMoments = pSpecEntry->IncludeNoncompositeMomentsForNegMomentDesign();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);

   std::vector<Float64> vUnused;
   std::vector<VehicleIndexType> vUnusedIndex;

   GET_IFACE(ICombinedForces2,pCombinedForces);
   GET_IFACE(IProductForces2,pProductForces);
   if ( bPositiveMoment || (!bPositiveMoment && bIncludeNoncompositeMoments) )
   {
      if ( analysisType == pgsTypes::Envelope )
      {
         vDCmin = pCombinedForces->GetMoment(lcDC,liveLoadIntervalIdx,vPOI,ctCummulative,pgsTypes::MinSimpleContinuousEnvelope);
         vDCmax = pCombinedForces->GetMoment(lcDC,liveLoadIntervalIdx,vPOI,ctCummulative,pgsTypes::MaxSimpleContinuousEnvelope);
         vDWmin = pCombinedForces->GetMoment(lcDW,liveLoadIntervalIdx,vPOI,ctCummulative,pgsTypes::MinSimpleContinuousEnvelope);
         vDWmax = pCombinedForces->GetMoment(lcDW,liveLoadIntervalIdx,vPOI,ctCummulative,pgsTypes::MaxSimpleContinuousEnvelope);

         if ( vehicleIdx == INVALID_INDEX )
         {
            pProductForces->GetLiveLoadMoment(llType,liveLoadIntervalIdx,vPOI,pgsTypes::MinSimpleContinuousEnvelope,true,true,&vLLIMmin,&vUnused,&vMinTruckIndex,&vUnusedIndex);
            pProductForces->GetLiveLoadMoment(llType,liveLoadIntervalIdx,vPOI,pgsTypes::MaxSimpleContinuousEnvelope,true,true,&vUnused,&vLLIMmax,&vUnusedIndex,&vMaxTruckIndex);
         }
         else
         {
            pProductForces->GetVehicularLiveLoadMoment(llType,vehicleIdx,liveLoadIntervalIdx,vPOI,pgsTypes::MinSimpleContinuousEnvelope,true,true,&vLLIMmin,&vUnused,NULL,NULL);
            pProductForces->GetVehicularLiveLoadMoment(llType,vehicleIdx,liveLoadIntervalIdx,vPOI,pgsTypes::MaxSimpleContinuousEnvelope,true,true,&vUnused,&vLLIMmax,NULL,NULL);
         }

         pCombinedForces->GetCombinedLiveLoadMoment( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPOI, pgsTypes::MaxSimpleContinuousEnvelope, &vUnused, &vPLmax );
         pCombinedForces->GetCombinedLiveLoadMoment( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPOI, pgsTypes::MinSimpleContinuousEnvelope, &vPLmin, &vUnused );
      }
      else
      {
         vDCmax = pCombinedForces->GetMoment(lcDC,liveLoadIntervalIdx,vPOI,ctCummulative,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         vDCmin = vDCmax;
         vDWmax = pCombinedForces->GetMoment(lcDW,liveLoadIntervalIdx,vPOI,ctCummulative,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         vDWmin = vDWmax;

         if ( vehicleIdx == INVALID_INDEX )
            pProductForces->GetLiveLoadMoment(llType,liveLoadIntervalIdx,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,&vLLIMmin,&vLLIMmax,&vMinTruckIndex,&vMaxTruckIndex);
         else
            pProductForces->GetVehicularLiveLoadMoment(llType,vehicleIdx,liveLoadIntervalIdx,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,&vLLIMmin,&vLLIMmax,NULL,NULL);

         pCombinedForces->GetCombinedLiveLoadMoment( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPOI, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &vPLmin, &vPLmax );
      }
   }
   else
   {
      // Because of the construction sequence, some loads don't contribute to the negative moment..
      // Those loads applied to the simple span girders before continuity don't contribute to the
      // negative moment resisted by the deck.
      //
      // Special load processing is required
      GET_IFACE(IBridge,pBridge);

      std::vector<Float64> vConstructionMin, vConstructionMax;
      std::vector<Float64> vSlabMin, vSlabMax;
      std::vector<Float64> vSlabPanelMin, vSlabPanelMax;
      std::vector<Float64> vDiaphragmMin, vDiaphragmMax;
      std::vector<Float64> vShearKeyMin, vShearKeyMax;
      std::vector<Float64> vUserDC1Min, vUserDC1Max;
      std::vector<Float64> vUserDW1Min, vUserDW1Max;
      std::vector<Float64> vUserDC2Min, vUserDC2Max;
      std::vector<Float64> vUserDW2Min, vUserDW2Max;
      std::vector<Float64> vTrafficBarrierMin, vTrafficBarrierMax;
      std::vector<Float64> vSidewalkMin, vSidewalkMax;
      std::vector<Float64> vOverlayMin, vOverlayMax;

      bool bFutureOverlay = pBridge->HasOverlay() && pBridge->IsFutureOverlay();

      // Get all the product load responces
      GET_IFACE(IProductForces2,pProductForces);
      if ( analysisType == pgsTypes::Envelope )
      {
         // Get all the product loads
         vConstructionMin = pProductForces->GetMoment(castDeckIntervalIdx,pftConstruction,vPOI,pgsTypes::MinSimpleContinuousEnvelope);
         vConstructionMax = pProductForces->GetMoment(castDeckIntervalIdx,pftConstruction,vPOI,pgsTypes::MaxSimpleContinuousEnvelope);

         vSlabMin = pProductForces->GetMoment(castDeckIntervalIdx,pftSlab,vPOI,pgsTypes::MinSimpleContinuousEnvelope);
         vSlabMax = pProductForces->GetMoment(castDeckIntervalIdx,pftSlab,vPOI,pgsTypes::MaxSimpleContinuousEnvelope);

         vSlabPanelMin = pProductForces->GetMoment(castDeckIntervalIdx,pftSlabPanel,vPOI,pgsTypes::MinSimpleContinuousEnvelope);
         vSlabPanelMax = pProductForces->GetMoment(castDeckIntervalIdx,pftSlabPanel,vPOI,pgsTypes::MaxSimpleContinuousEnvelope);

         vDiaphragmMin = pProductForces->GetMoment(castDeckIntervalIdx,pftDiaphragm,vPOI,pgsTypes::MinSimpleContinuousEnvelope);
         vDiaphragmMax = pProductForces->GetMoment(castDeckIntervalIdx,pftDiaphragm,vPOI,pgsTypes::MaxSimpleContinuousEnvelope);

         vShearKeyMin = pProductForces->GetMoment(castDeckIntervalIdx,pftShearKey,vPOI,pgsTypes::MinSimpleContinuousEnvelope);
         vShearKeyMax = pProductForces->GetMoment(castDeckIntervalIdx,pftShearKey,vPOI,pgsTypes::MaxSimpleContinuousEnvelope);

         vUserDC1Min = pProductForces->GetMoment(castDeckIntervalIdx,pftUserDC,vPOI,pgsTypes::MinSimpleContinuousEnvelope);
         vUserDC1Max = pProductForces->GetMoment(castDeckIntervalIdx,pftUserDC,vPOI,pgsTypes::MaxSimpleContinuousEnvelope);

         vUserDW1Min = pProductForces->GetMoment(castDeckIntervalIdx,pftUserDW,vPOI,pgsTypes::MinSimpleContinuousEnvelope);
         vUserDW1Max = pProductForces->GetMoment(castDeckIntervalIdx,pftUserDW,vPOI,pgsTypes::MaxSimpleContinuousEnvelope);

         vUserDC2Min = pProductForces->GetMoment(compositeDeckIntervalIdx,pftUserDC,vPOI,pgsTypes::MinSimpleContinuousEnvelope);
         vUserDC2Max = pProductForces->GetMoment(compositeDeckIntervalIdx,pftUserDC,vPOI,pgsTypes::MaxSimpleContinuousEnvelope);

         vUserDW2Min = pProductForces->GetMoment(compositeDeckIntervalIdx,pftUserDW,vPOI,pgsTypes::MinSimpleContinuousEnvelope);
         vUserDW2Max = pProductForces->GetMoment(compositeDeckIntervalIdx,pftUserDW,vPOI,pgsTypes::MaxSimpleContinuousEnvelope);

         vTrafficBarrierMin = pProductForces->GetMoment(railingSystemIntervalIdx,pftTrafficBarrier,vPOI,pgsTypes::MinSimpleContinuousEnvelope);
         vTrafficBarrierMax = pProductForces->GetMoment(railingSystemIntervalIdx,pftTrafficBarrier,vPOI,pgsTypes::MaxSimpleContinuousEnvelope);

         vSidewalkMin = pProductForces->GetMoment(railingSystemIntervalIdx,pftSidewalk,vPOI,pgsTypes::MinSimpleContinuousEnvelope);
         vSidewalkMax = pProductForces->GetMoment(railingSystemIntervalIdx,pftSidewalk,vPOI,pgsTypes::MaxSimpleContinuousEnvelope);

         if ( !bFutureOverlay )
         {
            vOverlayMin = pProductForces->GetMoment(overlayIntervalIdx,pftOverlay,vPOI,pgsTypes::MinSimpleContinuousEnvelope);
            vOverlayMax = pProductForces->GetMoment(overlayIntervalIdx,pftOverlay,vPOI,pgsTypes::MaxSimpleContinuousEnvelope);
         }
         else
         {
            vOverlayMin.resize(vPOI.size(),0);
            vOverlayMax.resize(vPOI.size(),0);
         }

         if ( vehicleIdx == INVALID_INDEX )
         {
            pProductForces->GetLiveLoadMoment( llType, liveLoadIntervalIdx, vPOI, pgsTypes::MinSimpleContinuousEnvelope, true, true, &vLLIMmin, &vUnused, &vMinTruckIndex, &vUnusedIndex );
            pProductForces->GetLiveLoadMoment( llType, liveLoadIntervalIdx, vPOI, pgsTypes::MaxSimpleContinuousEnvelope, true, true, &vUnused, &vLLIMmax, &vUnusedIndex, &vMaxTruckIndex );
         }
         else
         {
            pProductForces->GetVehicularLiveLoadMoment(llType,vehicleIdx,liveLoadIntervalIdx,vPOI,pgsTypes::MinSimpleContinuousEnvelope,true,true,&vLLIMmin,&vUnused,NULL,NULL);
            pProductForces->GetVehicularLiveLoadMoment(llType,vehicleIdx,liveLoadIntervalIdx,vPOI,pgsTypes::MaxSimpleContinuousEnvelope,true,true,&vUnused,&vLLIMmax,NULL,NULL);
         }
      }
      else
      {
         vConstructionMin = pProductForces->GetMoment(castDeckIntervalIdx,pftConstruction,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         vConstructionMax = vConstructionMin;

         vSlabMin = pProductForces->GetMoment(castDeckIntervalIdx,pftSlab,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         vSlabMax = vSlabMin;

         vSlabPanelMin = pProductForces->GetMoment(castDeckIntervalIdx,pftSlabPanel,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         vSlabPanelMax = vSlabPanelMin;

         vDiaphragmMin = pProductForces->GetMoment(castDeckIntervalIdx,pftDiaphragm,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         vDiaphragmMax = vDiaphragmMin;

         vShearKeyMin = pProductForces->GetMoment(castDeckIntervalIdx,pftShearKey,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         vShearKeyMax = vShearKeyMin;

         vUserDC1Min = pProductForces->GetMoment(castDeckIntervalIdx,pftUserDC,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         vUserDC1Max = vUserDC1Min;

         vUserDW1Min = pProductForces->GetMoment(castDeckIntervalIdx,pftUserDW,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         vUserDW1Max = vUserDW1Min;

         vUserDC2Min = pProductForces->GetMoment(compositeDeckIntervalIdx,pftUserDC,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         vUserDC2Max = vUserDC2Min;

         vUserDW2Min = pProductForces->GetMoment(compositeDeckIntervalIdx,pftUserDW,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         vUserDW2Max = vUserDW2Min;

         vTrafficBarrierMin = pProductForces->GetMoment(railingSystemIntervalIdx,pftTrafficBarrier,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         vTrafficBarrierMax = vTrafficBarrierMin;

         vSidewalkMin = pProductForces->GetMoment(railingSystemIntervalIdx,pftSidewalk,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         vSidewalkMax = vSidewalkMin;

         if ( !bFutureOverlay )
         {
            vOverlayMin = pProductForces->GetMoment(overlayIntervalIdx,pftOverlay,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         }
         else
         {
            vOverlayMin.resize(vPOI.size(),0);
         }
         vOverlayMax = vOverlayMin;

         if ( vehicleIdx == INVALID_INDEX )
            pProductForces->GetLiveLoadMoment(llType,liveLoadIntervalIdx,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,&vLLIMmin,&vLLIMmax,&vMinTruckIndex,&vMaxTruckIndex);
         else
            pProductForces->GetVehicularLiveLoadMoment(llType,vehicleIdx,liveLoadIntervalIdx,vPOI,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,&vLLIMmin,&vLLIMmax,NULL,NULL);

         pCombinedForces->GetCombinedLiveLoadMoment( pgsTypes::lltPedestrian, liveLoadIntervalIdx, vPOI, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &vPLmin, &vPLmax );
      }

      // sum DC and DW

      // initialize
      vDCmin.resize(vPOI.size(),0);
      vDCmax.resize(vPOI.size(),0);
      vDWmin.resize(vPOI.size(),0);
      vDWmax.resize(vPOI.size(),0);

      GET_IFACE(IPointOfInterest,pPoi);
      special_transform(pBridge,pPoi,pIntervals,vPOI.begin(),vPOI.end(),vConstructionMin.begin(),vDCmin.begin(),vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPOI.begin(),vPOI.end(),vConstructionMax.begin(),vDCmax.begin(),vDCmax.begin());
      special_transform(pBridge,pPoi,pIntervals,vPOI.begin(),vPOI.end(),vSlabMin.begin(),vDCmin.begin(),vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPOI.begin(),vPOI.end(),vSlabMax.begin(),vDCmax.begin(),vDCmax.begin());
      special_transform(pBridge,pPoi,pIntervals,vPOI.begin(),vPOI.end(),vSlabPanelMin.begin(),vDCmin.begin(),vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPOI.begin(),vPOI.end(),vSlabPanelMax.begin(),vDCmax.begin(),vDCmax.begin());
      special_transform(pBridge,pPoi,pIntervals,vPOI.begin(),vPOI.end(),vDiaphragmMin.begin(),vDCmin.begin(),vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPOI.begin(),vPOI.end(),vDiaphragmMax.begin(),vDCmax.begin(),vDCmax.begin());
      special_transform(pBridge,pPoi,pIntervals,vPOI.begin(),vPOI.end(),vShearKeyMin.begin(),vDCmin.begin(),vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPOI.begin(),vPOI.end(),vShearKeyMax.begin(),vDCmax.begin(),vDCmax.begin());

      std::transform(vUserDC1Min.begin(),vUserDC1Min.end(),vDCmin.begin(),vDCmin.begin(),sum_values);
      std::transform(vUserDC1Max.begin(),vUserDC1Max.end(),vDCmax.begin(),vDCmax.begin(),sum_values);

      std::transform(vUserDW1Min.begin(),vUserDW1Min.end(),vDWmin.begin(),vDWmin.begin(),sum_values);
      std::transform(vUserDW1Max.begin(),vUserDW1Max.end(),vDWmax.begin(),vDWmax.begin(),sum_values);

      std::transform(vUserDC2Min.begin(),vUserDC2Min.end(),vDCmin.begin(),vDCmin.begin(),sum_values);
      std::transform(vUserDC2Max.begin(),vUserDC2Max.end(),vDCmax.begin(),vDCmax.begin(),sum_values);

      std::transform(vUserDW2Min.begin(),vUserDW2Min.end(),vDWmin.begin(),vDWmin.begin(),sum_values);
      std::transform(vUserDW2Max.begin(),vUserDW2Max.end(),vDWmax.begin(),vDWmax.begin(),sum_values);

      std::transform(vTrafficBarrierMin.begin(),vTrafficBarrierMin.end(),vDCmin.begin(),vDCmin.begin(),sum_values);
      std::transform(vTrafficBarrierMax.begin(),vTrafficBarrierMax.end(),vDCmax.begin(),vDCmax.begin(),sum_values);

      std::transform(vSidewalkMin.begin(),vSidewalkMin.end(),vDCmin.begin(),vDCmin.begin(),sum_values);
      std::transform(vSidewalkMax.begin(),vSidewalkMax.end(),vDCmax.begin(),vDCmax.begin(),sum_values);

      std::transform(vOverlayMin.begin(),vOverlayMin.end(),vDWmin.begin(),vDWmin.begin(),sum_values);
      std::transform(vOverlayMax.begin(),vOverlayMax.end(),vDWmax.begin(),vDWmax.begin(),sum_values);
   }
}

void special_transform(IBridge* pBridge,IPointOfInterest* pPoi,IIntervals* pIntervals,
                       std::vector<pgsPointOfInterest>::const_iterator poiBeginIter,
                       std::vector<pgsPointOfInterest>::const_iterator poiEndIter,
                       std::vector<Float64>::iterator forceBeginIter,
                       std::vector<Float64>::iterator resultBeginIter,
                       std::vector<Float64>::iterator outputBeginIter)
{
   std::vector<pgsPointOfInterest>::const_iterator poiIter( poiBeginIter );
   std::vector<Float64>::iterator forceIter( forceBeginIter );
   std::vector<Float64>::iterator resultIter( resultBeginIter );
   std::vector<Float64>::iterator outputIter( outputBeginIter );

   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

   for ( ; poiIter != poiEndIter; poiIter++, forceIter++, resultIter++, outputIter++ )
   {
      const pgsPointOfInterest& poi = *poiIter;
      const CSegmentKey& segmentKey = poi.GetSegmentKey();
      SpanIndexType spanIdx = pPoi->GetSpan(poi);

      EventIndexType start,end,dummy;
      PierIndexType prevPierIdx = spanIdx;
      PierIndexType nextPierIdx = prevPierIdx + 1;

      pBridge->GetContinuityEventIndex(prevPierIdx,&dummy,&start);
      pBridge->GetContinuityEventIndex(nextPierIdx,&end,&dummy);

      IntervalIndexType startPierContinuityIntervalIdx = pIntervals->GetInterval(start);
      IntervalIndexType endPierContinuityIntervalIdx   = pIntervals->GetInterval(end);

      if ( startPierContinuityIntervalIdx == compositeDeckIntervalIdx && 
           endPierContinuityIntervalIdx   == compositeDeckIntervalIdx )
      {
         *outputIter = (*forceIter + *resultIter);
      }
   }
}

Float64 pgsLoadRater::GetStrengthLiveLoadFactor(pgsTypes::LoadRatingType ratingType,AxleConfiguration& axleConfig)
{
   Float64 sum_axle_weight = 0; // sum of axle weights on the bridge
   Float64 firstAxleLocation = -1;
   Float64 lastAxleLocation = 0;
   AxleConfiguration::iterator iter(axleConfig.begin());
   AxleConfiguration::iterator end(axleConfig.end());
   for ( ; iter != end; iter++ )
   {
      AxlePlacement& axle_placement = *iter;
      sum_axle_weight += axle_placement.Weight;

      if ( !IsZero(axle_placement.Weight) )
      {
         if ( firstAxleLocation < 0 )
            firstAxleLocation = axle_placement.Location;

         lastAxleLocation = axle_placement.Location;
      }
   }
   
   Float64 AL = fabs(firstAxleLocation - lastAxleLocation); // front axle to rear axle length (for axles on the bridge)

   Float64 gLL = 0;
   GET_IFACE(IRatingSpecification,pRatingSpec);
   GET_IFACE(ILibrary,pLibrary);
   const RatingLibraryEntry* pRatingEntry = pLibrary->GetRatingEntry( pRatingSpec->GetRatingSpecification().c_str() );
   if ( pRatingEntry->GetSpecificationVersion() < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      CLiveLoadFactorModel model;
      if ( ratingType == pgsTypes::lrPermit_Routine )
         model = pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrPermit_Routine);
      else if ( ratingType == pgsTypes::lrPermit_Special )
         model = pRatingEntry->GetLiveLoadFactorModel(pRatingSpec->GetSpecialPermitType());
      else
         model = pRatingEntry->GetLiveLoadFactorModel(ratingType);

      gLL = model.GetStrengthLiveLoadFactor(pRatingSpec->GetADTT(),sum_axle_weight);
   }
   else
   {
      Float64 GVW = sum_axle_weight;
      Float64 PermitWeightRatio = IsZero(AL) ? 0 : GVW/AL;
      CLiveLoadFactorModel2 model;
      if ( ratingType == pgsTypes::lrPermit_Routine )
         model = pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrPermit_Routine);
      else if ( ratingType == pgsTypes::lrPermit_Special )
         model = pRatingEntry->GetLiveLoadFactorModel2(pRatingSpec->GetSpecialPermitType());
      else
         model = pRatingEntry->GetLiveLoadFactorModel2(ratingType);

      gLL = model.GetStrengthLiveLoadFactor(pRatingSpec->GetADTT(),PermitWeightRatio);
   }

   return gLL;
}

Float64 pgsLoadRater::GetServiceLiveLoadFactor(pgsTypes::LoadRatingType ratingType)
{
   Float64 gLL = 0;
   GET_IFACE(IRatingSpecification,pRatingSpec);
   GET_IFACE(ILibrary,pLibrary);
   const RatingLibraryEntry* pRatingEntry = pLibrary->GetRatingEntry( pRatingSpec->GetRatingSpecification().c_str() );
   if ( pRatingEntry->GetSpecificationVersion() < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      CLiveLoadFactorModel model;
      if ( ratingType == pgsTypes::lrPermit_Routine )
         model = pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrPermit_Routine);
      else if ( ratingType == pgsTypes::lrPermit_Special )
         model = pRatingEntry->GetLiveLoadFactorModel(pRatingSpec->GetSpecialPermitType());
      else
         model = pRatingEntry->GetLiveLoadFactorModel(ratingType);

      gLL = model.GetServiceLiveLoadFactor(pRatingSpec->GetADTT());
   }
   else
   {
      CLiveLoadFactorModel2 model;
      if ( ratingType == pgsTypes::lrPermit_Routine )
         model = pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrPermit_Routine);
      else if ( ratingType == pgsTypes::lrPermit_Special )
         model = pRatingEntry->GetLiveLoadFactorModel2(pRatingSpec->GetSpecialPermitType());
      else
         model = pRatingEntry->GetLiveLoadFactorModel2(ratingType);

      gLL = model.GetServiceLiveLoadFactor(pRatingSpec->GetADTT());
   }

   return gLL;
}

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

   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi(  pPOI->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS)) );

   pgsRatingArtifact ratingArtifact;

   // Rate for positive moment - flexure
   MomentRating(girderKey,vPoi,true,ratingType,vehicleIdx,ratingArtifact);

   // Rate for negative moment - flexure, if applicable
   GET_IFACE(IBridge,pBridge);
   if ( pBridge->ProcessNegativeMoments(ALL_SPANS) )
   {
      MomentRating(girderKey,vPoi,false,ratingType,vehicleIdx,ratingArtifact);
   }

   // Rate for shear if applicable
   if ( pRatingSpec->RateForShear(ratingType) )
   {
      ShearRating(girderKey,vPoi,ratingType,vehicleIdx,ratingArtifact);
   }

   // Rate for stress if applicable
   if ( pRatingSpec->RateForStress(ratingType) )
   {
      if ( ratingType == pgsTypes::lrPermit_Routine || ratingType == pgsTypes::lrPermit_Special )
      {
         // Service I reinforcement yield check if permit rating
         CheckReinforcementYielding(girderKey,vPoi,ratingType,vehicleIdx,true,ratingArtifact);

         if ( pBridge->ProcessNegativeMoments(ALL_SPANS) )
         {
            CheckReinforcementYielding(girderKey,vPoi,ratingType,vehicleIdx,false,ratingArtifact);
         }
      }
      else
      {
         // Service III flexure if other rating type
         StressRating(girderKey,vPoi,ratingType,vehicleIdx,ratingArtifact);
      }
   }

   return ratingArtifact;
}

void pgsLoadRater::MomentRating(const CGirderKey& girderKey,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,pgsRatingArtifact& ratingArtifact)
{
   std::vector<Float64> vDCmin, vDCmax;
   std::vector<Float64> vDWmin, vDWmax;
   std::vector<Float64> vCRmin, vCRmax;
   std::vector<Float64> vSHmin, vSHmax;
   std::vector<Float64> vREmin, vREmax;
   std::vector<Float64> vPSmin, vPSmax;
   std::vector<Float64> vLLIMmin,vLLIMmax;
   std::vector<Float64> vPLmin,vPLmax;
   std::vector<VehicleIndexType> vMinTruckIndex, vMaxTruckIndex;
   GetMoments(girderKey,bPositiveMoment,ratingType, vehicleIdx, vPoi, vDCmin, vDCmax, vDWmin, vDWmax, vCRmin, vCRmax, vSHmin, vSHmax, vREmin, vREmax, vPSmin, vPSmax, vLLIMmin, vMinTruckIndex, vLLIMmax, vMaxTruckIndex, vPLmin, vPLmax);

   CGirderKey thisGirderKey(girderKey);
   if ( thisGirderKey.groupIndex == ALL_GROUPS )
   {
      thisGirderKey.groupIndex = 0;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType loadRatingIntervalIdx = pIntervals->GetLoadRatingInterval(thisGirderKey);

   GET_IFACE(IMomentCapacity,pMomentCapacity);
   std::vector<MOMENTCAPACITYDETAILS> vM = pMomentCapacity->GetMomentCapacityDetails(loadRatingIntervalIdx,vPoi,bPositiveMoment);
   std::vector<MINMOMENTCAPDETAILS> vMmin = pMomentCapacity->GetMinMomentCapacityDetails(loadRatingIntervalIdx,vPoi,bPositiveMoment);

   ATLASSERT(vPoi.size()     == vDCmax.size());
   ATLASSERT(vPoi.size()     == vDWmax.size());
   ATLASSERT(vPoi.size()     == vLLIMmax.size());
   ATLASSERT(vPoi.size()     == vM.size());
   ATLASSERT(vDCmin.size()   == vDCmax.size());
   ATLASSERT(vDWmin.size()   == vDWmax.size());
   ATLASSERT(vCRmin.size()   == vCRmax.size());
   ATLASSERT(vSHmin.size()   == vSHmax.size());
   ATLASSERT(vREmin.size()   == vREmax.size());
   ATLASSERT(vPSmin.size()   == vPSmax.size());
   ATLASSERT(vLLIMmin.size() == vLLIMmax.size());

   GET_IFACE(IRatingSpecification,pRatingSpec);
   Float64 system_factor    = pRatingSpec->GetSystemFactorFlexure();
   bool bIncludePL = pRatingSpec->IncludePedestrianLiveLoad();

   pgsTypes::LimitState ls = GetStrengthLimitStateType(ratingType);

   Float64 gDC = pRatingSpec->GetDeadLoadFactor(ls);
   Float64 gDW = pRatingSpec->GetWearingSurfaceFactor(ls);
   Float64 gCR = pRatingSpec->GetCreepFactor(ls);
   Float64 gSH = pRatingSpec->GetShrinkageFactor(ls);
   Float64 gRE = pRatingSpec->GetRelaxationFactor(ls);
   Float64 gPS = pRatingSpec->GetSecondaryEffectsFactor(ls);

   GET_IFACE(IProductLoads,pProductLoads);
   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);
   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   CollectionIndexType nPOI = vPoi.size();
   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      const pgsPointOfInterest& poi = vPoi[i];

      Float64 condition_factor = (bPositiveMoment ? pRatingSpec->GetGirderConditionFactor(poi.GetSegmentKey()) 
                                                  : pRatingSpec->GetDeckConditionFactor() );

      Float64 DC   = (bPositiveMoment ? vDCmax[i]   : vDCmin[i]);
      Float64 DW   = (bPositiveMoment ? vDWmax[i]   : vDWmin[i]);
      Float64 CR   = (bPositiveMoment ? vCRmax[i]   : vCRmin[i]);
      Float64 SH   = (bPositiveMoment ? vSHmax[i]   : vSHmin[i]);
      Float64 RE   = (bPositiveMoment ? vREmax[i]   : vREmin[i]);
      Float64 PS   = (bPositiveMoment ? vPSmax[i]   : vPSmin[i]);
      Float64 LLIM = (bPositiveMoment ? vLLIMmax[i] : vLLIMmin[i]);
      Float64 PL   = (bIncludePL ? (bPositiveMoment ? vPLmax[i]   : vPLmin[i]) : 0.0);

      VehicleIndexType truck_index = vehicleIdx;
      if ( vehicleIdx == INVALID_INDEX )
      {
         truck_index = (bPositiveMoment ? vMaxTruckIndex[i] : vMinTruckIndex[i]);
      }

      Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
      if ( gLL < 0 )
      {
         if ( ::IsStrengthLimitState(ls) )
         {
            GET_IFACE(IProductForces,pProductForces);

            Float64 Mmin, Mmax, Dummy;
            AxleConfiguration MinAxleConfig, MaxAxleConfig, DummyAxleConfig;
            if ( analysisType == pgsTypes::Envelope )
            {
               pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,truck_index,poi,pgsTypes::MinSimpleContinuousEnvelope,true,true,&Mmin,&Dummy,&MinAxleConfig,&DummyAxleConfig);
               pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,truck_index,poi,pgsTypes::MaxSimpleContinuousEnvelope,true,true,&Dummy,&Mmax,&DummyAxleConfig,&MaxAxleConfig);
            }
            else
            {
               pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,truck_index,poi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,&Mmin,&Mmax,&MinAxleConfig,&MaxAxleConfig);
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
      {
         K = 1.0;
      }

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
      momentArtifact.SetCreepFactor(gCR);
      momentArtifact.SetCreepMoment(CR);
      momentArtifact.SetShrinkageFactor(gSH);
      momentArtifact.SetShrinkageMoment(SH);
      momentArtifact.SetRelaxationFactor(gRE);
      momentArtifact.SetRelaxationMoment(RE);
      momentArtifact.SetSecondaryEffectsFactor(gPS);
      momentArtifact.SetSecondaryEffectsMoment(PS);
      momentArtifact.SetLiveLoadFactor(gLL);
      momentArtifact.SetLiveLoadMoment(LLIM+PL);

      ratingArtifact.AddArtifact(poi,momentArtifact,bPositiveMoment);
   }
}

void pgsLoadRater::ShearRating(const CGirderKey& girderKey,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,pgsRatingArtifact& ratingArtifact)
{
   CGirderKey thisGirderKey(girderKey);
   if ( thisGirderKey.groupIndex == ALL_GROUPS )
   {
      thisGirderKey.groupIndex = 0;
   }

   pgsTypes::LimitState ls = GetStrengthLimitStateType(ratingType);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType loadRatingIntervalIdx = pIntervals->GetLoadRatingInterval(thisGirderKey);

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   std::vector<sysSectionValue> vDCmin, vDCmax;
   std::vector<sysSectionValue> vDWmin, vDWmax;
   std::vector<sysSectionValue> vCRmin, vCRmax;
   std::vector<sysSectionValue> vSHmin, vSHmax;
   std::vector<sysSectionValue> vREmin, vREmax;
   std::vector<sysSectionValue> vPSmin, vPSmax;
   std::vector<sysSectionValue> vLLIMmin,vLLIMmax;
   std::vector<sysSectionValue> vUnused;
   std::vector<VehicleIndexType> vMinTruckIndex, vMaxTruckIndex, vUnusedIndex;
   std::vector<sysSectionValue> vPLmin, vPLmax;

   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);

   GET_IFACE(ICombinedForces2,pCombinedForces);
   GET_IFACE(IProductForces2,pProductForces);
   if ( analysisType == pgsTypes::Envelope )
   {
      vDCmin = pCombinedForces->GetShear(loadRatingIntervalIdx,lcDC,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
      vDCmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcDC,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);

      vDWmin = pCombinedForces->GetShear(loadRatingIntervalIdx,lcDWRating,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
      vDWmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcDWRating,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);

      vCRmin = pCombinedForces->GetShear(loadRatingIntervalIdx,lcCR,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
      vCRmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcCR,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);

      vSHmin = pCombinedForces->GetShear(loadRatingIntervalIdx,lcSH,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
      vSHmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcSH,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);

      vREmin = pCombinedForces->GetShear(loadRatingIntervalIdx,lcRE,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
      vREmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcRE,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);

      vPSmin = pCombinedForces->GetShear(loadRatingIntervalIdx,lcPS,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
      vPSmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcPS,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);

      if ( vehicleIdx == INVALID_INDEX )
      {
         pProductForces->GetLiveLoadShear( loadRatingIntervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, true, &vLLIMmin, &vUnused, &vMinTruckIndex, &vUnusedIndex );
         pProductForces->GetLiveLoadShear( loadRatingIntervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, true, &vUnused, &vLLIMmax, &vUnusedIndex, &vMaxTruckIndex );
      }
      else
      {
         pProductForces->GetVehicularLiveLoadShear( loadRatingIntervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, true, &vLLIMmin, &vUnused, NULL,NULL,NULL,NULL);
         pProductForces->GetVehicularLiveLoadShear( loadRatingIntervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, true, &vUnused, &vLLIMmax, NULL,NULL,NULL,NULL);
      }

      pCombinedForces->GetCombinedLiveLoadShear( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, false, &vUnused, &vPLmax );
      pCombinedForces->GetCombinedLiveLoadShear( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, pgsTypes::MinSimpleContinuousEnvelope, false, &vPLmin, &vUnused );
   }
   else
   {
      vDCmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcDC,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative);
      vDCmin = vDCmax;

      vDWmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcDWRating,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative);
      vDWmin = vDWmax;

      vCRmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcCR,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative);
      vCRmin = vCRmax;

      vSHmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcSH,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative);
      vSHmin = vSHmax;

      vREmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcRE,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative);
      vREmin = vREmax;

      vPSmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcPS,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative);
      vPSmin = vPSmax;

      if ( vehicleIdx == INVALID_INDEX )
      {
         pProductForces->GetLiveLoadShear( loadRatingIntervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, true, &vLLIMmin, &vLLIMmax, &vMinTruckIndex, &vMaxTruckIndex );
      }
      else
      {
         pProductForces->GetVehicularLiveLoadShear( loadRatingIntervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, true, &vLLIMmin, &vLLIMmax, NULL, NULL, NULL, NULL);
      }

      pCombinedForces->GetCombinedLiveLoadShear( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, false, &vPLmin, &vPLmax );
   }

   GET_IFACE(IShearCapacity,pShearCapacity);
   std::vector<SHEARCAPACITYDETAILS> vVn = pShearCapacity->GetShearCapacityDetails(ls,loadRatingIntervalIdx,vPoi);

   ATLASSERT(vPoi.size()     == vDCmax.size());
   ATLASSERT(vPoi.size()     == vDWmax.size());
   ATLASSERT(vPoi.size()     == vLLIMmax.size());
   ATLASSERT(vPoi.size()     == vVn.size());
   ATLASSERT(vDCmin.size()   == vDCmax.size());
   ATLASSERT(vDWmin.size()   == vDWmax.size());
   ATLASSERT(vCRmin.size()   == vCRmax.size());
   ATLASSERT(vSHmin.size()   == vSHmax.size());
   ATLASSERT(vREmin.size()   == vREmax.size());
   ATLASSERT(vPSmin.size()   == vPSmax.size());
   ATLASSERT(vLLIMmin.size() == vLLIMmax.size());
   ATLASSERT(vPLmin.size()   == vPLmax.size());

   GET_IFACE(IRatingSpecification,pRatingSpec);
   Float64 system_factor    = pRatingSpec->GetSystemFactorShear();
   bool bIncludePL = pRatingSpec->IncludePedestrianLiveLoad();

   Float64 gDC = pRatingSpec->GetDeadLoadFactor(ls);
   Float64 gDW = pRatingSpec->GetWearingSurfaceFactor(ls);
   Float64 gCR = pRatingSpec->GetCreepFactor(ls);
   Float64 gSH = pRatingSpec->GetShrinkageFactor(ls);
   Float64 gRE = pRatingSpec->GetRelaxationFactor(ls);
   Float64 gPS = pRatingSpec->GetSecondaryEffectsFactor(ls);

   GET_IFACE(IProductLoads,pProductLoads);
   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

   CollectionIndexType nPOI = vPoi.size();
   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      const pgsPointOfInterest& poi = vPoi[i];

      Float64 condition_factor = pRatingSpec->GetGirderConditionFactor(poi.GetSegmentKey());

      Float64 DCmin   = Min(vDCmin[i].Left(),  vDCmin[i].Right());
      Float64 DCmax   = Max(vDCmax[i].Left(),  vDCmax[i].Right());
      Float64 DWmin   = Min(vDWmin[i].Left(),  vDWmin[i].Right());
      Float64 DWmax   = Max(vDWmax[i].Left(),  vDWmax[i].Right());
      Float64 CRmin   = Min(vCRmin[i].Left(),  vCRmin[i].Right());
      Float64 CRmax   = Max(vCRmax[i].Left(),  vCRmax[i].Right());
      Float64 SHmin   = Min(vSHmin[i].Left(),  vSHmin[i].Right());
      Float64 SHmax   = Max(vSHmax[i].Left(),  vSHmax[i].Right());
      Float64 REmin   = Min(vREmin[i].Left(),  vREmin[i].Right());
      Float64 REmax   = Max(vREmax[i].Left(),  vREmax[i].Right());
      Float64 PSmin   = Min(vPSmin[i].Left(),  vPSmin[i].Right());
      Float64 PSmax   = Max(vPSmax[i].Left(),  vPSmax[i].Right());
      Float64 LLIMmin = Min(vLLIMmin[i].Left(),vLLIMmin[i].Right());
      Float64 LLIMmax = Max(vLLIMmax[i].Left(),vLLIMmax[i].Right());
      Float64 PLmin   = Min(vPLmin[i].Left(),  vPLmin[i].Right());
      Float64 PLmax   = Max(vPLmax[i].Left(),  vPLmax[i].Right());

      Float64 DC   = Max(fabs(DCmin),fabs(DCmax));
      Float64 DW   = Max(fabs(DWmin),fabs(DWmax));
      Float64 CR   = Max(fabs(CRmin),fabs(CRmax));
      Float64 SH   = Max(fabs(SHmin),fabs(SHmax));
      Float64 RE   = Max(fabs(REmin),fabs(REmax));
      Float64 PS   = Max(fabs(PSmin),fabs(PSmax));
      Float64 LLIM = Max(fabs(LLIMmin),fabs(LLIMmax));
      Float64 PL   = (bIncludePL ? Max(fabs(PLmin),fabs(PLmax)) : 0);
      VehicleIndexType truck_index = vehicleIdx;
      if ( vehicleIdx == INVALID_INDEX )
      {
         truck_index = (fabs(LLIMmin) < fabs(LLIMmax) ? vMaxTruckIndex[i] : vMinTruckIndex[i]);
      }

      Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
      if ( gLL < 0 )
      {
         // need to compute gLL based on axle weights
         if ( ::IsStrengthLimitState(ls) )
         {
            GET_IFACE(IProductForces,pProductForce);

            sysSectionValue Vmin, Vmax, Dummy;
            AxleConfiguration MinLeftAxleConfig, MaxLeftAxleConfig, MinRightAxleConfig, MaxRightAxleConfig, DummyLeftAxleConfig, DummyRightAxleConfig;
            if ( analysisType == pgsTypes::Envelope )
            {
               pProductForce->GetVehicularLiveLoadShear(loadRatingIntervalIdx,llType,truck_index,poi,pgsTypes::MinSimpleContinuousEnvelope,true,true,&Vmin,&Dummy,&MinLeftAxleConfig,&MinRightAxleConfig,&DummyLeftAxleConfig,&DummyRightAxleConfig);
               pProductForce->GetVehicularLiveLoadShear(loadRatingIntervalIdx,llType,truck_index,poi,pgsTypes::MaxSimpleContinuousEnvelope,true,true,&Dummy,&Vmax,&DummyLeftAxleConfig,&DummyRightAxleConfig,&MaxLeftAxleConfig,&MaxRightAxleConfig);
            }
            else
            {
               pProductForce->GetVehicularLiveLoadShear(loadRatingIntervalIdx,llType,truck_index,poi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,&Vmin,&Vmax,&MinLeftAxleConfig,&MinRightAxleConfig,&MaxLeftAxleConfig,&MaxRightAxleConfig);
            }

            if ( fabs(LLIMmin) < fabs(LLIMmax) )
            {
               if (IsEqual(fabs(vLLIMmax[i].Left()),fabs(LLIMmax)))
               {
                  gLL = GetStrengthLiveLoadFactor(ratingType,MaxLeftAxleConfig);
               }
               else
               {
                  gLL = GetStrengthLiveLoadFactor(ratingType,MaxRightAxleConfig);
               }
            }
            else
            {
               if (IsEqual(fabs(vLLIMmin[i].Left()),fabs(LLIMmin)))
               {
                  gLL = GetStrengthLiveLoadFactor(ratingType,MinLeftAxleConfig);
               }
               else
               {
                  gLL = GetStrengthLiveLoadFactor(ratingType,MinRightAxleConfig);
               }
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
      shearArtifact.SetCreepFactor(gCR);
      shearArtifact.SetCreepShear(CR);
      shearArtifact.SetShrinkageFactor(gSH);
      shearArtifact.SetShrinkageShear(SH);
      shearArtifact.SetRelaxationFactor(gRE);
      shearArtifact.SetRelaxationShear(RE);
      shearArtifact.SetSecondaryEffectsFactor(gPS);
      shearArtifact.SetSecondaryEffectsShear(PS);
      shearArtifact.SetLiveLoadFactor(gLL);
      shearArtifact.SetLiveLoadShear(LLIM+PL);

      // longitudinal steel check
      pgsLongReinfShearArtifact l_artifact;
      SHEARCAPACITYDETAILS scd;
      pgsDesigner2 designer;
      designer.SetBroker(m_pBroker);
      pShearCapacity->GetShearCapacityDetails(ls,loadRatingIntervalIdx,poi,&scd);
      designer.InitShearCheck(poi.GetSegmentKey(),loadRatingIntervalIdx,ls,NULL);
      designer.CheckLongReinfShear(poi,loadRatingIntervalIdx,ls,scd,NULL,&l_artifact);
      shearArtifact.SetLongReinfShearArtifact(l_artifact);

      ratingArtifact.AddArtifact(poi,shearArtifact);
   }
}

void pgsLoadRater::StressRating(const CGirderKey& girderKey,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,pgsRatingArtifact& ratingArtifact)
{
   ATLASSERT(ratingType == pgsTypes::lrDesign_Inventory || 
             ratingType == pgsTypes::lrLegal_Routine    ||
             ratingType == pgsTypes::lrLegal_Special ); // see MBE C6A.5.4.1


   CGirderKey thisGirderKey(girderKey);
   if ( thisGirderKey.groupIndex == ALL_GROUPS )
   {
      thisGirderKey.groupIndex = 0;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType loadRatingIntervalIdx = pIntervals->GetLoadRatingInterval(thisGirderKey);

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   std::vector<Float64> vDCTopMin, vDCBotMin, vDCTopMax, vDCBotMax;
   std::vector<Float64> vDWTopMin, vDWBotMin, vDWTopMax, vDWBotMax;
   std::vector<Float64> vCRTopMin, vCRBotMin, vCRTopMax, vCRBotMax;
   std::vector<Float64> vSHTopMin, vSHBotMin, vSHTopMax, vSHBotMax;
   std::vector<Float64> vRETopMin, vREBotMin, vRETopMax, vREBotMax;
   std::vector<Float64> vPSTopMin, vPSBotMin, vPSTopMax, vPSBotMax;
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
      pCombinedForces->GetStress(loadRatingIntervalIdx,lcDC,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative,topLocation,botLocation,&vDCTopMin,&vDCBotMin);
      pCombinedForces->GetStress(loadRatingIntervalIdx,lcDC,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative,topLocation,botLocation,&vDCTopMax,&vDCBotMax);
      pCombinedForces->GetStress(loadRatingIntervalIdx,lcDWRating,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative,topLocation,botLocation,&vDWTopMin,&vDWBotMin);
      pCombinedForces->GetStress(loadRatingIntervalIdx,lcDWRating,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative,topLocation,botLocation,&vDWTopMax,&vDWBotMax);

      pCombinedForces->GetStress(loadRatingIntervalIdx,lcCR,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative,topLocation,botLocation,&vCRTopMin,&vCRBotMin);
      pCombinedForces->GetStress(loadRatingIntervalIdx,lcCR,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative,topLocation,botLocation,&vCRTopMax,&vCRBotMax);
      pCombinedForces->GetStress(loadRatingIntervalIdx,lcSH,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative,topLocation,botLocation,&vSHTopMin,&vSHBotMin);
      pCombinedForces->GetStress(loadRatingIntervalIdx,lcSH,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative,topLocation,botLocation,&vSHTopMax,&vSHBotMax);
      pCombinedForces->GetStress(loadRatingIntervalIdx,lcRE,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative,topLocation,botLocation,&vRETopMin,&vREBotMin);
      pCombinedForces->GetStress(loadRatingIntervalIdx,lcRE,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative,topLocation,botLocation,&vRETopMax,&vREBotMax);
      pCombinedForces->GetStress(loadRatingIntervalIdx,lcPS,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative,topLocation,botLocation,&vPSTopMin,&vPSBotMin);
      pCombinedForces->GetStress(loadRatingIntervalIdx,lcPS,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative,topLocation,botLocation,&vPSTopMax,&vPSBotMax);

      if ( vehicleIdx == INVALID_INDEX )
      {
         pProductForces->GetLiveLoadStress(loadRatingIntervalIdx,llType,vPoi,pgsTypes::MinSimpleContinuousEnvelope,true,true,topLocation,botLocation,&vLLIMTopMin,&vUnused1,&vLLIMBotMin,&vUnused2,&vTruckIndexTopMin, &vUnusedIndex1, &vTruckIndexBotMin, &vUnusedIndex2);
         pProductForces->GetLiveLoadStress(loadRatingIntervalIdx,llType,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,true,true,topLocation,botLocation,&vUnused1,&vLLIMTopMax,&vUnused2,&vLLIMBotMax,&vUnusedIndex1, &vTruckIndexTopMax, &vUnusedIndex2, &vTruckIndexBotMax);
      }
      else
      {
         pProductForces->GetVehicularLiveLoadStress(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,pgsTypes::MinSimpleContinuousEnvelope,true,true,topLocation,botLocation,&vLLIMTopMin,&vUnused1,&vLLIMBotMin,&vUnused2,NULL,NULL,NULL,NULL);
         pProductForces->GetVehicularLiveLoadStress(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,true,true,topLocation,botLocation,&vUnused1,&vLLIMTopMax,&vUnused2,&vLLIMBotMax,NULL,NULL,NULL,NULL);
      }

      pCombinedForces->GetCombinedLiveLoadStress( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, topLocation, botLocation, &vUnused1,  &vPLTopMax, &vUnused2, &vPLBotMax );
      pCombinedForces->GetCombinedLiveLoadStress( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, pgsTypes::MinSimpleContinuousEnvelope, topLocation, botLocation, &vPLTopMin, &vUnused2,  &vPLBotMin, &vUnused2 );
   }
   else
   {
      pCombinedForces->GetStress(loadRatingIntervalIdx,lcDC,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative,topLocation,botLocation,&vDCTopMin,&vDCBotMin);
      vDCTopMax = vDCTopMin;
      vDCBotMax = vDCBotMin;

      pCombinedForces->GetStress(loadRatingIntervalIdx,lcDWRating,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative,topLocation,botLocation,&vDWTopMin,&vDWBotMin);
      vDWTopMax = vDWTopMin;
      vDWBotMax = vDWBotMin;

      pCombinedForces->GetStress(loadRatingIntervalIdx,lcCR,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative,topLocation,botLocation,&vCRTopMin,&vCRBotMin);
      vCRTopMax = vCRTopMin;
      vCRBotMax = vCRBotMin;

      pCombinedForces->GetStress(loadRatingIntervalIdx,lcSH,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative,topLocation,botLocation,&vSHTopMin,&vSHBotMin);
      vSHTopMax = vSHTopMin;
      vSHBotMax = vSHBotMin;

      pCombinedForces->GetStress(loadRatingIntervalIdx,lcRE,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative,topLocation,botLocation,&vRETopMin,&vREBotMin);
      vRETopMax = vRETopMin;
      vREBotMax = vREBotMin;

      pCombinedForces->GetStress(loadRatingIntervalIdx,lcPS,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative,topLocation,botLocation,&vPSTopMin,&vPSBotMin);
      vPSTopMax = vPSTopMin;
      vPSBotMax = vPSBotMin;

      if ( vehicleIdx == INVALID_INDEX )
      {
         pProductForces->GetLiveLoadStress(loadRatingIntervalIdx,llType,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,topLocation,botLocation,&vLLIMTopMin,&vLLIMTopMax,&vLLIMBotMin,&vLLIMBotMax,&vTruckIndexTopMin, &vTruckIndexTopMax, &vTruckIndexBotMin, &vTruckIndexBotMax);
      }
      else
      {
         pProductForces->GetVehicularLiveLoadStress(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,topLocation,botLocation,&vLLIMTopMin,&vLLIMTopMax,&vLLIMBotMin,&vLLIMBotMax,NULL,NULL,NULL,NULL);
      }

      pCombinedForces->GetCombinedLiveLoadStress( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, topLocation, botLocation, &vPLTopMin, &vPLTopMax,  &vPLBotMin, &vPLBotMax );
   }

   ATLASSERT(vPoi.size() == vDCTopMin.size());
   ATLASSERT(vPoi.size() == vDCTopMax.size());
   ATLASSERT(vPoi.size() == vDCBotMin.size());
   ATLASSERT(vPoi.size() == vDCBotMax.size());

   ATLASSERT(vPoi.size() == vDWTopMin.size());
   ATLASSERT(vPoi.size() == vDWTopMax.size());
   ATLASSERT(vPoi.size() == vDWBotMin.size());
   ATLASSERT(vPoi.size() == vDWBotMax.size());

   ATLASSERT(vPoi.size() == vCRTopMin.size());
   ATLASSERT(vPoi.size() == vCRTopMax.size());
   ATLASSERT(vPoi.size() == vCRBotMin.size());
   ATLASSERT(vPoi.size() == vCRBotMax.size());

   ATLASSERT(vPoi.size() == vSHTopMin.size());
   ATLASSERT(vPoi.size() == vSHTopMax.size());
   ATLASSERT(vPoi.size() == vSHBotMin.size());
   ATLASSERT(vPoi.size() == vSHBotMax.size());

   ATLASSERT(vPoi.size() == vRETopMin.size());
   ATLASSERT(vPoi.size() == vRETopMax.size());
   ATLASSERT(vPoi.size() == vREBotMin.size());
   ATLASSERT(vPoi.size() == vREBotMax.size());

   ATLASSERT(vPoi.size() == vPSTopMin.size());
   ATLASSERT(vPoi.size() == vPSTopMax.size());
   ATLASSERT(vPoi.size() == vPSBotMin.size());
   ATLASSERT(vPoi.size() == vPSBotMax.size());

   ATLASSERT(vPoi.size() == vLLIMTopMin.size());
   ATLASSERT(vPoi.size() == vLLIMTopMax.size());
   ATLASSERT(vPoi.size() == vLLIMBotMin.size());
   ATLASSERT(vPoi.size() == vLLIMBotMax.size());

   ATLASSERT(vPoi.size() == vPLTopMin.size());
   ATLASSERT(vPoi.size() == vPLTopMax.size());
   ATLASSERT(vPoi.size() == vPLBotMin.size());
   ATLASSERT(vPoi.size() == vPLBotMax.size());

   GET_IFACE(IPretensionStresses,pPrestress);
   std::vector<Float64> vPS = pPrestress->GetStress(loadRatingIntervalIdx,vPoi,pgsTypes::BottomGirder);

   GET_IFACE(IPosttensionStresses,pPostTension); // this is the primary, direct post-tensioning stress (P*e)
   std::vector<Float64> vPT = pPostTension->GetStress(loadRatingIntervalIdx,vPoi,pgsTypes::BottomGirder,ALL_DUCTS);

   GET_IFACE(IRatingSpecification,pRatingSpec);

   Float64 system_factor    = pRatingSpec->GetSystemFactorFlexure();
   bool bIncludePL = pRatingSpec->IncludePedestrianLiveLoad();

   pgsTypes::LimitState ls = GetServiceLimitStateType(ratingType);

   Float64 gDC = pRatingSpec->GetDeadLoadFactor(ls);
   Float64 gDW = pRatingSpec->GetWearingSurfaceFactor(ls);
   Float64 gCR = pRatingSpec->GetCreepFactor(ls);
   Float64 gSH = pRatingSpec->GetShrinkageFactor(ls);
   Float64 gRE = pRatingSpec->GetRelaxationFactor(ls);
   Float64 gPS = pRatingSpec->GetSecondaryEffectsFactor(ls);

   GET_IFACE(IProductLoads,pProductLoads);
   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

   CollectionIndexType nPOI = vPoi.size();
   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      const pgsPointOfInterest& poi = vPoi[i];

      // do this in the loop because the vector of POI can be for multiple segments
      Float64 condition_factor = pRatingSpec->GetGirderConditionFactor(poi.GetSegmentKey());
      Float64 fr               = pRatingSpec->GetAllowableTension(ratingType,poi.GetSegmentKey());
      Float64 ps   = vPS[i];
      Float64 pt   = vPT[i];

      Float64 DC   = vDCBotMax[i];
      Float64 DW   = vDWBotMax[i];
      Float64 CR   = vCRBotMax[i];
      Float64 SH   = vSHBotMax[i];
      Float64 RE   = vREBotMax[i];
      Float64 PS   = vPSBotMax[i];
      Float64 LLIM = vLLIMBotMax[i];
      Float64 PL   = (bIncludePL ? vPLBotMax[i] : 0);

      VehicleIndexType truck_index = vehicleIdx;
      if ( vehicleIdx == INVALID_INDEX )
      {
         truck_index = vTruckIndexBotMax[i];
      }

      Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
      if ( gLL < 0 )
      {
         if ( ::IsStrengthLimitState(ls) )
         {
            // need to compute gLL based on axle weights
            GET_IFACE(IProductForces,pProductForce);

            Float64 fMinTop, fMinBot, fMaxTop, fMaxBot, fDummyTop, fDummyBot;
            AxleConfiguration MinAxleConfigTop, MinAxleConfigBot, MaxAxleConfigTop, MaxAxleConfigBot, DummyAxleConfigTop, DummyAxleConfigBot;
            if ( analysisType == pgsTypes::Envelope )
            {
               pProductForce->GetVehicularLiveLoadStress(loadRatingIntervalIdx,llType,truck_index,poi,pgsTypes::MinSimpleContinuousEnvelope,true,true,topLocation,botLocation,&fMinTop,&fDummyTop,&fMinBot,&fDummyBot,&MinAxleConfigTop,&DummyAxleConfigTop,&MinAxleConfigBot,&DummyAxleConfigBot);
               pProductForce->GetVehicularLiveLoadStress(loadRatingIntervalIdx,llType,truck_index,poi,pgsTypes::MaxSimpleContinuousEnvelope,true,true,topLocation,botLocation,&fDummyTop,&fMaxTop,&fDummyBot,&fMaxBot,&DummyAxleConfigTop,&MaxAxleConfigTop,&DummyAxleConfigBot,&MaxAxleConfigBot);
            }
            else
            {
               pProductForce->GetVehicularLiveLoadStress(loadRatingIntervalIdx,llType,truck_index,poi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,topLocation,botLocation,&fMinTop,&fMaxTop,&fMaxTop,&fMaxBot,&MinAxleConfigTop,&MaxAxleConfigTop,&MinAxleConfigBot,&MaxAxleConfigBot);
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
      stressArtifact.SetPrestressStress(ps);
      stressArtifact.SetPostTensionStress(pt);
      stressArtifact.SetDeadLoadFactor(gDC);
      stressArtifact.SetDeadLoadStress(DC);
      stressArtifact.SetWearingSurfaceFactor(gDW);
      stressArtifact.SetWearingSurfaceStress(DW);
      stressArtifact.SetCreepFactor(gCR);
      stressArtifact.SetCreepStress(CR);
      stressArtifact.SetShrinkageFactor(gSH);
      stressArtifact.SetShrinkageStress(SH);
      stressArtifact.SetRelaxationFactor(gRE);
      stressArtifact.SetRelaxationStress(RE);
      stressArtifact.SetSecondaryEffectsFactor(gPS);
      stressArtifact.SetSecondaryEffectsStress(PS);
      stressArtifact.SetLiveLoadFactor(gLL);
      stressArtifact.SetLiveLoadStress(LLIM+PL);
   
      ratingArtifact.AddArtifact(poi,stressArtifact);
   }
}

void pgsLoadRater::CheckReinforcementYielding(const CGirderKey& girderKey,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,bool bPositiveMoment,pgsRatingArtifact& ratingArtifact)
{
   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);

   if ( bPositiveMoment )
   {
      GET_IFACE(IProductLoads,pProductLoads);
      VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
      VehicleIndexType startVehicleIdx = (vehicleIdx == INVALID_INDEX ? 0 : vehicleIdx);
      VehicleIndexType endVehicleIdx   = (vehicleIdx == INVALID_INDEX ? nVehicles-1: startVehicleIdx);
      for ( VehicleIndexType vehIdx = startVehicleIdx; vehIdx <= endVehicleIdx; vehIdx++ )
      {
         pgsTypes::LiveLoadApplicabilityType applicability = pProductLoads->GetLiveLoadApplicability(llType,vehIdx);
         if ( applicability == pgsTypes::llaNegMomentAndInteriorPierReaction )
         {
            // we are processing positive moments and the live load vehicle is only applicable to negative moments
            return;
         }
      }
   }

   CGirderKey thisGirderKey(girderKey);
   if ( thisGirderKey.groupIndex == ALL_GROUPS )
   {
      thisGirderKey.groupIndex = 0;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType loadRatingIntervalIdx = pIntervals->GetLoadRatingInterval(thisGirderKey);

   ATLASSERT(ratingType == pgsTypes::lrPermit_Routine || ratingType == pgsTypes::lrPermit_Special);

   std::vector<Float64> vDCmin, vDCmax;
   std::vector<Float64> vDWmin, vDWmax;
   std::vector<Float64> vCRmin, vCRmax;
   std::vector<Float64> vSHmin, vSHmax;
   std::vector<Float64> vREmin, vREmax;
   std::vector<Float64> vPSmin, vPSmax;
   std::vector<Float64> vLLIMmin,vLLIMmax;
   std::vector<VehicleIndexType> vMinTruckIndex, vMaxTruckIndex;
   std::vector<Float64> vPLmin,vPLmax;
   GetMoments(girderKey,bPositiveMoment,ratingType, vehicleIdx, vPoi, vDCmin, vDCmax, vDWmin, vDWmax, vCRmin, vCRmax, vSHmin, vSHmax, vREmin, vREmax, vPSmin, vPSmax, vLLIMmin, vMinTruckIndex, vLLIMmax, vMaxTruckIndex, vPLmin, vPLmax);

   pgsTypes::LimitState ls = GetServiceLimitStateType(ratingType);

   GET_IFACE(IRatingSpecification,pRatingSpec);
   bool bIncludePL = pRatingSpec->IncludePedestrianLiveLoad();

   GET_IFACE(IMomentCapacity,pMomentCapacity);
   std::vector<CRACKINGMOMENTDETAILS> vMcr = pMomentCapacity->GetCrackingMomentDetails(loadRatingIntervalIdx,vPoi,bPositiveMoment);
   std::vector<MOMENTCAPACITYDETAILS> vM = pMomentCapacity->GetMomentCapacityDetails(loadRatingIntervalIdx,vPoi,bPositiveMoment);

   GET_IFACE(ICrackedSection,pCrackedSection);
   std::vector<CRACKEDSECTIONDETAILS> vCrackedSection = pCrackedSection->GetCrackedSectionDetails(vPoi,bPositiveMoment);
   
   ATLASSERT(vPoi.size()     == vDCmax.size());
   ATLASSERT(vPoi.size()     == vDWmax.size());
   ATLASSERT(vPoi.size()     == vCRmax.size());
   ATLASSERT(vPoi.size()     == vSHmax.size());
   ATLASSERT(vPoi.size()     == vREmax.size());
   ATLASSERT(vPoi.size()     == vPSmax.size());
   ATLASSERT(vPoi.size()     == vLLIMmax.size());
   ATLASSERT(vPoi.size()     == vMcr.size());
   ATLASSERT(vPoi.size()     == vM.size());
   ATLASSERT(vPoi.size()     == vCrackedSection.size());
   ATLASSERT(vDCmin.size()   == vDCmax.size());
   ATLASSERT(vDWmin.size()   == vDWmax.size());
   ATLASSERT(vCRmin.size()   == vCRmax.size());
   ATLASSERT(vSHmin.size()   == vSHmax.size());
   ATLASSERT(vREmin.size()   == vREmax.size());
   ATLASSERT(vPSmin.size()   == vPSmax.size());
   ATLASSERT(vLLIMmin.size() == vLLIMmax.size());
   ATLASSERT(vPLmin.size()   == vPLmax.size());


   // Get load factors
   Float64 gDC = pRatingSpec->GetDeadLoadFactor(ls);
   Float64 gDW = pRatingSpec->GetWearingSurfaceFactor(ls);
   Float64 gCR = pRatingSpec->GetCreepFactor(ls);
   Float64 gSH = pRatingSpec->GetShrinkageFactor(ls);
   Float64 gRE = pRatingSpec->GetRelaxationFactor(ls);
   Float64 gPS = pRatingSpec->GetSecondaryEffectsFactor(ls);

   // parameter needed for negative moment evalation
   // (get it outside the loop so we don't have to get it over and over)
   GET_IFACE(ILongRebarGeometry,pLongRebar);
   Float64 top_slab_cover = pLongRebar->GetCoverTopMat();

   GET_IFACE(IProductLoads,pProductLoads);
   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   pgsTypes::StressLocation topLocation = pgsTypes::TopGirder;
   pgsTypes::StressLocation botLocation = pgsTypes::BottomGirder;

   // Create artifacts
   CollectionIndexType nPOI = vPoi.size();
   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      const pgsPointOfInterest& poi = vPoi[i];

      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      // Get material properties
      GET_IFACE(IMaterials,pMaterials);
      Float64 Eg = pMaterials->GetSegmentEc(segmentKey,loadRatingIntervalIdx);
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
      
         GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
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
      Float64 CR   = (bPositiveMoment ? vCRmax[i]   : vCRmin[i]);
      Float64 SH   = (bPositiveMoment ? vSHmax[i]   : vSHmin[i]);
      Float64 RE   = (bPositiveMoment ? vREmax[i]   : vREmin[i]);
      Float64 PS   = (bPositiveMoment ? vPSmax[i]   : vPSmin[i]);
      Float64 LLIM = (bPositiveMoment ? vLLIMmax[i] : vLLIMmin[i]);
      Float64 PL   = (bIncludePL ? (bPositiveMoment ? vPLmax[i]   : vPLmin[i]) : 0.0);
      VehicleIndexType truck_index = vehicleIdx;
      if ( vehicleIdx == INVALID_INDEX )
      {
         truck_index = (bPositiveMoment ? vMaxTruckIndex[i] : vMinTruckIndex[i]);
      }

      Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
      if ( gLL < 0 )
      {
         if ( ::IsStrengthLimitState(ls) )
         {
            // need to compute gLL based on axle weights
            GET_IFACE(IProductForces,pProductForces);
            Float64 fMinTop, fMinBot, fMaxTop, fMaxBot, fDummyTop, fDummyBot;
            AxleConfiguration MinAxleConfigTop, MinAxleConfigBot, MaxAxleConfigTop, MaxAxleConfigBot, DummyAxleConfigTop, DummyAxleConfigBot;
            if ( analysisType == pgsTypes::Envelope )
            {
               pProductForces->GetVehicularLiveLoadStress(loadRatingIntervalIdx,llType,truck_index,poi,pgsTypes::MinSimpleContinuousEnvelope,true,true,topLocation,botLocation,&fMinTop,&fDummyTop,&fMinBot,&fDummyBot,&MinAxleConfigTop,&DummyAxleConfigTop,&MinAxleConfigBot,&DummyAxleConfigBot);
               pProductForces->GetVehicularLiveLoadStress(loadRatingIntervalIdx,llType,truck_index,poi,pgsTypes::MaxSimpleContinuousEnvelope,true,true,topLocation,botLocation,&fDummyTop,&fMaxTop,&fDummyBot,&fMaxBot,&DummyAxleConfigTop,&MaxAxleConfigTop,&DummyAxleConfigBot,&MaxAxleConfigBot);
            }
            else
            {
               pProductForces->GetVehicularLiveLoadStress(loadRatingIntervalIdx,llType,truck_index,poi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,topLocation,botLocation,&fMinTop,&fMaxTop,&fMaxTop,&fMaxBot,&MinAxleConfigTop,&MaxAxleConfigTop,&MinAxleConfigBot,&MaxAxleConfigBot);
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
         GET_IFACE(IPretensionForce,pPrestressForce);
         fpe = pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,loadRatingIntervalIdx,pgsTypes::Middle);
      }
      else
      {
         // negative moment - compute stress in deck rebar for uncracked section
         GET_IFACE(ISectionProperties,pSectProp);
         Float64 I = pSectProp->GetIx(loadRatingIntervalIdx,poi);
         Float64 y = pSectProp->GetY(loadRatingIntervalIdx,poi,pgsTypes::TopDeck);
         
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
      stressRatioArtifact.SetCreepFactor(gCR);
      stressRatioArtifact.SetCreepMoment(CR);
      stressRatioArtifact.SetShrinkageFactor(gSH);
      stressRatioArtifact.SetShrinkageMoment(SH);
      stressRatioArtifact.SetRelaxationFactor(gRE);
      stressRatioArtifact.SetRelaxationMoment(RE);
      stressRatioArtifact.SetSecondaryEffectsFactor(gPS);
      stressRatioArtifact.SetSecondaryEffectsMoment(PS);
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

void pgsLoadRater::GetMoments(const CGirderKey& girderKey,bool bPositiveMoment,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx, const std::vector<pgsPointOfInterest>& vPoi, std::vector<Float64>& vDCmin, std::vector<Float64>& vDCmax,std::vector<Float64>& vDWmin, std::vector<Float64>& vDWmax,std::vector<Float64>& vCRmin, std::vector<Float64>& vCRmax,std::vector<Float64>& vSHmin, std::vector<Float64>& vSHmax,std::vector<Float64>& vREmin, std::vector<Float64>& vREmax,std::vector<Float64>& vPSmin, std::vector<Float64>& vPSmax, std::vector<Float64>& vLLIMmin, std::vector<VehicleIndexType>& vMinTruckIndex,std::vector<Float64>& vLLIMmax,std::vector<VehicleIndexType>& vMaxTruckIndex,std::vector<Float64>& vPLmin,std::vector<Float64>& vPLmax)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bIncludeNoncompositeMoments = pSpecEntry->IncludeNoncompositeMomentsForNegMomentDesign();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   CGirderKey thisGirderKey(girderKey);
   if ( thisGirderKey.groupIndex == ALL_GROUPS )
   {
      thisGirderKey.groupIndex = 0;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval(thisGirderKey);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(thisGirderKey);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval(thisGirderKey);
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval(thisGirderKey);
   IntervalIndexType loadRatingIntervalIdx    = pIntervals->GetLoadRatingInterval(thisGirderKey);

   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);

   std::vector<Float64> vUnused;
   std::vector<VehicleIndexType> vUnusedIndex;

   GET_IFACE(ICombinedForces2,pCombinedForces);
   GET_IFACE(IProductForces2,pProductForces);
   if ( bPositiveMoment || (!bPositiveMoment && bIncludeNoncompositeMoments) )
   {
      if ( analysisType == pgsTypes::Envelope )
      {
         vDCmin = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcDC,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
         vDCmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcDC,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);

         vDWmin = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcDWRating,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
         vDWmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcDWRating,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);

         vCRmin = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcCR,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
         vCRmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcCR,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);

         vSHmin = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcSH,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
         vSHmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcSH,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);

         vREmin = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcRE,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
         vREmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcRE,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);

         vPSmin = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcPS,vPoi,pgsTypes::MinSimpleContinuousEnvelope,rtCumulative);
         vPSmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcPS,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,rtCumulative);

         if ( vehicleIdx == INVALID_INDEX )
         {
            pProductForces->GetLiveLoadMoment(loadRatingIntervalIdx,llType,vPoi,pgsTypes::MinSimpleContinuousEnvelope,true,true,&vLLIMmin,&vUnused,&vMinTruckIndex,&vUnusedIndex);
            pProductForces->GetLiveLoadMoment(loadRatingIntervalIdx,llType,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,true,true,&vUnused,&vLLIMmax,&vUnusedIndex,&vMaxTruckIndex);
         }
         else
         {
            pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,pgsTypes::MinSimpleContinuousEnvelope,true,true,&vLLIMmin,&vUnused,NULL,NULL);
            pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,true,true,&vUnused,&vLLIMmax,NULL,NULL);
         }

         pCombinedForces->GetCombinedLiveLoadMoment( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &vUnused, &vPLmax );
         pCombinedForces->GetCombinedLiveLoadMoment( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &vPLmin, &vUnused );
      }
      else
      {
         vDCmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcDC,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative);
         vDCmin = vDCmax;

         vDWmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcDWRating,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative);
         vDWmin = vDWmax;
         
         vCRmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcCR,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative);
         vCRmin = vCRmax;
         
         vSHmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcSH,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative);
         vSHmin = vSHmax;
         
         vREmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcRE,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative);
         vREmin = vREmax;
         
         vPSmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcPS,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,rtCumulative);
         vPSmin = vPSmax;

         if ( vehicleIdx == INVALID_INDEX )
         {
            pProductForces->GetLiveLoadMoment(loadRatingIntervalIdx,llType,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,&vLLIMmin,&vLLIMmax,&vMinTruckIndex,&vMaxTruckIndex);
         }
         else
         {
            pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,&vLLIMmin,&vLLIMmax,NULL,NULL);
         }

         pCombinedForces->GetCombinedLiveLoadMoment( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &vPLmin, &vPLmax );
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
      std::vector<Float64> vCreepMin, vCreepMax;
      std::vector<Float64> vShrinkageMin, vShrinkageMax;
      std::vector<Float64> vRelaxationMin, vRelaxationMax;
      std::vector<Float64> vSecondaryMin, vSecondaryMax;

      bool bFutureOverlay = pBridge->HasOverlay() && pBridge->IsFutureOverlay();

      // Get all the product load responces
      GET_IFACE(IProductForces2,pProductForces);
      if ( analysisType == pgsTypes::Envelope )
      {
         // Get all the product loads
         vConstructionMin = pProductForces->GetMoment(castDeckIntervalIdx,pftConstruction,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtIncremental);
         vConstructionMax = pProductForces->GetMoment(castDeckIntervalIdx,pftConstruction,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtIncremental);

         vSlabMin = pProductForces->GetMoment(castDeckIntervalIdx,pftSlab,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtIncremental);
         vSlabMax = pProductForces->GetMoment(castDeckIntervalIdx,pftSlab,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtIncremental);

         vSlabPanelMin = pProductForces->GetMoment(castDeckIntervalIdx,pftSlabPanel,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtIncremental);
         vSlabPanelMax = pProductForces->GetMoment(castDeckIntervalIdx,pftSlabPanel,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtIncremental);

         vDiaphragmMin = pProductForces->GetMoment(castDeckIntervalIdx,pftDiaphragm,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtIncremental);
         vDiaphragmMax = pProductForces->GetMoment(castDeckIntervalIdx,pftDiaphragm,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtIncremental);

         vShearKeyMin = pProductForces->GetMoment(castDeckIntervalIdx,pftShearKey,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtIncremental);
         vShearKeyMax = pProductForces->GetMoment(castDeckIntervalIdx,pftShearKey,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtIncremental);

         vUserDC1Min = pProductForces->GetMoment(castDeckIntervalIdx,pftUserDC,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtIncremental);
         vUserDC1Max = pProductForces->GetMoment(castDeckIntervalIdx,pftUserDC,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtIncremental);

         vUserDW1Min = pProductForces->GetMoment(castDeckIntervalIdx,pftUserDW,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtIncremental);
         vUserDW1Max = pProductForces->GetMoment(castDeckIntervalIdx,pftUserDW,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtIncremental);

         vUserDC2Min = pProductForces->GetMoment(compositeDeckIntervalIdx,pftUserDC,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtIncremental);
         vUserDC2Max = pProductForces->GetMoment(compositeDeckIntervalIdx,pftUserDC,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtIncremental);

         vUserDW2Min = pProductForces->GetMoment(compositeDeckIntervalIdx,pftUserDW,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtIncremental);
         vUserDW2Max = pProductForces->GetMoment(compositeDeckIntervalIdx,pftUserDW,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtIncremental);

         vTrafficBarrierMin = pProductForces->GetMoment(railingSystemIntervalIdx,pftTrafficBarrier,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtIncremental);
         vTrafficBarrierMax = pProductForces->GetMoment(railingSystemIntervalIdx,pftTrafficBarrier,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtIncremental);

         vSidewalkMin = pProductForces->GetMoment(railingSystemIntervalIdx,pftSidewalk,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtIncremental);
         vSidewalkMax = pProductForces->GetMoment(railingSystemIntervalIdx,pftSidewalk,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtIncremental);

         if ( !bFutureOverlay )
         {
            vOverlayMin = pProductForces->GetMoment(overlayIntervalIdx,pftOverlay,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtIncremental);
            vOverlayMax = pProductForces->GetMoment(overlayIntervalIdx,pftOverlay,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtIncremental);
         }
         else
         {
            vOverlayMin.resize(vPoi.size(),0);
            vOverlayMax.resize(vPoi.size(),0);
         }

         vCreepMin = pProductForces->GetMoment(loadRatingIntervalIdx,pftCreep,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtCumulative);
         vCreepMax = pProductForces->GetMoment(loadRatingIntervalIdx,pftCreep,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtCumulative);

         vShrinkageMin = pProductForces->GetMoment(loadRatingIntervalIdx,pftShrinkage,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtCumulative);
         vShrinkageMax = pProductForces->GetMoment(loadRatingIntervalIdx,pftShrinkage,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtCumulative);

         vRelaxationMin = pProductForces->GetMoment(loadRatingIntervalIdx,pftRelaxation,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtCumulative);
         vRelaxationMax = pProductForces->GetMoment(loadRatingIntervalIdx,pftRelaxation,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtCumulative);

         vSecondaryMin = pProductForces->GetMoment(loadRatingIntervalIdx,pftSecondaryEffects,vPoi,pgsTypes::MinSimpleContinuousEnvelope, rtCumulative);
         vSecondaryMax = pProductForces->GetMoment(loadRatingIntervalIdx,pftSecondaryEffects,vPoi,pgsTypes::MaxSimpleContinuousEnvelope, rtCumulative);

         if ( vehicleIdx == INVALID_INDEX )
         {
            pProductForces->GetLiveLoadMoment( loadRatingIntervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, true, &vLLIMmin, &vUnused, &vMinTruckIndex, &vUnusedIndex );
            pProductForces->GetLiveLoadMoment( loadRatingIntervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, true, &vUnused, &vLLIMmax, &vUnusedIndex, &vMaxTruckIndex );
         }
         else
         {
            pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,pgsTypes::MinSimpleContinuousEnvelope,true,true,&vLLIMmin,&vUnused,NULL,NULL);
            pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,pgsTypes::MaxSimpleContinuousEnvelope,true,true,&vUnused,&vLLIMmax,NULL,NULL);
         }
      }
      else
      {
         vConstructionMin = pProductForces->GetMoment(castDeckIntervalIdx,pftConstruction,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental);
         vConstructionMax = vConstructionMin;

         vSlabMin = pProductForces->GetMoment(castDeckIntervalIdx,pftSlab,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental);
         vSlabMax = vSlabMin;

         vSlabPanelMin = pProductForces->GetMoment(castDeckIntervalIdx,pftSlabPanel,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental);
         vSlabPanelMax = vSlabPanelMin;

         vDiaphragmMin = pProductForces->GetMoment(castDeckIntervalIdx,pftDiaphragm,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental);
         vDiaphragmMax = vDiaphragmMin;

         vShearKeyMin = pProductForces->GetMoment(castDeckIntervalIdx,pftShearKey,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental);
         vShearKeyMax = vShearKeyMin;

         vUserDC1Min = pProductForces->GetMoment(castDeckIntervalIdx,pftUserDC,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental);
         vUserDC1Max = vUserDC1Min;

         vUserDW1Min = pProductForces->GetMoment(castDeckIntervalIdx,pftUserDW,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental);
         vUserDW1Max = vUserDW1Min;

         vUserDC2Min = pProductForces->GetMoment(compositeDeckIntervalIdx,pftUserDC,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental);
         vUserDC2Max = vUserDC2Min;

         vUserDW2Min = pProductForces->GetMoment(compositeDeckIntervalIdx,pftUserDW,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental);
         vUserDW2Max = vUserDW2Min;

         vTrafficBarrierMin = pProductForces->GetMoment(railingSystemIntervalIdx,pftTrafficBarrier,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental);
         vTrafficBarrierMax = vTrafficBarrierMin;

         vSidewalkMin = pProductForces->GetMoment(railingSystemIntervalIdx,pftSidewalk,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental);
         vSidewalkMax = vSidewalkMin;

         vCreepMin = pProductForces->GetMoment(loadRatingIntervalIdx,pftCreep,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
         vCreepMax = vCreepMin;

         vShrinkageMin = pProductForces->GetMoment(loadRatingIntervalIdx,pftShrinkage,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
         vShrinkageMax = vShrinkageMin;

         vRelaxationMin = pProductForces->GetMoment(loadRatingIntervalIdx,pftRelaxation,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
         vRelaxationMax = vRelaxationMin;

         vSecondaryMin = pProductForces->GetMoment(loadRatingIntervalIdx,pftSecondaryEffects,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtCumulative);
         vSecondaryMax = vSecondaryMin;

         if ( !bFutureOverlay )
         {
            vOverlayMin = pProductForces->GetMoment(overlayIntervalIdx,pftOverlay,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, rtIncremental);
         }
         else
         {
            vOverlayMin.resize(vPoi.size(),0);
         }
         vOverlayMax = vOverlayMin;

         if ( vehicleIdx == INVALID_INDEX )
         {
            pProductForces->GetLiveLoadMoment(loadRatingIntervalIdx,llType,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,&vLLIMmin,&vLLIMmax,&vMinTruckIndex,&vMaxTruckIndex);
         }
         else
         {
            pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,true,true,&vLLIMmin,&vLLIMmax,NULL,NULL);
         }

         pCombinedForces->GetCombinedLiveLoadMoment( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &vPLmin, &vPLmax );
      }

      // sum DC and DW

      // initialize
      vDCmin.resize(vPoi.size(),0);
      vDCmax.resize(vPoi.size(),0);
      vDWmin.resize(vPoi.size(),0);
      vDWmax.resize(vPoi.size(),0);
      vCRmin.resize(vPoi.size(),0);
      vCRmax.resize(vPoi.size(),0);
      vSHmin.resize(vPoi.size(),0);
      vSHmax.resize(vPoi.size(),0);
      vREmin.resize(vPoi.size(),0);
      vREmax.resize(vPoi.size(),0);
      vPSmin.resize(vPoi.size(),0);
      vPSmax.resize(vPoi.size(),0);

      GET_IFACE(IPointOfInterest,pPoi);
      special_transform(pBridge,pPoi,pIntervals,vPoi.begin(),vPoi.end(),vConstructionMin.begin(),vDCmin.begin(),vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.begin(),vPoi.end(),vConstructionMax.begin(),vDCmax.begin(),vDCmax.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.begin(),vPoi.end(),vSlabMin.begin(),vDCmin.begin(),vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.begin(),vPoi.end(),vSlabMax.begin(),vDCmax.begin(),vDCmax.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.begin(),vPoi.end(),vSlabPanelMin.begin(),vDCmin.begin(),vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.begin(),vPoi.end(),vSlabPanelMax.begin(),vDCmax.begin(),vDCmax.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.begin(),vPoi.end(),vDiaphragmMin.begin(),vDCmin.begin(),vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.begin(),vPoi.end(),vDiaphragmMax.begin(),vDCmax.begin(),vDCmax.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.begin(),vPoi.end(),vShearKeyMin.begin(),vDCmin.begin(),vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.begin(),vPoi.end(),vShearKeyMax.begin(),vDCmax.begin(),vDCmax.begin());

      std::transform(vUserDC1Min.begin(),vUserDC1Min.end(),vDCmin.begin(),vDCmin.begin(),std::plus<Float64>());
      std::transform(vUserDC1Max.begin(),vUserDC1Max.end(),vDCmax.begin(),vDCmax.begin(),std::plus<Float64>());

      std::transform(vUserDW1Min.begin(),vUserDW1Min.end(),vDWmin.begin(),vDWmin.begin(),std::plus<Float64>());
      std::transform(vUserDW1Max.begin(),vUserDW1Max.end(),vDWmax.begin(),vDWmax.begin(),std::plus<Float64>());

      std::transform(vUserDC2Min.begin(),vUserDC2Min.end(),vDCmin.begin(),vDCmin.begin(),std::plus<Float64>());
      std::transform(vUserDC2Max.begin(),vUserDC2Max.end(),vDCmax.begin(),vDCmax.begin(),std::plus<Float64>());

      std::transform(vUserDW2Min.begin(),vUserDW2Min.end(),vDWmin.begin(),vDWmin.begin(),std::plus<Float64>());
      std::transform(vUserDW2Max.begin(),vUserDW2Max.end(),vDWmax.begin(),vDWmax.begin(),std::plus<Float64>());

      std::transform(vTrafficBarrierMin.begin(),vTrafficBarrierMin.end(),vDCmin.begin(),vDCmin.begin(),std::plus<Float64>());
      std::transform(vTrafficBarrierMax.begin(),vTrafficBarrierMax.end(),vDCmax.begin(),vDCmax.begin(),std::plus<Float64>());

      std::transform(vSidewalkMin.begin(),vSidewalkMin.end(),vDCmin.begin(),vDCmin.begin(),std::plus<Float64>());
      std::transform(vSidewalkMax.begin(),vSidewalkMax.end(),vDCmax.begin(),vDCmax.begin(),std::plus<Float64>());

      std::transform(vOverlayMin.begin(),vOverlayMin.end(),vDWmin.begin(),vDWmin.begin(),std::plus<Float64>());
      std::transform(vOverlayMax.begin(),vOverlayMax.end(),vDWmax.begin(),vDWmax.begin(),std::plus<Float64>());

      std::transform(vCreepMin.begin(),vCreepMin.end(),vCRmin.begin(),vCRmin.begin(),std::plus<Float64>());
      std::transform(vCreepMax.begin(),vCreepMax.end(),vCRmax.begin(),vCRmax.begin(),std::plus<Float64>());

      std::transform(vShrinkageMin.begin(),vShrinkageMin.end(),vSHmin.begin(),vSHmin.begin(),std::plus<Float64>());
      std::transform(vShrinkageMax.begin(),vShrinkageMax.end(),vSHmax.begin(),vSHmax.begin(),std::plus<Float64>());

      std::transform(vRelaxationMin.begin(),vRelaxationMin.end(),vREmin.begin(),vREmin.begin(),std::plus<Float64>());
      std::transform(vRelaxationMax.begin(),vRelaxationMax.end(),vREmax.begin(),vREmax.begin(),std::plus<Float64>());

      std::transform(vSecondaryMin.begin(),vSecondaryMin.end(),vPSmin.begin(),vPSmin.begin(),std::plus<Float64>());
      std::transform(vSecondaryMax.begin(),vSecondaryMax.end(),vPSmax.begin(),vPSmax.begin(),std::plus<Float64>());
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


   for ( ; poiIter != poiEndIter; poiIter++, forceIter++, resultIter++, outputIter++ )
   {
      const pgsPointOfInterest& poi = *poiIter;
      const CSegmentKey& segmentKey = poi.GetSegmentKey();
      CSpanKey spanKey;
      Float64 Xspan;
      pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

      EventIndexType start,end,dummy;
      PierIndexType prevPierIdx = spanKey.spanIndex;
      PierIndexType nextPierIdx = prevPierIdx + 1;

      pBridge->GetContinuityEventIndex(prevPierIdx,&dummy,&start);
      pBridge->GetContinuityEventIndex(nextPierIdx,&end,&dummy);

      IntervalIndexType compositeDeckIntervalIdx       = pIntervals->GetCompositeDeckInterval(segmentKey);
      IntervalIndexType startPierContinuityIntervalIdx = pIntervals->GetInterval(segmentKey,start);
      IntervalIndexType endPierContinuityIntervalIdx   = pIntervals->GetInterval(segmentKey,end);

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
         {
            firstAxleLocation = axle_placement.Location;
         }

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
      {
         model = pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrPermit_Routine);
      }
      else if ( ratingType == pgsTypes::lrPermit_Special )
      {
         model = pRatingEntry->GetLiveLoadFactorModel(pRatingSpec->GetSpecialPermitType());
      }
      else
      {
         model = pRatingEntry->GetLiveLoadFactorModel(ratingType);
      }

      gLL = model.GetStrengthLiveLoadFactor(pRatingSpec->GetADTT(),sum_axle_weight);
   }
   else
   {
      Float64 GVW = sum_axle_weight;
      Float64 PermitWeightRatio = IsZero(AL) ? 0 : GVW/AL;
      CLiveLoadFactorModel2 model;
      if ( ratingType == pgsTypes::lrPermit_Routine )
      {
         model = pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrPermit_Routine);
      }
      else if ( ratingType == pgsTypes::lrPermit_Special )
      {
         model = pRatingEntry->GetLiveLoadFactorModel2(pRatingSpec->GetSpecialPermitType());
      }
      else
      {
         model = pRatingEntry->GetLiveLoadFactorModel2(ratingType);
      }

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
      {
         model = pRatingEntry->GetLiveLoadFactorModel(pgsTypes::lrPermit_Routine);
      }
      else if ( ratingType == pgsTypes::lrPermit_Special )
      {
         model = pRatingEntry->GetLiveLoadFactorModel(pRatingSpec->GetSpecialPermitType());
      }
      else
      {
         model = pRatingEntry->GetLiveLoadFactorModel(ratingType);
      }

      gLL = model.GetServiceLiveLoadFactor(pRatingSpec->GetADTT());
   }
   else
   {
      CLiveLoadFactorModel2 model;
      if ( ratingType == pgsTypes::lrPermit_Routine )
      {
         model = pRatingEntry->GetLiveLoadFactorModel2(pgsTypes::lrPermit_Routine);
      }
      else if ( ratingType == pgsTypes::lrPermit_Special )
      {
         model = pRatingEntry->GetLiveLoadFactorModel2(pRatingSpec->GetSpecialPermitType());
      }
      else
      {
         model = pRatingEntry->GetLiveLoadFactorModel2(ratingType);
      }

      gLL = model.GetServiceLiveLoadFactor(pRatingSpec->GetADTT());
   }

   return gLL;
}

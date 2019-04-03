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
#include <IFace\Allowables.h>
#include <EAF\EAFDisplayUnits.h>

#if defined _USE_MULTITHREADING
#include <future>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void special_transform(IBridge* pBridge,IPointOfInterest* pPoi,IIntervals* pIntervals,
                       PoiList::const_iterator poiBeginIter,
                       PoiList::const_iterator poiEndIter,
                       std::vector<Float64>::const_iterator forceBeginIter,
                       std::vector<Float64>::const_iterator resultBeginIter,
                       std::vector<Float64>::iterator outputBeginIter);

inline bool AxleHasWeight(AxlePlacement& placement)
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

pgsRatingArtifact pgsLoadRater::Rate(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const
{
   GET_IFACE(IRatingSpecification,pRatingSpec);

   GET_IFACE(IPointOfInterest,pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS),&vPoi); // gets all POI
   // remove poi at points that don't matter for load rating
   pPoi->RemovePointsOfInterest(vPoi,POI_RELEASED_SEGMENT,POI_SPAN); // retain span points
   pPoi->RemovePointsOfInterest(vPoi,POI_LIFT_SEGMENT,    POI_SPAN);
   pPoi->RemovePointsOfInterest(vPoi,POI_STORAGE_SEGMENT, POI_SPAN);
   pPoi->RemovePointsOfInterest(vPoi,POI_HAUL_SEGMENT,    POI_SPAN);

   // we don't load rate for shear in interior piers so make another collection
   // of POI for shear... Same as for flexure but remove the POIs that are outside of the bearings
   PoiList vShearPoi(vPoi);
   GET_IFACE(IBridge,pBridge);
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? pBridge->GetGirderGroupCount()-1 : firstGroupIdx);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
      CGirderKey thisGirderKey(grpIdx,gdrIdx);
      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(thisGirderKey,segIdx);
         Float64 segmentSpanLength = pBridge->GetSegmentSpanLength(segmentKey);
         Float64 endDist   = pBridge->GetSegmentStartEndDistance(segmentKey);
         std::remove_if(std::begin(vShearPoi), std::end(vShearPoi), PoiIsOutsideOfBearings(segmentKey,endDist,endDist+segmentSpanLength));
      }
   }

   pPoi->SortPoiList(&vShearPoi); // sort and remove duplicates

   pgsRatingArtifact ratingArtifact(ratingType);

   bool bNegativeMoments = pBridge->ProcessNegativeMoments(ALL_SPANS);

   Moments positive_moments, negative_moments;
   GetMoments(girderKey, true, ratingType, vehicleIdx, vPoi, &positive_moments);
   if (bNegativeMoments)
   {
      GetMoments(girderKey, false, ratingType, vehicleIdx, vPoi, &negative_moments);
   }

   // Rate for positive moment - flexure
   MomentRating(girderKey,vPoi,true,ratingType,vehicleIdx,positive_moments,ratingArtifact);

   // Rate for negative moment - flexure, if applicable
   if ( bNegativeMoments )
   {
      MomentRating(girderKey,vPoi,false,ratingType,vehicleIdx,negative_moments,ratingArtifact);
   }

   // Rate for shear if applicable
   if ( pRatingSpec->RateForShear(ratingType) )
   {
      ShearRating(girderKey,vShearPoi,ratingType,vehicleIdx,ratingArtifact);
   }

   // Rate for stress if applicable
   if ( pRatingSpec->RateForStress(ratingType) )
   {
      StressRating(girderKey,vPoi,ratingType,vehicleIdx,ratingArtifact);
   }

   if ( pRatingSpec->CheckYieldStress(ratingType) )
   {
      CheckReinforcementYielding(girderKey,vPoi,true,ratingType,vehicleIdx,positive_moments,ratingArtifact);

      if ( bNegativeMoments)
      {
         CheckReinforcementYielding(girderKey,vPoi,false,ratingType,vehicleIdx,negative_moments,ratingArtifact);
      }
   }

   return ratingArtifact;
}

void pgsLoadRater::MomentRating(const CGirderKey& girderKey,const PoiList& vPoi,bool bPositiveMoment,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx, const Moments& moments,pgsRatingArtifact& ratingArtifact) const
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Load rating for moment"));

   CGirderKey thisGirderKey(girderKey);
   if ( thisGirderKey.groupIndex == ALL_GROUPS )
   {
      thisGirderKey.groupIndex = 0;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType loadRatingIntervalIdx = pIntervals->GetLoadRatingInterval();

   GET_IFACE( ILossParameters, pLossParams);
   bool bTimeStep = (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP ? true : false);

   GET_IFACE(IMomentCapacity,pMomentCapacity);
   std::vector<const MOMENTCAPACITYDETAILS*> vM = pMomentCapacity->GetMomentCapacityDetails(loadRatingIntervalIdx,vPoi,bPositiveMoment);
   std::vector<const MINMOMENTCAPDETAILS*> vMmin = pMomentCapacity->GetMinMomentCapacityDetails(loadRatingIntervalIdx,vPoi,bPositiveMoment);

   ATLASSERT(vPoi.size()     == moments.vDCmax.size());
   ATLASSERT(vPoi.size()     == moments.vDWmax.size());
   ATLASSERT(vPoi.size()     == moments.vLLIMmax.size());
   ATLASSERT(vPoi.size()     == vM.size());
   ATLASSERT(moments.vDCmin.size()   == moments.vDCmax.size());
   ATLASSERT(moments.vDWmin.size()   == moments.vDWmax.size());
   ATLASSERT(moments.vCRmin.size()   == moments.vCRmax.size());
   ATLASSERT(moments.vSHmin.size()   == moments.vSHmax.size());
   ATLASSERT(moments.vREmin.size()   == moments.vREmax.size());
   ATLASSERT(moments.vPSmin.size()   == moments.vPSmax.size());
   ATLASSERT(moments.vLLIMmin.size() == moments.vLLIMmax.size());

   GET_IFACE(IRatingSpecification,pRatingSpec);
   Float64 system_factor    = pRatingSpec->GetSystemFactorFlexure();
   bool bIncludePL = pRatingSpec->IncludePedestrianLiveLoad();

   pgsTypes::LimitState ls = ::GetStrengthLimitStateType(ratingType);

   Float64 gDC = pRatingSpec->GetDeadLoadFactor(ls);
   Float64 gDW = pRatingSpec->GetWearingSurfaceFactor(ls);
   Float64 gCR = pRatingSpec->GetCreepFactor(ls);
   Float64 gSH = pRatingSpec->GetShrinkageFactor(ls);
   Float64 gRE = pRatingSpec->GetRelaxationFactor(ls);
   Float64 gPS = pRatingSpec->GetSecondaryEffectsFactor(ls);

   GET_IFACE(IProductLoads,pProductLoads);
   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);
   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

   GET_IFACE(IBridge, pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   IndexType i = 0;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      Float64 condition_factor = 1.0;
      if (bPositiveMoment)
      {
         condition_factor = pRatingSpec->GetGirderConditionFactor(poi.GetSegmentKey());
      }
      else
      {
         if (IsNonstructuralDeck(deckType))
         {
            // there is no deck, or the deck overlay is not a structural element
            // use the condition of the girder
            condition_factor = pRatingSpec->GetGirderConditionFactor(poi.GetSegmentKey());
         }
         else
         {
            condition_factor = pRatingSpec->GetDeckConditionFactor();
         }
      }

      Float64 DC, DW, CR, SH, RE, PS, LLIM, PL;
      DC   = (bPositiveMoment ? moments.vDCmax[i]   : moments.vDCmin[i]);
      DW   = (bPositiveMoment ? moments.vDWmax[i]   : moments.vDWmin[i]);

      if ( bTimeStep )
      {
         CR   = (bPositiveMoment ? moments.vCRmax[i]   : moments.vCRmin[i]);
         SH   = (bPositiveMoment ? moments.vSHmax[i]   : moments.vSHmin[i]);
         RE   = (bPositiveMoment ? moments.vREmax[i]   : moments.vREmin[i]);
         PS   = (bPositiveMoment ? moments.vPSmax[i]   : moments.vPSmin[i]);
      }
      else
      {
         CR = 0;
         SH = 0;
         RE = 0;
         PS = 0;
      }

      LLIM = (bPositiveMoment ? moments.vLLIMmax[i] : moments.vLLIMmin[i]);
      PL   = (bIncludePL ? (bPositiveMoment ? moments.vPLmax[i]   : moments.vPLmin[i]) : 0.0);

      VehicleIndexType truck_index = vehicleIdx;
      if ( vehicleIdx == INVALID_INDEX )
      {
         truck_index = (bPositiveMoment ? moments.vMaxTruckIndex[i] : moments.vMinTruckIndex[i]);
      }

      std::_tstring strVehicleName = strLLNames[truck_index];

      CString strProgress;
      if ( poi.HasGirderCoordinate() )
      {
         strProgress.Format(_T("Load rating %s for %s moment at %s"),strVehicleName.c_str(),bPositiveMoment ? _T("positive") : _T("negative"),
            ::FormatDimension(poi.GetGirderCoordinate(),pDisplayUnits->GetSpanLengthUnit()));
      }
      else
      {
         strProgress.Format(_T("Load rating %s for %s moment"),strVehicleName.c_str(),bPositiveMoment ? _T("positive") : _T("negative"));
      }
      pProgress->UpdateMessage(strProgress);

      Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,true);
      if ( gLL < 0 )
      {
         if ( ::IsStrengthLimitState(ls) )
         {
            GET_IFACE(IProductForces,pProductForces);

            pgsTypes::BridgeAnalysisType batMin = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);
            pgsTypes::BridgeAnalysisType batMax = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);

            Float64 Mmin, Mmax, Dummy;
            AxleConfiguration MinAxleConfig, MaxAxleConfig, DummyAxleConfig;
            pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,truck_index,poi,batMin,true,true,&Mmin,&Dummy,&MinAxleConfig,&DummyAxleConfig);
            pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,truck_index,poi,batMax,true,true,&Dummy,&Mmax,&DummyAxleConfig,&MaxAxleConfig);

            gLL = pRatingSpec->GetStrengthLiveLoadFactor(ratingType,bPositiveMoment ? MaxAxleConfig : MinAxleConfig);
         }
         else
         {
            gLL = pRatingSpec->GetServiceLiveLoadFactor(ratingType);
         }
      }

      Float64 phi_moment = vM[i]->Phi; 
      Float64 Mn = vM[i]->Mn;

      // NOTE: K can be less than zero when we are rating for negative moment and the minumum moment demand (Mu)
      // is positive. This happens near the simple ends of spans. For example Mr < 0 because we are rating for
      // negative moment and Mmin = min (1.2Mcr and 1.33Mu)... Mcr < 0 because we are looking at negative moment
      // and Mu > 0.... Since we are looking at the negative end of things, Mmin = 1.33Mu. +/- = -... it doesn't
      // make since for K to be negative... K < 0 indicates that the section is most definate NOT under reinforced.
      // No adjustment needs to be made for underreinforcement so take K = 1.0
      Float64 K = (IsZero(vMmin[i]->MrMin) ? 1.0 : vMmin[i]->Mr/vMmin[i]->MrMin); // MBE 6A.5.6
      if ( K < 0.0 || 1.0 < K )
      {
         K = 1.0;
      }

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

      i++;
   }
}

void pgsLoadRater::InitCriticalSectionZones(const CGirderKey& girderKey,pgsTypes::LimitState limitState) const
{
   m_CriticalSections.clear();

   GET_IFACE(IPointOfInterest, pPoi);
   GET_IFACE(IShearCapacity, pShearCapacity);
   GET_IFACE(IBridge, pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? nGroups - 1 : girderKey.groupIndex);

   for (GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++)
   {
      CGirderKey thisGirderKey(girderKey);
      thisGirderKey.groupIndex = grpIdx;
      ASSERT_GIRDER_KEY(thisGirderKey);

      PoiList vCSPoi;
      pPoi->GetCriticalSections(limitState, thisGirderKey,&vCSPoi);
      std::vector<CRITSECTDETAILS> vCS = pShearCapacity->GetCriticalSectionDetails(limitState, thisGirderKey);
      if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEdition2004)
      {
         // if the LRFD is before 2004, critical section for shear was a function of loading.... we end up with many critical section POIs but
         // only a few (usually 2) critical section details. Match the details to the POIs and throw out the other POIs. LRFD 2004 and later only depend on Mu
         // so the number of CS POIs and CS details should always match.
         vCSPoi.erase(
            std::remove_if(vCSPoi.begin(), vCSPoi.end(), [&vCS](const pgsPointOfInterest& poi) 
               {
                  return std::find_if(vCS.begin(), vCS.end(), [&poi](const auto& csDetails) {return csDetails.GetPointOfInterest().AtExactSamePlace(poi);}) == vCS.cend();
               }),
            vCSPoi.end());
      }
      ATLASSERT(vCSPoi.size() == vCS.size());
      auto iter(vCS.begin());
      auto end(vCS.end());
      auto poiIter(vCSPoi.begin());
      for (; iter != end; iter++, poiIter++)
      {
         CRITSECTDETAILS& csDetails(*iter);
         if (csDetails.bAtFaceOfSupport)
         {
            csDetails.poiFaceOfSupport = *poiIter;
         }
         else
         {
            csDetails.pCriticalSection->Poi = *poiIter;
         }
#if defined _DEBUG
         const pgsPointOfInterest& csPoi = csDetails.GetPointOfInterest();
         ATLASSERT(csPoi.GetID() != INVALID_ID);
         ATLASSERT(csPoi.GetSegmentKey() == poiIter->get().GetSegmentKey());
         ATLASSERT(IsEqual(csPoi.GetDistFromStart(), poiIter->get().GetDistFromStart()));
#endif
         m_CriticalSections.push_back(csDetails);
      }
   }
}

ZoneIndexType pgsLoadRater::GetCriticalSectionZone(const pgsPointOfInterest& poi, bool bIncludeCS) const
{
   Float64 Xpoi = poi.GetDistFromStart();

   auto& iter(m_CriticalSections.cbegin());
   const auto& end(m_CriticalSections.cend());
   for (; iter != end; iter++)
   {
      const CRITSECTDETAILS& csDetails(*iter);
      const pgsPointOfInterest& csPoi = csDetails.GetPointOfInterest();
      const CSegmentKey& csSegmentKey = csPoi.GetSegmentKey();

      if (csSegmentKey == poi.GetSegmentKey() && ::InRange(csDetails.Start, Xpoi, csDetails.End, pgsPointOfInterest::GetTolerance()))
      {
         // poi is in the critical section zone
         if (!bIncludeCS && csPoi.AtSamePlace(poi))
         {
            // we want to exclude the actual critical section and the poi is at the same place as the critical section
            // return now with INVALID_INDEX since there is no reason to keep going through the loop
            return INVALID_INDEX;
         }

         // we found the critical section zone that contains our poi
         return (ZoneIndexType)(std::distance(m_CriticalSections.cbegin(),iter));
      }
   }

   return INVALID_INDEX;
}

void pgsLoadRater::ShearRating(const CGirderKey& girderKey,const PoiList& vPoi,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,pgsRatingArtifact& ratingArtifact) const
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Load rating for shear"));

   CGirderKey thisGirderKey(girderKey);
   if ( thisGirderKey.groupIndex == ALL_GROUPS )
   {
      thisGirderKey.groupIndex = 0;
   }

   pgsTypes::LimitState limitState = ::GetStrengthLimitStateType(ratingType);
   InitCriticalSectionZones(girderKey, limitState);

   GET_IFACE(IPointOfInterest, pPoi);
   PoiList vMyPoi(vPoi);

   GET_IFACE(IBridge, pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();

   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? nGroups - 1 : girderKey.groupIndex);
   PoiList vCSPoi;
   for (GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++)
   {
      CGirderKey thisGirderKey(girderKey);
      thisGirderKey.groupIndex = grpIdx;
      ASSERT_GIRDER_KEY(thisGirderKey);
      PoiList csPoi;
      pPoi->GetCriticalSections(limitState, thisGirderKey, &csPoi);
      vCSPoi.insert(std::end(vCSPoi), std::cbegin(csPoi), std::cend(csPoi));
   }
   pPoi->MergePoiLists(vMyPoi, vCSPoi, &vMyPoi);

   // remove all POIs that are in a critical section zone
   vMyPoi.erase(std::remove_if(vMyPoi.begin(), vMyPoi.end(), [&](const pgsPointOfInterest& poi) {return GetCriticalSectionZone(poi) != INVALID_INDEX;}), vMyPoi.end());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType loadRatingIntervalIdx = pIntervals->GetLoadRatingInterval();

   GET_IFACE( ILossParameters, pLossParams);
   bool bTimeStep = (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP ? true : false);

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

   GET_IFACE(IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType batMin = pProdForces->GetBridgeAnalysisType(pgsTypes::Minimize);
   pgsTypes::BridgeAnalysisType batMax = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   GET_IFACE(ICombinedForces2,pCombinedForces);
   GET_IFACE(IProductForces2,pProductForces);
   vDCmin = pCombinedForces->GetShear(loadRatingIntervalIdx,lcDC,vMyPoi,batMin,rtCumulative);
   vDCmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcDC,vMyPoi,batMax,rtCumulative);

   vDWmin = pCombinedForces->GetShear(loadRatingIntervalIdx,lcDWRating,vMyPoi,batMin,rtCumulative);
   vDWmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcDWRating,vMyPoi,batMax,rtCumulative);

   if ( bTimeStep )
   {
      vCRmin = pCombinedForces->GetShear(loadRatingIntervalIdx,lcCR,vMyPoi,batMin,rtCumulative);
      vCRmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcCR,vMyPoi,batMax,rtCumulative);

      vSHmin = pCombinedForces->GetShear(loadRatingIntervalIdx,lcSH,vMyPoi,batMin,rtCumulative);
      vSHmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcSH,vMyPoi,batMax,rtCumulative);

      vREmin = pCombinedForces->GetShear(loadRatingIntervalIdx,lcRE,vMyPoi,batMin,rtCumulative);
      vREmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcRE,vMyPoi,batMax,rtCumulative);

      vPSmin = pCombinedForces->GetShear(loadRatingIntervalIdx,lcPS,vMyPoi,batMin,rtCumulative);
      vPSmax = pCombinedForces->GetShear(loadRatingIntervalIdx,lcPS,vMyPoi,batMax,rtCumulative);
   }

   if ( vehicleIdx == INVALID_INDEX )
   {
      pProductForces->GetLiveLoadShear( loadRatingIntervalIdx, llType, vMyPoi, batMin, true, true, &vLLIMmin, &vUnused, &vMinTruckIndex, &vUnusedIndex );
      pProductForces->GetLiveLoadShear( loadRatingIntervalIdx, llType, vMyPoi, batMax, true, true, &vUnused, &vLLIMmax, &vUnusedIndex, &vMaxTruckIndex );
   }
   else
   {
      pProductForces->GetVehicularLiveLoadShear( loadRatingIntervalIdx, llType, vehicleIdx, vMyPoi, batMin, true, true, &vLLIMmin, &vUnused, nullptr,nullptr,nullptr,nullptr);
      pProductForces->GetVehicularLiveLoadShear( loadRatingIntervalIdx, llType, vehicleIdx, vMyPoi, batMax, true, true, &vUnused, &vLLIMmax, nullptr,nullptr,nullptr,nullptr);
   }

   pCombinedForces->GetCombinedLiveLoadShear( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vMyPoi, batMax, false, &vUnused, &vPLmax );
   pCombinedForces->GetCombinedLiveLoadShear( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vMyPoi, batMin, false, &vPLmin, &vUnused );

   GET_IFACE(IShearCapacity,pShearCapacity);
   std::vector<SHEARCAPACITYDETAILS> vVn = pShearCapacity->GetShearCapacityDetails(limitState,loadRatingIntervalIdx,vMyPoi);

   ATLASSERT(vMyPoi.size()     == vDCmax.size());
   ATLASSERT(vMyPoi.size()     == vDWmax.size());
   ATLASSERT(vMyPoi.size()     == vLLIMmax.size());
   ATLASSERT(vMyPoi.size()     == vVn.size());
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

   Float64 gDC = pRatingSpec->GetDeadLoadFactor(limitState);
   Float64 gDW = pRatingSpec->GetWearingSurfaceFactor(limitState);
   Float64 gCR = pRatingSpec->GetCreepFactor(limitState);
   Float64 gSH = pRatingSpec->GetShrinkageFactor(limitState);
   Float64 gRE = pRatingSpec->GetRelaxationFactor(limitState);
   Float64 gPS = pRatingSpec->GetSecondaryEffectsFactor(limitState);

   GET_IFACE(IProductLoads,pProductLoads);
   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

   CollectionIndexType nPOI = vMyPoi.size();
   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      const pgsPointOfInterest& poi = vMyPoi[i];

      Float64 condition_factor = pRatingSpec->GetGirderConditionFactor(poi.GetSegmentKey());

      Float64 DCmin   = Min(vDCmin[i].Left(),  vDCmin[i].Right());
      Float64 DCmax   = Max(vDCmax[i].Left(),  vDCmax[i].Right());
      Float64 DWmin   = Min(vDWmin[i].Left(),  vDWmin[i].Right());
      Float64 DWmax   = Max(vDWmax[i].Left(),  vDWmax[i].Right());

      Float64 CRmin(0), CRmax(0), SHmin(0), SHmax(0), REmin(0), REmax(0), PSmin(0), PSmax(0);
      if ( bTimeStep )
      {
         CRmin = Min(vCRmin[i].Left(),  vCRmin[i].Right());
         CRmax = Max(vCRmax[i].Left(),  vCRmax[i].Right());
         SHmin = Min(vSHmin[i].Left(),  vSHmin[i].Right());
         SHmax = Max(vSHmax[i].Left(),  vSHmax[i].Right());
         REmin = Min(vREmin[i].Left(),  vREmin[i].Right());
         REmax = Max(vREmax[i].Left(),  vREmax[i].Right());
         PSmin = Min(vPSmin[i].Left(),  vPSmin[i].Right());
         PSmax = Max(vPSmax[i].Left(),  vPSmax[i].Right());
      }
      else
      {
         CRmin = 0;
         CRmax = 0;
         SHmin = 0;
         SHmax = 0;
         REmin = 0;
         REmax = 0;
         PSmin = 0;
         PSmax = 0;
      }

      Float64 LLIMmin = Min(vLLIMmin[i].Left(),vLLIMmin[i].Right());
      Float64 LLIMmax = Max(vLLIMmax[i].Left(),vLLIMmax[i].Right());
      Float64 PLmin   = Min(vPLmin[i].Left(),  vPLmin[i].Right());
      Float64 PLmax   = Max(vPLmax[i].Left(),  vPLmax[i].Right());

      Float64 DC   = MaxMagnitude(DCmin,DCmax);
      Float64 DW   = MaxMagnitude(DWmin,DWmax);
      Float64 CR   = MaxMagnitude(CRmin,CRmax);
      Float64 SH   = MaxMagnitude(SHmin,SHmax);
      Float64 RE   = MaxMagnitude(REmin,REmax);
      Float64 PS   = MaxMagnitude(PSmin,PSmax);
      Float64 LLIM = MaxMagnitude(LLIMmin,LLIMmax);
      Float64 PL   = (bIncludePL ? MaxMagnitude(PLmin,PLmax) : 0);
      VehicleIndexType truck_index = vehicleIdx;
      if ( vehicleIdx == INVALID_INDEX )
      {
         truck_index = (fabs(LLIMmin) < fabs(LLIMmax) ? vMaxTruckIndex[i] : vMinTruckIndex[i]);
      }

      std::_tstring strVehicleName = strLLNames[truck_index];

      CString strProgress;
      if ( poi.HasGirderCoordinate() )
      {
         strProgress.Format(_T("Load rating %s for shear at %s"),strVehicleName.c_str(),
            ::FormatDimension(poi.GetGirderCoordinate(),pDisplayUnits->GetSpanLengthUnit()));
      }
      else
      {
         strProgress.Format(_T("Load rating %s for shear"),strVehicleName.c_str());
      }
      pProgress->UpdateMessage(strProgress);

      Float64 gLL = pRatingSpec->GetLiveLoadFactor(limitState,true);
      if ( gLL < 0 )
      {
         // need to compute gLL based on axle weights
         if ( ::IsStrengthLimitState(limitState) )
         {
            GET_IFACE(IProductForces,pProductForce);
            pgsTypes::BridgeAnalysisType batMin = pProductForce->GetBridgeAnalysisType(pgsTypes::Minimize);
            pgsTypes::BridgeAnalysisType batMax = pProductForce->GetBridgeAnalysisType(pgsTypes::Maximize);

            sysSectionValue Vmin, Vmax, Dummy;
            AxleConfiguration MinLeftAxleConfig, MaxLeftAxleConfig, MinRightAxleConfig, MaxRightAxleConfig, DummyLeftAxleConfig, DummyRightAxleConfig;
            pProductForce->GetVehicularLiveLoadShear(loadRatingIntervalIdx,llType,truck_index,poi,batMin,true,true,&Vmin,&Dummy,&MinLeftAxleConfig,&MinRightAxleConfig,&DummyLeftAxleConfig,&DummyRightAxleConfig);
            pProductForce->GetVehicularLiveLoadShear(loadRatingIntervalIdx,llType,truck_index,poi,batMax,true,true,&Dummy,&Vmax,&DummyLeftAxleConfig,&DummyRightAxleConfig,&MaxLeftAxleConfig,&MaxRightAxleConfig);

            if ( fabs(LLIMmin) < fabs(LLIMmax) )
            {
               if (IsEqual(fabs(vLLIMmax[i].Left()),fabs(LLIMmax)))
               {
                  gLL = pRatingSpec->GetStrengthLiveLoadFactor(ratingType,MaxLeftAxleConfig);
               }
               else
               {
                  gLL = pRatingSpec->GetStrengthLiveLoadFactor(ratingType,MaxRightAxleConfig);
               }
            }
            else
            {
               if (IsEqual(fabs(vLLIMmin[i].Left()),fabs(LLIMmin)))
               {
                  gLL = pRatingSpec->GetStrengthLiveLoadFactor(ratingType,MinLeftAxleConfig);
               }
               else
               {
                  gLL = pRatingSpec->GetStrengthLiveLoadFactor(ratingType,MinRightAxleConfig);
               }
            }
         }
         else
         {
            gLL = pRatingSpec->GetServiceLiveLoadFactor(ratingType);
         }
      }

      Float64 phi_shear = vVn[i].Phi; 
      Float64 Vn = vVn[i].Vn;

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
      pShearCapacity->GetShearCapacityDetails(limitState,loadRatingIntervalIdx,poi,nullptr,&scd);
      designer.InitShearCheck(poi.GetSegmentKey(),loadRatingIntervalIdx, limitState,nullptr);
      designer.CheckLongReinfShear(poi,loadRatingIntervalIdx, limitState,scd,nullptr,&l_artifact);
      shearArtifact.SetLongReinfShearArtifact(l_artifact);

      ratingArtifact.AddArtifact(poi,shearArtifact);
   }
}

void pgsLoadRater::StressRating(const CGirderKey& girderKey,const PoiList& vPoi,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,pgsRatingArtifact& ratingArtifact) const
{
   ATLASSERT(ratingType == pgsTypes::lrDesign_Inventory || 
             ratingType == pgsTypes::lrLegal_Routine    ||
             ratingType == pgsTypes::lrLegal_Special    || // see MBE C6A.5.4.1
             ratingType == pgsTypes::lrLegal_Emergency ||
             ratingType == pgsTypes::lrPermit_Routine   || // WSDOT BDM
             ratingType == pgsTypes::lrPermit_Special );   // WSDOT BDM

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Load rating for flexural stresses"));

   CGirderKey thisGirderKey(girderKey);
   if ( thisGirderKey.groupIndex == ALL_GROUPS )
   {
      thisGirderKey.groupIndex = 0;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType loadRatingIntervalIdx = pIntervals->GetLoadRatingInterval();

   GET_IFACE( ILossParameters, pLossParams);
   bool bTimeStep = (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP ? true : false);

   GET_IFACE(IPrecompressedTensileZone,pPTZ);

   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);

   GET_IFACE(ICombinedForces,pCombinedForces);
   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize); // only doing stress rating for tension so we want to maximize results

   GET_IFACE(IPretensionStresses,pPrestress);
   GET_IFACE(IRatingSpecification,pRatingSpec);
   GET_IFACE(IAllowableConcreteStress,pAllowables);

   Float64 system_factor = pRatingSpec->GetSystemFactorFlexure();
   bool bIncludePL = pRatingSpec->IncludePedestrianLiveLoad();

   pgsTypes::LimitState limitState = ::GetServiceLimitStateType(ratingType);
   ATLASSERT(IsServiceIIILimitState(limitState)); // must be one of the Service III limit states

   Float64 gDC = pRatingSpec->GetDeadLoadFactor(limitState);
   Float64 gDW = pRatingSpec->GetWearingSurfaceFactor(limitState);
   Float64 gCR = pRatingSpec->GetCreepFactor(limitState);
   Float64 gSH = pRatingSpec->GetShrinkageFactor(limitState);
   Float64 gRE = pRatingSpec->GetRelaxationFactor(limitState);
   Float64 gPS = pRatingSpec->GetSecondaryEffectsFactor(limitState);

   GET_IFACE(IProductLoads,pProductLoads);
   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

   for(const pgsPointOfInterest& poi : vPoi)
   {
      std::vector<pgsTypes::StressLocation> vStressLocations;
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::StressLocation topStressLocation = (i == 0 ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
         pgsTypes::StressLocation botStressLocation = (i == 0 ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

         bool bTopPTZ, bBotPTZ;
         pPTZ->IsInPrecompressedTensileZone(poi,limitState,topStressLocation,botStressLocation,&bTopPTZ,&bBotPTZ);
         if ( bTopPTZ )
         {
            vStressLocations.push_back(topStressLocation);
         }

         if ( bBotPTZ )
         {
            vStressLocations.push_back(botStressLocation);
         }
      }

      if ( vStressLocations.size() == 0 )
      {
         // no sections are in the precompressed tensile zone... how is this possible?
         pgsStressRatingArtifact stressArtifact;
         ratingArtifact.AddArtifact(poi,stressArtifact);
         continue; // next POI
      }

      for (const auto& stressLocation : vStressLocations)
      {
         Float64 fDummy, fDC, fDW, fCR, fSH, fRE, fPS, fLLIM, fPL;
         pCombinedForces->GetStress(loadRatingIntervalIdx,lcDC,      poi,bat,rtCumulative,stressLocation,stressLocation,&fDummy,&fDC);
         pCombinedForces->GetStress(loadRatingIntervalIdx,lcDWRating,poi,bat,rtCumulative,stressLocation,stressLocation,&fDummy,&fDW);

         if ( bTimeStep )
         {
            pCombinedForces->GetStress(loadRatingIntervalIdx,lcCR,poi,bat,rtCumulative,stressLocation,stressLocation,&fDummy,&fCR);
            pCombinedForces->GetStress(loadRatingIntervalIdx,lcSH,poi,bat,rtCumulative,stressLocation,stressLocation,&fDummy,&fSH);
            pCombinedForces->GetStress(loadRatingIntervalIdx,lcRE,poi,bat,rtCumulative,stressLocation,stressLocation,&fDummy,&fRE);
            pCombinedForces->GetStress(loadRatingIntervalIdx,lcPS,poi,bat,rtCumulative,stressLocation,stressLocation,&fDummy,&fPS);
         }
         else
         {
            fCR = 0;
            fSH = 0;
            fRE = 0;
            fPS = 0;
         }


         Float64 fDummy1, fDummy2, fDummy3;
         VehicleIndexType truckIndex, dummyIndex1, dummyIndex2, dummyIndex3;

         if ( vehicleIdx == INVALID_INDEX )
         {
            pProductForces->GetLiveLoadStress(loadRatingIntervalIdx,llType,poi,bat,true,true,stressLocation,stressLocation,&fDummy1,&fDummy2,&fDummy3,&fLLIM,&dummyIndex1, &dummyIndex2, &dummyIndex3, &truckIndex);
         }
         else
         {
            pProductForces->GetVehicularLiveLoadStress(loadRatingIntervalIdx,llType,vehicleIdx,poi,bat,true,true,stressLocation,stressLocation,&fDummy1,&fDummy2,&fDummy3,&fLLIM,nullptr,nullptr,nullptr,nullptr);
         }

         if ( bIncludePL )
         {
            pCombinedForces->GetCombinedLiveLoadStress( loadRatingIntervalIdx, pgsTypes::lltPedestrian, poi, bat, stressLocation, stressLocation, &fDummy,  &fDummy2, &fDummy3, &fPL);
         }
         else
         {
            fPL = 0;
         }

         Float64 fps = pPrestress->GetStress(loadRatingIntervalIdx,poi,stressLocation,true/*include live load if applicable*/,limitState,vehicleIdx);

         Float64 fpt = 0;
         if ( bTimeStep )
         {
            pProductForces->GetStress(loadRatingIntervalIdx,pgsTypes::pftPostTensioning,poi,bat,rtCumulative,stressLocation,stressLocation,&fpt,&fDummy);
         }

         // do this in the loop because the vector of POI can be for multiple segments
         Float64 condition_factor = pRatingSpec->GetGirderConditionFactor(poi.GetSegmentKey());
         Float64 fr               = pAllowables->GetAllowableTensionStress(ratingType,poi,stressLocation);

         VehicleIndexType truck_index = vehicleIdx;
         if ( vehicleIdx == INVALID_INDEX )
         {
            truck_index = truckIndex;
         }

         std::_tstring strVehicleName = strLLNames[truck_index];

         CString strProgress;
         if ( poi.HasGirderCoordinate() )
         {
            strProgress.Format(_T("Load rating %s for flexural stress at %s"),strVehicleName.c_str(),
               ::FormatDimension(poi.GetGirderCoordinate(),pDisplayUnits->GetSpanLengthUnit()));
         }
         else
         {
            strProgress.Format(_T("Load rating %s for flexural stress"),strVehicleName.c_str());
         }
         pProgress->UpdateMessage(strProgress);

         Float64 gLL = pRatingSpec->GetLiveLoadFactor(limitState,true);
         if ( gLL < 0 )
         {
            gLL = pRatingSpec->GetServiceLiveLoadFactor(ratingType);
         }

         Float64 W = pProductLoads->GetVehicleWeight(llType,truck_index);

         pgsStressRatingArtifact stressArtifact;
         stressArtifact.SetStressLocation(stressLocation);
         stressArtifact.SetRatingType(ratingType);
         stressArtifact.SetPointOfInterest(poi);
         stressArtifact.SetVehicleIndex(truck_index);
         stressArtifact.SetVehicleWeight(W);
         stressArtifact.SetVehicleName(strVehicleName.c_str());
         stressArtifact.SetAllowableStress(fr);
         stressArtifact.SetPrestressStress(fps);
         stressArtifact.SetPostTensionStress(fpt);
         stressArtifact.SetDeadLoadFactor(gDC);
         stressArtifact.SetDeadLoadStress(fDC);
         stressArtifact.SetWearingSurfaceFactor(gDW);
         stressArtifact.SetWearingSurfaceStress(fDW);
         stressArtifact.SetCreepFactor(gCR);
         stressArtifact.SetCreepStress(fCR);
         stressArtifact.SetShrinkageFactor(gSH);
         stressArtifact.SetShrinkageStress(fSH);
         stressArtifact.SetRelaxationFactor(gRE);
         stressArtifact.SetRelaxationStress(fRE);
         stressArtifact.SetSecondaryEffectsFactor(gPS);
         stressArtifact.SetSecondaryEffectsStress(fPS);
         stressArtifact.SetLiveLoadFactor(gLL);
         stressArtifact.SetLiveLoadStress(fLLIM+fPL);

         ratingArtifact.AddArtifact(poi,stressArtifact);
      }
   } // next poi
}

void pgsLoadRater::CheckReinforcementYielding(const CGirderKey& girderKey,const PoiList& vPoi,bool bPositiveMoment,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,const Moments& moments,pgsRatingArtifact& ratingArtifact) const
{
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Checking for reinforcement yielding"));

   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);

   if ( bPositiveMoment )
   {
      GET_IFACE(IProductLoads,pProductLoads);
      VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
      VehicleIndexType startVehicleIdx = (vehicleIdx == INVALID_INDEX ? 0 : vehicleIdx);
      VehicleIndexType endVehicleIdx   = (vehicleIdx == INVALID_INDEX ? nVehicles-1: startVehicleIdx);
      for ( VehicleIndexType vehicleIdx = startVehicleIdx; vehicleIdx <= endVehicleIdx; vehicleIdx++ )
      {
         pgsTypes::LiveLoadApplicabilityType applicability = pProductLoads->GetLiveLoadApplicability(llType,vehicleIdx);
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
   IntervalIndexType loadRatingIntervalIdx = pIntervals->GetLoadRatingInterval();

   GET_IFACE( ILossParameters, pLossParams);
   bool bTimeStep = (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP ? true : false);

   ATLASSERT(ratingType == pgsTypes::lrPermit_Routine || ratingType == pgsTypes::lrPermit_Special);

   pgsTypes::LimitState ls = (ratingType == pgsTypes::lrPermit_Routine ? pgsTypes::ServiceI_PermitRoutine : pgsTypes::ServiceI_PermitSpecial);

   GET_IFACE(IRatingSpecification,pRatingSpec);
   bool bIncludePL = pRatingSpec->IncludePedestrianLiveLoad();

   GET_IFACE(IMomentCapacity,pMomentCapacity);
   std::vector<const CRACKINGMOMENTDETAILS*> vMcr = pMomentCapacity->GetCrackingMomentDetails(loadRatingIntervalIdx,vPoi,bPositiveMoment);

   GET_IFACE(ICrackedSection,pCrackedSection);
   std::vector<const CRACKEDSECTIONDETAILS*> vCrackedSection = pCrackedSection->GetCrackedSectionDetails(vPoi,bPositiveMoment);
   
   ATLASSERT(vPoi.size()     == moments.vDCmax.size());
   ATLASSERT(vPoi.size()     == moments.vDWmax.size());
   ATLASSERT(bTimeStep ? vPoi.size() == moments.vCRmax.size() : true);
   ATLASSERT(bTimeStep ? vPoi.size() == moments.vSHmax.size() : true);
   ATLASSERT(bTimeStep ? vPoi.size() == moments.vREmax.size() : true);
   ATLASSERT(bTimeStep ? vPoi.size() == moments.vPSmax.size() : true);
   ATLASSERT(vPoi.size()     == moments.vLLIMmax.size());
   ATLASSERT(vPoi.size()     == vMcr.size());
   ATLASSERT(vPoi.size()     == vCrackedSection.size());
   ATLASSERT(moments.vDCmin.size()   == moments.vDCmax.size());
   ATLASSERT(moments.vDWmin.size()   == moments.vDWmax.size());
   ATLASSERT(moments.vCRmin.size()   == moments.vCRmax.size());
   ATLASSERT(moments.vSHmin.size()   == moments.vSHmax.size());
   ATLASSERT(moments.vREmin.size()   == moments.vREmax.size());
   ATLASSERT(moments.vPSmin.size()   == moments.vPSmax.size());
   ATLASSERT(moments.vLLIMmin.size() == moments.vLLIMmax.size());
   ATLASSERT(moments.vPLmin.size()   == moments.vPLmax.size());


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

   pgsTypes::StressLocation topLocation = pgsTypes::TopDeck;
   pgsTypes::StressLocation botLocation = pgsTypes::BottomGirder;

   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IPointOfInterest,pPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ILongRebarGeometry,pRebarGeom);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(ITendonGeometry,pTendonGeom);

   // Create artifacts
   CollectionIndexType nPOI = vPoi.size();
   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      const pgsPointOfInterest& poi = vPoi[i];
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      // since girderKey covers all groups, we have to get nDucts based on
      // the girder associated with the current poi (remember that segmentKey can act as a girderKey)
      DuctIndexType nDucts = pTendonGeom->GetDuctCount(segmentKey);

      CClosureKey closureKey;
      bool bIsInClosureJoint = pPoi->IsInClosureJoint(poi,&closureKey);
      bool bIsOnSegment = pPoi->IsOnSegment(poi);
      bool bIsOnGirder = pPoi->IsOnGirder(poi);

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      Float64 Hg = pSectProp->GetHg(releaseIntervalIdx,poi);
      Float64 ts = pBridge->GetStructuralSlabDepth(poi);
      if ( !bIsOnGirder || bIsInClosureJoint )
      {
         Hg = pSectProp->GetHg(loadRatingIntervalIdx,poi);
         Hg -= ts;
      }

      // Get material properties
      GET_IFACE(IMaterials,pMaterials);
      Float64 Eg = pMaterials->GetSegmentEc(segmentKey,loadRatingIntervalIdx);

      Float64 Eb, Eps, Ept; // mod E of rebar, strand, tendon
      Float64 fyb, fyps, fypt; // yield strength of bar, strand, tendon
      Float64 fu;

      if ( bPositiveMoment )
      {
         // extreme tension rebar is going to be in the girder
         if ( bIsInClosureJoint )
         {
            pMaterials->GetClosureJointLongitudinalRebarProperties(closureKey,&Eb,&fyb,&fu);
         }
         else
         {
            pMaterials->GetSegmentLongitudinalRebarProperties(segmentKey,&Eb,&fyb,&fu);
         }
      }
      else
      {
         // extreme tension rebar is going to be in the deck
         pMaterials->GetDeckRebarProperties(&Eb,&fyb,&fu);
      }

      Eps  = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Permanent)->GetE();
      fyps = pMaterials->GetStrandMaterial(segmentKey,pgsTypes::Permanent)->GetYieldStrength();

      // NOTE: it is important to use segmentKey here
      Ept  = pMaterials->GetTendonMaterial(segmentKey)->GetE();
      fypt = pMaterials->GetTendonMaterial(segmentKey)->GetYieldStrength();


      // Get distance to reinforcement from extreme compression face

      // keep track of whether or not the type of reinforcement is used
      bool bRebar   = false;
      bool bStrands = false;
      bool bTendons = false;

      // rebar
      Float64 db;
      if ( bPositiveMoment )
      {
         // extreme tension rebar is going to be in the girder, furthest from the top of the girder
         CComPtr<IRebarSection> rebarSection;
         pRebarGeom->GetRebars(poi,&rebarSection);

         IndexType count;
         rebarSection->get_Count(&count);
         if ( 0 < count )
         {
            bRebar = true;
         }

         CComPtr<IEnumRebarSectionItem> enumRebarSectionItem;
         rebarSection->get__EnumRebarSectionItem(&enumRebarSectionItem);

         Float64 Y = DBL_MAX;
         CComPtr<IRebarSectionItem> rebarSectionItem;
         while ( enumRebarSectionItem->Next(1,&rebarSectionItem,nullptr) != S_FALSE )
         {
            CComPtr<IPoint2d> location;
            rebarSectionItem->get_Location(&location);
            Float64 y;
            location->get_Y(&y); // this is in girder section coordinates so (0,0) is at top center... y should be < 0
            ATLASSERT(y < 0);
            Y = Min(Y,y);
            rebarSectionItem.Release();
         }

         db = ts - Y;
      }
      else
      {
         // extreme tension rebar is going to be in the deck
         Float64 As;
         Float64 Y; // distance from top of girder to rebar
         pRebarGeom->GetDeckReinforcing(poi,pgsTypes::drmTop,pgsTypes::drbAll,pgsTypes::drcAll,false,&As,&Y);
         if ( IsZero(As) )
         {
            // no top mat, try bottom mat
            pRebarGeom->GetDeckReinforcing(poi,pgsTypes::drmBottom,pgsTypes::drbAll,pgsTypes::drcAll,false,&As,&Y);
         }

         db = Hg + Y;

         if ( !IsZero(As) )
         {
            bRebar = true;
         }
      }

      // strand
      Float64 dps;
      Float64 Y = (bPositiveMoment ? DBL_MAX : -DBL_MAX);
      if ( bIsOnSegment )
      {
         for ( int j = 0; j < 2; j++ )
         {
            pgsTypes::StrandType strandType = (pgsTypes::StrandType)j;
            CComPtr<IPoint2dCollection> strandPoints;
            pStrandGeom->GetStrandPositions(poi, strandType, &strandPoints);
            IndexType nStrands;
            strandPoints->get_Count(&nStrands);
            if ( 0 < nStrands )
            {
               bStrands = true;
            }
            for ( IndexType strandIdx = 0; strandIdx < nStrands; strandIdx++ )
            {
               CComPtr<IPoint2d> pnt;
               strandPoints->get_Item(strandIdx,&pnt);
               Float64 y;
               pnt->get_Y(&y);
               ATLASSERT( y < 0 );
               if ( bPositiveMoment )
               {
                  Y = Min(Y,y); // want furthest from top
               }
               else
               {
                  Y = Max(Y,y); // want closest to top
               }
            }
         } // next strand type
      }
      if ( bPositiveMoment )
      {
         dps = ts - Y;
      }
      else
      {
         dps = Hg + Y;
      }

      // tendon
      Float64 dpt;
      DuctIndexType ductIdx = INVALID_INDEX; // index of the duct that is furthest from the compression face
      Y = (bPositiveMoment ? DBL_MAX : -DBL_MAX);
      if ( bIsOnGirder )
      {
         if ( 0 < nDucts )
         {
            bTendons = true;
         }
         for ( DuctIndexType theDuctIdx = 0; theDuctIdx < nDucts; theDuctIdx++ )
         {
            CComPtr<IPoint2d> pnt;
            pTendonGeom->GetDuctPoint(poi,theDuctIdx,&pnt);
            Float64 y;
            pnt->get_Y(&y);
            ATLASSERT(y < 0);
            if ( bPositiveMoment )
            {
               if ( MinIndex(Y,y) == 1 )
               {
                  ductIdx = theDuctIdx;
               }

               Y = Min(Y,y); // want furthest from top
            }
            else
            {
               if ( MaxIndex(Y,y) == 1 )
               {
                  ductIdx = theDuctIdx;
               }

               Y = Max(Y,y); // want closest to top
            }
         }
      }
      if ( bPositiveMoment )
      {
         dpt = ts - Y;
      }
      else
      {
         dpt = Hg + Y;
      }
      
      // Get allowable
      Float64 K = pRatingSpec->GetYieldStressLimitCoefficient();

      Float64 gM;
      Float64 vLLIMmax = moments.vLLIMmax[i];
      Float64 vLLIMmin = moments.vLLIMmin[i];
      if ( ratingType == pgsTypes::lrPermit_Special ) // if it is any of the special permit types
      {
         // The live load distribution factor used for special permits is one loaded lane without multiple presense factor.
         // This is the Fatigue LLDF. See MBE 6A.4.5.4.2b and C6A.5.4.2.2b.
         // However, when evaluating the reinforcement yielding in the Service I limit state (the thing this function does)
         // the controlling of one loaded lane and two or more loaded lanes live load distribution factors is to be used.
         // See 6A.5.4.2.2b
         //
         // vLLIMmin and vLLIMmax includes the one lane LLDF (Fatigue)... divide out this LLDF and multiply by the correct LLDF (Service I)

         Float64 gpM_Service, gnM_Service, gV;
         Float64 gpM_Fatigue, gnM_Fatigue;


         GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
         pLLDF->GetDistributionFactors(poi,pgsTypes::FatigueI,              &gpM_Fatigue,&gnM_Fatigue,&gV);
         pLLDF->GetDistributionFactors(poi,pgsTypes::ServiceI_PermitSpecial,&gpM_Service,&gnM_Service,&gV);

         if ( bPositiveMoment )
         {
            gM = gpM_Service;
            vLLIMmax *= gpM_Service / gpM_Fatigue;
         }
         else
         {
            gM = gnM_Service;
            vLLIMmin *= gnM_Service / gnM_Fatigue;
         }
      }
      else
      {
         ATLASSERT(ratingType == pgsTypes::lrPermit_Routine);
         Float64 gpM, gnM, gV;
         GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
         pLLDF->GetDistributionFactors(poi,pgsTypes::ServiceI_PermitRoutine,&gpM,&gnM,&gV);
         gM = (bPositiveMoment ? gpM : gnM);
      }

      Float64 DC   = (bPositiveMoment ? moments.vDCmax[i] : moments.vDCmin[i]);
      Float64 DW   = (bPositiveMoment ? moments.vDWmax[i] : moments.vDWmin[i]);

      Float64 CR, SH, RE, PS;
      if ( bTimeStep )
      {
         CR = (bPositiveMoment ? moments.vCRmax[i] : moments.vCRmin[i]);
         SH = (bPositiveMoment ? moments.vSHmax[i] : moments.vSHmin[i]);
         RE = (bPositiveMoment ? moments.vREmax[i] : moments.vREmin[i]);
         PS = (bPositiveMoment ? moments.vPSmax[i] : moments.vPSmin[i]);
      }
      else
      {
         CR = 0;
         SH = 0;
         RE = 0;
         PS = 0;
      }

      Float64 LLIM = (bPositiveMoment ? vLLIMmax : vLLIMmin);
      Float64 PL   = (bIncludePL ? (bPositiveMoment ? moments.vPLmax[i]   : moments.vPLmin[i]) : 0.0);
      VehicleIndexType truck_index = vehicleIdx;
      if ( vehicleIdx == INVALID_INDEX )
      {
         truck_index = (bPositiveMoment ? moments.vMaxTruckIndex[i] : moments.vMinTruckIndex[i]);
      }

      std::_tstring strVehicleName = strLLNames[truck_index];

      CString strProgress;
      if ( poi.HasGirderCoordinate() )
      {
         GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
         strProgress.Format(_T("Checking for reinforcement yielding %s for %s moment at %s"),strVehicleName.c_str(),bPositiveMoment ? _T("positive") : _T("negative"),
            ::FormatDimension(poi.GetGirderCoordinate(),pDisplayUnits->GetSpanLengthUnit()));
      }
      else
      {
         strProgress.Format(_T("Checking for reinforcement yielding %s for %s moment"),strVehicleName.c_str(),bPositiveMoment ? _T("positive") : _T("negative"));
      }
      pProgress->UpdateMessage(strProgress);

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

            gLL = pRatingSpec->GetStrengthLiveLoadFactor(ratingType,MaxAxleConfigBot);
         }
         else
         {
            gLL = pRatingSpec->GetServiceLiveLoadFactor(ratingType);
         }
      }


      Float64 Mcr = vMcr[i]->Mcr;

      Float64 Icr = vCrackedSection[i]->Icr;
      Float64 c   = vCrackedSection[i]->c; // measured from tension face to crack

      // make sure reinforcement is on the tension side of the crack
      bRebar   = (db  < Hg + ts - c ? false : bRebar);
      bStrands = (dps < Hg + ts - c ? false : bStrands);
      bTendons = (dpt < Hg + ts - c ? false : bTendons);

      // Stress in reinforcement before cracking
      Float64 fb  = 0;
      if ( bRebar )
      {
         Float64 I = pSectProp->GetIxx(loadRatingIntervalIdx,poi);
         Float64 y;
         if ( bPositiveMoment )
         {
            Float64 Ytg = pSectProp->GetY(loadRatingIntervalIdx,poi,pgsTypes::TopGirder);
            y = db - Ytg;
         }
         else
         {
            Float64 Ybg = pSectProp->GetY(loadRatingIntervalIdx,poi,pgsTypes::BottomGirder);
            y = db - Ybg;
         }

         fb = (Eb/Eg)*fabs(Mcr)*y/I;
      }

      Float64 fps = 0;
      if ( bStrands )
      {
         // because we have to play games with the live load distribution factors (as described above)
         // we can't get the effective prestress force with live load directly.
         GET_IFACE(ISpecification, pSpec);
         GET_IFACE(ILibrary, pLibrary);
         const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());
         Float64 K_liveload = pSpecEntry->GetLiveLoadElasticGain();

         if (!IsZero(K_liveload))
         {
            GET_IFACE(IPretensionForce, pPrestressForce);
            fps = pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, loadRatingIntervalIdx, pgsTypes::End);

            GET_IFACE(ILosses, pLosses);
            const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi, loadRatingIntervalIdx);

            Float64 llGain = 0;
            if ( pDetails->pLosses )
            {
               llGain = pDetails->pLosses->ElasticGainDueToLiveLoad(LLIM);
            }
            else
            {
               Float64 neff;
               Float64 ep = pStrandGeom->GetEccentricity(loadRatingIntervalIdx, poi, pgsTypes::Permanent, &neff);
               Float64 Ixx = pSectProp->GetIxx(loadRatingIntervalIdx, poi);
               llGain = LLIM*ep / Ixx;
            }
            llGain *= K_liveload;

            fps += llGain;
         }
      }

      Float64 fpt = 0;
      if ( bTendons )
      {
         GET_IFACE(IPosttensionForce,pPTForce);
         bool bIncludeMinLiveLoad = !bPositiveMoment;
         bool bIncludeMaxLiveLoad = bPositiveMoment;
         fpt = pPTForce->GetTendonStress(poi,loadRatingIntervalIdx,pgsTypes::End,ductIdx,bIncludeMinLiveLoad,bIncludeMaxLiveLoad);
      }


      Float64 W = pProductLoads->GetVehicleWeight(llType,truck_index);

      pgsYieldStressRatioArtifact stressRatioArtifact;
      stressRatioArtifact.SetRatingType(ratingType);
      stressRatioArtifact.SetPointOfInterest(poi);
      stressRatioArtifact.SetVehicleIndex(truck_index);
      stressRatioArtifact.SetVehicleWeight(W);
      stressRatioArtifact.SetVehicleName(strVehicleName.c_str());
      stressRatioArtifact.SetAllowableStressRatio(K);
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
      stressRatioArtifact.SetLiveLoadDistributionFactor(gM);
      stressRatioArtifact.SetLiveLoadFactor(gLL);
      stressRatioArtifact.SetLiveLoadMoment(LLIM+PL);
      stressRatioArtifact.SetCrackingMoment(Mcr);
      stressRatioArtifact.SetIcr(Icr);
      stressRatioArtifact.SetCrackDepth(vCrackedSection[i]->c);
      stressRatioArtifact.SetEg(Eg);

      if ( bRebar )
      {
         stressRatioArtifact.SetRebar(db,fb,fyb,Eb);
      }

      if ( bStrands )
      {
         stressRatioArtifact.SetStrand(dps,fps,fyps,Eps);
      }

      if ( bTendons )
      {
         stressRatioArtifact.SetTendon(dpt,fpt,fypt,Ept);
      }

      ratingArtifact.AddArtifact(poi,stressRatioArtifact,bPositiveMoment);
   }
}

void pgsLoadRater::GetMoments(const CGirderKey& girderKey, bool bPositiveMoment, pgsTypes::LoadRatingType ratingType, VehicleIndexType vehicleIdx, const PoiList& vPoi, Moments* pMoments) const
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bIncludeNoncompositeMoments = pSpecEntry->IncludeNoncompositeMomentsForNegMomentDesign();

   CGirderKey thisGirderKey(girderKey);
   if ( thisGirderKey.groupIndex == ALL_GROUPS )
   {
      thisGirderKey.groupIndex = 0;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType constructionLoadIntervalIdx = pIntervals->GetConstructionLoadInterval();
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
   IntervalIndexType castShearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType loadRatingIntervalIdx    = pIntervals->GetLoadRatingInterval();
   IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();

   GET_IFACE( ILossParameters, pLossParams);
   bool bTimeStep = (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP ? true : false);

   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);

   std::vector<Float64> vUnused;
   std::vector<VehicleIndexType> vUnusedIndex;

   GET_IFACE(IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType batMin = pProdForces->GetBridgeAnalysisType(pgsTypes::Minimize);
   pgsTypes::BridgeAnalysisType batMax = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   GET_IFACE(ICombinedForces2,pCombinedForces);
   GET_IFACE(IProductForces2,pProductForces);
   if ( bPositiveMoment || (!bPositiveMoment && bIncludeNoncompositeMoments) )
   {
      pMoments->vDCmin = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcDC,vPoi,batMin,rtCumulative);
      pMoments->vDCmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcDC,vPoi,batMax,rtCumulative);

      pMoments->vDWmin = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcDWRating,vPoi,batMin,rtCumulative);
      pMoments->vDWmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcDWRating,vPoi,batMax,rtCumulative);

      if ( bTimeStep )
      {
         pMoments->vCRmin = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcCR,vPoi,batMin,rtCumulative);
         pMoments->vCRmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcCR,vPoi,batMax,rtCumulative);

         pMoments->vSHmin = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcSH,vPoi,batMin,rtCumulative);
         pMoments->vSHmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcSH,vPoi,batMax,rtCumulative);

         pMoments->vREmin = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcRE,vPoi,batMin,rtCumulative);
         pMoments->vREmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcRE,vPoi,batMax,rtCumulative);

         pMoments->vPSmin = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcPS,vPoi,batMin,rtCumulative);
         pMoments->vPSmax = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcPS,vPoi,batMax,rtCumulative);
      }

      if ( vehicleIdx == INVALID_INDEX )
      {
         pProductForces->GetLiveLoadMoment(loadRatingIntervalIdx,llType,vPoi,batMin,true,true,&pMoments->vLLIMmin,&vUnused,&pMoments->vMinTruckIndex,&vUnusedIndex);
         pProductForces->GetLiveLoadMoment(loadRatingIntervalIdx,llType,vPoi,batMax,true,true,&vUnused,&pMoments->vLLIMmax,&vUnusedIndex,&pMoments->vMaxTruckIndex);
      }
      else
      {
         pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,batMin,true,true,&pMoments->vLLIMmin,&vUnused,nullptr,nullptr);
         pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,batMax,true,true,&vUnused,&pMoments->vLLIMmax,nullptr,nullptr);
      }

      pCombinedForces->GetCombinedLiveLoadMoment( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, batMin, &pMoments->vPLmin, &vUnused );
      pCombinedForces->GetCombinedLiveLoadMoment( loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, batMax, &vUnused, &pMoments->vPLmax );
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

      vConstructionMin = pProductForces->GetMoment(constructionLoadIntervalIdx,pgsTypes::pftConstruction,vPoi,batMin, rtIncremental);
      vConstructionMax = pProductForces->GetMoment(constructionLoadIntervalIdx,pgsTypes::pftConstruction,vPoi,batMax, rtIncremental);

      vSlabMin = pProductForces->GetMoment(castDeckIntervalIdx,pgsTypes::pftSlab,vPoi,batMin, rtIncremental);
      vSlabMax = pProductForces->GetMoment(castDeckIntervalIdx,pgsTypes::pftSlab,vPoi,batMax, rtIncremental);

      vSlabPanelMin = pProductForces->GetMoment(castDeckIntervalIdx,pgsTypes::pftSlabPanel,vPoi,batMin, rtIncremental);
      vSlabPanelMax = pProductForces->GetMoment(castDeckIntervalIdx,pgsTypes::pftSlabPanel,vPoi,batMax, rtIncremental);

      vDiaphragmMin = pProductForces->GetMoment(castDiaphragmIntervalIdx,pgsTypes::pftDiaphragm,vPoi,batMin, rtIncremental);
      vDiaphragmMax = pProductForces->GetMoment(castDiaphragmIntervalIdx,pgsTypes::pftDiaphragm,vPoi,batMax, rtIncremental);

      vShearKeyMin = pProductForces->GetMoment(castShearKeyIntervalIdx,pgsTypes::pftShearKey,vPoi,batMin, rtIncremental);
      vShearKeyMax = pProductForces->GetMoment(castShearKeyIntervalIdx,pgsTypes::pftShearKey,vPoi,batMax, rtIncremental);

      vUserDC1Min = pProductForces->GetMoment(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDC,vPoi,batMin, rtIncremental);
      vUserDC1Max = pProductForces->GetMoment(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDC,vPoi,batMax, rtIncremental);

      vUserDW1Min = pProductForces->GetMoment(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDW,vPoi,batMin, rtIncremental);
      vUserDW1Max = pProductForces->GetMoment(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDW,vPoi,batMax, rtIncremental);

      vUserDC2Min = pProductForces->GetMoment(compositeUserLoadIntervalIdx,pgsTypes::pftUserDC,vPoi,batMin, rtIncremental);
      vUserDC2Max = pProductForces->GetMoment(compositeUserLoadIntervalIdx,pgsTypes::pftUserDC,vPoi,batMax, rtIncremental);

      vUserDW2Min = pProductForces->GetMoment(compositeUserLoadIntervalIdx,pgsTypes::pftUserDW,vPoi,batMin, rtIncremental);
      vUserDW2Max = pProductForces->GetMoment(compositeUserLoadIntervalIdx,pgsTypes::pftUserDW,vPoi,batMax, rtIncremental);

      vTrafficBarrierMin = pProductForces->GetMoment(railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,vPoi,batMin, rtIncremental);
      vTrafficBarrierMax = pProductForces->GetMoment(railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,vPoi,batMax, rtIncremental);

      vSidewalkMin = pProductForces->GetMoment(railingSystemIntervalIdx,pgsTypes::pftSidewalk,vPoi,batMin, rtIncremental);
      vSidewalkMax = pProductForces->GetMoment(railingSystemIntervalIdx,pgsTypes::pftSidewalk,vPoi,batMax, rtIncremental);

      if ( !bFutureOverlay )
      {
         vOverlayMin = pProductForces->GetMoment(overlayIntervalIdx,pgsTypes::pftOverlay,vPoi,batMin, rtIncremental);
         vOverlayMax = pProductForces->GetMoment(overlayIntervalIdx,pgsTypes::pftOverlay,vPoi,batMax, rtIncremental);
      }
      else
      {
         vOverlayMin.resize(vPoi.size(),0);
         vOverlayMax.resize(vPoi.size(),0);
      }

      if ( bTimeStep )
      {
         vCreepMin = pProductForces->GetMoment(loadRatingIntervalIdx,pgsTypes::pftCreep,vPoi,batMin, rtCumulative);
         vCreepMax = pProductForces->GetMoment(loadRatingIntervalIdx,pgsTypes::pftCreep,vPoi,batMax, rtCumulative);

         vShrinkageMin = pProductForces->GetMoment(loadRatingIntervalIdx,pgsTypes::pftShrinkage,vPoi,batMin, rtCumulative);
         vShrinkageMax = pProductForces->GetMoment(loadRatingIntervalIdx,pgsTypes::pftShrinkage,vPoi,batMax, rtCumulative);

         vRelaxationMin = pProductForces->GetMoment(loadRatingIntervalIdx,pgsTypes::pftRelaxation,vPoi,batMin, rtCumulative);
         vRelaxationMax = pProductForces->GetMoment(loadRatingIntervalIdx,pgsTypes::pftRelaxation,vPoi,batMax, rtCumulative);

         vSecondaryMin = pProductForces->GetMoment(loadRatingIntervalIdx,pgsTypes::pftSecondaryEffects,vPoi,batMin, rtCumulative);
         vSecondaryMax = pProductForces->GetMoment(loadRatingIntervalIdx,pgsTypes::pftSecondaryEffects,vPoi,batMax, rtCumulative);
      }

      if ( vehicleIdx == INVALID_INDEX )
      {
         pProductForces->GetLiveLoadMoment( loadRatingIntervalIdx, llType, vPoi, batMin, true, true, &pMoments->vLLIMmin, &vUnused, &pMoments->vMinTruckIndex, &vUnusedIndex );
         pProductForces->GetLiveLoadMoment( loadRatingIntervalIdx, llType, vPoi, batMax, true, true, &vUnused, &pMoments->vLLIMmax, &vUnusedIndex, &pMoments->vMaxTruckIndex );
      }
      else
      {
         pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,batMin,true,true,&pMoments->vLLIMmin,&vUnused,nullptr,nullptr);
         pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,batMax,true,true,&vUnused,&pMoments->vLLIMmax,nullptr,nullptr);
      }

      // sum DC and DW

      // initialize
      pMoments->vDCmin.resize(vPoi.size(),0);
      pMoments->vDCmax.resize(vPoi.size(),0);
      pMoments->vDWmin.resize(vPoi.size(),0);
      pMoments->vDWmax.resize(vPoi.size(),0);

      if ( bTimeStep )
      {
         pMoments->vCRmin.resize(vPoi.size(),0);
         pMoments->vCRmax.resize(vPoi.size(),0);
         pMoments->vSHmin.resize(vPoi.size(),0);
         pMoments->vSHmax.resize(vPoi.size(),0);
         pMoments->vREmin.resize(vPoi.size(),0);
         pMoments->vREmax.resize(vPoi.size(),0);
      }

      pMoments->vPSmin.resize(vPoi.size(),0);
      pMoments->vPSmax.resize(vPoi.size(),0);

      GET_IFACE(IPointOfInterest,pPoi);
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vConstructionMin.cbegin(), pMoments->vDCmin.begin(), pMoments->vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vConstructionMax.cbegin(), pMoments->vDCmax.begin(), pMoments->vDCmax.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vSlabMin.cbegin(), pMoments->vDCmin.begin(), pMoments->vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vSlabMax.cbegin(), pMoments->vDCmax.begin(), pMoments->vDCmax.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vSlabPanelMin.cbegin(), pMoments->vDCmin.begin(), pMoments->vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vSlabPanelMax.cbegin(), pMoments->vDCmax.begin(), pMoments->vDCmax.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vDiaphragmMin.cbegin(), pMoments->vDCmin.begin(), pMoments->vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vDiaphragmMax.cbegin(), pMoments->vDCmax.begin(), pMoments->vDCmax.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vShearKeyMin.cbegin(), pMoments->vDCmin.begin(), pMoments->vDCmin.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vShearKeyMax.cbegin(), pMoments->vDCmax.begin(), pMoments->vDCmax.begin());

#if defined _USE_MULTITHREADING
      // we are doing a bunch of summing on independent data structures... why not do it in parallel?
      // spawn a bunch of async method calls and wait for them to complete
      std::vector<std::future<void>> vFutures;

      vFutures.emplace_back(std::async([&] {std::transform(vUserDC1Min.cbegin(), vUserDC1Min.cend(), pMoments->vDCmin.cbegin(), pMoments->vDCmin.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      vFutures.emplace_back(std::async([&] {std::transform(vUserDC1Max.cbegin(), vUserDC1Max.cend(), pMoments->vDCmax.cbegin(), pMoments->vDCmax.begin(), [](const auto& a, const auto& b) {return a + b;});}));

      vFutures.emplace_back(std::async([&] {std::transform(vUserDW1Min.cbegin(), vUserDW1Min.cend(), pMoments->vDWmin.cbegin(), pMoments->vDWmin.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      vFutures.emplace_back(std::async([&] {std::transform(vUserDW1Max.cbegin(), vUserDW1Max.cend(), pMoments->vDWmax.cbegin(), pMoments->vDWmax.begin(), [](const auto& a, const auto& b) {return a + b;});}));

      vFutures.emplace_back(std::async([&] {std::transform(vUserDC2Min.cbegin(), vUserDC2Min.cend(), pMoments->vDCmin.cbegin(), pMoments->vDCmin.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      vFutures.emplace_back(std::async([&] {std::transform(vUserDC2Max.cbegin(), vUserDC2Max.cend(), pMoments->vDCmax.cbegin(), pMoments->vDCmax.begin(), [](const auto& a, const auto& b) {return a + b;});}));

      vFutures.emplace_back(std::async([&] {std::transform(vUserDW2Min.cbegin(), vUserDW2Min.cend(), pMoments->vDWmin.cbegin(), pMoments->vDWmin.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      vFutures.emplace_back(std::async([&] {std::transform(vUserDW2Max.cbegin(), vUserDW2Max.cend(), pMoments->vDWmax.cbegin(), pMoments->vDWmax.begin(), [](const auto& a, const auto& b) {return a + b;});}));

      vFutures.emplace_back(std::async([&] {std::transform(vTrafficBarrierMin.cbegin(), vTrafficBarrierMin.cend(), pMoments->vDCmin.cbegin(), pMoments->vDCmin.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      vFutures.emplace_back(std::async([&] {std::transform(vTrafficBarrierMax.cbegin(), vTrafficBarrierMax.cend(), pMoments->vDCmax.cbegin(), pMoments->vDCmax.begin(), [](const auto& a, const auto& b) {return a + b;});}));

      vFutures.emplace_back(std::async([&] {std::transform(vSidewalkMin.cbegin(), vSidewalkMin.cend(), pMoments->vDCmin.cbegin(), pMoments->vDCmin.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      vFutures.emplace_back(std::async([&] {std::transform(vSidewalkMax.cbegin(), vSidewalkMax.cend(), pMoments->vDCmax.cbegin(), pMoments->vDCmax.begin(), [](const auto& a, const auto& b) {return a + b;});}));

      vFutures.emplace_back(std::async([&] {std::transform(vOverlayMin.cbegin(), vOverlayMin.cend(), pMoments->vDWmin.cbegin(), pMoments->vDWmin.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      vFutures.emplace_back(std::async([&] {std::transform(vOverlayMax.cbegin(), vOverlayMax.cend(), pMoments->vDWmax.cbegin(), pMoments->vDWmax.begin(), [](const auto& a, const auto& b) {return a + b;});}));

      if (bTimeStep)
      {
         vFutures.emplace_back(std::async([&] {std::transform(vCreepMin.cbegin(), vCreepMin.cend(), pMoments->vCRmin.cbegin(), pMoments->vCRmin.begin(), [](const auto& a, const auto& b) {return a + b;});}));
         vFutures.emplace_back(std::async([&] {std::transform(vCreepMax.cbegin(), vCreepMax.cend(), pMoments->vCRmax.cbegin(), pMoments->vCRmax.begin(), [](const auto& a, const auto& b) {return a + b;});}));

         vFutures.emplace_back(std::async([&] {std::transform(vShrinkageMin.cbegin(), vShrinkageMin.cend(), pMoments->vSHmin.cbegin(), pMoments->vSHmin.begin(), [](const auto& a, const auto& b) {return a + b;});}));
         vFutures.emplace_back(std::async([&] {std::transform(vShrinkageMax.cbegin(), vShrinkageMax.cend(), pMoments->vSHmax.cbegin(), pMoments->vSHmax.begin(), [](const auto& a, const auto& b) {return a + b;});}));

         vFutures.emplace_back(std::async([&] {std::transform(vRelaxationMin.cbegin(), vRelaxationMin.cend(), pMoments->vREmin.cbegin(), pMoments->vREmin.begin(), [](const auto& a, const auto& b) {return a + b;});}));
         vFutures.emplace_back(std::async([&] {std::transform(vRelaxationMax.cbegin(), vRelaxationMax.cend(), pMoments->vREmax.cbegin(), pMoments->vREmax.begin(), [](const auto& a, const auto& b) {return a + b;});}));

         vFutures.emplace_back(std::async([&] {std::transform(vSecondaryMin.cbegin(), vSecondaryMin.cend(), pMoments->vPSmin.cbegin(), pMoments->vPSmin.begin(), [](const auto& a, const auto& b) {return a + b;});}));
         vFutures.emplace_back(std::async([&] {std::transform(vSecondaryMax.cbegin(), vSecondaryMax.cend(), pMoments->vPSmax.cbegin(), pMoments->vPSmax.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      }

      // wait for async calls to complete
      for (auto& f : vFutures)
      {
         f.wait();
      }
#else
      std::transform(vUserDC1Min.cbegin(),vUserDC1Min.cend(), pMoments->vDCmin.cbegin(), pMoments->vDCmin.begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(vUserDC1Max.cbegin(),vUserDC1Max.cend(), pMoments->vDCmax.cbegin(), pMoments->vDCmax.begin(),[](const auto& a, const auto& b) {return a + b;});

      std::transform(vUserDW1Min.cbegin(),vUserDW1Min.cend(), pMoments->vDWmin.cbegin(), pMoments->vDWmin.begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(vUserDW1Max.cbegin(),vUserDW1Max.cend(), pMoments->vDWmax.cbegin(), pMoments->vDWmax.begin(),[](const auto& a, const auto& b) {return a + b;});

      std::transform(vUserDC2Min.cbegin(),vUserDC2Min.cend(), pMoments->vDCmin.cbegin(), pMoments->vDCmin.begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(vUserDC2Max.cbegin(),vUserDC2Max.cend(), pMoments->vDCmax.cbegin(), pMoments->vDCmax.begin(),[](const auto& a, const auto& b) {return a + b;});

      std::transform(vUserDW2Min.cbegin(),vUserDW2Min.cend(), pMoments->vDWmin.cbegin(), pMoments->vDWmin.begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(vUserDW2Max.cbegin(),vUserDW2Max.cend(), pMoments->vDWmax.cbegin(), pMoments->vDWmax.begin(),[](const auto& a, const auto& b) {return a + b;});

      std::transform(vTrafficBarrierMin.cbegin(),vTrafficBarrierMin.cend(), pMoments->vDCmin.cbegin(), pMoments->vDCmin.begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(vTrafficBarrierMax.cbegin(),vTrafficBarrierMax.cend(), pMoments->vDCmax.cbegin(), pMoments->vDCmax.begin(),[](const auto& a, const auto& b) {return a + b;});

      std::transform(vSidewalkMin.cbegin(),vSidewalkMin.cend(), pMoments->vDCmin.cbegin(), pMoments->vDCmin.begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(vSidewalkMax.cbegin(),vSidewalkMax.cend(), pMoments->vDCmax.cbegin(), pMoments->vDCmax.begin(),[](const auto& a, const auto& b) {return a + b;});

      std::transform(vOverlayMin.cbegin(),vOverlayMin.cend(), pMoments->vDWmin.cbegin(), pMoments->vDWmin.begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(vOverlayMax.cbegin(),vOverlayMax.cend(), pMoments->vDWmax.cbegin(), pMoments->vDWmax.begin(),[](const auto& a, const auto& b) {return a + b;});

      if ( bTimeStep )
      {
         std::transform(vCreepMin.cbegin(),vCreepMin.cend(), pMoments->vCRmin.cbegin(), pMoments->vCRmin.begin(),[](const auto& a, const auto& b) {return a + b;});
         std::transform(vCreepMax.cbegin(),vCreepMax.cend(), pMoments->vCRmax.cbegin(), pMoments->vCRmax.begin(),[](const auto& a, const auto& b) {return a + b;});

         std::transform(vShrinkageMin.cbegin(),vShrinkageMin.cend(), pMoments->vSHmin.cbegin(), pMoments->vSHmin.begin(),[](const auto& a, const auto& b) {return a + b;});
         std::transform(vShrinkageMax.cbegin(),vShrinkageMax.cend(), pMoments->vSHmax.cbegin(), pMoments->vSHmax.begin(),[](const auto& a, const auto& b) {return a + b;});

         std::transform(vRelaxationMin.cbegin(),vRelaxationMin.cend(), pMoments->vREmin.cbegin(), pMoments->vREmin.begin(),[](const auto& a, const auto& b) {return a + b;});
         std::transform(vRelaxationMax.cbegin(),vRelaxationMax.cend(), pMoments->vREmax.cbegin(), pMoments->vREmax.begin(),[](const auto& a, const auto& b) {return a + b;});

         std::transform(vSecondaryMin.cbegin(),vSecondaryMin.cend(), pMoments->vPSmin.cbegin(), pMoments->vPSmin.begin(),[](const auto& a, const auto& b) {return a + b;});
         std::transform(vSecondaryMax.cbegin(),vSecondaryMax.cend(), pMoments->vPSmax.cbegin(), pMoments->vPSmax.begin(),[](const auto& a, const auto& b) {return a + b;});
      }
#endif
   }
}

void special_transform(IBridge* pBridge,IPointOfInterest* pPoi,IIntervals* pIntervals,
                       PoiList::const_iterator poiBeginIter,
                       PoiList::const_iterator poiEndIter,
                       std::vector<Float64>::const_iterator forceBeginIter,
                       std::vector<Float64>::const_iterator resultBeginIter,
                       std::vector<Float64>::iterator outputBeginIter)
{
   auto poiIter( poiBeginIter );
   auto forceIter( forceBeginIter );
   auto resultIter( resultBeginIter );
   auto outputIter( outputBeginIter );

   for ( ; poiIter != poiEndIter; poiIter++, forceIter++, resultIter++, outputIter++ )
   {
      const pgsPointOfInterest& poi = *poiIter;
      const CSegmentKey& segmentKey = poi.GetSegmentKey();
      CSpanKey spanKey;
      Float64 Xspan;
      pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

      PierIndexType prevPierIdx = spanKey.spanIndex;
      PierIndexType nextPierIdx = prevPierIdx + 1;

      IntervalIndexType start,end,dummy;
      pIntervals->GetContinuityInterval(prevPierIdx,&dummy,&start);
      pIntervals->GetContinuityInterval(nextPierIdx,&end,&dummy);

      IntervalIndexType compositeDeckIntervalIdx       = pIntervals->GetCompositeDeckInterval();
      IntervalIndexType startPierContinuityIntervalIdx = start;
      IntervalIndexType endPierContinuityIntervalIdx   = end;

      if ( startPierContinuityIntervalIdx == compositeDeckIntervalIdx && 
           endPierContinuityIntervalIdx   == compositeDeckIntervalIdx )
      {
         *outputIter = (*forceIter + *resultIter);
      }
   }
}

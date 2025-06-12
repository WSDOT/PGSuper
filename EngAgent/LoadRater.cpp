///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include "EngAgent.h"
#include "LoadRater.h"
#include "Designer2.h"

#include <IFace\Project.h> // for ISpecification
#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <IFace\CrackedSection.h>
#include <IFace\PrestressForce.h>
#include <EAF\EAFDisplayUnits.h>

#include <psgLib/MomentCapacityCriteria.h>

#if defined _USE_MULTITHREADING
#include <future>
#endif

void special_transform(std::shared_ptr<IBridge> pBridge,std::shared_ptr<IPointOfInterest> pPoi,std::shared_ptr<IIntervals> pIntervals,
                       PoiList::const_iterator poiBeginIter,
                       PoiList::const_iterator poiEndIter,
                       std::vector<Float64>::const_iterator forceBeginIter,
                       std::vector<Float64>::const_iterator resultBeginIter,
                       std::vector<Float64>::iterator outputBeginIter);

inline bool AxleHasWeight(AxlePlacement& placement)
{
   return !IsZero(placement.Weight);
}

pgsLoadRater::pgsLoadRater(std::weak_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID) :
   m_pBroker(pBroker), m_StatusGroupID(statusGroupID)
{
   CREATE_LOGFILE("LoadRating");
}

pgsLoadRater::~pgsLoadRater()
{
   CLOSE_LOGFILE;
}

pgsRatingArtifact pgsLoadRater::Rate(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const
{
   GET_IFACE2(GetBroker(),IRatingSpecification,pRatingSpec);
   GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
   GET_IFACE2(GetBroker(),IIntervals, pIntervals);
   GET_IFACE2(GetBroker(),ILossParameters, pLossParams);

   GET_IFACE2(GetBroker(),IBridge, pBridge);
   auto firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   auto lastGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? pBridge->GetGirderGroupCount() - 1 : girderKey.groupIndex);

   pgsRatingArtifact ratingArtifact(ratingType);

   for (auto groupIdx = firstGroupIdx; groupIdx <= lastGroupIdx; groupIdx++)
   {
      CGirderKey this_girder_key(groupIdx, girderKey.girderIndex);
      //
      // Rate for moment
      //


      // Get POI for flexure load ratings
      PoiList vPoi;
      pPoi->GetPointsOfInterest(CSegmentKey(this_girder_key, ALL_SEGMENTS),&vPoi); // gets all POI
      // remove poi at points that don't matter for load rating
      pPoi->RemovePointsOfInterest(vPoi,POI_RELEASED_SEGMENT,POI_SPAN); // retain span points
      pPoi->RemovePointsOfInterest(vPoi,POI_LIFT_SEGMENT,    POI_SPAN);
      pPoi->RemovePointsOfInterest(vPoi,POI_STORAGE_SEGMENT, POI_SPAN);
      pPoi->RemovePointsOfInterest(vPoi,POI_HAUL_SEGMENT,    POI_SPAN);
      pPoi->RemovePointsOfInterest(vPoi,POI_BOUNDARY_PIER,   POI_SPAN);
   
      // get some general information that all ratings need
      IntervalIndexType loadRatingIntervalIdx = pIntervals->GetLoadRatingInterval();

      bool bNegativeMoments = pBridge->ProcessNegativeMoments(ALL_SPANS);

      bool bTimeStep = (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP ? true : false);

      // get the moments for flexure rating
      Moments positive_moments, negative_moments;
      GetMoments(this_girder_key, ratingType, vehicleIdx, vPoi, bTimeStep, &positive_moments, bNegativeMoments ? &negative_moments : nullptr);

      // do the flexure rating.. this rates moment capacity, flexural stress, and reinforcement yielding
      FlexureRating(this_girder_key, vPoi, ratingType, vehicleIdx, loadRatingIntervalIdx, bTimeStep, &positive_moments, bNegativeMoments ? &negative_moments : nullptr, ratingArtifact);

      // Rate for shear if applicable
      if ( pRatingSpec->RateForShear(ratingType) )
      {
         // we don't load rate for shear in interior piers so make another collection
         // of POI for shear... Same as for flexure but remove the POIs that are outside of the bearings
         PoiList vShearPoi(vPoi);
         SegmentIndexType nSegments = pBridge->GetSegmentCount(this_girder_key);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            CSegmentKey segmentKey(this_girder_key, segIdx);
            Float64 segmentSpanLength = pBridge->GetSegmentSpanLength(segmentKey);
            Float64 endDist = pBridge->GetSegmentStartEndDistance(segmentKey);
            vShearPoi.erase(std::remove_if(std::begin(vShearPoi), std::end(vShearPoi), PoiIsOutsideOfBearings(segmentKey, endDist, endDist + segmentSpanLength)), std::end(vShearPoi));
         }

         pPoi->SortPoiList(&vShearPoi); // sort and remove duplicates

         ShearRating(this_girder_key,vShearPoi,ratingType,vehicleIdx, loadRatingIntervalIdx, bTimeStep, ratingArtifact);
         LongitudinalReinforcementForShearRating(this_girder_key, vShearPoi, ratingType, vehicleIdx, loadRatingIntervalIdx, bTimeStep, ratingArtifact);
      }
   }

   return ratingArtifact;
}

void pgsLoadRater::FlexureRating(const CGirderKey& girderKey, const PoiList& vPoi, pgsTypes::LoadRatingType ratingType, VehicleIndexType vehicleIdx, IntervalIndexType loadRatingIntervalIdx, bool bTimeStep, const Moments* pMaxMoments, const Moments* pMinMoments, pgsRatingArtifact& ratingArtifact) const
{
   GET_IFACE2(GetBroker(),IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE2(GetBroker(),IEAFProgress, pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Load rating for moment"));

   GET_IFACE2(GetBroker(),IRatingSpecification, pRatingSpec);
   Float64 system_factor = pRatingSpec->GetSystemFactorFlexure();
   bool bIncludePL = pRatingSpec->IncludePedestrianLiveLoad();


   GET_IFACE2(GetBroker(),IMomentCapacity, pMomentCapacity);
   std::array<std::vector<const MOMENTCAPACITYDETAILS*>, 2> vpM{ pMomentCapacity->GetMomentCapacityDetails(loadRatingIntervalIdx, vPoi, true), pMinMoments ? pMomentCapacity->GetMomentCapacityDetails(loadRatingIntervalIdx, vPoi, false) : std::vector<const MOMENTCAPACITYDETAILS*>() };
   std::array<std::vector<const MINMOMENTCAPDETAILS*>, 2> vpMmin{ pMomentCapacity->GetMinMomentCapacityDetails(loadRatingIntervalIdx, vPoi, true), pMinMoments ? pMomentCapacity->GetMinMomentCapacityDetails(loadRatingIntervalIdx, vPoi, false) : std::vector<const MINMOMENTCAPDETAILS*>() };

   // get parameters for flexure rating that apply to all POI
   MomentRatingParams momentRatingParams;
   momentRatingParams.ratingType = ratingType;
   momentRatingParams.loadRatingIntervalIdx = loadRatingIntervalIdx;
   momentRatingParams.bTimeStep = bTimeStep;

   momentRatingParams.system_factor = system_factor;
   momentRatingParams.bIncludePL = bIncludePL;

   momentRatingParams.limitState = ::GetStrengthLimitStateType(ratingType);

   momentRatingParams.gDC = pRatingSpec->GetDeadLoadFactor(momentRatingParams.limitState);
   momentRatingParams.gDW = pRatingSpec->GetWearingSurfaceFactor(momentRatingParams.limitState);
   momentRatingParams.gCR = pRatingSpec->GetCreepFactor(momentRatingParams.limitState);
   momentRatingParams.gSH = pRatingSpec->GetShrinkageFactor(momentRatingParams.limitState);
   momentRatingParams.gRE = pRatingSpec->GetRelaxationFactor(momentRatingParams.limitState);
   momentRatingParams.gPS = pRatingSpec->GetSecondaryEffectsFactor(momentRatingParams.limitState);
   momentRatingParams.gLL = pRatingSpec->GetLiveLoadFactor(momentRatingParams.limitState, true);

   GET_IFACE2(GetBroker(),IProductLoads, pProductLoads);
   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);
   momentRatingParams.llType = llType;
   
   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType, girderKey);

   GET_IFACE2(GetBroker(),IBridge, pBridge);
   momentRatingParams.deckType = pBridge->GetDeckType();

   ASSIGN_IFACE2(GetBroker(),IProductForces,momentRatingParams.pProductForces);
   momentRatingParams.pRatingSpec = pRatingSpec;

   // get parameters for stress rating that apply to all POI
   bool bRateForStress = pRatingSpec->RateForStress(ratingType);
   StressRatingParams stressRatingParams;
   if (bRateForStress)
   {
      stressRatingParams.ratingType = ratingType;
      stressRatingParams.system_factor = system_factor;
      stressRatingParams.bIncludePL = bIncludePL;
      stressRatingParams.bTimeStep = bTimeStep;
      stressRatingParams.loadRatingIntervalIdx = loadRatingIntervalIdx;
      stressRatingParams.limitState = ::GetServiceLimitStateType(ratingType);
      ATLASSERT(IsServiceIIILimitState(stressRatingParams.limitState)); // must be one of the Service III limit states

      stressRatingParams.bat = momentRatingParams.pProductForces.lock()->GetBridgeAnalysisType(pgsTypes::Maximize); // only doing stress rating for tension so we want to maximize results

      stressRatingParams.gDC = pRatingSpec->GetDeadLoadFactor(stressRatingParams.limitState);
      stressRatingParams.gDW = pRatingSpec->GetWearingSurfaceFactor(stressRatingParams.limitState);
      stressRatingParams.gCR = pRatingSpec->GetCreepFactor(stressRatingParams.limitState);
      stressRatingParams.gSH = pRatingSpec->GetShrinkageFactor(stressRatingParams.limitState);
      stressRatingParams.gRE = pRatingSpec->GetRelaxationFactor(stressRatingParams.limitState);
      stressRatingParams.gPS = pRatingSpec->GetSecondaryEffectsFactor(stressRatingParams.limitState);
      stressRatingParams.gLL = pRatingSpec->GetLiveLoadFactor(stressRatingParams.limitState, true);
      if (stressRatingParams.gLL < 0)
      {
         stressRatingParams.gLL = pRatingSpec->GetServiceLiveLoadFactor(ratingType);
      }

      stressRatingParams.llType = llType;
      stressRatingParams.vehicleIdx = vehicleIdx;
      stressRatingParams.strLLNames = strLLNames;

      ASSIGN_IFACE2(GetBroker(),ICombinedForces, stressRatingParams.pCombinedForces);
      ASSIGN_IFACE2(GetBroker(),IPretensionStresses, stressRatingParams.pPrestress);
      ASSIGN_IFACE2(GetBroker(),IConcreteStressLimits, stressRatingParams.pLimits);
      stressRatingParams.pRatingSpec = pRatingSpec;
      stressRatingParams.pProductForces = momentRatingParams.pProductForces;
      stressRatingParams.pProductLoads = pProductLoads;
   }

   // get parameters for yield stress check that apply to all POI
   ASSIGN_IFACE2(GetBroker(),IPrecompressedTensileZone, stressRatingParams.pPTZ);
   bool bIsDeckPrecompressed = stressRatingParams.pPTZ.lock()->IsDeckPrecompressed(girderKey);
   bool bCheckYieldStressEnabled = pRatingSpec->CheckYieldStress(ratingType);
   bool bCheckPositiveMomentYieldStress = true;
   bool bCheckNegativeMomentYieldStress = (bIsDeckPrecompressed ? true : false);
   YieldingRatingParams yieldingRatingParams;
   if (bCheckYieldStressEnabled)
   {
      // checking yield stress option is enabled... figure out if we need to check it for positive moments
      VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
      VehicleIndexType startVehicleIdx = (vehicleIdx == INVALID_INDEX ? 0 : vehicleIdx);
      VehicleIndexType endVehicleIdx = (vehicleIdx == INVALID_INDEX ? nVehicles - 1 : startVehicleIdx);
      for (VehicleIndexType vehicleIdx = startVehicleIdx; vehicleIdx <= endVehicleIdx; vehicleIdx++)
      {
         pgsTypes::LiveLoadApplicabilityType applicability = pProductLoads->GetLiveLoadApplicability(llType, vehicleIdx);
         if (applicability == pgsTypes::llaNegMomentAndInteriorPierReaction)
         {
            // we are processing positive moments and the live load vehicle is only applicable to negative moments
            bCheckPositiveMomentYieldStress = false;
            break;
         }
      }
   }

   std::array<std::vector<const CRACKINGMOMENTDETAILS*>, 2> vpMcr;
   std::array<std::vector<const CRACKEDSECTIONDETAILS*>, 2> vpCrackedSection;

   if (bCheckYieldStressEnabled && (bCheckPositiveMomentYieldStress || bCheckNegativeMomentYieldStress))
   {
      // checking yield stress option is enabled and we have to check either positive or negative moment cases
      // so we will fill up the parameters structured

      ASSIGN_IFACE2(GetBroker(),ISectionProperties, yieldingRatingParams.pSectProp);
      ASSIGN_IFACE2(GetBroker(),IPointOfInterest, yieldingRatingParams.pPoi);
      ASSIGN_IFACE2(GetBroker(),IBridge, yieldingRatingParams.pBridge);
      ASSIGN_IFACE2(GetBroker(),IIntervals, yieldingRatingParams.pIntervals);
      ASSIGN_IFACE2(GetBroker(),ILongRebarGeometry, yieldingRatingParams.pRebarGeom);
      ASSIGN_IFACE2(GetBroker(),IMaterials, yieldingRatingParams.pMaterials);
      ASSIGN_IFACE2(GetBroker(),ILiveLoadDistributionFactors, yieldingRatingParams.pLLDF);
      ASSIGN_IFACE2(GetBroker(),IStrandGeometry, yieldingRatingParams.pStrandGeometry);
      ASSIGN_IFACE2(GetBroker(),ISegmentTendonGeometry, yieldingRatingParams.pSegmentTendonGeometry);
      ASSIGN_IFACE2(GetBroker(),IGirderTendonGeometry, yieldingRatingParams.pGirderTendonGeometry);


      yieldingRatingParams.ratingType = ratingType;
      yieldingRatingParams.system_factor = system_factor;
      yieldingRatingParams.bIncludePL = bIncludePL;
      yieldingRatingParams.bTimeStep = bTimeStep;
      yieldingRatingParams.loadRatingIntervalIdx = loadRatingIntervalIdx;

      ATLASSERT(ratingType == pgsTypes::lrPermit_Routine || ratingType == pgsTypes::lrPermit_Special);
      yieldingRatingParams.limitState  = (ratingType == pgsTypes::lrPermit_Routine ? pgsTypes::ServiceI_PermitRoutine : pgsTypes::ServiceI_PermitSpecial);

      yieldingRatingParams.YieldStressLimitCoefficient = pRatingSpec->GetYieldStressLimitCoefficient();

      yieldingRatingParams.gDC = pRatingSpec->GetDeadLoadFactor(yieldingRatingParams.limitState);
      yieldingRatingParams.gDW = pRatingSpec->GetWearingSurfaceFactor(yieldingRatingParams.limitState);
      yieldingRatingParams.gCR = pRatingSpec->GetCreepFactor(yieldingRatingParams.limitState);
      yieldingRatingParams.gSH = pRatingSpec->GetShrinkageFactor(yieldingRatingParams.limitState);
      yieldingRatingParams.gRE = pRatingSpec->GetRelaxationFactor(yieldingRatingParams.limitState);
      yieldingRatingParams.gPS = pRatingSpec->GetSecondaryEffectsFactor(yieldingRatingParams.limitState);
      yieldingRatingParams.gLL = pRatingSpec->GetLiveLoadFactor(yieldingRatingParams.limitState, true);

      GET_IFACE2(GetBroker(),ISpecification, pSpec);
      GET_IFACE2(GetBroker(),ILibrary, pLibrary);
      const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry(pSpec->GetSpecification().c_str());
      const auto& prestress_loss_criteria = pSpecEntry->GetPrestressLossCriteria();
      yieldingRatingParams.K_liveload = prestress_loss_criteria.LiveLoadElasticGain;

      yieldingRatingParams.analysisType = pSpec->GetAnalysisType();

      GET_IFACE2(GetBroker(),ICrackedSection, pCrackedSection);
      vpMcr[0] = pMomentCapacity->GetCrackingMomentDetails(loadRatingIntervalIdx, vPoi, true); // positive moments
      vpCrackedSection[0] = pCrackedSection->GetCrackedSectionDetails(vPoi, true);
      if (pMinMoments)
      {
         vpMcr[1] = pMomentCapacity->GetCrackingMomentDetails(loadRatingIntervalIdx, vPoi, false); // negative moments
         vpCrackedSection[1] = pCrackedSection->GetCrackedSectionDetails(vPoi, false);
      }
   }


   // Compute the rating factors
   IndexType poiIdx = 0;
   for (const pgsPointOfInterest& poi : vPoi)
   {
      int nMomentTypes = (pMinMoments ? 2 : 1);
      for (int momentType = 0; momentType < nMomentTypes; momentType++)
      {
         bool bPositiveMoment = (momentType == 0 ? true : false);

         // poi specific parameters for flexure rating
         momentRatingParams.pMnDetails = vpM[momentType][poiIdx];
         momentRatingParams.pMminDetails = vpMmin[momentType][poiIdx];

         // moments for flexure capacity and yield stress check are the same
         // capture them for this POI
         MomentsAtPoi moments;

         const Moments* pMoments = (momentType == 0 ? pMaxMoments : pMinMoments);
         moments.DC = pMoments->vDC[poiIdx];
         moments.DW = pMoments->vDW[poiIdx];

         if (bTimeStep)
         {
            moments.CR = pMoments->vCR[poiIdx];
            moments.SH = pMoments->vSH[poiIdx];
            moments.RE = pMoments->vRE[poiIdx];
            moments.PS = pMoments->vPS[poiIdx];
         }
         else
         {
            moments.CR = 0;
            moments.SH = 0;
            moments.RE = 0;
            moments.PS = 0;
         }

         moments.LLIM = pMoments->vLLIM[poiIdx];
         moments.PL = (momentRatingParams.bIncludePL ? pMoments->vPL[poiIdx] : 0.0);

         moments.truck_index = vehicleIdx;
         if (vehicleIdx == INVALID_INDEX)
         {
            moments.truck_index = pMoments->vTruckIndex[poiIdx];
         }

         moments.strVehicleName = strLLNames[moments.truck_index];
         moments.VehicleWeight = pProductLoads->GetVehicleWeight(momentRatingParams.llType, moments.truck_index);

         CString strProgress;
         if (poi.HasGirderCoordinate())
         {
            strProgress.Format(_T("Load rating %s for %s moment at %s"), moments.strVehicleName.c_str(), bPositiveMoment ? _T("positive") : _T("negative"),
               ::FormatDimension(poi.GetGirderCoordinate(), pDisplayUnits->GetSpanLengthUnit()));
         }
         else
         {
            strProgress.Format(_T("Load rating %s for %s moment"), moments.strVehicleName.c_str(), bPositiveMoment ? _T("positive") : _T("negative"));
         }
         pProgress->UpdateMessage(strProgress);

         MomentRating(poi, bPositiveMoment, moments, momentRatingParams, ratingArtifact);


         // do yield stress check if applicable
         if (bCheckYieldStressEnabled && ((bCheckPositiveMomentYieldStress && bPositiveMoment) || (bCheckNegativeMomentYieldStress && !bPositiveMoment)))
         {
            CString strProgress;
            if (poi.HasGirderCoordinate())
            {
               GET_IFACE2(GetBroker(),IEAFDisplayUnits, pDisplayUnits);
               strProgress.Format(_T("Checking for reinforcement yielding %s for %s moment at %s"), moments.strVehicleName.c_str(), bPositiveMoment ? _T("positive") : _T("negative"),
                  ::FormatDimension(poi.GetGirderCoordinate(), pDisplayUnits->GetSpanLengthUnit()));
            }
            else
            {
               strProgress.Format(_T("Checking for reinforcement yielding %s for %s moment"), moments.strVehicleName.c_str(), bPositiveMoment ? _T("positive") : _T("negative"));
            }
            pProgress->UpdateMessage(strProgress);

            yieldingRatingParams.pMcrDetails = vpMcr[momentType][poiIdx];
            yieldingRatingParams.pCrackedSectionDetails = vpCrackedSection[momentType][poiIdx];

            CheckReinforcementYielding(poi, bPositiveMoment, moments, yieldingRatingParams, ratingArtifact);
         }
      } // next moment type

      // do flexure stress rating if applicable
      if (bRateForStress)
      {
         CString strProgress;
         if (vehicleIdx == INVALID_INDEX)
         {
            if (poi.HasGirderCoordinate())
            {
               strProgress.Format(_T("Load rating stress at %s"), ::FormatDimension(poi.GetGirderCoordinate(), pDisplayUnits->GetSpanLengthUnit()));
            }
            else
            {
               strProgress.Format(_T("Load rating stress"));
            }
            pProgress->UpdateMessage(strProgress);
         }
         else
         {
            std::_tstring strVehicleName = strLLNames[vehicleIdx];
            if (poi.HasGirderCoordinate())
            {
               strProgress.Format(_T("Load rating %s for stress at %s"), strVehicleName.c_str(),
                  ::FormatDimension(poi.GetGirderCoordinate(), pDisplayUnits->GetSpanLengthUnit()));
            }
            else
            {
               strProgress.Format(_T("Load rating %s for stress"), strVehicleName.c_str());
            }
         }

         StressRating(poi, stressRatingParams, ratingArtifact);
      }

      poiIdx++;
   } // next poi
}

void pgsLoadRater::MomentRating(const pgsPointOfInterest& poi, bool bPositiveMoment, const MomentsAtPoi& moments,const MomentRatingParams& ratingParams, pgsRatingArtifact& ratingArtifact) const
{
   Float64 condition_factor = 1.0;
   if (bPositiveMoment)
   {
      condition_factor = ratingParams.pRatingSpec.lock()->GetGirderConditionFactor(poi.GetSegmentKey());
   }
   else
   {
      if (IsNonstructuralDeck(ratingParams.deckType))
      {
         // there is no deck, or the deck overlay is not a structural element
         // use the condition of the girder
         condition_factor = ratingParams.pRatingSpec.lock()->GetGirderConditionFactor(poi.GetSegmentKey());
      }
      else
      {
         condition_factor = ratingParams.pRatingSpec.lock()->GetDeckConditionFactor();
      }
   }

   Float64 gLL = ratingParams.gLL;
   if (gLL < 0)
   {
      if (::IsStrengthLimitState(ratingParams.limitState))
      {
         pgsTypes::BridgeAnalysisType batMin = ratingParams.pProductForces.lock()->GetBridgeAnalysisType(pgsTypes::Minimize);
         pgsTypes::BridgeAnalysisType batMax = ratingParams.pProductForces.lock()->GetBridgeAnalysisType(pgsTypes::Maximize);

         Float64 Mmin, Mmax, Dummy;
         AxleConfiguration MinAxleConfig, MaxAxleConfig, DummyAxleConfig;
         ratingParams.pProductForces.lock()->GetVehicularLiveLoadMoment(ratingParams.loadRatingIntervalIdx, ratingParams.llType, moments.truck_index, poi, batMin, true, true, &Mmin, &Dummy, &MinAxleConfig, &DummyAxleConfig);
         ratingParams.pProductForces.lock()->GetVehicularLiveLoadMoment(ratingParams.loadRatingIntervalIdx, ratingParams.llType, moments.truck_index, poi, batMax, true, true, &Dummy, &Mmax, &DummyAxleConfig, &MaxAxleConfig);

         gLL = ratingParams.pRatingSpec.lock()->GetStrengthLiveLoadFactor(ratingParams.ratingType, bPositiveMoment ? MaxAxleConfig : MinAxleConfig);
      }
      else
      {
         gLL = ratingParams.pRatingSpec.lock()->GetServiceLiveLoadFactor(ratingParams.ratingType);
      }
   }

   Float64 phi_moment = ratingParams.pMnDetails->Phi;
   Float64 Mn = ratingParams.pMnDetails->Mn;

   // NOTE: K can be less than zero when we are rating for negative moment and the minumum moment demand (Mu)
   // is positive. This happens near the simple ends of spans. For example Mr < 0 because we are rating for
   // negative moment and Mmin = min (1.2Mcr and 1.33Mu)... Mcr < 0 because we are looking at negative moment
   // and Mu > 0.... Since we are looking at the negative end of things, Mmin = 1.33Mu. +/- = -... it doesn't
   // make since for K to be negative... K < 0 indicates that the section is most definitely NOT under reinforced.
   // No adjustment needs to be made for under reinforcement so take K = 1.0
   Float64 K = (IsZero(ratingParams.pMminDetails->MrMin) ? 1.0 : ratingParams.pMminDetails->Mr / ratingParams.pMminDetails->MrMin); // MBE 6A.5.6
   if (K < 0.0 || 1.0 < K)
   {
      K = 1.0;
   }

   pgsMomentRatingArtifact momentArtifact;
   momentArtifact.SetRatingType(ratingParams.ratingType);
   momentArtifact.SetPointOfInterest(poi);
   momentArtifact.SetVehicleIndex(moments.truck_index);
   momentArtifact.SetVehicleWeight(moments.VehicleWeight);
   momentArtifact.SetVehicleName(moments.strVehicleName.c_str());
   momentArtifact.SetSystemFactor(ratingParams.system_factor);
   momentArtifact.SetConditionFactor(condition_factor);
   momentArtifact.SetCapacityReductionFactor(phi_moment);
   momentArtifact.SetMinimumReinforcementFactor(K);
   momentArtifact.SetNominalMomentCapacity(Mn);
   momentArtifact.SetDeadLoadFactor(ratingParams.gDC);
   momentArtifact.SetDeadLoadMoment(moments.DC);
   momentArtifact.SetWearingSurfaceFactor(ratingParams.gDW);
   momentArtifact.SetWearingSurfaceMoment(moments.DW);
   momentArtifact.SetCreepFactor(ratingParams.gCR);
   momentArtifact.SetCreepMoment(moments.CR);
   momentArtifact.SetShrinkageFactor(ratingParams.gSH);
   momentArtifact.SetShrinkageMoment(moments.SH);
   momentArtifact.SetRelaxationFactor(ratingParams.gRE);
   momentArtifact.SetRelaxationMoment(moments.RE);
   momentArtifact.SetSecondaryEffectsFactor(ratingParams.gPS);
   momentArtifact.SetSecondaryEffectsMoment(moments.PS);
   momentArtifact.SetLiveLoadFactor(gLL);
   momentArtifact.SetLiveLoadMoment(moments.LLIM + moments.PL);

   ratingArtifact.AddArtifact(poi, momentArtifact, bPositiveMoment);
}

void pgsLoadRater::GetCriticalSectionZones(const CGirderKey& girderKey,pgsTypes::LimitState limitState,std::vector<CRITSECTDETAILS>* pCriticalSections) const
{
   pCriticalSections->clear();

   GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
   GET_IFACE2(GetBroker(),IShearCapacity, pShearCapacity);
   GET_IFACE2(GetBroker(),IBridge, pBridge);
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
      if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEdition2004)
      {
         // if the LRFD is before 2004, critical section for shear was a function of loading.... we end up with many critical section POIs but
         // only a few (usually 2) critical section details. Match the details to the POIs and throw out the other POIs. LRFD 2004 and later only depend on Mu
         // so the number of CS POIs and CS details should always match.
         vCSPoi.erase(
            std::remove_if(vCSPoi.begin(), vCSPoi.end(), [&vCS](const pgsPointOfInterest& poi) 
               {
                  return std::find_if(vCS.begin(), vCS.end(), [&poi](const auto& csDetails) {return csDetails.GetPointOfInterest().AtSamePlace(poi);}) == vCS.cend();
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
         pCriticalSections->push_back(csDetails);
      }
   }
}

ZoneIndexType pgsLoadRater::GetCriticalSectionZone(const pgsPointOfInterest& poi, const std::vector<CRITSECTDETAILS>& criticalSections, bool bIncludeCS) const
{
   Float64 Xpoi = poi.GetDistFromStart();

   auto iter(criticalSections.cbegin());
   auto end(criticalSections.cend());
   for (; iter != end; iter++)
   {
      const CRITSECTDETAILS& csDetails(*iter);
      const pgsPointOfInterest& csPoi = csDetails.GetPointOfInterest();
      const CSegmentKey& csSegmentKey = csPoi.GetSegmentKey();

      if (csSegmentKey == poi.GetSegmentKey() && ::InRange(csDetails.Start, Xpoi, csDetails.End))
      {
         // poi is in the critical section zone
         if (!bIncludeCS && csPoi.AtSamePlace(poi))
         {
            // we want to exclude the actual critical section and the poi is at the same place as the critical section
            // return now with INVALID_INDEX since there is no reason to keep going through the loop
            return INVALID_INDEX;
         }

         // we found the critical section zone that contains our poi
         return (ZoneIndexType)(std::distance(criticalSections.cbegin(),iter));
      }
   }

   return INVALID_INDEX;
}

void pgsLoadRater::ShearRating(const CGirderKey& girderKey,const PoiList& vPoi,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx, IntervalIndexType loadRatingIntervalIdx, bool bTimeStep,pgsRatingArtifact& ratingArtifact) const
{
   GET_IFACE2(GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(GetBroker(),IEAFProgress, pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Load rating for shear"));

   pgsTypes::LimitState limitState = ::GetStrengthLimitStateType(ratingType);
   
   std::vector<CRITSECTDETAILS> criticalSections;
   GetCriticalSectionZones(girderKey, limitState, &criticalSections);

   GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
   PoiList vMyPoi(vPoi);

   GET_IFACE2(GetBroker(),IBridge, pBridge);
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
   vMyPoi.erase(std::remove_if(vMyPoi.begin(), vMyPoi.end(), [&](const pgsPointOfInterest& poi) {return GetCriticalSectionZone(poi,criticalSections) != INVALID_INDEX;}), vMyPoi.end());

   std::vector<WBFL::System::SectionValue> vDCmin, vDCmax;
   std::vector<WBFL::System::SectionValue> vDWmin, vDWmax;
   std::vector<WBFL::System::SectionValue> vCRmin, vCRmax;
   std::vector<WBFL::System::SectionValue> vSHmin, vSHmax;
   std::vector<WBFL::System::SectionValue> vREmin, vREmax;
   std::vector<WBFL::System::SectionValue> vPSmin, vPSmax;
   std::vector<WBFL::System::SectionValue> vLLIMmin,vLLIMmax;
   std::vector<WBFL::System::SectionValue> vUnused;
   std::vector<VehicleIndexType> vMinTruckIndex, vMaxTruckIndex, vUnusedIndex;
   std::vector<WBFL::System::SectionValue> vPLmin, vPLmax;

   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);

   GET_IFACE2(GetBroker(),IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType batMin = pProdForces->GetBridgeAnalysisType(pgsTypes::Minimize);
   pgsTypes::BridgeAnalysisType batMax = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   GET_IFACE2(GetBroker(),ICombinedForces2,pCombinedForces);
   GET_IFACE2(GetBroker(),IProductForces2,pProductForces);
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

   GET_IFACE2(GetBroker(),IShearCapacity,pShearCapacity);
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

   GET_IFACE2(GetBroker(),IRatingSpecification,pRatingSpec);
   Float64 system_factor    = pRatingSpec->GetSystemFactorShear();
   bool bIncludePL = pRatingSpec->IncludePedestrianLiveLoad();

   Float64 gDC = pRatingSpec->GetDeadLoadFactor(limitState);
   Float64 gDW = pRatingSpec->GetWearingSurfaceFactor(limitState);
   Float64 gCR = pRatingSpec->GetCreepFactor(limitState);
   Float64 gSH = pRatingSpec->GetShrinkageFactor(limitState);
   Float64 gRE = pRatingSpec->GetRelaxationFactor(limitState);
   Float64 gPS = pRatingSpec->GetSecondaryEffectsFactor(limitState);

   GET_IFACE2(GetBroker(),IProductLoads,pProductLoads);
   std::vector<std::_tstring> strLLNames = pProductLoads->GetVehicleNames(llType,girderKey);

   IndexType nPOI = vMyPoi.size();
   for ( IndexType i = 0; i < nPOI; i++ )
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
            WBFL::System::SectionValue Vmin, Vmax, Dummy;
            AxleConfiguration MinLeftAxleConfig, MaxLeftAxleConfig, MinRightAxleConfig, MaxRightAxleConfig, DummyLeftAxleConfig, DummyRightAxleConfig;
            pProdForces->GetVehicularLiveLoadShear(loadRatingIntervalIdx,llType,truck_index,poi,batMin,true,true,&Vmin,&Dummy,&MinLeftAxleConfig,&MinRightAxleConfig,&DummyLeftAxleConfig,&DummyRightAxleConfig);
            pProdForces->GetVehicularLiveLoadShear(loadRatingIntervalIdx,llType,truck_index,poi,batMax,true,true,&Dummy,&Vmax,&DummyLeftAxleConfig,&DummyRightAxleConfig,&MaxLeftAxleConfig,&MaxRightAxleConfig);

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

      // horizontal interface shear check
      pgsHorizontalShearArtifact h_artifact;

      pgsDesigner2 designer(m_pBroker,m_StatusGroupID);

      h_artifact.SetApplicability(false);
      if (pBridge->IsCompositeDeck())
      {
         h_artifact.SetApplicability(true);
         Float64 Vu = gDC*DC + gDW*DW + gCR*CR + gSH*SH + gRE*RE + gPS*PS + gLL*(LLIM + PL);

         IndexType castingRegionIdx = pPoi->GetDeckCastingRegion(poi);

         GET_IFACE2(GetBroker(),IMaterials, pMaterials);
         Float64 fcSlab = pMaterials->GetDeckFc(castingRegionIdx, loadRatingIntervalIdx);
         Float64 fcGirder;
         Float64 E, fy, fu;
         CClosureKey closureKey;
         if (pPoi->IsInClosureJoint(poi, &closureKey))
         {
            fcGirder = pMaterials->GetClosureJointFc(closureKey, loadRatingIntervalIdx);
            pMaterials->GetClosureJointTransverseRebarProperties(closureKey, &E, &fy, &fu);
         }
         else
         {
            fcGirder = pMaterials->GetSegmentFc(poi.GetSegmentKey(), loadRatingIntervalIdx);
            pMaterials->GetSegmentTransverseRebarProperties(poi.GetSegmentKey(), &E, &fy, &fu);
         }
         designer.CheckHorizontalShear(limitState, poi, Vu, fcSlab, fcGirder, fy, nullptr, &h_artifact);
      }
      shearArtifact.SetHorizontalInterfaceShearArtifact(h_artifact);

      ratingArtifact.AddArtifact(poi,shearArtifact);
   }
}

void pgsLoadRater::LongitudinalReinforcementForShearRating(const CGirderKey& girderKey, const PoiList& vPoi, pgsTypes::LoadRatingType ratingType, VehicleIndexType vehicleIdx, IntervalIndexType loadRatingIntervalIdx, bool bTimeStep, pgsRatingArtifact& ratingArtifact) const
{
   GET_IFACE2_NOCHECK(GetBroker(),IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE2(GetBroker(),IEAFProgress, pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Load rating for longitudinal reinforcement for shear"));

   pgsTypes::LimitState limitState = ::GetStrengthLimitStateType(ratingType);

   GET_IFACE2(GetBroker(),IShearCapacity, pShearCapacity);

   for(const pgsPointOfInterest& poi : vPoi)
   {
      pgsLongReinfShearArtifact l_artifact;
      SHEARCAPACITYDETAILS scd;
      pgsDesigner2 designer(m_pBroker,m_StatusGroupID);
      pShearCapacity->GetShearCapacityDetails(limitState, loadRatingIntervalIdx, poi, nullptr, &scd);
      designer.InitShearCheck(poi.GetSegmentKey(), loadRatingIntervalIdx, limitState, nullptr);
      designer.CheckLongReinfShear(poi, loadRatingIntervalIdx, limitState, scd, nullptr, &l_artifact);
      ratingArtifact.AddArtifact(poi, l_artifact);
   }
}

void pgsLoadRater::StressRating(const pgsPointOfInterest& poi, const StressRatingParams& ratingParams, pgsRatingArtifact& ratingArtifact) const
{
   ATLASSERT(ratingParams.ratingType == pgsTypes::lrDesign_Inventory ||
      ratingParams.ratingType == pgsTypes::lrLegal_Routine ||
      ratingParams.ratingType == pgsTypes::lrLegal_Special || // see MBE C6A.5.4.1
      ratingParams.ratingType == pgsTypes::lrLegal_Emergency ||
      ratingParams.ratingType == pgsTypes::lrPermit_Routine || // WSDOT BDM
      ratingParams.ratingType == pgsTypes::lrPermit_Special);   // WSDOT BDM


   std::vector<pgsTypes::StressLocation> vStressLocations;
   for (int i = 0; i < 2; i++)
   {
      pgsTypes::StressLocation topStressLocation = (i == 0 ? pgsTypes::TopGirder : pgsTypes::TopDeck);
      pgsTypes::StressLocation botStressLocation = (i == 0 ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

      bool bTopPTZ, bBotPTZ;
      ratingParams.pPTZ.lock()->IsInPrecompressedTensileZone(poi, ratingParams.limitState, topStressLocation, botStressLocation, &bTopPTZ, &bBotPTZ);
      if (bTopPTZ)
      {
         vStressLocations.push_back(topStressLocation);
      }

      if (bBotPTZ)
      {
         vStressLocations.push_back(botStressLocation);
      }
   }

   if (vStressLocations.size() == 0)
   {
      // no sections are in the precompressed tensile zone... how is this possible?
      pgsStressRatingArtifact stressArtifact;
      ratingArtifact.AddArtifact(poi, stressArtifact);
      return; // next POI
   }

   for (const auto& stressLocation : vStressLocations)
   {
      Float64 fDummy, fDC, fDW, fCR, fSH, fRE, fPS, fLLIM, fPL;
      ratingParams.pCombinedForces.lock()->GetStress(ratingParams.loadRatingIntervalIdx, lcDC, poi, ratingParams.bat, rtCumulative, stressLocation, stressLocation, &fDummy, &fDC);
      ratingParams.pCombinedForces.lock()->GetStress(ratingParams.loadRatingIntervalIdx, lcDWRating, poi, ratingParams.bat, rtCumulative, stressLocation, stressLocation, &fDummy, &fDW);

      if (ratingParams.bTimeStep)
      {
         ratingParams.pCombinedForces.lock()->GetStress(ratingParams.loadRatingIntervalIdx, lcCR, poi, ratingParams.bat, rtCumulative, stressLocation, stressLocation, &fDummy, &fCR);
         ratingParams.pCombinedForces.lock()->GetStress(ratingParams.loadRatingIntervalIdx, lcSH, poi, ratingParams.bat, rtCumulative, stressLocation, stressLocation, &fDummy, &fSH);
         ratingParams.pCombinedForces.lock()->GetStress(ratingParams.loadRatingIntervalIdx, lcRE, poi, ratingParams.bat, rtCumulative, stressLocation, stressLocation, &fDummy, &fRE);
         ratingParams.pCombinedForces.lock()->GetStress(ratingParams.loadRatingIntervalIdx, lcPS, poi, ratingParams.bat, rtCumulative, stressLocation, stressLocation, &fDummy, &fPS);
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

      if (ratingParams.vehicleIdx == INVALID_INDEX)
      {
         ratingParams.pProductForces.lock()->GetLiveLoadStress(ratingParams.loadRatingIntervalIdx, ratingParams.llType, poi, ratingParams.bat, true, true, stressLocation, stressLocation, &fDummy1, &fDummy2, &fDummy3, &fLLIM, &dummyIndex1, &dummyIndex2, &dummyIndex3, &truckIndex);
      }
      else
      {
         ratingParams.pProductForces.lock()->GetVehicularLiveLoadStress(ratingParams.loadRatingIntervalIdx, ratingParams.llType, ratingParams.vehicleIdx, poi, ratingParams.bat, true, true, stressLocation, stressLocation, &fDummy1, &fDummy2, &fDummy3, &fLLIM, nullptr, nullptr, nullptr, nullptr);
      }

      if (ratingParams.bIncludePL)
      {
         ratingParams.pCombinedForces.lock()->GetCombinedLiveLoadStress(ratingParams.loadRatingIntervalIdx, pgsTypes::lltPedestrian, poi, ratingParams.bat, stressLocation, stressLocation, &fDummy, &fDummy2, &fDummy3, &fPL);
      }
      else
      {
         fPL = 0;
      }

      Float64 fps = ratingParams.pPrestress.lock()->GetStress(ratingParams.loadRatingIntervalIdx, poi, stressLocation, true/*include live load if applicable*/, ratingParams.limitState, ratingParams.vehicleIdx);

      Float64 fpt = 0;
      if (ratingParams.bTimeStep)
      {
         ratingParams.pProductForces.lock()->GetStress(ratingParams.loadRatingIntervalIdx, pgsTypes::pftPostTensioning, poi, ratingParams.bat, rtCumulative, stressLocation, stressLocation, &fpt, &fDummy);
      }

      // do this in the loop because the vector of POI can be for multiple segments
      Float64 condition_factor = ratingParams.pRatingSpec.lock()->GetGirderConditionFactor(poi.GetSegmentKey());
      Float64 fr = ratingParams.pLimits.lock()->GetConcreteTensionStressLimit(ratingParams.ratingType, poi, stressLocation);

      VehicleIndexType truck_index = ratingParams.vehicleIdx;
      if (ratingParams.vehicleIdx == INVALID_INDEX)
      {
         truck_index = truckIndex;
      }

      std::_tstring strVehicleName = ratingParams.strLLNames[truck_index];
      Float64 W = ratingParams.pProductLoads.lock()->GetVehicleWeight(ratingParams.llType, truck_index);

      pgsStressRatingArtifact stressArtifact;
      stressArtifact.SetStressLocation(stressLocation);
      stressArtifact.SetRatingType(ratingParams.ratingType);
      stressArtifact.SetPointOfInterest(poi);
      stressArtifact.SetVehicleIndex(truck_index);
      stressArtifact.SetVehicleWeight(W);
      stressArtifact.SetVehicleName(strVehicleName.c_str());
      stressArtifact.SetAllowableStress(fr);
      stressArtifact.SetPrestressStress(fps);
      stressArtifact.SetPostTensionStress(fpt);
      stressArtifact.SetDeadLoadFactor(ratingParams.gDC);
      stressArtifact.SetDeadLoadStress(fDC);
      stressArtifact.SetWearingSurfaceFactor(ratingParams.gDW);
      stressArtifact.SetWearingSurfaceStress(fDW);
      stressArtifact.SetCreepFactor(ratingParams.gCR);
      stressArtifact.SetCreepStress(fCR);
      stressArtifact.SetShrinkageFactor(ratingParams.gSH);
      stressArtifact.SetShrinkageStress(fSH);
      stressArtifact.SetRelaxationFactor(ratingParams.gRE);
      stressArtifact.SetRelaxationStress(fRE);
      stressArtifact.SetSecondaryEffectsFactor(ratingParams.gPS);
      stressArtifact.SetSecondaryEffectsStress(fPS);
      stressArtifact.SetLiveLoadFactor(ratingParams.gLL);
      stressArtifact.SetLiveLoadStress(fLLIM + fPL);

      ratingArtifact.AddArtifact(poi, stressArtifact);
   }
}

void pgsLoadRater::CheckReinforcementYielding(const pgsPointOfInterest& poi, bool bPositiveMoment, const MomentsAtPoi& moments,const YieldingRatingParams& ratingParams, pgsRatingArtifact& ratingArtifact) const
{
   pgsTypes::StressLocation topLocation = pgsTypes::TopDeck;
   pgsTypes::StressLocation botLocation = pgsTypes::BottomGirder;

   // Create artifacts
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   DuctIndexType nSegmentDucts = ratingParams.pSegmentTendonGeometry.lock()->GetDuctCount(segmentKey);
   DuctIndexType nGirderDucts = ratingParams.pGirderTendonGeometry.lock()->GetDuctCount(segmentKey);

   CClosureKey closureKey;
   bool bIsInClosureJoint = ratingParams.pPoi.lock()->IsInClosureJoint(poi, &closureKey);
   bool bIsOnSegment = ratingParams.pPoi.lock()->IsOnSegment(poi);
   bool bIsOnGirder = ratingParams.pPoi.lock()->IsOnGirder(poi);

   IntervalIndexType releaseIntervalIdx = ratingParams.pIntervals.lock()->GetPrestressReleaseInterval(segmentKey);
   Float64 Hg = ratingParams.pSectProp.lock()->GetHg(releaseIntervalIdx, poi);
   Float64 ts = ratingParams.pBridge.lock()->GetStructuralSlabDepth(poi);
   if (!bIsOnGirder || bIsInClosureJoint)
   {
      Hg = ratingParams.pSectProp.lock()->GetHg(ratingParams.loadRatingIntervalIdx, poi);
      Hg -= ts;
   }

   // Get material properties
   Float64 Eg = ratingParams.pMaterials.lock()->GetSegmentEc(segmentKey, ratingParams.loadRatingIntervalIdx);

   Float64 Eb, Eps, EptSegment, EptGirder; // mod E of rebar, strand, tendon
   Float64 fyb, fyps, fyptSegment, fyptGirder; // yield strength of bar, strand, tendon
   Float64 fu;

   if (bPositiveMoment)
   {
      // extreme tension rebar is going to be in the girder
      if (bIsInClosureJoint)
      {
         ratingParams.pMaterials.lock()->GetClosureJointLongitudinalRebarProperties(closureKey, &Eb, &fyb, &fu);
      }
      else
      {
         ratingParams.pMaterials.lock()->GetSegmentLongitudinalRebarProperties(segmentKey, &Eb, &fyb, &fu);
      }
   }
   else
   {
      // extreme tension rebar is going to be in the deck
      ratingParams.pMaterials.lock()->GetDeckRebarProperties(&Eb, &fyb, &fu);
   }

   // just getting material properties so assuming straight strands is fine
   Eps = ratingParams.pMaterials.lock()->GetStrandMaterial(segmentKey, pgsTypes::Straight)->GetE();
   fyps = ratingParams.pMaterials.lock()->GetStrandMaterial(segmentKey, pgsTypes::Straight)->GetYieldStrength();

   // NOTE: it is ok to use segmentKey here because it promotes to a girder key
   EptSegment = ratingParams.pMaterials.lock()->GetSegmentTendonMaterial(segmentKey)->GetE();
   fyptSegment = ratingParams.pMaterials.lock()->GetSegmentTendonMaterial(segmentKey)->GetYieldStrength();
   EptGirder = ratingParams.pMaterials.lock()->GetGirderTendonMaterial(segmentKey)->GetE();
   fyptGirder = ratingParams.pMaterials.lock()->GetGirderTendonMaterial(segmentKey)->GetYieldStrength();


   // Get distance to reinforcement from extreme compression face

   // keep track of whether or not the type of reinforcement is used
   bool bRebar = false;
   bool bStrands = false;
   bool bSegmentTendons = false;
   bool bGirderTendons = false;

   // rebar
   Float64 db;
   if (bPositiveMoment)
   {
      // extreme tension rebar is going to be in the girder, furthest from the top of the girder
      CComPtr<IRebarSection> rebarSection;
      ratingParams.pRebarGeom.lock()->GetRebars(poi, &rebarSection);

      IndexType count;
      rebarSection->get_Count(&count);
      if (0 < count)
      {
         bRebar = true;
      }

      CComPtr<IEnumRebarSectionItem> enumRebarSectionItem;
      rebarSection->get__EnumRebarSectionItem(&enumRebarSectionItem);

      Float64 Y = DBL_MAX;
      CComPtr<IRebarSectionItem> rebarSectionItem;
      while (enumRebarSectionItem->Next(1, &rebarSectionItem, nullptr) != S_FALSE)
      {
         CComPtr<IPoint2d> location;
         rebarSectionItem->get_Location(&location);
         Float64 y;
         location->get_Y(&y); // this is in girder section coordinates so (0,0) is at top center... y should be < 0
         ATLASSERT(y < 0);
         Y = Min(Y, y);
         rebarSectionItem.Release();
      }

      db = ts - Y;
   }
   else
   {
      // extreme tension rebar is going to be in the deck
      Float64 As;
      Float64 Y; // distance from top of girder to rebar
      ratingParams.pRebarGeom.lock()->GetDeckReinforcing(poi, pgsTypes::drmTop, pgsTypes::drbAll, pgsTypes::drcAll, false, &As, &Y);
      if (IsZero(As))
      {
         // no top mat, try bottom mat
         ratingParams.pRebarGeom.lock()->GetDeckReinforcing(poi, pgsTypes::drmBottom, pgsTypes::drbAll, pgsTypes::drcAll, false, &As, &Y);
      }

      db = Hg + Y;

      if (!IsZero(As))
      {
         bRebar = true;
      }
   }

   // strand
   Float64 dps;
   Float64 Y = (bPositiveMoment ? DBL_MAX : -DBL_MAX);
   if (bIsOnSegment)
   {
      for (int j = 0; j < 2; j++)
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)j;
         CComPtr<IPoint2dCollection> strandPoints;
         ratingParams.pStrandGeometry.lock()->GetStrandPositions(poi, strandType, &strandPoints);
         IndexType nStrands;
         strandPoints->get_Count(&nStrands);
         if (0 < nStrands)
         {
            bStrands = true;
         }
         for (IndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
         {
            CComPtr<IPoint2d> pnt;
            strandPoints->get_Item(strandIdx, &pnt);
            Float64 y;
            pnt->get_Y(&y);
            ATLASSERT(y < 0);
            if (bPositiveMoment)
            {
               Y = Min(Y, y); // want furthest from top
            }
            else
            {
               Y = Max(Y, y); // want closest to top
            }
         }
      } // next strand type
   }

   if (bPositiveMoment)
   {
      dps = ts - Y;
   }
   else
   {
      dps = Hg + Y;
   }

   // segment tendons
   Float64 dptSegment;
   DuctIndexType segmentTendonIdx = INVALID_INDEX; // index of the segment tendon that is furthest from the compression face
   Y = (bPositiveMoment ? DBL_MAX : -DBL_MAX);
   if (bIsOnSegment)
   {
      if (0 < nSegmentDucts)
      {
         bSegmentTendons = true;
      }

      for (DuctIndexType theDuctIdx = 0; theDuctIdx < nSegmentDucts; theDuctIdx++)
      {
         if (ratingParams.pSegmentTendonGeometry.lock()->IsOnDuct(poi))
         {
            CComPtr<IPoint2d> pnt;
            ratingParams.pSegmentTendonGeometry.lock()->GetSegmentDuctPoint(poi, theDuctIdx, &pnt);
            Float64 y;
            pnt->get_Y(&y);
            ATLASSERT(y < 0);
            if (bPositiveMoment)
            {
               if (MinIndex(Y, y) == 1)
               {
                  segmentTendonIdx = theDuctIdx;
               }

               Y = Min(Y, y); // want furthest from top
            }
            else
            {
               if (MaxIndex(Y, y) == 1)
               {
                  segmentTendonIdx = theDuctIdx;
               }

               Y = Max(Y, y); // want closest to top
            }
         }
      }
   }

   if (bPositiveMoment)
   {
      dptSegment = ts - Y;
   }
   else
   {
      dptSegment = Hg + Y;
   }

   // girder tendons
   Float64 dptGirder;
   DuctIndexType girderTendonIdx = INVALID_INDEX; // index of the girder tendon that is furthest from the compression face
   Y = (bPositiveMoment ? DBL_MAX : -DBL_MAX);
   if (bIsOnGirder)
   {
      if (0 < nGirderDucts)
      {
         bGirderTendons = true;
      }

      for (DuctIndexType theDuctIdx = 0; theDuctIdx < nGirderDucts; theDuctIdx++)
      {
         if (ratingParams.pGirderTendonGeometry.lock()->IsOnDuct(poi, theDuctIdx))
         {
            CComPtr<IPoint2d> pnt;
            ratingParams.pGirderTendonGeometry.lock()->GetGirderDuctPoint(poi, theDuctIdx, &pnt);
            Float64 y;
            pnt->get_Y(&y);
            ATLASSERT(y < 0);
            if (bPositiveMoment)
            {
               if (MinIndex(Y, y) == 1)
               {
                  girderTendonIdx = theDuctIdx;
               }

               Y = Min(Y, y); // want furthest from top
            }
            else
            {
               if (MaxIndex(Y, y) == 1)
               {
                  girderTendonIdx = theDuctIdx;
               }

               Y = Max(Y, y); // want closest to top
            }
         }
      }
   }

   if (bPositiveMoment)
   {
      dptGirder = ts - Y;
   }
   else
   {
      dptGirder = Hg + Y;
   }

   // Get allowable
   Float64 gM;
   Float64 LLIM = moments.LLIM;
   if (ratingParams.ratingType == pgsTypes::lrPermit_Special) // if it is any of the special permit types
   {
      // The live load distribution factor used for special permits is one loaded lane without multiple presence factor.
      // This is the Fatigue LLDF. See MBE 6A.4.5.4.2b and C6A.5.4.2.2b.
      // However, when evaluating the reinforcement yielding in the Service I limit state (the thing this function does)
      // the controlling of one loaded lane and two or more loaded lanes live load distribution factors is to be used.
      // See 6A.5.4.2.2b
      //
      // LLIM includes the one lane LLDF (Fatigue)... divide out this LLDF and multiply by the correct LLDF (Service I)

      Float64 gpM_Service, gnM_Service, gV;
      Float64 gpM_Fatigue, gnM_Fatigue;


      ratingParams.pLLDF.lock()->GetDistributionFactors(poi, pgsTypes::FatigueI, &gpM_Fatigue, &gnM_Fatigue, &gV);
      ratingParams.pLLDF.lock()->GetDistributionFactors(poi, pgsTypes::ServiceI_PermitSpecial, &gpM_Service, &gnM_Service, &gV);

      if (bPositiveMoment)
      {
         gM = gpM_Service;
         LLIM *= gpM_Service / gpM_Fatigue;
      }
      else
      {
         gM = gnM_Service;
         LLIM *= gnM_Service / gnM_Fatigue;
      }
   }
   else
   {
      ATLASSERT(ratingParams.ratingType == pgsTypes::lrPermit_Routine);
      Float64 gpM, gnM, gV;
      GET_IFACE2(GetBroker(),ILiveLoadDistributionFactors, pLLDF);
      pLLDF->GetDistributionFactors(poi, pgsTypes::ServiceI_PermitRoutine, &gpM, &gnM, &gV);
      gM = (bPositiveMoment ? gpM : gnM);
   }

   Float64 gLL = ratingParams.gLL;
   if (gLL < 0)
   {
      GET_IFACE2(GetBroker(),IRatingSpecification, pRatingSpec);
      if (::IsStrengthLimitState(ratingParams.limitState))
      {
         // need to compute gLL based on axle weights
         GET_IFACE2(GetBroker(),IProductForces, pProductForces);
         Float64 fMinTop, fMinBot, fMaxTop, fMaxBot, fDummyTop, fDummyBot;
         AxleConfiguration MinAxleConfigTop, MinAxleConfigBot, MaxAxleConfigTop, MaxAxleConfigBot, DummyAxleConfigTop, DummyAxleConfigBot;
         if (ratingParams.analysisType == pgsTypes::Envelope)
         {
            pProductForces->GetVehicularLiveLoadStress(ratingParams.loadRatingIntervalIdx, ratingParams.llType, moments.truck_index, poi, pgsTypes::MinSimpleContinuousEnvelope, true, true, topLocation, botLocation, &fMinTop, &fDummyTop, &fMinBot, &fDummyBot, &MinAxleConfigTop, &DummyAxleConfigTop, &MinAxleConfigBot, &DummyAxleConfigBot);
            pProductForces->GetVehicularLiveLoadStress(ratingParams.loadRatingIntervalIdx, ratingParams.llType, moments.truck_index, poi, pgsTypes::MaxSimpleContinuousEnvelope, true, true, topLocation, botLocation, &fDummyTop, &fMaxTop, &fDummyBot, &fMaxBot, &DummyAxleConfigTop, &MaxAxleConfigTop, &DummyAxleConfigBot, &MaxAxleConfigBot);
         }
         else
         {
            pProductForces->GetVehicularLiveLoadStress(ratingParams.loadRatingIntervalIdx, ratingParams.llType, moments.truck_index, poi, ratingParams.analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, true, topLocation, botLocation, &fMinTop, &fMaxTop, &fMaxTop, &fMaxBot, &MinAxleConfigTop, &MaxAxleConfigTop, &MinAxleConfigBot, &MaxAxleConfigBot);
         }

         gLL = pRatingSpec->GetStrengthLiveLoadFactor(ratingParams.ratingType, MaxAxleConfigBot);
      }
      else
      {
         gLL = pRatingSpec->GetServiceLiveLoadFactor(ratingParams.ratingType);
      }
   }


   Float64 Mcr = ratingParams.pMcrDetails->Mcr;

   Float64 Icr = ratingParams.pCrackedSectionDetails->Icr;
   Float64 c = ratingParams.pCrackedSectionDetails->c; // measured from tension face to crack

   // make sure reinforcement is on the tension side of the crack
   bRebar = (db  < Hg + ts - c ? false : bRebar);
   bStrands = (dps < Hg + ts - c ? false : bStrands);
   bSegmentTendons = (dptSegment < Hg + ts - c ? false : bSegmentTendons);
   bGirderTendons = (dptGirder < Hg + ts - c ? false : bGirderTendons);

   // Stress in reinforcement before cracking
   Float64 fb = 0;
   if (bRebar)
   {
      Float64 I = ratingParams.pSectProp.lock()->GetIxx(ratingParams.loadRatingIntervalIdx, poi);
      Float64 y;
      if (bPositiveMoment)
      {
         Float64 Ytg = ratingParams.pSectProp.lock()->GetY(ratingParams.loadRatingIntervalIdx, poi, pgsTypes::TopGirder);
         y = db - Ytg;
      }
      else
      {
         Float64 Ybg = ratingParams.pSectProp.lock()->GetY(ratingParams.loadRatingIntervalIdx, poi, pgsTypes::BottomGirder);
         y = db - Ybg;
      }

      fb = (Eb / Eg)*fabs(Mcr)*y / I;
   }

   Float64 fps = 0;
   if (bStrands)
   {
      // because we have to play games with the live load distribution factors (as described above)
      // we can't get the effective prestress force with live load directly.

      if (!IsZero(ratingParams.K_liveload))
      {
         GET_IFACE2(GetBroker(),IPretensionForce, pPrestressForce);
         fps = pPrestressForce->GetEffectivePrestress(poi, pgsTypes::Permanent, ratingParams.loadRatingIntervalIdx, pgsTypes::End);

         GET_IFACE2(GetBroker(),ILosses, pLosses);
         const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi, ratingParams.loadRatingIntervalIdx);

         Float64 llGain = 0;
         if (pDetails->pLosses)
         {
            llGain = pDetails->pLosses->ElasticGainDueToLiveLoad(LLIM);
         }
         else
         {
            Float64 ep = ratingParams.pStrandGeometry.lock()->GetEccentricity(ratingParams.loadRatingIntervalIdx, poi, pgsTypes::Permanent).Y();
            Float64 Ixx = ratingParams.pSectProp.lock()->GetIxx(ratingParams.loadRatingIntervalIdx, poi);
            llGain = LLIM*ep / Ixx;
         }
         llGain *= ratingParams.K_liveload;

         fps += llGain;
      }
   }

   Float64 fptSegment = 0;
   if (bSegmentTendons)
   {
      GET_IFACE2(GetBroker(),IPosttensionForce, pPTForce);
      bool bIncludeMinLiveLoad = !bPositiveMoment;
      bool bIncludeMaxLiveLoad = bPositiveMoment;
      fptSegment = pPTForce->GetSegmentTendonStress(poi, ratingParams.loadRatingIntervalIdx, pgsTypes::End, segmentTendonIdx, bIncludeMinLiveLoad, bIncludeMaxLiveLoad);
   }

   Float64 fptGirder = 0;
   if (bGirderTendons)
   {
      GET_IFACE2(GetBroker(),IPosttensionForce, pPTForce);
      bool bIncludeMinLiveLoad = !bPositiveMoment;
      bool bIncludeMaxLiveLoad = bPositiveMoment;
      fptGirder = pPTForce->GetGirderTendonStress(poi, ratingParams.loadRatingIntervalIdx, pgsTypes::End, girderTendonIdx, bIncludeMinLiveLoad, bIncludeMaxLiveLoad);
   }

   pgsYieldStressRatioArtifact stressRatioArtifact;
   stressRatioArtifact.SetRatingType(ratingParams.ratingType);
   stressRatioArtifact.SetPointOfInterest(poi);
   stressRatioArtifact.SetVehicleIndex(moments.truck_index);
   stressRatioArtifact.SetVehicleWeight(moments.VehicleWeight);
   stressRatioArtifact.SetVehicleName(moments.strVehicleName.c_str());
   stressRatioArtifact.SetAllowableStressRatio(ratingParams.YieldStressLimitCoefficient);
   stressRatioArtifact.SetDeadLoadFactor(ratingParams.gDC);
   stressRatioArtifact.SetDeadLoadMoment(moments.DC);
   stressRatioArtifact.SetWearingSurfaceFactor(ratingParams.gDW);
   stressRatioArtifact.SetWearingSurfaceMoment(moments.DW);
   stressRatioArtifact.SetCreepFactor(ratingParams.gCR);
   stressRatioArtifact.SetCreepMoment(moments.CR);
   stressRatioArtifact.SetShrinkageFactor(ratingParams.gSH);
   stressRatioArtifact.SetShrinkageMoment(moments.SH);
   stressRatioArtifact.SetRelaxationFactor(ratingParams.gRE);
   stressRatioArtifact.SetRelaxationMoment(moments.RE);
   stressRatioArtifact.SetSecondaryEffectsFactor(ratingParams.gPS);
   stressRatioArtifact.SetSecondaryEffectsMoment(moments.PS);
   stressRatioArtifact.SetLiveLoadDistributionFactor(gM);
   stressRatioArtifact.SetLiveLoadFactor(gLL);
   stressRatioArtifact.SetLiveLoadMoment(LLIM + moments.PL); // note: note using moments.LLIM on purpose... see above
   stressRatioArtifact.SetCrackingMoment(Mcr);
   stressRatioArtifact.SetIcr(Icr);
   stressRatioArtifact.SetCrackDepth(c);
   stressRatioArtifact.SetEg(Eg);

   if (bRebar)
   {
      stressRatioArtifact.SetRebar(db, fb, fyb, Eb);
   }

   if (bStrands)
   {
      stressRatioArtifact.SetStrand(dps, fps, fyps, Eps);
   }

   if (bSegmentTendons)
   {
      stressRatioArtifact.SetSegmentTendon(dptSegment, fptSegment, fyptSegment, EptSegment);
   }

   if (bGirderTendons)
   {
      stressRatioArtifact.SetGirderTendon(dptGirder, fptGirder, fyptGirder, EptGirder);
   }

   ratingArtifact.AddArtifact(poi, stressRatioArtifact, bPositiveMoment);
}

void pgsLoadRater::GetMoments(const CGirderKey& girderKey, pgsTypes::LoadRatingType ratingType, VehicleIndexType vehicleIdx, const PoiList& vPoi, bool bTimeStep, Moments* pMaxMoments, Moments* pMinMoments) const
{
   GET_IFACE2(GetBroker(),ILibrary,pLib);
   GET_IFACE2(GetBroker(),ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& moment_capacity_criteria = pSpecEntry->GetMomentCapacityCriteria();
   bool bIncludeNoncompositeMoments = moment_capacity_criteria.bIncludeNoncompositeMomentsForNegMomentDesign;

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType constructionLoadIntervalIdx     = pIntervals->GetConstructionLoadInterval();
   IntervalIndexType castDiaphragmIntervalIdx        = pIntervals->GetCastIntermediateDiaphragmsInterval();
   IntervalIndexType castShearKeyIntervalIdx         = pIntervals->GetCastShearKeyInterval();
   IntervalIndexType castDeckIntervalIdx             = pIntervals->GetLastCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx        = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx              = pIntervals->GetOverlayInterval();
   IntervalIndexType loadRatingIntervalIdx           = pIntervals->GetLoadRatingInterval();
   IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
   IntervalIndexType compositeUserLoadIntervalIdx    = pIntervals->GetCompositeUserLoadInterval();

   pgsTypes::LiveLoadType llType = GetLiveLoadType(ratingType);

   std::vector<Float64> vUnused;
   std::vector<VehicleIndexType> vUnusedIndex;

   GET_IFACE2(GetBroker(),IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType batMin = pProdForces->GetBridgeAnalysisType(pgsTypes::Minimize);
   pgsTypes::BridgeAnalysisType batMax = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   GET_IFACE2(GetBroker(),ICombinedForces2,pCombinedForces);
   GET_IFACE2(GetBroker(),IProductForces2,pProductForces);
   int n = (pMinMoments && bIncludeNoncompositeMoments ? 2 : 1);
   for (int i = 0; i < n; i++)
   {
      Moments* pMoments = (i == 0 ? pMaxMoments : pMinMoments);
      pgsTypes::BridgeAnalysisType bat = (i == 0 ? batMax : batMin);

      pMoments->vDC = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcDC,vPoi,bat,rtCumulative);
      pMoments->vDW = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcDWRating,vPoi,bat,rtCumulative);

      if ( bTimeStep )
      {
         pMoments->vCR = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcCR,vPoi,bat,rtCumulative);
         pMoments->vSH = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcSH,vPoi,bat,rtCumulative);
         pMoments->vRE = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcRE,vPoi,bat,rtCumulative);
         pMoments->vPS = pCombinedForces->GetMoment(loadRatingIntervalIdx,lcPS,vPoi,bat,rtCumulative);
      }

      if ( vehicleIdx == INVALID_INDEX )
      {
         if (i == 0)
         {
            pProductForces->GetLiveLoadMoment(loadRatingIntervalIdx, llType, vPoi, bat, true, true, &vUnused, &pMoments->vLLIM, &vUnusedIndex, &pMoments->vTruckIndex);
         }
         else
         {
            pProductForces->GetLiveLoadMoment(loadRatingIntervalIdx, llType, vPoi, bat, true, true, &pMoments->vLLIM, &vUnused, &pMoments->vTruckIndex, &vUnusedIndex);
         }
      }
      else
      {
         if (i == 0)
         {
            pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx, llType, vehicleIdx, vPoi, bat, true, true, &vUnused, &pMoments->vLLIM, nullptr, nullptr);
         }
         else
         {
            pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx, llType, vehicleIdx, vPoi, bat, true, true, &pMoments->vLLIM, &vUnused, nullptr, nullptr);
         }
      }

      if (i == 0)
      {
         pCombinedForces->GetCombinedLiveLoadMoment(loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, bat, &vUnused, &pMoments->vPL);
      }
      else
      {
         pCombinedForces->GetCombinedLiveLoadMoment(loadRatingIntervalIdx, pgsTypes::lltPedestrian, vPoi, bat, &pMoments->vPL, &vUnused);
      }
   }

   if (pMinMoments && !bIncludeNoncompositeMoments)
   {
      // Because of the construction sequence, some loads don't contribute to the negative moment..
      // Those loads applied to the simple span girders before continuity don't contribute to the
      // negative moment resisted by the deck.
      //
      // Special load processing is required
      GET_IFACE2(GetBroker(),IBridge,pBridge);

      std::vector<Float64> vConstruction;
      std::vector<Float64> vSlab;
      std::vector<Float64> vSlabPanel;
      std::vector<Float64> vDiaphragm;
      std::vector<Float64> vShearKey;
      std::vector<Float64> vUserDC1;
      std::vector<Float64> vUserDW1;
      std::vector<Float64> vUserDC2;
      std::vector<Float64> vUserDW2;
      std::vector<Float64> vTrafficBarrier;
      std::vector<Float64> vSidewalk;
      std::vector<Float64> vOverlay;
      std::vector<Float64> vCreep;
      std::vector<Float64> vShrinkage;
      std::vector<Float64> vRelaxation;
      std::vector<Float64> vSecondary;

      bool bFutureOverlay = pBridge->HasOverlay() && pBridge->IsFutureOverlay();

      // Get all the product load responses
      GET_IFACE2(GetBroker(),IProductForces2,pProductForces);

      if (constructionLoadIntervalIdx == INVALID_INDEX)
      {
         vConstruction.resize(vPoi.size(), 0);
      }
      else
      {
         vConstruction = pProductForces->GetMoment(constructionLoadIntervalIdx, pgsTypes::pftConstruction, vPoi, batMin, rtIncremental);
      }

      if (castDeckIntervalIdx == INVALID_INDEX)
      {
         vSlab.resize(vPoi.size(), 0);
         vSlabPanel.resize(vPoi.size(), 0);
      }
      else
      {
         vSlab = pProductForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlab, vPoi, batMin, rtIncremental);
         vSlabPanel = pProductForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlabPanel, vPoi, batMin, rtIncremental);
      }

      if (castDiaphragmIntervalIdx == INVALID_INDEX)
      {
         vDiaphragm.resize(vPoi.size(), 0);
      }
      else
      {
         vDiaphragm = pProductForces->GetMoment(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, vPoi, batMin, rtIncremental);
      }

      if (castShearKeyIntervalIdx == INVALID_INDEX)
      {
         vShearKey.resize(vPoi.size(), 0);
      }
      else
      {
         vShearKey = pProductForces->GetMoment(castShearKeyIntervalIdx, pgsTypes::pftShearKey, vPoi, batMin, rtIncremental);
      }

      vUserDC1        = pProductForces->GetMoment(noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDC,         vPoi, batMin, rtIncremental);
      vUserDW1        = pProductForces->GetMoment(noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDW,         vPoi, batMin, rtIncremental);
      vUserDC2        = pProductForces->GetMoment(compositeUserLoadIntervalIdx,    pgsTypes::pftUserDC,         vPoi, batMin, rtIncremental);
      vUserDW2        = pProductForces->GetMoment(compositeUserLoadIntervalIdx,    pgsTypes::pftUserDW,         vPoi, batMin, rtIncremental);
      vTrafficBarrier = pProductForces->GetMoment(railingSystemIntervalIdx,        pgsTypes::pftTrafficBarrier, vPoi, batMin, rtIncremental);
      vSidewalk       = pProductForces->GetMoment(railingSystemIntervalIdx,        pgsTypes::pftSidewalk,       vPoi, batMin, rtIncremental);

      if ( bFutureOverlay )
      {
         vOverlay.resize(vPoi.size(), 0);
      }
      else
      {
         vOverlay = pProductForces->GetMoment(overlayIntervalIdx, pgsTypes::pftOverlay, vPoi, batMin, rtIncremental);
      }

      if ( bTimeStep )
      {
         vCreep = pProductForces->GetMoment(loadRatingIntervalIdx,pgsTypes::pftCreep,vPoi,batMin, rtCumulative);
         vShrinkage = pProductForces->GetMoment(loadRatingIntervalIdx,pgsTypes::pftShrinkage,vPoi,batMin, rtCumulative);
         vRelaxation = pProductForces->GetMoment(loadRatingIntervalIdx,pgsTypes::pftRelaxation,vPoi,batMin, rtCumulative);
         vSecondary = pProductForces->GetMoment(loadRatingIntervalIdx,pgsTypes::pftSecondaryEffects,vPoi,batMin, rtCumulative);
      }

      if ( vehicleIdx == INVALID_INDEX )
      {
         pProductForces->GetLiveLoadMoment( loadRatingIntervalIdx, llType, vPoi, batMin, true, true, &pMinMoments->vLLIM, &vUnused, &pMinMoments->vTruckIndex, &vUnusedIndex );
      }
      else
      {
         pProductForces->GetVehicularLiveLoadMoment(loadRatingIntervalIdx,llType,vehicleIdx,vPoi,batMin,true,true,&pMinMoments->vLLIM,&vUnused,nullptr,nullptr);
      }

      // sum DC and DW

      // initialize
      pMinMoments->vDC.resize(vPoi.size(),0);
      pMinMoments->vDW.resize(vPoi.size(),0);

      if ( bTimeStep )
      {
         pMinMoments->vCR.resize(vPoi.size(),0);
         pMinMoments->vSH.resize(vPoi.size(),0);
         pMinMoments->vRE.resize(vPoi.size(),0);
      }

      pMinMoments->vPS.resize(vPoi.size(),0);

      GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vConstruction.cbegin(), pMinMoments->vDC.begin(), pMinMoments->vDC.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vSlab.cbegin(), pMinMoments->vDC.begin(), pMinMoments->vDC.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vSlabPanel.cbegin(), pMinMoments->vDC.begin(), pMinMoments->vDC.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vDiaphragm.cbegin(), pMinMoments->vDC.begin(), pMinMoments->vDC.begin());
      special_transform(pBridge,pPoi,pIntervals,vPoi.cbegin(),vPoi.cend(),vShearKey.cbegin(), pMinMoments->vDC.begin(), pMinMoments->vDC.begin());

#if defined _USE_MULTITHREADING
      // we are doing a bunch of summing on independent data structures... why not do it in parallel?
      // spawn a bunch of async method calls and wait for them to complete
      std::vector<std::future<void>> vFutures;
      vFutures.emplace_back(std::async([&] {std::transform(vUserDC1.cbegin(), vUserDC1.cend(), pMinMoments->vDC.cbegin(), pMinMoments->vDC.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      vFutures.emplace_back(std::async([&] {std::transform(vUserDW1.cbegin(), vUserDW1.cend(), pMinMoments->vDW.cbegin(), pMinMoments->vDW.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      vFutures.emplace_back(std::async([&] {std::transform(vUserDC2.cbegin(), vUserDC2.cend(), pMinMoments->vDC.cbegin(), pMinMoments->vDC.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      vFutures.emplace_back(std::async([&] {std::transform(vUserDW2.cbegin(), vUserDW2.cend(), pMinMoments->vDW.cbegin(), pMinMoments->vDW.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      vFutures.emplace_back(std::async([&] {std::transform(vTrafficBarrier.cbegin(), vTrafficBarrier.cend(), pMinMoments->vDC.cbegin(), pMinMoments->vDC.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      vFutures.emplace_back(std::async([&] {std::transform(vSidewalk.cbegin(), vSidewalk.cend(), pMinMoments->vDC.cbegin(), pMinMoments->vDC.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      vFutures.emplace_back(std::async([&] {std::transform(vOverlay.cbegin(), vOverlay.cend(), pMinMoments->vDW.cbegin(), pMinMoments->vDW.begin(), [](const auto& a, const auto& b) {return a + b;});}));

      if (bTimeStep)
      {
         vFutures.emplace_back(std::async([&] {std::transform(vCreep.cbegin(), vCreep.cend(), pMinMoments->vCR.cbegin(), pMinMoments->vCR.begin(), [](const auto& a, const auto& b) {return a + b;});}));
         vFutures.emplace_back(std::async([&] {std::transform(vShrinkage.cbegin(), vShrinkage.cend(), pMinMoments->vSH.cbegin(), pMinMoments->vSH.begin(), [](const auto& a, const auto& b) {return a + b;});}));
         vFutures.emplace_back(std::async([&] {std::transform(vRelaxation.cbegin(), vRelaxation.cend(), pMinMoments->vRE.cbegin(), pMinMoments->vRE.begin(), [](const auto& a, const auto& b) {return a + b;});}));
         vFutures.emplace_back(std::async([&] {std::transform(vSecondary.cbegin(), vSecondary.cend(), pMinMoments->vPS.cbegin(), pMinMoments->vPS.begin(), [](const auto& a, const auto& b) {return a + b;});}));
      }

      // wait for async calls to complete
      for (auto& f : vFutures)
      {
         f.wait();
      }
#else
      std::transform(vUserDC1.cbegin(),vUserDC1.cend(), pMinMoments->vDC.cbegin(), pMinMoments->vDC.begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(vUserDW1.cbegin(),vUserDW1.cend(), pMinMoments->vDW.cbegin(), pMinMoments->vDW.begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(vUserDC2.cbegin(),vUserDC2.cend(), pMinMoments->vDC.cbegin(), pMinMoments->vDC.begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(vUserDW2.cbegin(),vUserDW2.cend(), pMinMoments->vDW.cbegin(), pMinMoments->vDW.begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(vTrafficBarrier.cbegin(),vTrafficBarrier.cend(), pMinMoments->vDC.cbegin(), pMinMoments->vDC.begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(vSidewalk.cbegin(),vSidewalk.cend(), pMinMoments->vDC.cbegin(), pMinMoments->vDC.begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(vOverlay.cbegin(),vOverlay.cend(), pMinMoments->vDW.cbegin(), pMinMoments->vDW.begin(),[](const auto& a, const auto& b) {return a + b;});

      if ( bTimeStep )
      {
         std::transform(vCreep.cbegin(),vCreep.cend(), pMinMoments->vCR.cbegin(), pMinMoments->vCR.begin(),[](const auto& a, const auto& b) {return a + b;});
         std::transform(vShrinkage.cbegin(),vShrinkage.cend(), pMinMoments->vSH.cbegin(), pMinMoments->vSH.begin(),[](const auto& a, const auto& b) {return a + b;});
         std::transform(vRelaxation.cbegin(),vRelaxation.cend(), pMinMoments->vRE.cbegin(), pMinMoments->vRE.begin(),[](const auto& a, const auto& b) {return a + b;});
         std::transform(vSecondary.cbegin(),vSecondary.cend(), pMinMoments->vPS.cbegin(), pMinMoments->vPS.begin(),[](const auto& a, const auto& b) {return a + b;});
      }
#endif
   }

   ATLASSERT(vPoi.size() == pMaxMoments->vDC.size());
   ATLASSERT(vPoi.size() == pMaxMoments->vDW.size());
   ATLASSERT(bTimeStep ? vPoi.size() == pMaxMoments->vCR.size() : true);
   ATLASSERT(bTimeStep ? vPoi.size() == pMaxMoments->vSH.size() : true);
   ATLASSERT(bTimeStep ? vPoi.size() == pMaxMoments->vRE.size() : true);
   ATLASSERT(bTimeStep ? vPoi.size() == pMaxMoments->vPS.size() : true);
   ATLASSERT(vPoi.size() == pMaxMoments->vLLIM.size());
#if defined _DEBUG
   if (pMinMoments)
   {
      ATLASSERT(vPoi.size() == pMinMoments->vDC.size());
      ATLASSERT(vPoi.size() == pMinMoments->vDW.size());
      ATLASSERT(bTimeStep ? vPoi.size() == pMinMoments->vCR.size() : true);
      ATLASSERT(bTimeStep ? vPoi.size() == pMinMoments->vSH.size() : true);
      ATLASSERT(bTimeStep ? vPoi.size() == pMinMoments->vRE.size() : true);
      ATLASSERT(bTimeStep ? vPoi.size() == pMinMoments->vPS.size() : true);
      ATLASSERT(vPoi.size() == pMinMoments->vLLIM.size());
   }
#endif
}

void special_transform(std::shared_ptr<IBridge> pBridge,std::shared_ptr<IPointOfInterest> pPoi,std::shared_ptr<IIntervals> pIntervals,
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

      IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
      ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);

      IntervalIndexType compositeDeckIntervalIdx       = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);
      IntervalIndexType startPierContinuityIntervalIdx = start;
      IntervalIndexType endPierContinuityIntervalIdx   = end;

      if ( startPierContinuityIntervalIdx == compositeDeckIntervalIdx && 
           endPierContinuityIntervalIdx   == compositeDeckIntervalIdx )
      {
         *outputIter = (*forceIter + *resultIter);
      }
   }
}

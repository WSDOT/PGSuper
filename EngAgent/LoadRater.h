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

#pragma once

#include <PgsExt\RatingArtifact.h>

#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\DistributionFactors.h>
#include <IFace\RatingSpecification.h>
#include <IFace/Limits.h>
#include <IFace/PointOfInterest.h>

class pgsLoadRater
{
public:
   pgsLoadRater(std::weak_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID);
   ~pgsLoadRater();

   pgsRatingArtifact Rate(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const;

protected:
   std::weak_ptr<WBFL::EAF::Broker> m_pBroker; // weak reference
   inline std::shared_ptr<WBFL::EAF::Broker> GetBroker() const { return m_pBroker.lock(); }

   StatusGroupIDType m_StatusGroupID;

   struct Moments
   {
      std::vector<Float64> vDC;
      std::vector<Float64> vDW;
      std::vector<Float64> vCR;
      std::vector<Float64> vSH;
      std::vector<Float64> vRE;
      std::vector<Float64> vPS;
      std::vector<Float64> vLLIM;
      std::vector<Float64> vPL;
      std::vector<VehicleIndexType> vTruckIndex;
   };

   // moments at a section
   struct MomentsAtPoi
   {
      Float64 DC, DW, CR, SH, RE, PS, LLIM, PL;

      std::_tstring strVehicleName;
      VehicleIndexType truck_index;
      Float64 VehicleWeight;
   };

   // parameters common to moment and reinforcement yielding ratings
   struct RatingParams
   {
      IntervalIndexType loadRatingIntervalIdx;
      Float64 system_factor;
      bool bIncludePL;
      bool bTimeStep;
      pgsTypes::LoadRatingType ratingType;

      pgsTypes::LimitState limitState;
      Float64 gDC, gDW, gCR, gSH, gRE, gPS, gLL;

      pgsTypes::LiveLoadType llType;
   };

   // parameters for moment capacity ratings
   struct MomentRatingParams : public RatingParams
   {
      const MOMENTCAPACITYDETAILS* pMnDetails;
      const MINMOMENTCAPDETAILS* pMminDetails;

      pgsTypes::SupportedDeckType deckType;

      // interfaces used at each POI.
      // NOTE: Storing agent interface pointers is generally a bad idea as it causes circular references.
      // For this reason we store weak_ptrs. However, this leads to an ugly implementation because you
      // must call the lock() method on the weak_ptr to use the pointer. Bad ideas lead to ugly implementation.
      std::weak_ptr<IRatingSpecification> pRatingSpec;
      std::weak_ptr<IProductForces> pProductForces;
   };

   // parameters for reinforcement yielding check
   struct YieldingRatingParams : public RatingParams
   {
      const CRACKINGMOMENTDETAILS* pMcrDetails = nullptr;
      const CRACKEDSECTIONDETAILS* pCrackedSectionDetails = nullptr;

      Float64 YieldStressLimitCoefficient;

      Float64 K_liveload; // live load elastic gain factor

      pgsTypes::AnalysisType analysisType;

      // interfaces used at each POI
      std::weak_ptr<ISectionProperties> pSectProp;
      std::weak_ptr<IPointOfInterest> pPoi;
      std::weak_ptr<IBridge> pBridge;
      std::weak_ptr<IIntervals> pIntervals;
      std::weak_ptr<ILongRebarGeometry> pRebarGeom;
      std::weak_ptr<IMaterials> pMaterials;
      std::weak_ptr<ILiveLoadDistributionFactors> pLLDF;
      std::weak_ptr<IStrandGeometry> pStrandGeometry;
      std::weak_ptr<ISegmentTendonGeometry> pSegmentTendonGeometry;
      std::weak_ptr<IGirderTendonGeometry> pGirderTendonGeometry;
   };

   // parameters for flexural stress ratings
   struct StressRatingParams
   {
      pgsTypes::LoadRatingType ratingType;
      Float64 system_factor;
      bool bIncludePL;
      IntervalIndexType loadRatingIntervalIdx;
      bool bTimeStep;
      pgsTypes::BridgeAnalysisType bat;

      pgsTypes::LimitState limitState;
      Float64 gDC, gDW, gCR, gSH, gRE, gPS, gLL;

      pgsTypes::LiveLoadType llType;
      VehicleIndexType vehicleIdx;
      std::vector<std::_tstring> strLLNames;

      // interfaces used at each POI
      std::weak_ptr<IPrecompressedTensileZone> pPTZ;
      std::weak_ptr<ICombinedForces> pCombinedForces;
      std::weak_ptr<IPretensionStresses> pPrestress;
      std::weak_ptr<IRatingSpecification> pRatingSpec;
      std::weak_ptr<IConcreteStressLimits> pLimits;
      std::weak_ptr<IProductForces> pProductForces;
      std::weak_ptr<IProductLoads> pProductLoads;
   };

   void FlexureRating(const CGirderKey& girderKey, const PoiList& vPoi, pgsTypes::LoadRatingType ratingType, VehicleIndexType vehicleIdx, IntervalIndexType loadRatingIntervalIdx, bool bTimeStep,const Moments* pMaxMoments, const Moments* pMinMoments, pgsRatingArtifact& ratingArtifact) const;
   void ShearRating(const CGirderKey& girderKey,const PoiList& vPoi,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx, IntervalIndexType loadRatingIntervalIdx, bool bTimeStep,pgsRatingArtifact& ratingArtifact) const;
   void LongitudinalReinforcementForShearRating(const CGirderKey& girderKey, const PoiList& vPoi, pgsTypes::LoadRatingType ratingType, VehicleIndexType vehicleIdx, IntervalIndexType loadRatingIntervalIdx, bool bTimeStep, pgsRatingArtifact& ratingArtifact) const;

   void MomentRating(const pgsPointOfInterest& poi, bool bPositiveMoment, const MomentsAtPoi& moments,const MomentRatingParams& ratingParams, pgsRatingArtifact& ratingArtifact) const;
   void StressRating(const pgsPointOfInterest& poi, const StressRatingParams& ratingParams, pgsRatingArtifact& ratingArtifact) const;
   void CheckReinforcementYielding(const pgsPointOfInterest& poi, bool bPositiveMoment, const MomentsAtPoi& moments, const YieldingRatingParams& ratingParams, pgsRatingArtifact& ratingArtifact) const;

   void GetMoments(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx, const PoiList& vPoi, bool bTimeStep, Moments* pMaxMoments, Moments* pMinMoments) const;

   void GetCriticalSectionZones(const CGirderKey& girderKey, pgsTypes::LimitState limitState, std::vector<CRITSECTDETAILS>* pvCriticalSections) const;
   ZoneIndexType GetCriticalSectionZone(const pgsPointOfInterest& poi, const std::vector<CRITSECTDETAILS>& criticalSections, bool bIncludeCS = false) const;

   DECLARE_LOGFILE;
};

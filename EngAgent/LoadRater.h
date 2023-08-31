///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

class pgsLoadRater
{
public:
   pgsLoadRater(void);
   virtual ~pgsLoadRater(void);

   void SetBroker(IBroker* pBroker);
   pgsRatingArtifact Rate(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const;

protected:
   IBroker* m_pBroker; // weak reference

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

      // interfaces used at each POI
      CComPtr<IRatingSpecification> pRatingSpec;
      CComPtr<IProductForces> pProductForces;
   };

   // parameters for reinforcement yielding check
   struct YieldingRatingParams : public RatingParams
   {
      const CRACKINGMOMENTDETAILS* pMcrDetails;
      const CRACKEDSECTIONDETAILS* pCrackedSectionDetails;

      Float64 YieldStressLimitCoefficient;

      Float64 K_liveload; // live load elastic gain factor

      pgsTypes::AnalysisType analysisType;

      // interfaces used at each POI
      CComPtr<ISectionProperties> pSectProp;
      CComPtr<IPointOfInterest> pPoi;
      CComPtr<IBridge> pBridge;
      CComPtr<IIntervals> pIntervals;
      CComPtr<ILongRebarGeometry> pRebarGeom;
      CComPtr<IMaterials> pMaterials;
      CComPtr<ILiveLoadDistributionFactors> pLLDF;
      CComPtr<IStrandGeometry> pStrandGeometry;
      CComPtr<ISegmentTendonGeometry> pSegmentTendonGeometry;
      CComPtr<IGirderTendonGeometry> pGirderTendonGeometry;
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
      CComPtr<IPrecompressedTensileZone> pPTZ;
      CComPtr<ICombinedForces> pCombinedForces;
      CComPtr<IPretensionStresses> pPrestress;
      CComPtr<IRatingSpecification> pRatingSpec;
      CComPtr<IConcreteStressLimits> pLimits;
      CComPtr<IProductForces> pProductForces;
      CComPtr<IProductLoads> pProductLoads;
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

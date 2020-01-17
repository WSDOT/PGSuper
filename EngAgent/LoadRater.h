///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

class pgsLoadRater
{
public:
   pgsLoadRater(void);
   virtual ~pgsLoadRater(void);

   void SetBroker(IBroker* pBroker);
   pgsRatingArtifact Rate(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const;

protected:
   IBroker* m_pBroker; // weak reference
   mutable std::vector<CRITSECTDETAILS> m_CriticalSections;

   struct Moments
   {
      std::vector<Float64> vDCmin, vDCmax;
      std::vector<Float64> vDWmin, vDWmax;
      std::vector<Float64> vCRmin, vCRmax;
      std::vector<Float64> vSHmin, vSHmax;
      std::vector<Float64> vREmin, vREmax;
      std::vector<Float64> vPSmin, vPSmax;
      std::vector<Float64> vLLIMmin, vLLIMmax;
      std::vector<Float64> vPLmin, vPLmax;
      std::vector<VehicleIndexType> vMinTruckIndex, vMaxTruckIndex;
   };

   void MomentRating(const CGirderKey& girderKey,const PoiList& vPoi,bool bPositiveMoment,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,const Moments& moments,pgsRatingArtifact& ratingArtifact) const;
   void ShearRating(const CGirderKey& girderKey,const PoiList& vPoi,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,pgsRatingArtifact& ratingArtifact) const;
   void StressRating(const CGirderKey& girderKey,const PoiList& vPoi,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,pgsRatingArtifact& ratingArtifact) const;
   void CheckReinforcementYielding(const CGirderKey& girderKey,const PoiList& vPoi, bool bPositiveMoment, pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,const Moments& moments,pgsRatingArtifact& ratingArtifact) const;

   void GetMoments(const CGirderKey& girderKey,bool bPositiveMoment,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx, const PoiList& vPoi, Moments* pMoments) const;

   void InitCriticalSectionZones(const CGirderKey& girderKey, pgsTypes::LimitState limitState) const;
   ZoneIndexType GetCriticalSectionZone(const pgsPointOfInterest& poi, bool bIncludeCS = false) const;

   DECLARE_LOGFILE;
};

///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
   pgsRatingArtifact Rate(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx);

protected:
   IBroker* m_pBroker;

   void MomentRating(const CGirderKey& girderKey,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,pgsRatingArtifact& ratingArtifact);
   void ShearRating(const CGirderKey& girderKey,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,pgsRatingArtifact& ratingArtifact);
   void StressRating(const CGirderKey& girderKey,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,pgsRatingArtifact& ratingArtifact);
   void CheckReinforcementYielding(const CGirderKey& girderKey,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx,bool bPositiveMoment,pgsRatingArtifact& ratingArtifact);

   void GetMoments(const CGirderKey& girderKey,bool bPositiveMoment,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx, const std::vector<pgsPointOfInterest>& vPOI, std::vector<Float64>& vDCmin, std::vector<Float64>& vDCmax,std::vector<Float64>& vDWmin, std::vector<Float64>& vDWmax,std::vector<Float64>& vCRmin, std::vector<Float64>& vCRmax,std::vector<Float64>& vSHmin, std::vector<Float64>& vSHmax,std::vector<Float64>& vREmin, std::vector<Float64>& vREmax,std::vector<Float64>& vPSmin, std::vector<Float64>& vPSmax, std::vector<Float64>& vLLIMmin, std::vector<VehicleIndexType>& vMinTruckIndex,std::vector<Float64>& vLLIMmax,std::vector<VehicleIndexType>& vMaxTruckIndex,std::vector<Float64>& vPLmin,std::vector<Float64>& vPLmax);

   DECLARE_LOGFILE;
};

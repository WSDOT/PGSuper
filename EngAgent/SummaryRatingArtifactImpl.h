///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include <PgsExt\ISummaryRatingArtifact.h>

class CEngAgentImp;

/*****************************************************************************
CLASS 
   pgsSummaryRatingArtifactImpl

   Encapsulates computation and data for rating summary information


DESCRIPTION

LOG
   rdp : 4.23.2020 : Created file 
*****************************************************************************/

class pgsSummaryRatingArtifactImpl: public pgsISummaryRatingArtifact
{
public:
   pgsSummaryRatingArtifactImpl(const std::vector<CGirderKey>& girderKeys,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx, const CEngAgentImp* pEngAgentImp);

   ~pgsSummaryRatingArtifactImpl() = default;

   Float64 GetMomentRatingFactor(bool bPositiveMoment) const override;
   Float64 GetMomentRatingFactorEx(bool bPositiveMoment,const pgsMomentRatingArtifact** ppArtifact) const override;

   Float64 GetShearRatingFactor() const override;
   Float64 GetShearRatingFactorEx(const pgsShearRatingArtifact** ppArtifact) const override;

   Float64 GetStressRatingFactor() const override;
   Float64 GetStressRatingFactorEx(const pgsStressRatingArtifact** ppArtifact) const override;

   Float64 GetYieldStressRatio(bool bPositiveMoment) const override;
   Float64 GetYieldStressRatioEx(bool bPositiveMoment,const pgsYieldStressRatioArtifact** ppArtifact) const override;

   Float64 GetRatingFactor() const override;
   Float64 GetRatingFactorEx(const pgsMomentRatingArtifact** ppPositiveMoment,const pgsMomentRatingArtifact** ppNegativeMoment,
                     const pgsShearRatingArtifact** ppShear,
                     const pgsStressRatingArtifact** ppStress) const override;
   Float64 GetYieldStressRatio(const pgsYieldStressRatioArtifact** ppYieldStressPositiveMoment, const pgsYieldStressRatioArtifact** ppYieldStressNegativeMoment) const override;

   bool IsLoadPostingRequired() const override;
   void GetSafePostingLoad(Float64* pPostingLoad,Float64* pWeight,Float64* pRF,std::_tstring* pVehicle) const override;

private:
   pgsSummaryRatingArtifactImpl();

   const CEngAgentImp* m_pEngAgentImp;
   std::vector<CGirderKey> m_GirderKeys;
   pgsTypes::LoadRatingType m_RatingType;
   VehicleIndexType m_VehicleIdx;
};

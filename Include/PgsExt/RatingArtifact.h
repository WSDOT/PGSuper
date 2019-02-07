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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\MomentRatingArtifact.h>
#include <PgsExt\ShearRatingArtifact.h>
#include <PgsExt\StressRatingArtifact.h>
#include <PgsExt\YieldStressRatioArtifact.h>

/*****************************************************************************
CLASS 
   pgsRatingArtifact

   Artifact for load rating analysis


DESCRIPTION
   Artifact for load rating analysis. Holds rating artificts for
   design, legal, and permit load ratings.

LOG
   rab : 12.07.2009 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsRatingArtifact
{
public:
   typedef std::vector<std::pair<pgsPointOfInterest,pgsMomentRatingArtifact>> MomentRatings;
   typedef std::vector<std::pair<pgsPointOfInterest,pgsShearRatingArtifact>>  ShearRatings;
   typedef std::vector<std::pair<pgsPointOfInterest,pgsStressRatingArtifact>> StressRatings;
   typedef std::vector<std::pair<pgsPointOfInterest,pgsYieldStressRatioArtifact>> YieldStressRatios;

   pgsRatingArtifact(pgsTypes::LoadRatingType ratingType);
   pgsRatingArtifact(const pgsRatingArtifact& rOther);
   virtual ~pgsRatingArtifact();

   pgsRatingArtifact& operator = (const pgsRatingArtifact& rOther);

   pgsTypes::LoadRatingType GetLoadRatingType() const;

   void AddArtifact(const pgsPointOfInterest& poi,const pgsMomentRatingArtifact& artifact,bool bPositiveMoment);
   void AddArtifact(const pgsPointOfInterest& poi,const pgsShearRatingArtifact& artifact);
   void AddArtifact(const pgsPointOfInterest& poi,const pgsStressRatingArtifact& artifact);
   void AddArtifact(const pgsPointOfInterest& poi,const pgsYieldStressRatioArtifact& artifact,bool bPositiveMoment);

   const MomentRatings& GetMomentRatings(bool bPositiveMoment) const;
   const ShearRatings& GetShearRatings() const;
   const StressRatings& GetStressRatings() const;
   const YieldStressRatios& GetYieldStressRatios(bool bPositiveMoment) const;

   Float64 GetMomentRatingFactor(bool bPositiveMoment) const;
   Float64 GetMomentRatingFactorEx(bool bPositiveMoment,const pgsMomentRatingArtifact** ppArtifact) const;

   Float64 GetShearRatingFactor() const;
   Float64 GetShearRatingFactorEx(const pgsShearRatingArtifact** ppArtifact) const;

   Float64 GetStressRatingFactor() const;
   Float64 GetStressRatingFactorEx(const pgsStressRatingArtifact** ppArtifact) const;

   Float64 GetYieldStressRatio(bool bPositiveMoment) const;
   Float64 GetYieldStressRatioEx(bool bPositiveMoment,const pgsYieldStressRatioArtifact** ppArtifact) const;

   Float64 GetRatingFactor() const;
   Float64 GetRatingFactorEx(const pgsMomentRatingArtifact** ppPositiveMoment,const pgsMomentRatingArtifact** ppNegativeMoment,
                             const pgsShearRatingArtifact** ppShear,
                             const pgsStressRatingArtifact** ppStress,
                             const pgsYieldStressRatioArtifact** ppYieldStressPositiveMoment,const pgsYieldStressRatioArtifact** ppYieldStressNegativeMoment) const;

   bool IsLoadPostingRequired() const;
   void GetSafePostingLoad(Float64* pPostingLoad,Float64* pWeight,Float64* pRF,std::_tstring* pVehicle) const;

protected:
   void MakeCopy(const pgsRatingArtifact& rOther);
   void MakeAssignment(const pgsRatingArtifact& rOther);

   pgsTypes::LoadRatingType m_RatingType;
   MomentRatings m_PositiveMomentRatings;
   MomentRatings m_NegativeMomentRatings;
   ShearRatings  m_ShearRatings;
   StressRatings m_StressRatings;
   YieldStressRatios m_PositiveMomentYieldStressRatios;
   YieldStressRatios m_NegativeMomentYieldStressRatios;

   mutable bool m_bPositiveMomentRatingCached;
   mutable Float64 m_RF_PositiveMoment;
   mutable const pgsMomentRatingArtifact* m_pControllingPositiveMoment;

   mutable bool m_bNegativeMomentRatingCached;
   mutable Float64 m_RF_NegativeMoment;
   mutable const pgsMomentRatingArtifact* m_pControllingNegativeMoment;

   mutable bool m_bShearRatingCached;
   mutable Float64 m_RF_Shear;
   mutable const pgsShearRatingArtifact* m_pControllingShear;

   mutable bool m_bStressRatingCached;
   mutable Float64 m_RF_Stress;
   mutable const pgsStressRatingArtifact* m_pControllingStress;

   mutable bool m_bPositiveYieldStressRatingCached;
   mutable Float64 m_RF_PositiveMomentYieldStress;
   mutable const pgsYieldStressRatioArtifact* m_pControllingPositiveMomentYieldStress;

   mutable bool m_bNegativeYieldStressRatingCached;
   mutable Float64 m_RF_NegativeMomentYieldStress;
   mutable const pgsYieldStressRatioArtifact* m_pControllingNegativeMomentYieldStress;
};

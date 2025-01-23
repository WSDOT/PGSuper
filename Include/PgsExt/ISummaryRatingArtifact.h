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

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\MomentRatingArtifact.h>
#include <PgsExt\ShearRatingArtifact.h>
#include <PgsExt\StressRatingArtifact.h>
#include <PgsExt\YieldStressRatioArtifact.h>

/*****************************************************************************
CLASS 
   pgsISummaryRatingArtifact

   Artifact for load rating analysis summary


DESCRIPTION
   Artifact interface for load rating analysis summary. Holds rating artificts for
   design, legal, and permit load ratings.

LOG
   rdp : 4.20.2020 : Created file
*****************************************************************************/

class pgsISummaryRatingArtifact
{
public:
   virtual Float64 GetMomentRatingFactor(bool bPositiveMoment) const = 0;
   virtual Float64 GetMomentRatingFactorEx(bool bPositiveMoment,const pgsMomentRatingArtifact** ppArtifact) const = 0;

   virtual Float64 GetShearRatingFactor() const = 0;
   virtual Float64 GetShearRatingFactorEx(const pgsShearRatingArtifact** ppArtifact) const = 0;

   virtual Float64 GetStressRatingFactor() const = 0;
   virtual Float64 GetStressRatingFactorEx(const pgsStressRatingArtifact** ppArtifact) const = 0;

   virtual Float64 GetYieldStressRatio(bool bPositiveMoment) const = 0;
   virtual Float64 GetYieldStressRatioEx(bool bPositiveMoment,const pgsYieldStressRatioArtifact** ppArtifact) const = 0;

   virtual Float64 GetRatingFactor() const = 0;
   virtual Float64 GetRatingFactorEx(const pgsMomentRatingArtifact** ppPositiveMoment,const pgsMomentRatingArtifact** ppNegativeMoment,
                             const pgsShearRatingArtifact** ppShear,
                             const pgsStressRatingArtifact** ppStress) const = 0;
   virtual Float64 GetYieldStressRatio(const pgsYieldStressRatioArtifact** ppYieldStressPositiveMoment, const pgsYieldStressRatioArtifact** ppYieldStressNegativeMoment) const = 0;

   virtual bool IsLoadPostingRequired() const = 0;
   virtual void GetSafePostingLoad(Float64* pPostingLoad,Float64* pWeight,Float64* pRF,std::_tstring* pVehicle) const = 0;
};

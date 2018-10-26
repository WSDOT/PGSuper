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

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\ReportPointOfInterest.h>

/*****************************************************************************
CLASS 
   pgsStressRatingArtifact

   Artifact for flexural stress load rating analysis


DESCRIPTION
   Artifact for load rating analysis. Holds rating artificts for
   design, legal, and permit load ratings.

LOG
   rab : 12.07.2009 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsStressRatingArtifact
{
public:
   pgsStressRatingArtifact();
   pgsStressRatingArtifact(const pgsStressRatingArtifact& rOther);
   virtual ~pgsStressRatingArtifact();

   pgsStressRatingArtifact& operator = (const pgsStressRatingArtifact& rOther);

   void SetPointOfInterest(const pgsPointOfInterest& poi);
   const pgsPointOfInterest& GetPointOfInterest() const;

   void SetStressLocation(pgsTypes::StressLocation stressLocation);
   pgsTypes::StressLocation GetStressLocation() const;

   void SetRatingType(pgsTypes::LoadRatingType ratingType);
   pgsTypes::LoadRatingType GetLoadRatingType() const;

   void SetVehicleIndex(VehicleIndexType vehicleIdx);
   VehicleIndexType GetVehicleIndex() const;

   void SetVehicleWeight(Float64 W);
   Float64 GetVehicleWeight() const;

   void SetVehicleName(LPCTSTR str);
   std::_tstring GetVehicleName() const;

   Float64 GetResistance() const;

   void SetAllowableStress(Float64 fr);
   Float64 GetAllowableStress() const;

   void SetDeadLoadFactor(Float64 gDC);
   Float64 GetDeadLoadFactor() const;

   void SetDeadLoadStress(Float64 fDC);
   Float64 GetDeadLoadStress() const;

   void SetPrestressStress(Float64 fps);
   Float64 GetPrestressStress() const;

   void SetPostTensionStress(Float64 fpt);
   Float64 GetPostTensionStress() const;

   void SetWearingSurfaceFactor(Float64 gDW);
   Float64 GetWearingSurfaceFactor() const;

   void SetWearingSurfaceStress(Float64 fDW);
   Float64 GetWearingSurfaceStress() const;

   void SetCreepFactor(Float64 gCR);
   Float64 GetCreepFactor() const;

   void SetCreepStress(Float64 fCR);
   Float64 GetCreepStress() const;

   void SetShrinkageFactor(Float64 gSH);
   Float64 GetShrinkageFactor() const;

   void SetShrinkageStress(Float64 fSH);
   Float64 GetShrinkageStress() const;

   void SetRelaxationFactor(Float64 gRE);
   Float64 GetRelaxationFactor() const;

   void SetRelaxationStress(Float64 fRE);
   Float64 GetRelaxationStress() const;

   void SetSecondaryEffectsFactor(Float64 gPS);
   Float64 GetSecondaryEffectsFactor() const;

   void SetSecondaryEffectsStress(Float64 fPS);
   Float64 GetSecondaryEffectsStress() const;

   void SetLiveLoadFactor(Float64 gLL);
   Float64 GetLiveLoadFactor() const;

   void SetLiveLoadStress(Float64 fllim);
   Float64 GetLiveLoadStress() const;

   Float64 GetRatingFactor() const;

protected:
   void MakeCopy(const pgsStressRatingArtifact& rOther);
   virtual void MakeAssignment(const pgsStressRatingArtifact& rOther);

   mutable bool m_bRFComputed;
   mutable Float64 m_RF;

   pgsPointOfInterest m_POI;

   pgsTypes::StressLocation m_StressLocation;

   pgsTypes::LoadRatingType m_RatingType;

   VehicleIndexType m_VehicleIndex;
   Float64 m_VehicleWeight;
   std::_tstring m_strVehicleName;

   Float64 m_fr;
   Float64 m_gDC;
   Float64 m_gDW;
   Float64 m_gCR;
   Float64 m_gSH;
   Float64 m_gRE;
   Float64 m_gPS; // secondary effects
   Float64 m_gLL;
   Float64 m_fDC;
   Float64 m_fDW;
   Float64 m_fCR;
   Float64 m_fSH;
   Float64 m_fRE;
   Float64 m_fPS; // secondary effects
   Float64 m_fLLIM;
   Float64 m_fps; // direct prestress
   Float64 m_fpt; // direct post-tensoin
};

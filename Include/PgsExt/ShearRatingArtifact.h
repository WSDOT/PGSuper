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

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\StirrupCheckAtPoisArtifact.h>

/*****************************************************************************
CLASS 
   pgsShearRatingArtifact

   Artifact for shear load rating analysis


DESCRIPTION
   Artifact for load rating analysis. Holds rating artificts for
   design, legal, and permit load ratings.

LOG
   rab : 12.07.2009 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsShearRatingArtifact
{
public:
   pgsShearRatingArtifact();
   pgsShearRatingArtifact(const pgsShearRatingArtifact& rOther);
   virtual ~pgsShearRatingArtifact();

   pgsShearRatingArtifact& operator = (const pgsShearRatingArtifact& rOther);

   void SetPointOfInterest(const pgsPointOfInterest& poi);
   const pgsPointOfInterest& GetPointOfInterest() const;

   void SetRatingType(pgsTypes::LoadRatingType ratingType);
   pgsTypes::LoadRatingType GetLoadRatingType() const;

   void SetVehicleIndex(VehicleIndexType vehicleIdx);
   VehicleIndexType GetVehicleIndex() const;

   void SetVehicleWeight(Float64 W);
   Float64 GetVehicleWeight() const;

   void SetVehicleName(LPCTSTR str);
   const std::_tstring& GetVehicleName() const;

   void SetSystemFactor(Float64 systemFactor);
   Float64 GetSystemFactor() const;

   void SetConditionFactor(Float64 conditionFactor);
   Float64 GetConditionFactor() const;

   void SetCapacityReductionFactor(Float64 phi);
   Float64 GetCapacityReductionFactor() const;

   void SetNominalShearCapacity(Float64 Vn);
   Float64 GetNominalShearCapacity() const;

   void SetDeadLoadFactor(Float64 gDC);
   Float64 GetDeadLoadFactor() const;

   void SetDeadLoadShear(Float64 Vdc);
   Float64 GetDeadLoadShear() const;

   void SetWearingSurfaceFactor(Float64 gDW);
   Float64 GetWearingSurfaceFactor() const;

   void SetWearingSurfaceShear(Float64 Vdw);
   Float64 GetWearingSurfaceShear() const;

   void SetCreepFactor(Float64 gCR);
   Float64 GetCreepFactor() const;

   void SetCreepShear(Float64 Mcr);
   Float64 GetCreepShear() const;

   void SetShrinkageFactor(Float64 gSH);
   Float64 GetShrinkageFactor() const;

   void SetShrinkageShear(Float64 Msh);
   Float64 GetShrinkageShear() const;

   void SetRelaxationFactor(Float64 gRE);
   Float64 GetRelaxationFactor() const;

   void SetRelaxationShear(Float64 Mre);
   Float64 GetRelaxationShear() const;

   void SetSecondaryEffectsFactor(Float64 gPS);
   Float64 GetSecondaryEffectsFactor() const;

   void SetSecondaryEffectsShear(Float64 Mps);
   Float64 GetSecondaryEffectsShear() const;

   void SetLiveLoadFactor(Float64 gLL);
   Float64 GetLiveLoadFactor() const;

   void SetLiveLoadShear(Float64 Vllim);
   Float64 GetLiveLoadShear() const;

   void SetLongReinfShearArtifact(const pgsLongReinfShearArtifact& artifact);
   const pgsLongReinfShearArtifact& GetLongReinfShearArtifact() const;

   Float64 GetRatingFactor() const;

protected:
   void MakeCopy(const pgsShearRatingArtifact& rOther);
   void MakeAssignment(const pgsShearRatingArtifact& rOther);

   mutable bool m_bRFComputed;
   mutable Float64 m_RF;

   pgsPointOfInterest m_POI;

   pgsTypes::LoadRatingType m_RatingType;

   VehicleIndexType m_VehicleIndex;
   Float64 m_VehicleWeight;
   std::_tstring m_strVehicleName;

   Float64 m_SystemFactor;
   Float64 m_ConditionFactor;
   Float64 m_CapacityRedutionFactor;
   Float64 m_Vn;
   Float64 m_gDC;
   Float64 m_gDW;
   Float64 m_gCR;
   Float64 m_gSH;
   Float64 m_gRE;
   Float64 m_gPS; // secondary effects
   Float64 m_gLL;
   Float64 m_Vdc;
   Float64 m_Vdw;
   Float64 m_Vcr;
   Float64 m_Vsh;
   Float64 m_Vre;
   Float64 m_Vps;
   Float64 m_Vllim;
   pgsLongReinfShearArtifact m_LongReinfShearArtifact;
};

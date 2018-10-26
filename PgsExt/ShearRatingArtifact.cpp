///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\ShearRatingArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsShearRatingArtifact
****************************************************************************/
pgsShearRatingArtifact::pgsShearRatingArtifact() :
m_strVehicleName(_T("Unknown"))
{
   m_bRFComputed = false;
   m_RF = 0;

   m_RatingType = pgsTypes::lrDesign_Inventory;

   m_VehicleIndex = INVALID_INDEX;
   m_VehicleWeight = -999999;

   m_SystemFactor = 1.0;
   m_ConditionFactor = 1.0;
   m_CapacityRedutionFactor = 0.9;
   m_Vn = 0;
   m_gDC = 1;
   m_gDW = 1;
   m_gCR = 1;
   m_gSH = 1;
   m_gRE = 1;
   m_gPS = 1;
   m_gLL = 1;
   m_Vdc = 0;
   m_Vdw = 0;
   m_Vcr = 0;
   m_Vsh = 0;
   m_Vre = 0;
   m_Vps = 0;
   m_Vllim = 0;
}

pgsShearRatingArtifact::pgsShearRatingArtifact(const pgsShearRatingArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsShearRatingArtifact::~pgsShearRatingArtifact()
{
}

pgsShearRatingArtifact& pgsShearRatingArtifact::operator=(const pgsShearRatingArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsShearRatingArtifact::SetPointOfInterest(const pgsPointOfInterest& poi)
{
   m_POI = poi;
}

const pgsPointOfInterest& pgsShearRatingArtifact::GetPointOfInterest() const
{
   return m_POI;
}

void pgsShearRatingArtifact::SetRatingType(pgsTypes::LoadRatingType ratingType)
{
   m_RatingType = ratingType;
}

pgsTypes::LoadRatingType pgsShearRatingArtifact::GetLoadRatingType() const
{
   return m_RatingType;
}

void pgsShearRatingArtifact::SetVehicleIndex(VehicleIndexType vehicleIdx)
{
   m_VehicleIndex = vehicleIdx;
}

VehicleIndexType pgsShearRatingArtifact::GetVehicleIndex() const
{
   return m_VehicleIndex;
}

void pgsShearRatingArtifact::SetVehicleWeight(Float64 W)
{
   m_VehicleWeight = W;
}

Float64 pgsShearRatingArtifact::GetVehicleWeight() const
{
   return m_VehicleWeight;
}

void pgsShearRatingArtifact::SetVehicleName(LPCTSTR str)
{
   m_strVehicleName = str;
}

std::_tstring pgsShearRatingArtifact::GetVehicleName() const
{
   return m_strVehicleName;
}

void pgsShearRatingArtifact::SetSystemFactor(Float64 systemFactor)
{
   m_SystemFactor = systemFactor;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetSystemFactor() const
{
   return m_SystemFactor;
}

void pgsShearRatingArtifact::SetConditionFactor(Float64 conditionFactor)
{
   m_ConditionFactor = conditionFactor;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetConditionFactor() const
{
   return m_ConditionFactor;
}

void pgsShearRatingArtifact::SetCapacityReductionFactor(Float64 phi)
{
   m_CapacityRedutionFactor = phi;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetCapacityReductionFactor() const
{
   return m_CapacityRedutionFactor;
}

void pgsShearRatingArtifact::SetNominalShearCapacity(Float64 Vn)
{
   m_Vn = Vn;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetNominalShearCapacity() const
{
   return m_Vn;
}

void pgsShearRatingArtifact::SetDeadLoadFactor(Float64 gDC)
{
   m_gDC = gDC;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetDeadLoadFactor() const
{
   return m_gDC;
}

void pgsShearRatingArtifact::SetDeadLoadShear(Float64 Vdc)
{
   m_Vdc = Vdc;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetDeadLoadShear() const
{
   return m_Vdc;
}

void pgsShearRatingArtifact::SetWearingSurfaceFactor(Float64 gDW)
{
   m_gDW = gDW;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetWearingSurfaceFactor() const
{
   return m_gDW;
}

void pgsShearRatingArtifact::SetWearingSurfaceShear(Float64 Vdw)
{
   m_Vdw = Vdw;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetWearingSurfaceShear() const
{
   return m_Vdw;
}

void pgsShearRatingArtifact::SetCreepFactor(Float64 gCR)
{
   m_gCR = gCR;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetCreepFactor() const
{
   return m_gCR;
}

void pgsShearRatingArtifact::SetCreepShear(Float64 Vcr)
{
   m_Vcr = Vcr;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetCreepShear() const
{
   return m_Vcr;
}

void pgsShearRatingArtifact::SetShrinkageFactor(Float64 gSH)
{
   m_gSH = gSH;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetShrinkageFactor() const
{
   return m_gSH;
}

void pgsShearRatingArtifact::SetShrinkageShear(Float64 Vsh)
{
   m_Vsh = Vsh;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetShrinkageShear() const
{
   return m_Vsh;
}

void pgsShearRatingArtifact::SetRelaxationFactor(Float64 gRE)
{
   m_gRE = gRE;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetRelaxationFactor() const
{
   return m_gRE;
}

void pgsShearRatingArtifact::SetRelaxationShear(Float64 Vre)
{
   m_Vre = Vre;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetRelaxationShear() const
{
   return m_Vre;
}

void pgsShearRatingArtifact::SetSecondaryEffectsFactor(Float64 gPS)
{
   m_gPS = gPS;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetSecondaryEffectsFactor() const
{
   return m_gPS;
}

void pgsShearRatingArtifact::SetSecondaryEffectsShear(Float64 Vps)
{
   m_Vps = Vps;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetSecondaryEffectsShear() const
{
   return m_Vps;
}

void pgsShearRatingArtifact::SetLiveLoadFactor(Float64 gLL)
{
   m_gLL = gLL;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetLiveLoadFactor() const
{
   return m_gLL;
}

void pgsShearRatingArtifact::SetLiveLoadShear(Float64 Vllim)
{
   m_Vllim = Vllim;
   m_bRFComputed = false;
}

Float64 pgsShearRatingArtifact::GetLiveLoadShear() const
{
   return m_Vllim;
}

void pgsShearRatingArtifact::SetLongReinfShearArtifact(const pgsLongReinfShearArtifact& artifact)
{
   m_LongReinfShearArtifact = artifact;
}

const pgsLongReinfShearArtifact& pgsShearRatingArtifact::GetLongReinfShearArtifact() const
{
   return m_LongReinfShearArtifact;
}

Float64 pgsShearRatingArtifact::GetRatingFactor() const
{
   if ( m_bRFComputed )
   {
      return m_RF;
   }


   if ( IsZero(m_Vllim) || IsZero(m_gLL) )
   {
      m_RF = DBL_MAX;
   }
   else
   {
      Float64 p = m_SystemFactor * m_ConditionFactor;
      if ( p < 0.85 )
      {
         p = 0.85; // 6A.4.2.1-3)
      }

      Float64 C = p * m_CapacityRedutionFactor * m_Vn;
      Float64 RF = (C - m_gDC*m_Vdc - m_gDW*m_Vdw - m_gCR*m_Vcr - m_gSH*m_Vsh - m_gRE*m_Vre - m_gPS*m_Vps)/(m_gLL*m_Vllim);

      if ( RF < 0 )
      {
         RF = 0;
      }

      m_RF = RF;
   }

   m_bRFComputed = true;
   return m_RF;
}

void pgsShearRatingArtifact::MakeCopy(const pgsShearRatingArtifact& rOther)
{
   m_POI                        = rOther.m_POI;
   m_RatingType                 = rOther.m_RatingType;
   m_VehicleIndex               = rOther.m_VehicleIndex;
   m_VehicleWeight              = rOther.m_VehicleWeight;
   m_strVehicleName             = rOther.m_strVehicleName;
   m_bRFComputed                = rOther.m_bRFComputed;
   m_RF                         = rOther.m_RF;
   m_SystemFactor               = rOther.m_SystemFactor;
   m_ConditionFactor            = rOther.m_ConditionFactor;
   m_CapacityRedutionFactor     = rOther.m_CapacityRedutionFactor;
   m_Vn                         = rOther.m_Vn;
   m_gDC                        = rOther.m_gDC;
   m_gDW                        = rOther.m_gDW;
   m_gCR                        = rOther.m_gCR;
   m_gSH                        = rOther.m_gSH;
   m_gRE                        = rOther.m_gRE;
   m_gPS                        = rOther.m_gPS;
   m_gLL                        = rOther.m_gLL;
   m_Vdc                        = rOther.m_Vdc;
   m_Vdw                        = rOther.m_Vdw;
   m_Vcr                        = rOther.m_Vcr;
   m_Vsh                        = rOther.m_Vsh;
   m_Vre                        = rOther.m_Vre;
   m_Vps                        = rOther.m_Vps;
   m_Vllim                      = rOther.m_Vllim;
   m_LongReinfShearArtifact     = rOther.m_LongReinfShearArtifact;
}

void pgsShearRatingArtifact::MakeAssignment(const pgsShearRatingArtifact& rOther)
{
   MakeCopy( rOther );
}

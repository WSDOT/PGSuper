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
#include <PgsExt\StressRatingArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsStressRatingArtifact
****************************************************************************/
pgsStressRatingArtifact::pgsStressRatingArtifact() :
m_strVehicleName(_T("Unknown"))
{
   m_bRFComputed = false;
   m_RF = 0;

   m_RatingType = pgsTypes::lrDesign_Inventory;

   m_VehicleIndex = INVALID_INDEX;
   m_VehicleWeight = -999999;

   m_fr = 0;
   m_fps = 0;
   m_fpt = 0;
   m_gDC = 0;
   m_gDW = 0;
   m_gCR = 0;
   m_gSH = 0;
   m_gRE = 0;
   m_gPS = 0;
   m_gLL = 0;
   m_fDC = 0;
   m_fDW = 0;
   m_fCR = 0;
   m_fSH = 0;
   m_fRE = 0;
   m_fPS = 0;
   m_fLLIM = 0;
}

pgsStressRatingArtifact::pgsStressRatingArtifact(const pgsStressRatingArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsStressRatingArtifact::~pgsStressRatingArtifact()
{
}

pgsStressRatingArtifact& pgsStressRatingArtifact::operator=(const pgsStressRatingArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsStressRatingArtifact::SetPointOfInterest(const pgsPointOfInterest& poi)
{
   m_POI = poi;
}

const pgsPointOfInterest& pgsStressRatingArtifact::GetPointOfInterest() const
{
   return m_POI;
}

void pgsStressRatingArtifact::SetRatingType(pgsTypes::LoadRatingType ratingType)
{
   m_RatingType = ratingType;
}

pgsTypes::LoadRatingType pgsStressRatingArtifact::GetLoadRatingType() const
{
   return m_RatingType;
}

void pgsStressRatingArtifact::SetVehicleIndex(VehicleIndexType vehicleIdx)
{
   m_VehicleIndex = vehicleIdx;
}

VehicleIndexType pgsStressRatingArtifact::GetVehicleIndex() const
{
   return m_VehicleIndex;
}

void pgsStressRatingArtifact::SetVehicleWeight(Float64 W)
{
   m_VehicleWeight = W;
}

Float64 pgsStressRatingArtifact::GetVehicleWeight() const
{
   return m_VehicleWeight;
}

void pgsStressRatingArtifact::SetVehicleName(LPCTSTR str)
{
   m_strVehicleName = str;
}

std::_tstring pgsStressRatingArtifact::GetVehicleName() const
{
   return m_strVehicleName;
}

Float64 pgsStressRatingArtifact::GetResistance() const
{
   Float64 fr = m_fr - m_fps - m_fpt; // See MBE A3.13.2.2
   return fr;
}

void pgsStressRatingArtifact::SetAllowableStress(Float64 fr)
{
   m_fr = fr;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetAllowableStress() const
{
   return m_fr;
}

void pgsStressRatingArtifact::SetDeadLoadFactor(Float64 gDC)
{
   m_gDC = gDC;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetDeadLoadFactor() const
{
   return m_gDC;
}

void pgsStressRatingArtifact::SetDeadLoadStress(Float64 fDC)
{
   m_fDC = fDC;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetDeadLoadStress() const
{
   return m_fDC;
}

void pgsStressRatingArtifact::SetPrestressStress(Float64 fps)
{
   m_fps = fps;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetPrestressStress() const
{
   return m_fps;
}

void pgsStressRatingArtifact::SetPostTensionStress(Float64 fpt)
{
   m_fpt = fpt;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetPostTensionStress() const
{
   return m_fpt;
}

void pgsStressRatingArtifact::SetWearingSurfaceFactor(Float64 gDW)
{
   m_gDW = gDW;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetWearingSurfaceFactor() const
{
   return m_gDW;
}

void pgsStressRatingArtifact::SetWearingSurfaceStress(Float64 fDW)
{
   m_fDW = fDW;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetWearingSurfaceStress() const
{
   return m_fDW;
}

void pgsStressRatingArtifact::SetCreepFactor(Float64 gCR)
{
   m_gCR = gCR;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetCreepFactor() const
{
   return m_gCR;
}

void pgsStressRatingArtifact::SetCreepStress(Float64 fCR)
{
   m_fCR = fCR;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetCreepStress() const
{
   return m_fCR;
}

void pgsStressRatingArtifact::SetShrinkageFactor(Float64 gSH)
{
   m_gSH = gSH;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetShrinkageFactor() const
{
   return m_gSH;
}

void pgsStressRatingArtifact::SetShrinkageStress(Float64 fSH)
{
   m_fSH = fSH;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetShrinkageStress() const
{
   return m_fSH;
}

void pgsStressRatingArtifact::SetRelaxationFactor(Float64 gRE)
{
   m_gRE = gRE;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetRelaxationFactor() const
{
   return m_gRE;
}

void pgsStressRatingArtifact::SetRelaxationStress(Float64 fRE)
{
   m_fRE = fRE;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetRelaxationStress() const
{
   return m_fRE;
}

void pgsStressRatingArtifact::SetSecondaryEffectsFactor(Float64 gPS)
{
   m_gPS = gPS;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetSecondaryEffectsFactor() const
{
   return m_gPS;
}

void pgsStressRatingArtifact::SetSecondaryEffectsStress(Float64 fPS)
{
   m_fPS = fPS;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetSecondaryEffectsStress() const
{
   return m_fPS;
}

void pgsStressRatingArtifact::SetLiveLoadFactor(Float64 gLL)
{
   m_gLL = gLL;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetLiveLoadFactor() const
{
   return m_gLL;
}

void pgsStressRatingArtifact::SetLiveLoadStress(Float64 fLLIM)
{
   m_fLLIM = fLLIM;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetLiveLoadStress() const
{
   return m_fLLIM;
}

Float64 pgsStressRatingArtifact::GetRatingFactor() const
{
   if ( m_bRFComputed )
   {
      return m_RF;
   }


   if ( IsZero(m_fLLIM) || IsZero(m_gLL) )
   {
      m_RF = DBL_MAX;
   }
   else
   {
      Float64 fr = GetResistance();
      Float64 RF = (fr - m_gDC*m_fDC - m_gDW*m_fDW - m_gCR*m_fCR - m_gSH*m_fSH - m_gRE*m_fRE - m_gPS*m_fPS)/(m_gLL*m_fLLIM);

      if ( RF < 0 )
      {
         RF = 0;
      }

      m_RF = RF;
   }

   m_bRFComputed = true;
   return m_RF;
}

void pgsStressRatingArtifact::MakeCopy(const pgsStressRatingArtifact& rOther)
{
   m_POI                        = rOther.m_POI;
   m_RatingType                 = rOther.m_RatingType;
   m_VehicleIndex               = rOther.m_VehicleIndex;
   m_bRFComputed                = rOther.m_bRFComputed;
   m_VehicleWeight              = rOther.m_VehicleWeight;
   m_strVehicleName             = rOther.m_strVehicleName;
   m_RF                         = rOther.m_RF;
   m_fr                         = rOther.m_fr;
   m_fps                        = rOther.m_fps;
   m_fpt                        = rOther.m_fpt;
   m_gDC                        = rOther.m_gDC;
   m_gDW                        = rOther.m_gDW;
   m_gCR                        = rOther.m_gCR;
   m_gSH                        = rOther.m_gSH;
   m_gRE                        = rOther.m_gRE;
   m_gPS                        = rOther.m_gPS;
   m_gLL                        = rOther.m_gLL;
   m_fDC                        = rOther.m_fDC;
   m_fDW                        = rOther.m_fDW;
   m_fCR                        = rOther.m_fCR;
   m_fSH                        = rOther.m_fSH;
   m_fRE                        = rOther.m_fRE;
   m_fPS                        = rOther.m_fPS;
   m_fLLIM                      = rOther.m_fLLIM;
}

void pgsStressRatingArtifact::MakeAssignment(const pgsStressRatingArtifact& rOther)
{
   MakeCopy( rOther );
}

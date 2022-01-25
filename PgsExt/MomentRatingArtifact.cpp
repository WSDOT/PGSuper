///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include <PgsExt\MomentRatingArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsMomentRatingArtifact
****************************************************************************/
pgsMomentRatingArtifact::pgsMomentRatingArtifact() :
m_strVehicleName(_T("Unknown"))
{
   m_bRFComputed = false;
   m_RF = 0;

   m_RatingType = pgsTypes::lrDesign_Inventory;

   m_VehicleIndex = INVALID_INDEX;
   m_VehicleWeight = -99999;

   m_SystemFactor = 1.0;
   m_ConditionFactor = 1.0;
   m_MinimumReinforcementFactor = 1.0;
   m_CapacityRedutionFactor = 1.0;
   m_Mn = 0;
   m_gDC = 1;
   m_gDW = 1;
   m_gCR = 1;
   m_gSH = 1;
   m_gRE = 1;
   m_gPS = 1;
   m_gLL = 1;
   m_Mdc = 0;
   m_Mdw = 0;
   m_Mcr = 0;
   m_Msh = 0;
   m_Mre = 0;
   m_Mps = 0;
   m_Mllim = 0;
}

pgsMomentRatingArtifact::pgsMomentRatingArtifact(const pgsMomentRatingArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsMomentRatingArtifact::~pgsMomentRatingArtifact()
{
}

pgsMomentRatingArtifact& pgsMomentRatingArtifact::operator=(const pgsMomentRatingArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsMomentRatingArtifact::SetPointOfInterest(const pgsPointOfInterest& poi)
{
   m_POI = poi;
}

const pgsPointOfInterest& pgsMomentRatingArtifact::GetPointOfInterest() const
{
   return m_POI;
}

void pgsMomentRatingArtifact::SetRatingType(pgsTypes::LoadRatingType ratingType)
{
   m_RatingType = ratingType;
}

pgsTypes::LoadRatingType pgsMomentRatingArtifact::GetLoadRatingType() const
{
   return m_RatingType;
}

void pgsMomentRatingArtifact::SetVehicleIndex(VehicleIndexType vehicleIdx)
{
   m_VehicleIndex = vehicleIdx;
}

VehicleIndexType pgsMomentRatingArtifact::GetVehicleIndex() const
{
   return m_VehicleIndex;
}

void pgsMomentRatingArtifact::SetVehicleName(LPCTSTR str)
{
   m_strVehicleName = str;
}

const std::_tstring& pgsMomentRatingArtifact::GetVehicleName() const
{
   return m_strVehicleName;
}

void pgsMomentRatingArtifact::SetVehicleWeight(Float64 W)
{
   m_VehicleWeight = W;
}

Float64 pgsMomentRatingArtifact::GetVehicleWeight() const
{
   return m_VehicleWeight;
}

void pgsMomentRatingArtifact::SetSystemFactor(Float64 systemFactor)
{
   m_SystemFactor = systemFactor;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetSystemFactor() const
{
   return m_SystemFactor;
}

void pgsMomentRatingArtifact::SetConditionFactor(Float64 conditionFactor)
{
   m_ConditionFactor = conditionFactor;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetConditionFactor() const
{
   return m_ConditionFactor;
}

void pgsMomentRatingArtifact::SetCapacityReductionFactor(Float64 phi)
{
   m_CapacityRedutionFactor = phi;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetCapacityReductionFactor() const
{
   return m_CapacityRedutionFactor;
}

void pgsMomentRatingArtifact::SetMinimumReinforcementFactor(Float64 K)
{
   m_MinimumReinforcementFactor = K;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetMinimumReinforcementFactor() const
{
   return m_MinimumReinforcementFactor;
}

void pgsMomentRatingArtifact::SetNominalMomentCapacity(Float64 Mn)
{
   m_Mn = Mn;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetNominalMomentCapacity() const
{
   return m_Mn;
}

void pgsMomentRatingArtifact::SetDeadLoadFactor(Float64 gDC)
{
   m_gDC = gDC;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetDeadLoadFactor() const
{
   return m_gDC;
}

void pgsMomentRatingArtifact::SetDeadLoadMoment(Float64 Mdc)
{
   m_Mdc = Mdc;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetDeadLoadMoment() const
{
   return m_Mdc;
}

void pgsMomentRatingArtifact::SetWearingSurfaceFactor(Float64 gDW)
{
   m_gDW = gDW;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetWearingSurfaceFactor() const
{
   return m_gDW;
}

void pgsMomentRatingArtifact::SetWearingSurfaceMoment(Float64 Mdw)
{
   m_Mdw = Mdw;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetWearingSurfaceMoment() const
{
   return m_Mdw;
}

void pgsMomentRatingArtifact::SetCreepFactor(Float64 gCR)
{
   m_gCR = gCR;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetCreepFactor() const
{
   return m_gCR;
}

void pgsMomentRatingArtifact::SetCreepMoment(Float64 Mcr)
{
   m_Mcr = Mcr;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetCreepMoment() const
{
   return m_Mcr;
}

void pgsMomentRatingArtifact::SetShrinkageFactor(Float64 gSH)
{
   m_gSH = gSH;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetShrinkageFactor() const
{
   return m_gSH;
}

void pgsMomentRatingArtifact::SetShrinkageMoment(Float64 Msh)
{
   m_Msh = Msh;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetShrinkageMoment() const
{
   return m_Msh;
}

void pgsMomentRatingArtifact::SetRelaxationFactor(Float64 gRE)
{
   m_gRE = gRE;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetRelaxationFactor() const
{
   return m_gRE;
}

void pgsMomentRatingArtifact::SetRelaxationMoment(Float64 Mre)
{
   m_Mre = Mre;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetRelaxationMoment() const
{
   return m_Mre;
}

void pgsMomentRatingArtifact::SetSecondaryEffectsFactor(Float64 gPS)
{
   m_gPS = gPS;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetSecondaryEffectsFactor() const
{
   return m_gPS;
}

void pgsMomentRatingArtifact::SetSecondaryEffectsMoment(Float64 Mps)
{
   m_Mps = Mps;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetSecondaryEffectsMoment() const
{
   return m_Mps;
}

void pgsMomentRatingArtifact::SetLiveLoadFactor(Float64 gLL)
{
   m_gLL = gLL;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetLiveLoadFactor() const
{
   return m_gLL;
}

void pgsMomentRatingArtifact::SetLiveLoadMoment(Float64 Mllim)
{
   m_Mllim = Mllim;
   m_bRFComputed = false;
}

Float64 pgsMomentRatingArtifact::GetLiveLoadMoment() const
{
   return m_Mllim;
}

Float64 pgsMomentRatingArtifact::GetRatingFactor() const
{
   if ( m_bRFComputed )
   {
      return m_RF;
   }


   Float64 p = Max(m_SystemFactor*m_ConditionFactor,0.85); // MBE 6A.4.2.1-3

   Float64 C = p * m_CapacityRedutionFactor * m_MinimumReinforcementFactor * m_Mn;
   Float64 RFtop = C - m_gDC*m_Mdc - m_gDW*m_Mdw - m_gCR*m_Mcr - m_gSH*m_Msh - m_gRE*m_Mre - m_gPS*m_Mps;
   Float64 RFbot = m_gLL*m_Mllim;

   if (IsZero(C) && IsZero(RFtop) && IsZero(RFbot))
   {
      m_RF = DBL_MAX;
   }
   else if ( IsZero(C) || (0 < C && RFtop < 0) || (C < 0 && 0 < RFtop) )
   {
      // There isn't any capacity remaining for live load
      m_RF = 0;
   }
   else if ( ::BinarySign(RFtop) != ::BinarySign(RFbot) && !IsZero(RFtop) )
   {
      // (C - DL) and LL have opposite signs
      // this case probably shouldn't happen, but if does,
      // the rating is great
      m_RF = DBL_MAX;
   }
   else if (IsZero(m_Mllim) || IsZero(m_gLL))
   {
      m_RF = DBL_MAX;
   }
   else
   {
      m_RF = RFtop/RFbot;
   }

   m_bRFComputed = true;
   return m_RF;
}

void pgsMomentRatingArtifact::MakeCopy(const pgsMomentRatingArtifact& rOther)
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
   m_MinimumReinforcementFactor = rOther.m_MinimumReinforcementFactor;
   m_CapacityRedutionFactor     = rOther.m_CapacityRedutionFactor;
   m_Mn                         = rOther.m_Mn;
   m_gDC                        = rOther.m_gDC;
   m_gDW                        = rOther.m_gDW;
   m_gCR                        = rOther.m_gCR;
   m_gSH                        = rOther.m_gSH;
   m_gRE                        = rOther.m_gRE;
   m_gPS                        = rOther.m_gPS;
   m_gLL                        = rOther.m_gLL;
   m_Mdc                        = rOther.m_Mdc;
   m_Mdw                        = rOther.m_Mdw;
   m_Mcr                        = rOther.m_Mcr;
   m_Msh                        = rOther.m_Msh;
   m_Mre                        = rOther.m_Mre;
   m_Mps                        = rOther.m_Mps;
   m_Mllim                      = rOther.m_Mllim;
}

void pgsMomentRatingArtifact::MakeAssignment(const pgsMomentRatingArtifact& rOther)
{
   MakeCopy( rOther );
}

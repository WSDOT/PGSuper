///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
pgsMomentRatingArtifact::pgsMomentRatingArtifact()
{
   m_bRFComputed = false;
   m_RF = 0;
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

void pgsMomentRatingArtifact::SetVehicleName(const char* str)
{
   m_strVehicleName = str;
}

std::string pgsMomentRatingArtifact::GetVehicleName() const
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
      return m_RF;


   if ( IsZero(m_Mllim) || IsZero(m_gLL) )
   {
      m_RF = DBL_MAX;
   }
   else
   {
      Float64 p = m_SystemFactor * m_ConditionFactor;
      if ( p < 0.85 )
         p = 0.85; // 6A.4.2.1-3)

      Float64 C = p * m_CapacityRedutionFactor * m_MinimumReinforcementFactor * m_Mn;
      Float64 RF = (C - m_gDC*m_Mdc - m_gDW*m_Mdw)/(m_gLL*m_Mllim);

      if ( RF < 0 )
         RF = 0;

      m_RF = RF;
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
   m_gLL                        = rOther.m_gLL;
   m_Mdc                        = rOther.m_Mdc;
   m_Mdw                        = rOther.m_Mdw;
   m_Mllim                      = rOther.m_Mllim;
}

void pgsMomentRatingArtifact::MakeAssignment(const pgsMomentRatingArtifact& rOther)
{
   MakeCopy( rOther );
}

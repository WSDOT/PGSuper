///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
pgsStressRatingArtifact::pgsStressRatingArtifact()
{
   m_bRFComputed = false;
   m_RF = 0;
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

void pgsStressRatingArtifact::SetDeadLoadStress(Float64 fdc)
{
   m_fdc = fdc;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetDeadLoadStress() const
{
   return m_fdc;
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

void pgsStressRatingArtifact::SetWearingSurfaceFactor(Float64 gDW)
{
   m_gDW = gDW;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetWearingSurfaceFactor() const
{
   return m_gDW;
}

void pgsStressRatingArtifact::SetWearingSurfaceStress(Float64 fdw)
{
   m_fdw = fdw;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetWearingSurfaceStress() const
{
   return m_fdw;
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

void pgsStressRatingArtifact::SetLiveLoadStress(Float64 fllim)
{
   m_fllim = fllim;
   m_bRFComputed = false;
}

Float64 pgsStressRatingArtifact::GetLiveLoadStress() const
{
   return m_fllim;
}

Float64 pgsStressRatingArtifact::GetRatingFactor() const
{
   if ( m_bRFComputed )
      return m_RF;


   if ( IsZero(m_fllim) || IsZero(m_gLL) )
   {
      m_RF = DBL_MAX;
   }
   else
   {
      Float64 RF = (m_fr - m_gDC*(m_fdc+m_fps) - m_gDW*m_fdw)/(m_gLL*m_fllim);

      if ( RF < 0 )
         RF = 0;

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
   m_gDC                        = rOther.m_gDC;
   m_gDW                        = rOther.m_gDW;
   m_gLL                        = rOther.m_gLL;
   m_fdc                        = rOther.m_fdc;
   m_fps                        = rOther.m_fps;
   m_fdw                        = rOther.m_fdw;
   m_fllim                      = rOther.m_fllim;
}

void pgsStressRatingArtifact::MakeAssignment(const pgsStressRatingArtifact& rOther)
{
   MakeCopy( rOther );
}

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
#include <PgsExt\YieldStressRatioArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsYieldStressRatioArtifact
****************************************************************************/
pgsYieldStressRatioArtifact::pgsYieldStressRatioArtifact()
{
   //m_bRFComputed = false;
   //m_RF = 0;
   m_bRFComputed = false;
   m_RF = 0;
   m_fcr = 0;
   m_fs = 0;

   m_RatingType = pgsTypes::lrDesign_Inventory;

   m_VehicleIndex = INVALID_INDEX;
   m_VehicleWeight = -9999999;
   m_strVehicleName = _T("Unknown");

   m_fr = 0;
   m_fpe = 0;
   m_Mdc = 0;
   m_Mdw = 0;
   m_Mcr = 0;
   m_Msh = 0;
   m_Mre = 0;
   m_Mps = 0;
   m_Mllim = 0;
   m_Mcrack = 0;
   m_Icrack = 0;
   m_c = 0;
   m_dps = 0;
   m_Es = 0;
   m_Eg = 0;
   m_gDC = 1.0;
   m_gDW = 1.0;
   m_gCR = 1.0;
   m_gSH = 1.0;
   m_gRE = 1.0;
   m_gPS = 1.0;
   m_gLL = 1.0;
}

pgsYieldStressRatioArtifact::pgsYieldStressRatioArtifact(const pgsYieldStressRatioArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsYieldStressRatioArtifact::~pgsYieldStressRatioArtifact()
{
}

pgsYieldStressRatioArtifact& pgsYieldStressRatioArtifact::operator=(const pgsYieldStressRatioArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsYieldStressRatioArtifact::SetPointOfInterest(const pgsPointOfInterest& poi)
{
   m_POI = poi;
}

const pgsPointOfInterest& pgsYieldStressRatioArtifact::GetPointOfInterest() const
{
   return m_POI;
}

void pgsYieldStressRatioArtifact::SetRatingType(pgsTypes::LoadRatingType ratingType)
{
   m_RatingType = ratingType;
}

pgsTypes::LoadRatingType pgsYieldStressRatioArtifact::GetLoadRatingType() const
{
   return m_RatingType;
}

void pgsYieldStressRatioArtifact::SetVehicleIndex(VehicleIndexType vehicleIdx)
{
   m_VehicleIndex = vehicleIdx;
}

VehicleIndexType pgsYieldStressRatioArtifact::GetVehicleIndex() const
{
   return m_VehicleIndex;
}

void pgsYieldStressRatioArtifact::SetVehicleWeight(Float64 W)
{
   m_VehicleWeight = W;
}

Float64 pgsYieldStressRatioArtifact::GetVehicleWeight() const
{
   return m_VehicleWeight;
}

void pgsYieldStressRatioArtifact::SetVehicleName(LPCTSTR str)
{
   m_strVehicleName = str;
}

std::_tstring pgsYieldStressRatioArtifact::GetVehicleName() const
{
   return m_strVehicleName;
}

void pgsYieldStressRatioArtifact::SetAllowableStress(Float64 fr)
{
   m_fr = fr;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetAllowableStress() const
{
   return m_fr;
}

void pgsYieldStressRatioArtifact::SetDeadLoadFactor(Float64 gDC)
{
   m_gDC = gDC;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetDeadLoadFactor() const
{
   return m_gDC;
}

void pgsYieldStressRatioArtifact::SetDeadLoadMoment(Float64 Mdc)
{
   m_Mdc = Mdc;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetDeadLoadMoment() const
{
   return m_Mdc;
}

void pgsYieldStressRatioArtifact::SetWearingSurfaceFactor(Float64 gDW)
{
   m_gDW = gDW;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetWearingSurfaceFactor() const
{
   return m_gDW;
}

void pgsYieldStressRatioArtifact::SetWearingSurfaceMoment(Float64 Mdw)
{
   m_Mdw = Mdw;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetWearingSurfaceMoment() const
{
   return m_Mdw;
}

void pgsYieldStressRatioArtifact::SetCreepFactor(Float64 gCR)
{
   m_gCR = gCR;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetCreepFactor() const
{
   return m_gCR;
}

void pgsYieldStressRatioArtifact::SetCreepMoment(Float64 Mcr)
{
   m_Mcr = Mcr;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetCreepMoment() const
{
   return m_Mcr;
}

void pgsYieldStressRatioArtifact::SetShrinkageFactor(Float64 gSH)
{
   m_gSH = gSH;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetShrinkageFactor() const
{
   return m_gSH;
}

void pgsYieldStressRatioArtifact::SetShrinkageMoment(Float64 Msh)
{
   m_Msh = Msh;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetShrinkageMoment() const
{
   return m_Msh;
}

void pgsYieldStressRatioArtifact::SetRelaxationFactor(Float64 gRE)
{
   m_gRE = gRE;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetRelaxationFactor() const
{
   return m_gRE;
}

void pgsYieldStressRatioArtifact::SetRelaxationMoment(Float64 Mre)
{
   m_Mre = Mre;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetRelaxationMoment() const
{
   return m_Mre;
}

void pgsYieldStressRatioArtifact::SetSecondaryEffectsFactor(Float64 gPS)
{
   m_gPS = gPS;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetSecondaryEffectsFactor() const
{
   return m_gPS;
}

void pgsYieldStressRatioArtifact::SetSecondaryEffectsMoment(Float64 Mps)
{
   m_Mps = Mps;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetSecondaryEffectsMoment() const
{
   return m_Mps;
}

void pgsYieldStressRatioArtifact::SetLiveLoadFactor(Float64 gLL)
{
   m_gLL = gLL;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetLiveLoadFactor() const
{
   return m_gLL;
}

void pgsYieldStressRatioArtifact::SetLiveLoadMoment(Float64 Mllim)
{
   m_Mllim = Mllim;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetLiveLoadMoment() const
{
   return m_Mllim;
}

void pgsYieldStressRatioArtifact::SetCrackingMoment(Float64 Mcr)
{
   m_Mcrack = Mcr;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetCrackingMoment() const
{
   return m_Mcrack;
}

void pgsYieldStressRatioArtifact::SetIcr(Float64 Icr)
{
   m_Icrack = Icr;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetIcr() const
{
   return m_Icrack;
}

void pgsYieldStressRatioArtifact::SetCrackDepth(Float64 c)
{
   m_c = c;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetCrackDepth() const
{
   return m_c;
}

void pgsYieldStressRatioArtifact::SetReinforcementDepth(Float64 dps)
{
   m_dps = dps;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetReinforcementDepth() const
{
   return m_dps;
}

Float64 pgsYieldStressRatioArtifact::GetCrackingStressIncrement() const
{
   GetStressRatio(); // causes fps to be computed
   return m_fcr;
}

void pgsYieldStressRatioArtifact::SetEffectivePrestress(Float64 fpe)
{
   m_fpe = fpe;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetEffectivePrestress() const
{
   return m_fpe;
}

void pgsYieldStressRatioArtifact::SetEs(Float64 Es)
{
   m_Es = Es;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetEs() const
{
   return m_Es;
}

void pgsYieldStressRatioArtifact::SetEg(Float64 Eg)
{
   m_Eg = Eg;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetEg() const
{
   return m_Eg;
}

Float64 pgsYieldStressRatioArtifact::GetExcessMoment() const
{
   Float64 M = m_gDC*m_Mdc + m_gDW*m_Mdw + m_gCR*m_Mcr + m_gSH*m_Msh + m_gRE*m_Mre + m_gPS*m_Mps + m_gLL*m_Mllim;
   if ( m_Mcrack < 0 )
   {
      // negative moment
      if ( m_Mcrack < M )
      {
         return 0; // section isn't cracked
      }
      else
      {
         return M - m_Mcrack;
      }
   }
   else
   {
      // positive moment
      if ( M < m_Mcrack )
      {
         return 0; // section isn't cracked
      }
      else
      {
         return M - m_Mcrack;
      }
   }
}

Float64 pgsYieldStressRatioArtifact::GetStrandStress() const
{
   GetStressRatio(); // causes fps to be computed
   return m_fs;
}

Float64 pgsYieldStressRatioArtifact::GetStressRatio() const
{
   if ( m_bRFComputed )
   {
      return m_RF;
   }

   // moment in excess of cracking
   Float64 M = GetExcessMoment();
   m_fcr = (m_Es/m_Eg)*fabs(M)*(m_dps-m_c)/m_Icrack; // stress added to strand at instance of cracking
   m_fs = m_fpe + m_fcr; // total stress in strand just after cracking
   if ( IsZero(m_fs) )
   {
      m_RF = DBL_MAX;
   }
   else
   {
      m_RF = m_fr/m_fs;
   }

   if ( m_RF < 0 )
   {
      m_RF = 0;
   }

   m_bRFComputed = true;
   return m_RF;
}

void pgsYieldStressRatioArtifact::MakeCopy(const pgsYieldStressRatioArtifact& rOther)
{
   m_POI            = rOther.m_POI;
   m_RatingType     = rOther.m_RatingType;
   m_VehicleIndex   = rOther.m_VehicleIndex;
   m_bRFComputed    = rOther.m_bRFComputed;
   m_VehicleWeight  = rOther.m_VehicleWeight;
   m_strVehicleName = rOther.m_strVehicleName;
   m_RF             = rOther.m_RF;
   m_fr             = rOther.m_fr;
   m_fcr            = rOther.m_fcr;
   m_fs             = rOther.m_fs;
   m_fpe            = rOther.m_fpe;
   m_Mdc            = rOther.m_Mdc;
   m_Mdw            = rOther.m_Mdw;
   m_Mcr            = rOther.m_Mcr;
   m_Msh            = rOther.m_Msh;
   m_Mre            = rOther.m_Mre;
   m_Mps            = rOther.m_Mps;
   m_Mllim          = rOther.m_Mllim;
   m_Mcrack         = rOther.m_Mcrack;
   m_Icrack         = rOther.m_Icrack;
   m_c              = rOther.m_c;
   m_dps            = rOther.m_dps;
   m_Es             = rOther.m_Es;
   m_Eg             = rOther.m_Eg;
   m_gDC            = rOther.m_gDC;
   m_gDW            = rOther.m_gDW;
   m_gCR            = rOther.m_gCR;
   m_gSH            = rOther.m_gSH;
   m_gRE            = rOther.m_gRE;
   m_gPS            = rOther.m_gPS;
   m_gLL            = rOther.m_gLL;
}

void pgsYieldStressRatioArtifact::MakeAssignment(const pgsYieldStressRatioArtifact& rOther)
{
   MakeCopy( rOther );
}

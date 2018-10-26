///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
   m_bRFComputed = false;

   m_RebarRF = 0;
   m_fsRebar = 0;
   m_fcrRebar = 0;

   m_StrandRF = 0;
   m_fsStrand = 0;
   m_fcrStrand = 0;

   m_TendonRF = 0;
   m_fsTendon = 0;
   m_fcrTendon = 0;

   m_RatingType = pgsTypes::lrDesign_Inventory;

   m_VehicleIndex = INVALID_INDEX;
   m_VehicleWeight = -9999999;
   m_strVehicleName = _T("Unknown");

   m_AllowableStressRatio = 0.9;
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
   m_Eg = 0;
   m_gDC = 1.0;
   m_gDW = 1.0;
   m_gCR = 1.0;
   m_gSH = 1.0;
   m_gRE = 1.0;
   m_gPS = 1.0;
   m_gLL = 1.0;
   m_gM  = 1.0;


   m_bRebar = false;
   m_db = 0;
   m_fb = 0;
   m_fyb = 0;
   m_Eb = 0;

   m_bStrand = false;
   m_dps  = 0;
   m_fps  = 0;
   m_fyps = 0;
   m_Eps  = 0;

   m_bTendon = false;
   m_dpt  = 0;
   m_fpt  = 0;
   m_fypt = 0;
   m_Ept  = 0;
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

void pgsYieldStressRatioArtifact::SetAllowableStressRatio(Float64 K)
{
   m_AllowableStressRatio = K;
   m_bRFComputed = false;
}

Float64 pgsYieldStressRatioArtifact::GetAllowableStressRatio() const
{
   return m_AllowableStressRatio;
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
   
void pgsYieldStressRatioArtifact::SetLiveLoadDistributionFactor(Float64 gM)
{
   // not used in computation of RF because it is included in LL moment
   m_gM = gM;
}

Float64 pgsYieldStressRatioArtifact::GetLiveLoadDistributionFactor() const
{
   return m_gM;
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

void pgsYieldStressRatioArtifact::SetRebar(Float64 db,Float64 fb,Float64 fyb,Float64 Eb)
{
   m_db  = db;
   m_fb  = fb;
   m_fyb = fyb;
   m_Eb  = Eb;
   m_bRebar = true;
}

bool pgsYieldStressRatioArtifact::GetRebar(Float64* pdb, Float64* pfb,Float64* pfyb,Float64* pEb) const
{
   *pdb  = m_db;
   *pfb  = m_fb;
   *pfyb = m_fyb;
   *pEb  = m_Eb;
   return m_bRebar;
}

void pgsYieldStressRatioArtifact::SetStrand(Float64 dps,Float64 fps,Float64 fyps,Float64 Eps)
{
   m_dps  = dps;
   m_fps  = fps;
   m_fyps = fyps;
   m_Eps  = Eps;
   m_bStrand = true;
   m_bRFComputed = false;
}

bool pgsYieldStressRatioArtifact::GetStrand(Float64* pdps,Float64* pfps,Float64* pfyps,Float64* pEps) const
{
   *pdps  = m_dps;
   *pfps  = m_fps;
   *pfyps = m_fyps;
   *pEps  = m_Eps;
   return m_bStrand;
}

void pgsYieldStressRatioArtifact::SetTendon(Float64 dpt,Float64 fpt,Float64 fypt,Float64 Ept)
{
   m_dpt  = dpt;
   m_fpt  = fpt;
   m_fypt = fypt;
   m_Ept  = Ept;
   m_bTendon = true;
   m_bRFComputed = false;
}

bool pgsYieldStressRatioArtifact::GetTendon(Float64* pdpt,Float64* pfpt,Float64* pfypt,Float64* pEpt) const
{
   *pdpt  = m_dpt;
   *pfpt  = m_fpt;
   *pfypt = m_fypt;
   *pEpt  = m_Ept;
   return m_bTendon;
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
   // NOTE: m_Mllim includes the LLDF... don't include m_gM here

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

Float64 pgsYieldStressRatioArtifact::GetRebarCrackingStressIncrement() const
{
   ComputeStressRatios();
   return m_fcrRebar;
}

Float64 pgsYieldStressRatioArtifact::GetRebarStress() const
{
   ComputeStressRatios();
   return m_fsRebar;
}

Float64 pgsYieldStressRatioArtifact::GetRebarStressRatio() const
{
   ComputeStressRatios();
   return m_RebarRF;
}

Float64 pgsYieldStressRatioArtifact::GetRebarAllowableStress() const
{
   return m_AllowableStressRatio*m_fyb;
}

Float64 pgsYieldStressRatioArtifact::GetStrandCrackingStressIncrement() const
{
   ComputeStressRatios();
   return m_fcrStrand;
}

Float64 pgsYieldStressRatioArtifact::GetStrandStress() const
{
   ComputeStressRatios();
   return m_fsStrand;
}

Float64 pgsYieldStressRatioArtifact::GetStrandStressRatio() const
{
   ComputeStressRatios();
   return m_StrandRF;
}

Float64 pgsYieldStressRatioArtifact::GetStrandAllowableStress() const
{
   return m_AllowableStressRatio*m_fyps;
}

Float64 pgsYieldStressRatioArtifact::GetTendonCrackingStressIncrement() const
{
   ComputeStressRatios();
   return m_fcrTendon;
}

Float64 pgsYieldStressRatioArtifact::GetTendonStress() const
{
   ComputeStressRatios();
   return m_fsTendon;
}

Float64 pgsYieldStressRatioArtifact::GetTendonStressRatio() const
{
   ComputeStressRatios();
   return m_TendonRF;
}

Float64 pgsYieldStressRatioArtifact::GetTendonAllowableStress() const
{
   return m_AllowableStressRatio*m_fypt;
}

Float64 pgsYieldStressRatioArtifact::GetStressRatio() const
{
   return Min(GetRebarStressRatio(),GetStrandStressRatio(),GetTendonStressRatio());
}

void pgsYieldStressRatioArtifact::MakeCopy(const pgsYieldStressRatioArtifact& rOther)
{
   m_POI            = rOther.m_POI;
   m_RatingType     = rOther.m_RatingType;
   m_VehicleIndex   = rOther.m_VehicleIndex;
   m_bRFComputed    = rOther.m_bRFComputed;
   m_VehicleWeight  = rOther.m_VehicleWeight;
   m_strVehicleName = rOther.m_strVehicleName;
   m_RebarRF        = rOther.m_RebarRF;
   m_StrandRF       = rOther.m_StrandRF;
   m_TendonRF       = rOther.m_TendonRF;
   m_AllowableStressRatio = rOther.m_AllowableStressRatio;
   m_fcrRebar       = rOther.m_fcrRebar;
   m_fcrStrand      = rOther.m_fcrStrand;
   m_fcrTendon      = rOther.m_fcrTendon;
   m_fsRebar        = rOther.m_fsRebar;
   m_fsStrand       = rOther.m_fsStrand;
   m_fsTendon       = rOther.m_fsTendon;
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
   m_Eg             = rOther.m_Eg;
   m_gDC            = rOther.m_gDC;
   m_gDW            = rOther.m_gDW;
   m_gCR            = rOther.m_gCR;
   m_gSH            = rOther.m_gSH;
   m_gRE            = rOther.m_gRE;
   m_gPS            = rOther.m_gPS;
   m_gLL            = rOther.m_gLL;
   m_gM             = rOther.m_gM;


   m_bRebar = rOther.m_bRebar;
   m_db     = rOther.m_db;
   m_fb     = rOther.m_fb;
   m_fyb    = rOther.m_fyb;
   m_Eb     = rOther.m_Eb;

   m_bStrand = rOther.m_bStrand;
   m_dps     = rOther.m_dps;;
   m_fps     = rOther.m_fps;
   m_fyps    = rOther.m_fyps;
   m_Eps     = rOther.m_Eps;

   m_bTendon = rOther.m_bTendon;
   m_dpt     = rOther.m_dpt;
   m_fpt     = rOther.m_fpt;
   m_fypt    = rOther.m_fypt;
   m_Ept     = rOther.m_Ept;
}

void pgsYieldStressRatioArtifact::MakeAssignment(const pgsYieldStressRatioArtifact& rOther)
{
   MakeCopy( rOther );
}


void pgsYieldStressRatioArtifact::ComputeStressRatios() const
{
   if ( m_bRFComputed )
   {
      return;
   }

   if ( m_bRebar )
   {
      ComputeStressRatio(m_db,m_Eb,m_fb,m_fyb,&m_fcrRebar,&m_fsRebar,&m_RebarRF);
   }
   else
   {
      m_RebarRF = DBL_MAX;
      m_fcrRebar = 0;
      m_fsRebar = 0;
   }

   if ( m_bStrand )
   {
      ComputeStressRatio(m_dps,m_Eps,m_fps,m_fyps,&m_fcrStrand,&m_fsStrand,&m_StrandRF);
   }
   else
   {
      m_StrandRF = DBL_MAX;
      m_fcrStrand = 0;
      m_fsStrand = 0;
   }

   if ( m_bTendon )
   {
      ComputeStressRatio(m_dpt,m_Ept,m_fpt,m_fypt,&m_fcrTendon,&m_fsTendon,&m_TendonRF);
   }
   else
   {
      m_TendonRF = DBL_MAX;
      m_fcrTendon = 0;
      m_fsTendon = 0;
   }
}

void pgsYieldStressRatioArtifact::ComputeStressRatio(Float64 d,Float64 E,Float64 fbcr,Float64 fy,Float64* pfcr,Float64* pfs,Float64* pRF) const
{
   // moment in excess of cracking
   Float64 M = GetExcessMoment();

   Float64 fcr = (E/m_Eg)*fabs(M)*(d-m_c)/m_Icrack; // stress added to strand at instance of cracking
   Float64 fs = fbcr + fcr; // total stress in strand just after cracking
   if ( IsLE(fs,0.0) )
   {
      *pRF = DBL_MAX;
   }
   else
   {
      *pRF = m_AllowableStressRatio*fy/fs;
   }

   ATLASSERT( 0 <= *pRF );

   *pfcr = fcr;
   *pfs = fs;
}

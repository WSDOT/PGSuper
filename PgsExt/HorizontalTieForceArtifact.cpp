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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\HorizontalTieForceArtifact.h>

void pgsHorizontalTieForceArtifact::SetLimitState(pgsTypes::LimitState ls)
{
   m_LimitState = ls;
}

pgsTypes::LimitState pgsHorizontalTieForceArtifact::GetLimitState() const
{
   return m_LimitState;
}

void pgsHorizontalTieForceArtifact::SetReactionLocation(const ReactionLocation& location)
{
   m_Location = location;
}

const ReactionLocation& pgsHorizontalTieForceArtifact::GetReactionLocation() const
{
   return m_Location;
}

void pgsHorizontalTieForceArtifact::IsApplicable(bool bIsApplicable)
{
   m_bIsApplicable = bIsApplicable;
}

bool pgsHorizontalTieForceArtifact::IsApplicable() const
{
   return m_bIsApplicable;
}

void pgsHorizontalTieForceArtifact::SetBearingWidth(Float64 bb)
{
   m_bb = bb;
}

Float64 pgsHorizontalTieForceArtifact::GetBearingWidth() const
{
   return m_bb;
}

void pgsHorizontalTieForceArtifact::SetBottomBulbDepth(Float64 hb)
{
   m_hb = hb;
}

Float64 pgsHorizontalTieForceArtifact::GetBottomBulbDepth() const
{
   return m_hb;
}

void pgsHorizontalTieForceArtifact::SetNumBondedStrandsInFlange(pgsTypes::SideType side, StrandIndexType nStrands)
{
   m_nf[side] = nStrands;
}

StrandIndexType pgsHorizontalTieForceArtifact::GetNumBondedStrandsInFlange(pgsTypes::SideType side) const
{
   return m_nf[side];
}

void pgsHorizontalTieForceArtifact::SetTotalNumBondedStrands(StrandIndexType nStrands)
{
   m_Nw = nStrands;
}

StrandIndexType pgsHorizontalTieForceArtifact::GetTotalNumBondedStrands() const
{
   return m_Nw;
}

void pgsHorizontalTieForceArtifact::SetHorizDistance(pgsTypes::SideType side, Float64 xp)
{
   m_xp[side] = xp;
}

Float64 pgsHorizontalTieForceArtifact::GetHorizDistance(pgsTypes::SideType side) const
{
   return m_xp[side];
}

void pgsHorizontalTieForceArtifact::SetVertDistance(pgsTypes::SideType side, Float64 yp)
{
   m_yp[side] = yp;
}

Float64 pgsHorizontalTieForceArtifact::GetVertDistance(pgsTypes::SideType side) const
{
   return m_yp[side];
}

void pgsHorizontalTieForceArtifact::SetShearForce(Float64 Vu)
{
   m_Vu = Vu;
}

Float64 pgsHorizontalTieForceArtifact::GetShearForce() const
{
   return m_Vu;
}

void pgsHorizontalTieForceArtifact::SetPhi(Float64 phi)
{ 
   m_phi = phi;
}

Float64 pgsHorizontalTieForceArtifact::GetPhi() const
{
   return m_phi;
}

void pgsHorizontalTieForceArtifact::SetTieArea(Float64 As)
{
   m_As = As;
}

Float64 pgsHorizontalTieForceArtifact::GetTieArea() const
{
   return m_As;
}

void pgsHorizontalTieForceArtifact::SetTieYieldStrength(Float64 fy)
{
   m_fy = fy;
}

Float64 pgsHorizontalTieForceArtifact::GetTieYieldStrength() const
{
   return m_fy;
}

Float64 pgsHorizontalTieForceArtifact::GetBearingReactionLocation(pgsTypes::SideType side) const
{
   // cast to float is important here. m_nf and m_Nw are both integers.
   // Nw > nf. The result of dividing them would be 0.
   return m_Nw == 0 ? 0. : (m_bb / 2.) * (1. - (Float64)m_nf[side] / (Float64)m_Nw);
}

Float64 pgsHorizontalTieForceArtifact::GetTieForce() const
{
   Float64 T = 0;
   for (int i = 0; i < 2; i++)
   {
      pgsTypes::SideType side = (pgsTypes::SideType)i;
      auto cb = GetBearingReactionLocation(side);
      Float64 h = ((m_hb - m_yp[side]) == 0. ? 0 : m_xp[side] / (m_hb - m_yp[side]));
      h += (m_yp[side] == 0 ? 0. : (m_xp[side] - cb) / m_yp[side]);
      Float64 t = m_Nw == 0 ? 0. : ((Float64)m_nf[side] / (Float64)m_Nw) * h * m_Vu / m_phi; // this is the total force assuming symmetry
      T += t;
   }
   T /= 2.; // average value.
   return T;
}

Float64 pgsHorizontalTieForceArtifact::GetTieResistance() const
{
   return m_As * m_fy;
}

bool pgsHorizontalTieForceArtifact::IsSymmetric() const
{
   return ::IsEqual(m_xp[pgsTypes::stLeft], m_xp[pgsTypes::stRight]) && ::IsEqual(m_yp[pgsTypes::stLeft], m_yp[pgsTypes::stRight]) && m_nf[pgsTypes::stLeft] == m_nf[pgsTypes::stRight];
}

bool pgsHorizontalTieForceArtifact::Passed() const
{
   // If this check is not applicable, return true. i.e. - you always pass this check
   if ( !m_bIsApplicable )
   {
      return true;
   }
   return GetTieForce() <= GetTieResistance();
}

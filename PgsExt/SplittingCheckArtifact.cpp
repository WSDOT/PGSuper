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
#include <PgsExt\SplittingCheckArtifact.h>
#include <PgsExt\SplittingCheckEngineer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsSplittingCheckArtifact
****************************************************************************/
pgsSplittingCheckArtifact::pgsSplittingCheckArtifact()
{
   Init();
}

pgsSplittingCheckArtifact::pgsSplittingCheckArtifact(const CSegmentKey& segmentKey) :
   m_SegmentKey(segmentKey)
{
   Init();
}

pgsSplittingCheckArtifact::~pgsSplittingCheckArtifact()
{
}

void pgsSplittingCheckArtifact::Init()
{
   for (int i = 0; i < 2; i++)
   {
      m_Aps[i].fill(0.0);
      m_Fpj[i].fill(0.0);
      m_dFpT[i].fill(0.0);
      m_Ps[i].fill(0.0);
   }
}

pgsTypes::SplittingDirection pgsSplittingCheckArtifact::GetSplittingDirection() const
{
   return m_SplittingDirection;
}

void pgsSplittingCheckArtifact::SetSplittingDirection(pgsTypes::SplittingDirection sd)
{
   m_SplittingDirection = sd;
}

Float64 pgsSplittingCheckArtifact::GetSplittingZoneLengthFactor() const
{
   return m_SplittingZoneLengthFactor;
}

void pgsSplittingCheckArtifact::SetSplittingZoneLengthFactor(Float64 bzlf)
{
   m_SplittingZoneLengthFactor = bzlf;
}

bool pgsSplittingCheckArtifact::Passed() const
{
   return Passed(pgsTypes::metStart) && Passed(pgsTypes::metEnd);
}

void pgsSplittingCheckArtifact::SetPointsOfInterest(const std::array<pgsPointOfInterest, 2>& pois)
{
   m_Pois = pois;
}

const pgsPointOfInterest& pgsSplittingCheckArtifact::GetPointOfInterest(pgsTypes::MemberEndType endType) const
{
   return m_Pois[endType];
}

Float64 pgsSplittingCheckArtifact::GetH(pgsTypes::MemberEndType endType) const
{
   return m_H[endType];
}

void pgsSplittingCheckArtifact::SetH(pgsTypes::MemberEndType endType,Float64 h)
{
   m_H[endType] = h;
}

Float64 pgsSplittingCheckArtifact::GetShearWidth(pgsTypes::MemberEndType endType) const
{
   return m_bv[endType];
}

void pgsSplittingCheckArtifact::SetShearWidth(pgsTypes::MemberEndType endType, Float64 bv)
{
   m_bv[endType] = bv;
}

Float64 pgsSplittingCheckArtifact::GetSplittingZoneLength(pgsTypes::MemberEndType endType) const
{
   return m_SplittingZoneLength[endType];
}

void pgsSplittingCheckArtifact::SetSplittingZoneLength(pgsTypes::MemberEndType endType,Float64 bzl)
{
   m_SplittingZoneLength[endType] = bzl;
}

Float64 pgsSplittingCheckArtifact::GetFs(pgsTypes::MemberEndType endType) const
{
   return m_fs[endType];
}

void pgsSplittingCheckArtifact::SetFs(pgsTypes::MemberEndType endType,Float64 fs)
{
   m_fs[endType] = fs;
}

void pgsSplittingCheckArtifact::SetAvs(pgsTypes::MemberEndType endType,Float64 avs)
{
   m_Avs[endType] = avs;
}

Float64 pgsSplittingCheckArtifact::GetAvs(pgsTypes::MemberEndType endType) const
{
   return m_Avs[endType];
}

Float64 pgsSplittingCheckArtifact::GetAps(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const
{
   if (strandType == pgsTypes::Permanent)
   {
      return m_Aps[endType][pgsTypes::Straight] + m_Aps[endType][pgsTypes::Harped];
   }
   else
   {
      return m_Aps[endType][strandType];
   }
}

void pgsSplittingCheckArtifact::SetAps(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType,Float64 aps)
{
   m_Aps[endType][strandType] = aps;
}

Float64 pgsSplittingCheckArtifact::GetFpj(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const
{
   if (strandType == pgsTypes::Permanent)
   {
      if (IsZero(m_Aps[endType][pgsTypes::Straight] + m_Aps[endType][pgsTypes::Harped]))
      {
         return 0;
      }
      else
      {
         return (m_Fpj[endType][pgsTypes::Straight] * m_Aps[endType][pgsTypes::Straight] + m_Fpj[endType][pgsTypes::Harped] * m_Aps[endType][pgsTypes::Harped]) / (m_Aps[endType][pgsTypes::Straight] + m_Aps[endType][pgsTypes::Harped]);
      }
   }
   else
   {
      return m_Fpj[endType][strandType];
   }
}

void pgsSplittingCheckArtifact::SetFpj(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType,Float64 fpj)
{
   m_Fpj[endType][strandType] = fpj;
}

Float64 pgsSplittingCheckArtifact::GetLossesAfterTransfer(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const
{
   if (strandType == pgsTypes::Permanent)
   {
      if (IsZero(m_Aps[endType][pgsTypes::Straight] + m_Aps[endType][pgsTypes::Harped]))
      {
         return 0;
      }
      else
      {
         return (m_dFpT[endType][pgsTypes::Straight] * m_Aps[endType][pgsTypes::Straight] + m_dFpT[endType][pgsTypes::Harped] * m_Aps[endType][pgsTypes::Harped]) / (m_Aps[endType][pgsTypes::Straight] + m_Aps[endType][pgsTypes::Harped]);
      }
   }
   else
   {
      return m_dFpT[endType][strandType];
   }
}

void pgsSplittingCheckArtifact::SetLossesAfterTransfer(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType,Float64 dFpT)
{
   m_dFpT[endType][strandType] = dFpT;
}

void pgsSplittingCheckArtifact::SetSplittingForce(pgsTypes::MemberEndType endType, pgsTypes::StrandType strandType, Float64 Ps)
{
   m_Ps[endType][strandType] = Ps;
}

Float64 pgsSplittingCheckArtifact::GetSplittingForce(pgsTypes::MemberEndType endType) const
{
   return GetSplittingForce(endType, pgsTypes::Permanent) + GetSplittingForce(endType, pgsTypes::Temporary);
}

Float64 pgsSplittingCheckArtifact::GetSplittingForce(pgsTypes::MemberEndType endType, pgsTypes::StrandType strandType) const
{
   if (strandType == pgsTypes::Permanent)
   {
      Float64 Ps = GetSplittingForce(endType, pgsTypes::Straight);
      Float64 Ph = GetSplittingForce(endType, pgsTypes::Harped);
      return Ps + Ph;
   }
   else
   {
      return m_Ps[endType][strandType];
   }
}

void pgsSplittingCheckArtifact::SetBurstingForce(pgsTypes::MemberEndType endType, Float64 Pb)
{
   m_Pb[endType] = Pb;
}

Float64 pgsSplittingCheckArtifact::GetBurstingForce(pgsTypes::MemberEndType endType) const
{
   return m_Pb[endType];
}

Float64 pgsSplittingCheckArtifact::GetSplittingResistance(pgsTypes::MemberEndType endType) const
{
   return m_Pr[endType];
}

void pgsSplittingCheckArtifact::SetSplittingResistance(pgsTypes::MemberEndType endType,Float64 p)
{
   m_Pr[endType] = p;
}

Float64 pgsSplittingCheckArtifact::GetSplittingDemand(pgsTypes::MemberEndType endType) const
{
   return Min(GetSplittingForce(endType), m_Pb[endType]);
}

Float64 pgsSplittingCheckArtifact::GetUHPCDesignTensileStrength() const
{
   return m_frr;
}

void pgsSplittingCheckArtifact::SetUHPCDesignTensileStrength(Float64 frr)
{
   m_frr = frr;
}

Float64 pgsSplittingCheckArtifact::GetTransferLength(pgsTypes::MemberEndType endType) const
{
   return m_lt[endType];
}

void pgsSplittingCheckArtifact::SetTransferLength(pgsTypes::MemberEndType endType, Float64 lt)
{
   m_lt[endType] = lt;
}

bool pgsSplittingCheckArtifact::Passed(pgsTypes::MemberEndType endType) const
{
   return GetSplittingForce(endType) <= GetSplittingResistance(endType);
}

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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include "ReactionLocation.h"

/// @brief Analysis artifact for LRFD 5.9.4.4.3 Horizontal Transverse Tension Tie Reinforcement check
/// Only applicable to LRFD 10th Edition and Later
/// Only applicable to single web beams with bottom flanges
class PGSEXTCLASS pgsHorizontalTieForceArtifact
{
public:
   pgsHorizontalTieForceArtifact(const ReactionLocation& location, pgsTypes::LimitState ls) : m_Location(location), m_LimitState(ls) {}
   pgsHorizontalTieForceArtifact(const pgsHorizontalTieForceArtifact& rOther) = default;
   ~pgsHorizontalTieForceArtifact() = default;

   pgsHorizontalTieForceArtifact& operator = (const pgsHorizontalTieForceArtifact& rOther) = default;

   void SetLimitState(pgsTypes::LimitState ls);
   pgsTypes::LimitState GetLimitState() const;

   void SetReactionLocation(const ReactionLocation& location);
   const ReactionLocation& GetReactionLocation() const;

   void IsApplicable(bool bIsApplicable);
   bool IsApplicable() const;

   void SetBearingWidth(Float64 bb);
   Float64 GetBearingWidth() const;

   void SetBottomBulbDepth(Float64 hb);
   Float64 GetBottomBulbDepth() const;

   void SetNumBondedStrandsInFlange(pgsTypes::SideType side,StrandIndexType nStrands);
   StrandIndexType GetNumBondedStrandsInFlange(pgsTypes::SideType side) const;

   void SetTotalNumBondedStrands(StrandIndexType nStrands);
   StrandIndexType GetTotalNumBondedStrands() const;

   void SetHorizDistance(pgsTypes::SideType side, Float64 xp);
   Float64 GetHorizDistance(pgsTypes::SideType side) const;

   void SetVertDistance(pgsTypes::SideType side, Float64 yp);
   Float64 GetVertDistance(pgsTypes::SideType side) const;

   void SetShearForce(Float64 Vu);
   Float64 GetShearForce() const;

   void SetPhi(Float64 phi);
   Float64 GetPhi() const;

   void SetTieArea(Float64 As);
   Float64 GetTieArea() const;

   void SetTieYieldStrength(Float64 fy);
   Float64 GetTieYieldStrength() const;


   // Get cb
   Float64 GetBearingReactionLocation(pgsTypes::SideType side) const;

   Float64 GetTieForce() const; 
   Float64 GetTieResistance() const;


   bool IsSymmetric() const;

   bool Passed() const;

private:
   ReactionLocation m_Location;
   pgsTypes::LimitState m_LimitState;
   bool m_bIsApplicable = false;
   Float64 m_bb;
   Float64 m_hb;
   StrandIndexType m_Nw;
   std::array<StrandIndexType, 2> m_nf;
   std::array<Float64, 2> m_xp;
   std::array<Float64, 2> m_yp;
   Float64 m_Vu;
   Float64 m_phi;
   Float64 m_As = 0;
   Float64 m_fy = 0;
};

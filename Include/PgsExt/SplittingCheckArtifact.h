///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <PgsExt\PointOfInterest.h>

class pgsSplittingCheckEngineer;
class rptChapter;
interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   pgsSplittingCheckArtifact

   Artifact that holds shear design/check results in the splitting zone.


DESCRIPTION
   Artifact that holds shear design/check results in the Splitting zone.

LOG
   rab : 12.29.1997 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsSplittingCheckArtifact
{
public:
   pgsSplittingCheckArtifact();
   pgsSplittingCheckArtifact(const CSegmentKey& segmentKey);
   ~pgsSplittingCheckArtifact();

   void SetSegmentKey(const CSegmentKey& segmentKey) { m_SegmentKey = segmentKey; }
   const CSegmentKey& GetSegmentKey() const { return m_SegmentKey; }


   pgsTypes::SplittingDirection GetSplittingDirection() const;
   void SetSplittingDirection(pgsTypes::SplittingDirection sd);
   Float64 GetSplittingZoneLengthFactor() const;
   void SetSplittingZoneLengthFactor(Float64 bzlf);

   void SetPointsOfInterest(const std::array<pgsPointOfInterest,2>& pois);
   const pgsPointOfInterest& GetPointOfInterest(pgsTypes::MemberEndType endType) const;

   Float64 GetH(pgsTypes::MemberEndType endType) const;
   void SetH(pgsTypes::MemberEndType endType,Float64 h);
   Float64 GetShearWidth(pgsTypes::MemberEndType endType) const;
   void SetShearWidth(pgsTypes::MemberEndType endType, Float64 bv);
   Float64 GetSplittingZoneLength(pgsTypes::MemberEndType endType) const;
   void SetSplittingZoneLength(pgsTypes::MemberEndType endType,Float64 bzl);
   Float64 GetFs(pgsTypes::MemberEndType endType) const;
   void SetFs(pgsTypes::MemberEndType endType,Float64 fs);
   Float64 GetAvs(pgsTypes::MemberEndType endType) const;
   void SetAvs(pgsTypes::MemberEndType endType,Float64 avs);
   Float64 GetAps(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const;
   void SetAps(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType, Float64 aps);
   Float64 GetFpj(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const;
   void SetFpj(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType, Float64 fpj);
   Float64 GetLossesAfterTransfer(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType) const;
   void SetLossesAfterTransfer(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType, Float64 dFpT);

   // This is the 0.04Ppo
   void SetSplittingForce(pgsTypes::MemberEndType endType,pgsTypes::StrandType strandType, Float64 Ps);
   Float64 GetSplittingForce(pgsTypes::MemberEndType endType, pgsTypes::StrandType strandType) const;
   Float64 GetSplittingForce(pgsTypes::MemberEndType endType) const;

   // This is the 0.021(h/lt)Ppo for PCI-UHPC
   void SetBurstingForce(pgsTypes::MemberEndType endType, Float64 Pb);
   Float64 GetBurstingForce(pgsTypes::MemberEndType endType) const;

   Float64 GetUHPCDesignTensileStrength() const;
   void SetUHPCDesignTensileStrength(Float64 frr);
   Float64 GetTransferLength(pgsTypes::MemberEndType endType) const;
   void SetTransferLength(pgsTypes::MemberEndType endType, Float64 lt);

   Float64 GetSplittingResistance(pgsTypes::MemberEndType endType) const;
   void SetSplittingResistance(pgsTypes::MemberEndType endType, Float64 p);

   // This is the controlling of Splitting Force (AASHTO) and Bursting Force (PCI UHPC)
   Float64 GetSplittingDemand(pgsTypes::MemberEndType endType) const;

   bool Passed(pgsTypes::MemberEndType end) const;
   bool Passed() const;

protected:

private:
   void Init();

   CSegmentKey m_SegmentKey;

   std::array<pgsPointOfInterest, 2> m_Pois;

   pgsTypes::SplittingDirection m_SplittingDirection{ pgsTypes::sdVertical };
   Float64 m_SplittingZoneLengthFactor{ 0.0 };
   Float64 m_frr{ 0.0 };

   // array index is pgsTypes::MemberEndType
   std::array<Float64, 2> m_SplittingZoneLength{ 0., 0. };
   std::array<Float64, 2> m_H{ 0., 0. };
   std::array<Float64, 2> m_lt{ 0., 0. }; // transfer length
   std::array<Float64, 2> m_bv{ 0., 0. };
   std::array<Float64, 2> m_Avs{ 0., 0. };
   std::array<std::array<Float64, 3>, 2> m_Aps;  //[endType][strandType]
   std::array<std::array<Float64, 3>, 2> m_Fpj; //[endType][strandType]
   std::array<std::array<Float64, 3>, 2> m_dFpT; //[endType][strandType]
   std::array<Float64, 2> m_fs{ 0., 0. }; // maximum bursting stress (20ksi)

   std::array<std::array<Float64, 3>, 2> m_Ps; // splitting force (0.04Ppo)
   std::array<Float64, 2> m_Pb{ Float64_Max, Float64_Max }; // bursting force (0.021(h/lt)Ppo) PCI UHPC - demand is minimum so make this value really high so it doesn't contrl if not used
   std::array<Float64, 2> m_Pr{ 0., 0. }; // resistance
};

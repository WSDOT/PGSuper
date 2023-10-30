///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

/*****************************************************************************
CLASS 
   pgsReinforcementFatigueArtifact

DESCRIPTION
   Artifact for checking reinforcement fatigue. This check is only for AASHTO UHPC
   concrete. See GS 1.5.3 and LRFD 5.5.3.1
*****************************************************************************/

class PGSEXTCLASS pgsReinforcementFatigueArtifact
{
public:
   pgsReinforcementFatigueArtifact();
   pgsReinforcementFatigueArtifact(const pgsReinforcementFatigueArtifact& rOther) = default;
   ~pgsReinforcementFatigueArtifact();

   pgsReinforcementFatigueArtifact& operator=(const pgsReinforcementFatigueArtifact& rOther) = default;

   void IsApplicable(bool bIsApplicable);
   bool IsApplicable() const;

   void SetLoadFactor(Float64 gamma);
   Float64 GetLoadFactor() const;

   void SetFatigueLiveLoadMoment(Float64 moment);
   Float64 GetFatigueLiveLoadMoment() const;

   void SetStrandEccentricity(Float64 e);
   Float64 GetStrandEccentricity() const;

   void SetMomentOfInertia(Float64 I);
   Float64 GetMomentOfInertia() const;

   void SetEps(Float64 Eps);
   Float64 GetEps() const;

   void SetEc(Float64 Ec);
   Float64 GetEc() const;

   void SetFatigueThreshold(Float64 deltaFth);
   Float64 GetFatigueThreshold() const;

   Float64 GetLiveLoadStressRange() const;

   bool Passed() const;

private:
   bool m_bIsApplicable{false};
   Float64 m_gamma;
   Float64 m_Mll;
   Float64 m_ecc; // eccentricity of the strand furthest from the Neutral Axis, not the ecc of the strand CG
   Float64 m_I;
   Float64 m_Eps;
   Float64 m_Ec;
   Float64 m_deltaFth;
};

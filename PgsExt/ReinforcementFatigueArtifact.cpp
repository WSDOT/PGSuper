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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\ReinforcementFatigueArtifact.h>


pgsReinforcementFatigueArtifact::pgsReinforcementFatigueArtifact()
{
}

pgsReinforcementFatigueArtifact::~pgsReinforcementFatigueArtifact()
{
}


void pgsReinforcementFatigueArtifact::IsApplicable(bool bIsApplicable)
{
   m_bIsApplicable = bIsApplicable;
}

bool pgsReinforcementFatigueArtifact::IsApplicable() const
{
   return m_bIsApplicable;
}

void pgsReinforcementFatigueArtifact::SetLoadFactor(Float64 gamma)
{
   m_gamma = gamma;
}

Float64 pgsReinforcementFatigueArtifact::GetLoadFactor() const
{
   return m_gamma;
}

void pgsReinforcementFatigueArtifact::SetFatigueLiveLoadMoment(Float64 moment)
{
   m_Mll = moment;
}

Float64 pgsReinforcementFatigueArtifact::GetFatigueLiveLoadMoment() const
{
   return m_Mll;
}

void pgsReinforcementFatigueArtifact::SetStrandEccentricity(Float64 e)
{
   m_ecc = e;
}

Float64 pgsReinforcementFatigueArtifact::GetStrandEccentricity() const
{
   return m_ecc;
}

void pgsReinforcementFatigueArtifact::SetMomentOfInertia(Float64 I)
{
   m_I = I;
}

Float64 pgsReinforcementFatigueArtifact::GetMomentOfInertia() const
{
   return m_I;
}

void pgsReinforcementFatigueArtifact::SetEps(Float64 Eps)
{
   m_Eps = Eps;
}

Float64 pgsReinforcementFatigueArtifact::GetEps() const
{
   return m_Eps;
}

void pgsReinforcementFatigueArtifact::SetEc(Float64 Ec)
{
   m_Ec = Ec;
}

Float64 pgsReinforcementFatigueArtifact::GetEc() const
{
   return m_Ec;
}

void pgsReinforcementFatigueArtifact::SetFatigueThreshold(Float64 deltaFth)
{
   m_deltaFth = deltaFth;
}

Float64 pgsReinforcementFatigueArtifact::GetFatigueThreshold() const
{
   return m_deltaFth;
}

Float64 pgsReinforcementFatigueArtifact::GetLiveLoadStressRange() const
{
   Float64 delta_f = -(m_Eps/m_Ec) * m_Mll * m_ecc / m_I;
   return delta_f;
}

bool pgsReinforcementFatigueArtifact::Passed() const
{
   // If this check is not applicable, return true. i.e. - you always pass this check
   if ( !m_bIsApplicable )
   {
      return true;
   }

   Float64 delta_f = GetLiveLoadStressRange();
   if (m_deltaFth < m_gamma*delta_f)
   {
      return false;
   }

   return true;
}

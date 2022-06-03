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
#include <PgsExt\PrincipalTensionStressArtifact.h>

pgsPrincipalTensionSectionArtifact::pgsPrincipalTensionSectionArtifact()
{
   m_StressLimit = -99999;
   m_fmax = -99999;
   m_Yg = -99999;
   m_strWebLocation = _T("Undefined");
   m_fcReqd = NO_AVAILABLE_CONCRETE_STRENGTH;
}

pgsPrincipalTensionSectionArtifact::pgsPrincipalTensionSectionArtifact(const pgsPointOfInterest& poi, Float64 fAllow, Float64 fmax, Float64 Yg, LPCTSTR lpszWebLocation, Float64 fcReqd)
{
   Init(poi, fAllow, fmax, Yg, lpszWebLocation, fcReqd);
}

void pgsPrincipalTensionSectionArtifact::Init(const pgsPointOfInterest& poi, Float64 fAllow, Float64 fmax, Float64 Yg, LPCTSTR lpszWebLocation, Float64 fcReqd)
{
   m_Poi = poi;
   m_StressLimit = fAllow;
   m_fmax = fmax;
   m_Yg = Yg;
   m_strWebLocation = lpszWebLocation;
   m_fcReqd = fcReqd;
}

const pgsPointOfInterest& pgsPrincipalTensionSectionArtifact::GetPointOfInterest() const
{
   return m_Poi;
}

void pgsPrincipalTensionSectionArtifact::GetfmaxDetails(Float64* pFmax, Float64* pYg, std::_tstring* pstrWebLocation) const
{
   *pFmax = m_fmax;
   *pYg = m_Yg;
   *pstrWebLocation = m_strWebLocation;
}

Float64 pgsPrincipalTensionSectionArtifact::Getfmax() const
{
   return m_fmax;
}

Float64 pgsPrincipalTensionSectionArtifact::GetWebElevation() const
{
   return m_Yg;
}

LPCTSTR pgsPrincipalTensionSectionArtifact::GetWebLocation() const
{
   return m_strWebLocation.c_str();
}

Float64 pgsPrincipalTensionSectionArtifact::GetRequiredConcreteStrength() const
{
   return m_fcReqd;
}

Float64 pgsPrincipalTensionSectionArtifact::GetStressLimit() const
{
   return m_StressLimit;
}

bool pgsPrincipalTensionSectionArtifact::Passed() const
{
   return IsLE(m_fmax, m_StressLimit) ? true : false;
}


///////////////////////////////////////////////////////////////////////////////////////

pgsPrincipalTensionStressArtifact::pgsPrincipalTensionStressArtifact() :
   m_Applicability(Applicable)
{
}

void pgsPrincipalTensionStressArtifact::AddPrincipalTensionStressArtifact(const pgsPrincipalTensionSectionArtifact& artifact)
{
   ATLASSERT(artifact.GetPointOfInterest().GetID() != INVALID_ID); // poi must have a valid ID
   m_Artifacts.emplace_back(artifact);
   std::sort(std::begin(m_Artifacts), std::end(m_Artifacts), [](const auto& a, const auto& b) {return a.GetPointOfInterest() < b.GetPointOfInterest(); });
}

CollectionIndexType pgsPrincipalTensionStressArtifact::GetPrincipalTensionStressArtifactCount() const
{
   return m_Artifacts.size();
}

const pgsPrincipalTensionSectionArtifact* pgsPrincipalTensionStressArtifact::GetPrincipalTensionStressArtifact(CollectionIndexType idx) const
{
   if (m_Artifacts.size() <= idx)
   {
      return nullptr;
   }
   else
   {
      return &m_Artifacts[idx];
   }
}

pgsPrincipalTensionSectionArtifact* pgsPrincipalTensionStressArtifact::GetPrincipalTensionStressArtifact(CollectionIndexType idx)
{
   if (m_Artifacts.size() <= idx)
   {
      return nullptr;
   }
   else
   {
      return &m_Artifacts[idx];
   }
}

const pgsPrincipalTensionSectionArtifact* pgsPrincipalTensionStressArtifact::GetPrincipalTensionStressArtifactAtPoi(PoiIDType poiID) const
{
   for (const auto& artifact : m_Artifacts)
   {
      if (artifact.GetPointOfInterest().GetID() == poiID)
      {
         return &artifact;
      }
   }

   return nullptr;
}

const std::vector<pgsPrincipalTensionSectionArtifact>* pgsPrincipalTensionStressArtifact::GetPrincipalTensionStressArtifacts() const
{
   return &m_Artifacts;
}

Float64 pgsPrincipalTensionStressArtifact::GetRequiredSegmentConcreteStrength() const
{
   Float64 fc_reqd = -1;
   for (const auto& artifact : m_Artifacts)
   {
      if (!artifact.GetPointOfInterest().HasAttribute(POI_CLOSURE))
      {
         fc_reqd = Max(fc_reqd, artifact.GetRequiredConcreteStrength());
      }
   }
   return fc_reqd;
}

Float64 pgsPrincipalTensionStressArtifact::GetRequiredClosureJointConcreteStrength() const
{
   Float64 fc_reqd = -1;
   for (const auto& artifact : m_Artifacts)
   {
      if (artifact.GetPointOfInterest().HasAttribute(POI_CLOSURE))
      {
         fc_reqd = Max(fc_reqd, artifact.GetRequiredConcreteStrength());
      }
   }
   return fc_reqd;
}

void pgsPrincipalTensionStressArtifact::SetApplicablity(pgsPrincipalTensionStressArtifact::Applicability applicability)
{
   m_Applicability = applicability;
}

pgsPrincipalTensionStressArtifact::Applicability pgsPrincipalTensionStressArtifact::GetApplicability() const
{
   return m_Applicability;
}

bool pgsPrincipalTensionStressArtifact::IsApplicable() const
{
   return m_Applicability == Applicable ? true : false;
}

bool pgsPrincipalTensionStressArtifact::Passed() const
{
   if (IsApplicable())
   {
      for (const auto& artifact : m_Artifacts)
      {
         if (!artifact.Passed())
         {
            return false;
         }
      }
   }

   return true;
}

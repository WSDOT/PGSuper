///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <PgsExt\PointOfInterest.h>

class PGSEXTCLASS pgsPrincipalTensionSectionArtifact
{
public:
   pgsPrincipalTensionSectionArtifact();
   pgsPrincipalTensionSectionArtifact(const pgsPointOfInterest& poi, Float64 fAllow, Float64 fmax, Float64 Yg, LPCTSTR lpszWebLocation, Float64 fcReqd);

   void Init(const pgsPointOfInterest& poi, Float64 fAllow, Float64 fmax, Float64 Yg, LPCTSTR lpszWebLocation, Float64 fcReqd);

   const pgsPointOfInterest& GetPointOfInterest() const;
   void GetfmaxDetails(Float64* pFmax, Float64* pYg, std::_tstring* pstrWebLocation) const;
   Float64 Getfmax() const;
   Float64 GetWebElevation() const;
   LPCTSTR GetWebLocation() const;
   Float64 GetStressLimit() const;
   Float64 GetRequiredConcreteStrength() const;

   bool Passed() const;

private:
   pgsPointOfInterest m_Poi; // location where prinicpal tension is evaluted
   Float64 m_StressLimit; // stress limit at this section
   Float64 m_fmax; // controlling stress
   Float64 m_Yg; // web elevation of controlling stress
   std::_tstring m_strWebLocation; // description of web elevation of controlling stress
   Float64 m_fcReqd;
};

class PGSEXTCLASS pgsPrincipalTensionStressArtifact
{
public:
   enum Applicability
   {
      Applicable, // check is applicable
      Specification, // check is not applicable because specification is before 8th Edition
      ConcreteStrength // check is not applicable because concrete strength does not exceed threshold value
   };
   pgsPrincipalTensionStressArtifact();
   pgsPrincipalTensionStressArtifact(const pgsPrincipalTensionStressArtifact& other) = default;
   pgsPrincipalTensionStressArtifact& operator=(pgsPrincipalTensionStressArtifact& other) = default;

   void AddPrincipalTensionStressArtifact(const pgsPrincipalTensionSectionArtifact& artifact);
   CollectionIndexType GetPrincipalTensionStressArtifactCount() const;
   const pgsPrincipalTensionSectionArtifact* GetPrincipalTensionStressArtifact(CollectionIndexType idx) const;
   pgsPrincipalTensionSectionArtifact* GetPrincipalTensionStressArtifact(CollectionIndexType idx);
   const pgsPrincipalTensionSectionArtifact* GetPrincipalTensionStressArtifactAtPoi(PoiIDType poiID) const;
   const std::vector<pgsPrincipalTensionSectionArtifact>* GetPrincipalTensionStressArtifacts() const;

   Float64 GetRequiredSegmentConcreteStrength() const;
   Float64 GetRequiredClosureJointConcreteStrength() const;

   void SetApplicablity(Applicability applicability);
   Applicability GetApplicability() const;
   bool IsApplicable() const;

   bool Passed() const;

private:
   Applicability m_Applicability;
   std::vector<pgsPrincipalTensionSectionArtifact> m_Artifacts;
};

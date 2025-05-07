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

#include <Details.h>
#include <IFace\PrincipalWebStress.h>

class pgsPrincipalWebStressEngineer
{
public:
   pgsPrincipalWebStressEngineer();
   pgsPrincipalWebStressEngineer(std::weak_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID);
   pgsPrincipalWebStressEngineer(const pgsPrincipalWebStressEngineer& other) = default;

   void SetBroker(std::weak_ptr<WBFL::EAF::Broker> pBroker);
   void SetStatusGroupID(StatusGroupIDType statusGroupID);

   const PRINCIPALSTRESSINWEBDETAILS* GetPrincipalStressInWeb(const pgsPointOfInterest& poi) const;

   const std::vector<TimeStepCombinedPrincipalWebStressDetailsAtWebSection>* GetTimeStepPrincipalWebStressDetails(const pgsPointOfInterest& poi, IntervalIndexType interval) const;

   void Check(const PoiList& vPois, pgsPrincipalTensionStressArtifact* pArtifact) const;

private:
   std::weak_ptr<WBFL::EAF::Broker> m_pBroker;
   inline std::shared_ptr<WBFL::EAF::Broker> GetBroker() const { return m_pBroker.lock(); }

   StatusGroupIDType m_StatusGroupID;

   void CheckTimeStep(const PoiList& vPois, pgsPrincipalTensionStressArtifact* pArtifact) const;
   void CheckSimpleLosses(const PoiList& vPois, pgsPrincipalTensionStressArtifact* pArtifact) const;

   // non-time step
   mutable std::map<PoiIDType, PRINCIPALSTRESSINWEBDETAILS> m_Details;

   // time-step
   // map index is hashed. see in cpp for details
   mutable std::map<IndexType, std::vector<TimeStepCombinedPrincipalWebStressDetailsAtWebSection>> m_TimeStepDetails;

   PRINCIPALSTRESSINWEBDETAILS ComputePrincipalStressInWeb(const pgsPointOfInterest& poi) const;

   std::vector<TimeStepCombinedPrincipalWebStressDetailsAtWebSection> ComputeTimeStepPrincipalWebStressDetails(const pgsPointOfInterest& poi, IntervalIndexType interval) const;

   WBFL::System::SectionValue GetNonCompositeShear(pgsTypes::BridgeAnalysisType bat, IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi) const;
   void GetCompositeShear(pgsTypes::BridgeAnalysisType bat, IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, WBFL::System::SectionValue* pVmin, WBFL::System::SectionValue* pVmax) const;
};

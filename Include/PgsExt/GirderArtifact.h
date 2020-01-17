///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include <PgsExt\SegmentArtifact.h>
#include <PgsExt\TendonStressArtifact.h>
#include <PgsExt\DuctSizeArtifact.h>
#include <set>

//////////////////////////////////////////////
// pgsGirderArtifact
//
// Code check artifact for a girder.
// Includes segment check artifacts for each segment in the girder

class PGSEXTCLASS pgsGirderArtifact
{
public:
   pgsGirderArtifact(const CGirderKey& girderKey);
   pgsGirderArtifact(const pgsGirderArtifact& other);
   pgsGirderArtifact& operator = (const pgsGirderArtifact& rOther);

   const CGirderKey& GetGirderKey() const;


   ////////////////////////
   // Flexural Capacity (Mu) artifacts
   ////////////////////////

   // add positive and negative moment capacity artifacts
   void AddPositiveMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx, pgsTypes::LimitState ls, const pgsFlexuralCapacityArtifact& artifact);
   void AddNegativeMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx, pgsTypes::LimitState ls, const pgsFlexuralCapacityArtifact& artifact);

   // get the number of artifacts
   CollectionIndexType GetPositiveMomentFlexuralCapacityArtifactCount(IntervalIndexType intervalIdx, pgsTypes::LimitState ls) const;
   CollectionIndexType GetNegativeMomentFlexuralCapacityArtifactCount(IntervalIndexType intervalIdx, pgsTypes::LimitState ls) const;

   // get an artifact
   const pgsFlexuralCapacityArtifact* GetPositiveMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,CollectionIndexType artifactIdx) const;
   const pgsFlexuralCapacityArtifact* GetNegativeMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,CollectionIndexType artifactIdx) const;

   // find an artifact for the specified POI
   const pgsFlexuralCapacityArtifact* FindPositiveMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi) const;
   const pgsFlexuralCapacityArtifact* FindNegativeMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi) const;

   //////////////////
   // Segment artifacts
   //////////////////

   // adds a segment artifact to this girder artifact
   void AddSegmentArtifact(const pgsSegmentArtifact& artifact);

   // returns a previously stored segment artifact. returns nullptr if
   // a previously stored one does not exist
   const pgsSegmentArtifact* GetSegmentArtifact(SegmentIndexType segIdx) const;

   // returns a previously stored segment artifact. creates a new segment artifact
   // if a previously stored one does not exist
   pgsSegmentArtifact* GetSegmentArtifact(SegmentIndexType segIdx);

   
   void SetConstructabilityArtifact(const pgsConstructabilityArtifact& artifact);
   const pgsConstructabilityArtifact* GetConstructabilityArtifact() const;
   pgsConstructabilityArtifact* GetConstructabilityArtifact();

   void SetTendonStressArtifact(DuctIndexType ductIdx,const pgsTendonStressArtifact& artifact);
   const pgsTendonStressArtifact* GetTendonStressArtifact(DuctIndexType ductIdx) const;
   pgsTendonStressArtifact* GetTendonStressArtifact(DuctIndexType ductIdx);

   void SetDuctSizeArtifact(DuctIndexType ductIdx,const pgsDuctSizeArtifact& artifact);
   const pgsDuctSizeArtifact* GetDuctSizeArtifact(DuctIndexType ductIdx) const;
   pgsDuctSizeArtifact* GetDuctSizeArtifact(DuctIndexType ductIdx);

   void AddDeflectionCheckArtifact(const pgsDeflectionCheckArtifact& artifact);
   IndexType GetDeflectionCheckArtifactCount();
   pgsDeflectionCheckArtifact* GetDeflectionCheckArtifact(IndexType idx);
   const pgsDeflectionCheckArtifact* GetDeflectionCheckArtifact(IndexType idx) const;


   // Returns true if a "with rebar" allowable tension stress was used anywhere along the girder
   bool WasWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation) const;
   bool WasGirderWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const;
   bool WasDeckWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const;

   // Returns true if a "with rebar" allowable tension stress limit was applicable anywhere along the girder
   // this is independent of whether or not it was actually used in the stress evaluation
   bool IsWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation) const;
   bool IsGirderWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation) const;
   bool IsDeckWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation) const;

   // Returns true if a flexural stress check is applicable anywhere along the girder
   bool IsFlexuralStressCheckApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stressType,pgsTypes::StressLocation stressLocation) const;


   Float64 GetRequiredGirderConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState) const;
   Float64 GetRequiredGirderConcreteStrength() const;
   Float64 GetRequiredDeckConcreteStrength() const;
   Float64 GetRequiredDeckConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState) const;
   Float64 GetRequiredReleaseStrength() const;

   bool Passed() const;

protected:
   void MakeCopy(const pgsGirderArtifact& rOther);
   void MakeAssignment(const pgsGirderArtifact& rOther);

private:
   CGirderKey m_GirderKey;
   std::map<DuctIndexType,pgsTendonStressArtifact> m_TendonStressArtifacts;
   std::map<DuctIndexType,pgsDuctSizeArtifact> m_DuctSizeArtifacts;

   typedef std::map<IntervalIndexType, std::vector<pgsFlexuralCapacityArtifact>> FlexuralCapacityContainer;
   FlexuralCapacityContainer m_FlexuralCapacityArtifacts[2][pgsTypes::LimitStateCount]; // pos=0,neg=1
   void AddFlexuralCapacityArtifact(FlexuralCapacityContainer* pArtifacts, IntervalIndexType intervalIdx, const pgsFlexuralCapacityArtifact& artifact);
   CollectionIndexType GetFlexuralCapacityArtifactCount(const FlexuralCapacityContainer* pArtifacts, IntervalIndexType intervalIdx) const;
   const pgsFlexuralCapacityArtifact* GetFlexuralCapacityArtifact(const FlexuralCapacityContainer* pArtifacts, IntervalIndexType intervalIdx, CollectionIndexType artifactIdx) const;
   const pgsFlexuralCapacityArtifact* FindFlexuralCapacityArtifact(const FlexuralCapacityContainer* pArtifacts, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const;

   std::map<CSegmentKey,pgsSegmentArtifact> m_SegmentArtifacts;

   pgsConstructabilityArtifact m_ConstructabilityArtifact;

   std::vector<pgsDeflectionCheckArtifact> m_DeflectionCheckArtifact;
};

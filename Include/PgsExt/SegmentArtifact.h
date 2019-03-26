///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include <PgsExt\StrandStressArtifact.h>
#include <PgsExt\FlexuralStressArtifact.h>
#include <PgsExt\FlexuralCapacityArtifact.h>
#include <PGSExt\StirrupCheckArtifact.h>
#include <PgsExt\PoiArtifactKey.h>
#include <PgsExt\StrandSlopeArtifact.h>
#include <PgsExt\HoldDownForceArtifact.h>
#include <PgsExt\ConstructabilityArtifact.h>
#include <PgsExt\SegmentStabilityArtifact.h>
#include <PgsExt\PrecastIGirderDetailingArtifact.h>
#include <PgsExt\HaulingAnalysisArtifact.h>
#include <PgsExt\DebondArtifact.h>
#include <PgsExt\StirrupCheckAtZonesArtifact.h>
#include <PgsExt\DeflectionCheckArtifact.h>

#include <Stability\Stability.h>

#include <map>


/*****************************************************************************
   pgsSegmentArtifact

   Code check artifact for a prestressed girder segment.
*****************************************************************************/
class PGSEXTCLASS pgsSegmentArtifact
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsSegmentArtifact(const CSegmentKey& segmentKey);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsSegmentArtifact(const pgsSegmentArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsSegmentArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsSegmentArtifact& operator = (const pgsSegmentArtifact& rOther);
   bool operator<(const pgsSegmentArtifact& rOther) const;

   // GROUP: OPERATIONS
   // GROUP: ACCESS

   //------------------------------------------------------------------------
   // Sets the strand stress artifact for this artifact
   void SetStrandStressArtifact(const pgsStrandStressArtifact& artifact);

   //------------------------------------------------------------------------
   // Returns a pointer to the strand stress artifact.
   const pgsStrandStressArtifact* GetStrandStressArtifact() const;
   pgsStrandStressArtifact* GetStrandStressArtifact();

   void SetStrandSlopeArtifact(const pgsStrandSlopeArtifact& artifact);
   const pgsStrandSlopeArtifact* GetStrandSlopeArtifact() const;
   pgsStrandSlopeArtifact* GetStrandSlopeArtifact();

   void SetHoldDownForceArtifact(const pgsHoldDownForceArtifact& artifact);
   const pgsHoldDownForceArtifact* GetHoldDownForceArtifact() const;
   pgsHoldDownForceArtifact* GetHoldDownForceArtifact();

   void AddFlexuralStressArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress,
                                  const pgsFlexuralStressArtifact& artifact);
   CollectionIndexType GetFlexuralStressArtifactCount(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress) const;
   const pgsFlexuralStressArtifact* GetFlexuralStressArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress,CollectionIndexType idx) const;
   pgsFlexuralStressArtifact* GetFlexuralStressArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress,CollectionIndexType idx);
   const pgsFlexuralStressArtifact* GetFlexuralStressArtifactAtPoi(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress,PoiIDType poiID) const;

   pgsStirrupCheckArtifact* GetStirrupCheckArtifact();
   const pgsStirrupCheckArtifact* GetStirrupCheckArtifact() const;

   pgsPrecastIGirderDetailingArtifact* GetPrecastIGirderDetailingArtifact();
   const pgsPrecastIGirderDetailingArtifact* GetPrecastIGirderDetailingArtifact() const;

   void SetSegmentStabilityArtifact(const pgsSegmentStabilityArtifact& artifact);
   const pgsSegmentStabilityArtifact* GetSegmentStabilityArtifact() const;
   pgsSegmentStabilityArtifact* GetSegmentStabilityArtifact();

   void SetLiftingCheckArtifact(const stbLiftingCheckArtifact* artifact);
   const stbLiftingCheckArtifact* GetLiftingCheckArtifact() const;

   void SetHaulingAnalysisArtifact(const pgsHaulingAnalysisArtifact*  artifact);
   const pgsHaulingAnalysisArtifact* GetHaulingAnalysisArtifact() const;

   // Returns true if flexural stress checks are applicable anywhere along the segment
   bool IsFlexuralStressCheckApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stressType,pgsTypes::StressLocation stressLocation) const;

   // returns true if the allowable tension capacity with adequate reinforcement
   // was used at any POI in this segment. If attribute = 0, only segments are checked
   // if attribute is POI_CLOSURE, only closure joints are checked
   bool WasWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation,PoiAttributeType attribute = 0) const;

   bool WasSegmentWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const;
   bool WasClosureJointWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bIsInPTZ) const;

   // returns true if the allowable tension capacity with adequate reinforcement was used
   // anywhere along this segment for the deck
   bool WasDeckWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const;


   // returns true if the allowable tension capacity with adequate reinforcement
   // is applicable at any POI in this segment. If attribute = 0, only segments are checked
   // if attribute is POI_CLOSURE, only closure joints are checked
   bool IsWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation,PoiAttributeType attribute = 0) const;

   bool IsSegmentWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const;
   bool IsClosureJointWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bIsInPTZ) const;

   // returns true if the allowable tension capacity with adequate reinforcement is applicable
   // anywhere along this segment for the deck
   bool IsDeckWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const;

   pgsDebondArtifact* GetDebondArtifact();
   const pgsDebondArtifact* GetDebondArtifact() const;
   
   bool Passed() const;

   bool DidSegmentFlexuralStressesPass() const;
   bool DidDeckFlexuralStressesPass() const;

   Float64 GetRequiredReleaseStrength() const;

   Float64 GetRequiredSegmentConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const;
   Float64 GetRequiredSegmentConcreteStrength() const;
   Float64 GetRequiredClosureJointConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const;
   Float64 GetRequiredClosureJointConcreteStrength() const;
   Float64 GetRequiredDeckConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const;
   Float64 GetRequiredDeckConcreteStrength() const;

   const CSegmentKey& GetSegmentKey() const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsSegmentArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsSegmentArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   CSegmentKey m_SegmentKey;

   pgsStrandStressArtifact     m_StrandStressArtifact;
   pgsStrandSlopeArtifact      m_StrandSlopeArtifact;
   pgsHoldDownForceArtifact    m_HoldDownForceArtifact;
   pgsSegmentStabilityArtifact m_StabilityArtifact;

   struct StressKey
   {
      IntervalIndexType intervalIdx;
      pgsTypes::LimitState ls;
      pgsTypes::StressType stress;
      bool operator<(const StressKey& key) const
      {
         if ( intervalIdx < key.intervalIdx )
            return true;

         if ( key.intervalIdx < intervalIdx )
            return false;

         if ( ls < key.ls )
            return true;

         if ( key.ls < ls )
            return false;

         if ( stress < key.stress )
            return true;

         if ( key.stress < stress )
            return false;

         return false;
      }
   };
   mutable std::map<StressKey,std::vector<pgsFlexuralStressArtifact>> m_FlexuralStressArtifacts;
   std::vector<pgsFlexuralStressArtifact>& GetFlexuralStressArtifacts(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress) const;
   bool DidFlexuralStressPass() const;

   pgsStirrupCheckArtifact m_StirrupCheckArtifact;

   pgsPrecastIGirderDetailingArtifact m_PrecastIGirderDetailingArtifact;

   const stbLiftingCheckArtifact* m_pLiftingCheckArtifact; // point is not owned by this object
   const pgsHaulingAnalysisArtifact* m_pHaulingAnalysisArtifact; // pointer is not owned by this object

   pgsDebondArtifact m_DebondArtifact;
};

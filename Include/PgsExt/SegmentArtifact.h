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
#include <PgsExt\StrandStressArtifact.h>
#include <PgsExt\FlexuralStressArtifact.h>
#include <PgsExt\FlexuralCapacityArtifact.h>
#include <PGSExt\StirrupCheckArtifact.h>
#include <PgsExt\PoiArtifactKey.h>
#include <PgsExt\StrandSlopeArtifact.h>
#include <PgsExt\HoldDownForceArtifact.h>
#include <PgsExt\PlantHandlingWeightArtifact.h>
#include <PgsExt\ConstructabilityArtifact.h>
#include <PgsExt\SegmentStabilityArtifact.h>
#include <PgsExt\PrecastIGirderDetailingArtifact.h>
#include <PgsExt\HaulingAnalysisArtifact.h>
#include <PgsExt\DebondArtifact.h>
#include <PgsExt\StirrupCheckAtZonesArtifact.h>
#include <PgsExt\DeflectionCheckArtifact.h>
#include <PgsExt\TendonStressArtifact.h>
#include <PgsExt\DuctSizeArtifact.h>
#include <PgsExt\PrincipalTensionStressArtifact.h>
#include <PgsExt\ReinforcementFatigueArtifact.h>
#include <PgsExt\DeckReinforcementArtifact.h>

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

   void SetPlantHandlingWeightArtifact(const pgsPlantHandlingWeightArtifact& artifact);
   const pgsPlantHandlingWeightArtifact* GetPlantHandlingWeightArtifact() const;
   pgsPlantHandlingWeightArtifact* GetPlantHandlingWeightArtifact();

   void AddFlexuralStressArtifact(const pgsFlexuralStressArtifact& artifact);
   IndexType GetFlexuralStressArtifactCount(const StressCheckTask& task) const;
   const pgsFlexuralStressArtifact* GetFlexuralStressArtifact(const StressCheckTask& task,IndexType idx) const;
   pgsFlexuralStressArtifact* GetFlexuralStressArtifact(const StressCheckTask& task,IndexType idx);
   const pgsFlexuralStressArtifact* GetFlexuralStressArtifactAtPoi(const StressCheckTask& task,PoiIDType poiID) const;

   pgsStirrupCheckArtifact* GetStirrupCheckArtifact();
   const pgsStirrupCheckArtifact* GetStirrupCheckArtifact() const;

   pgsPrecastIGirderDetailingArtifact* GetPrecastIGirderDetailingArtifact();
   const pgsPrecastIGirderDetailingArtifact* GetPrecastIGirderDetailingArtifact() const;

   void SetSegmentStabilityArtifact(const pgsSegmentStabilityArtifact& artifact);
   const pgsSegmentStabilityArtifact* GetSegmentStabilityArtifact() const;
   pgsSegmentStabilityArtifact* GetSegmentStabilityArtifact();

   void SetLiftingCheckArtifact(std::shared_ptr<const WBFL::Stability::LiftingCheckArtifact> artifact);
   std::shared_ptr<const WBFL::Stability::LiftingCheckArtifact> GetLiftingCheckArtifact() const;

   void SetHaulingAnalysisArtifact(std::shared_ptr<const pgsHaulingAnalysisArtifact>  artifact);
   std::shared_ptr<const pgsHaulingAnalysisArtifact> GetHaulingAnalysisArtifact() const;

   const pgsPrincipalTensionStressArtifact* GetPrincipalTensionStressArtifact() const;
   pgsPrincipalTensionStressArtifact* GetPrincipalTensionStressArtifact();

   const pgsReinforcementFatigueArtifact* GetReinforcementFatigueArtifact() const;
   pgsReinforcementFatigueArtifact* GetReinforcementFatigueArtifact();

   void SetTendonStressArtifact(DuctIndexType ductIdx, const pgsTendonStressArtifact& artifact);
   const pgsTendonStressArtifact* GetTendonStressArtifact(DuctIndexType ductIdx) const;
   pgsTendonStressArtifact* GetTendonStressArtifact(DuctIndexType ductIdx);

   void SetDuctSizeArtifact(DuctIndexType ductIdx, const pgsDuctSizeArtifact& artifact);
   const pgsDuctSizeArtifact* GetDuctSizeArtifact(DuctIndexType ductIdx) const;
   pgsDuctSizeArtifact* GetDuctSizeArtifact(DuctIndexType ductIdx);

   // returns true if the allowable tension capacity with adequate reinforcement
   // was used at any POI in this segment. If attribute = 0, only segments are checked
   // if attribute is POI_CLOSURE, only closure joints are checked
   bool WasWithRebarAllowableStressUsed(const StressCheckTask& task,pgsTypes::StressLocation stressLocation,PoiAttributeType attribute = 0) const;

   bool WasSegmentWithRebarAllowableStressUsed(const StressCheckTask& task) const;
   bool WasClosureJointWithRebarAllowableStressUsed(const StressCheckTask& task,bool bIsInPTZ) const;

   // returns true if the allowable tension capacity with adequate reinforcement was used
   // anywhere along this segment for the deck
   bool WasDeckWithRebarAllowableStressUsed(const StressCheckTask& task) const;


   // returns true if the allowable tension capacity with adequate reinforcement
   // is applicable at any POI in this segment. If attribute = 0, only segments are checked
   // if attribute is POI_CLOSURE, only closure joints are checked
   bool IsWithRebarAllowableStressApplicable(const StressCheckTask& task,pgsTypes::StressLocation stressLocation,PoiAttributeType attribute = 0) const;

   bool IsSegmentWithRebarAllowableStressApplicable(const StressCheckTask& task) const;
   bool IsClosureJointWithRebarAllowableStressApplicable(const StressCheckTask& task,bool bIsInPTZ) const;

   // returns true if the allowable tension capacity with adequate reinforcement is applicable
   // anywhere along this segment for the deck
   bool IsDeckWithRebarAllowableStressApplicable(const StressCheckTask& task) const;

   pgsDebondArtifact* GetDebondArtifact();
   const pgsDebondArtifact* GetDebondArtifact() const;

   pgsDeckReinforcementCheckArtifact* GetDeckReinforcementCheckArtifact();
   const pgsDeckReinforcementCheckArtifact* GetDeckReinforcementCheckArtifact() const;
   
   bool Passed() const;

   bool DidSegmentFlexuralStressesPass() const;
   bool DidDeckFlexuralStressesPass() const;

   Float64 GetRequiredReleaseStrength() const;

   Float64 GetRequiredSegmentConcreteStrength(const StressCheckTask& task) const;
   Float64 GetRequiredSegmentConcreteStrength() const;
   Float64 GetRequiredClosureJointConcreteStrength(const StressCheckTask& task) const;
   Float64 GetRequiredClosureJointConcreteStrength() const;
   Float64 GetRequiredDeckConcreteStrength(const StressCheckTask& task) const;
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
   pgsPlantHandlingWeightArtifact m_PlantHandlingWeightArtifact;
   pgsSegmentStabilityArtifact m_StabilityArtifact;
   pgsReinforcementFatigueArtifact m_ReinforcementFatigueArtifact;

   mutable std::map<StressCheckTask,std::vector<pgsFlexuralStressArtifact>> m_FlexuralStressArtifacts;
   std::vector<pgsFlexuralStressArtifact>& GetFlexuralStressArtifacts(const StressCheckTask& task) const;
   bool DidFlexuralStressPass() const;

   pgsStirrupCheckArtifact m_StirrupCheckArtifact;

   pgsPrecastIGirderDetailingArtifact m_PrecastIGirderDetailingArtifact;

   std::map<DuctIndexType, pgsTendonStressArtifact> m_TendonStressArtifacts;
   std::map<DuctIndexType, pgsDuctSizeArtifact> m_DuctSizeArtifacts;

   std::shared_ptr<const WBFL::Stability::LiftingCheckArtifact> m_pLiftingCheckArtifact;
   std::shared_ptr<const pgsHaulingAnalysisArtifact> m_pHaulingAnalysisArtifact;

   pgsDebondArtifact m_DebondArtifact;

   mutable pgsPrincipalTensionStressArtifact m_PrincipalTensionStressArtifact;
   bool DidPrincipalTensionStressPass() const;

   pgsDeckReinforcementCheckArtifact m_DeckReinforcementCheckArtifact;
};

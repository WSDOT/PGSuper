///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <map>
#include <PsgLib\ShearZoneData.h>
#include <PgsExt\Keys.h>
#include <PgsExt\PrecastSegmentData.h>

#include <Material\ConcreteEx.h>
#include <PGSuperTypes.h>

interface IBroker;

/*****************************************************************************
CLASS 
   pgsSegmentDesignArtifact

   Artifact that contains the results of a design attempt.


DESCRIPTION
   Artifact that contains the results of a design attempt.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 12.09.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsSegmentDesignArtifact
{
public:
   // GROUP: ENUMERATORS
   enum Outcome
   {
      Success,
      SuccessButLongitudinalBarsNeeded4FlexuralTensionCy,
      SuccessButLongitudinalBarsNeeded4FlexuralTensionLifting,
      SuccessButLongitudinalBarsNeeded4FlexuralTensionHauling,
      TooManyStrandsReqd,
      ReleaseStrength,
      OverReinforced,
      UnderReinforced,
      UltimateMomentCapacity,
      StrandSlopeOutOfRange,
      ExceededMaxHoldDownForce,
      ShearExceedsMaxConcreteStrength,
      TooManyStirrupsReqd,
      TooManyStirrupsReqdForHorizontalInterfaceShear,
      TooManyStirrupsReqdForSplitting,
      ConflictWithLongReinforcementShearSpec,
      TooManyBarsForLongReinfShear,
      TooMuchStrandsForLongReinfShear,
      StrandsReqdForLongReinfShearAndFlexureTurnedOff,
      NoDevelopmentLengthForLongReinfShear,
      NoStrandDevelopmentLengthForLongReinfShear,
      MaxIterExceeded,
      GirderLiftingConcreteStrength,
      GirderLiftingStability,
      GirderShippingConcreteStrength,
      GirderShippingStability,
      GirderShippingConfiguration,
      StressExceedsConcreteStrength,
      DebondDesignFailed,
      DesignCancelled,
      NoDesignRequested,
      LldfRangeOfApplicabilityError,
      DesignNotSupported // design could not be performed because design is not supported for the current application settings (e.g. time-step losses)
   };

   // Design outcome data that isn't neccessarily a failure
   enum DesignNote
   {
      dnExistingShearDesignPassedSpecCheck,
      dnShearRequiresStrutAndTie,
      dnStrandsAddedForLongReinfShear,
      dnStirrupsTightendedForLongReinfShear,
      dnLongitudinalBarsNeeded4FlexuralTensionCy,
      dnLongitudinalBarsNeeded4FlexuralTensionLifting,
      dnLongitudinalBarsNeeded4FlexuralTensionHauling,
      dnConcreteStrengthIncreasedForShearStress
   };


   // utility class to hold concrete strength design state information
   class PGSEXTCLASS ConcreteStrengthDesignState
   {
   public:
      enum Action {actStress, actShear}; // Concrete strength can be affected by flexural stress or shear stress

      ConcreteStrengthDesignState():
      m_Action(actStress),
      m_MinimumControls(true),
      m_RequiredAdditionalRebar(false),
      m_IntervalIdx(INVALID_INDEX)
      {;}

      // Conc strength controlled by flexural stress
      void SetStressState(bool controlledByMin, const CSegmentKey& segmentKey,IntervalIndexType intervalIdx, pgsTypes::StressType stressType, 
                    pgsTypes::LimitState limitState, pgsTypes::StressLocation stressLocation);

      // Conc strength controlled by shear stress
      void SetShearState(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx, pgsTypes::LimitState limitState);

      Action GetAction() const;
      bool WasControlledByMinimum() const;
      IntervalIndexType Interval() const;
      pgsTypes::StressType StressType() const;
      pgsTypes::LimitState LimitState() const;
      pgsTypes::StressLocation StressLocation() const;

      // Was additional rebar required to make tensile concrete strength?
      void SetRequiredAdditionalRebar(bool wasReqd);
      bool GetRequiredAdditionalRebar() const;

      std::_tstring AsString() const;

      bool operator==(const ConcreteStrengthDesignState& rOther) const;

      void Init() {m_MinimumControls=true;}

   private:
      Action m_Action;
      bool m_MinimumControls;
      bool m_RequiredAdditionalRebar;
      CSegmentKey              m_SegmentKey;
      IntervalIndexType        m_IntervalIdx;
      pgsTypes::StressType     m_StressType;
      pgsTypes::LimitState     m_LimitState; 
      pgsTypes::StressLocation m_StressLocation;
   };

   //------------------------------------------------------------------------
   // Default constructor
   pgsSegmentDesignArtifact();
   pgsSegmentDesignArtifact(const CSegmentKey& segmentKey);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsSegmentDesignArtifact(const pgsSegmentDesignArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsSegmentDesignArtifact();

   pgsSegmentDesignArtifact& operator = (const pgsSegmentDesignArtifact& rOther);

   void SetOutcome(Outcome outcome);
   Outcome GetOutcome() const;

   void AddDesignNote(DesignNote note);
   bool DoDesignNotesExist() const;
   std::vector<DesignNote> GetDesignNotes() const;

   // Any failed design strategies
   bool DoPreviouslyFailedDesignsExist() const;
   const std::vector<arFlexuralDesignType>& GetPreviouslyFailedFlexuralDesigns() const;
   void AddFailedDesign(const arDesignOptions& options);
   void UpdateFailedFlexuralDesigns(const pgsSegmentDesignArtifact& artifact) const;

   const CSegmentKey& GetSegmentKey() const;

   void SetDesignOptions(const arDesignOptions& options);
   const arDesignOptions& GetDesignOptions() const;

   // ==== Flexure-Related Properties ======
   //------------------------------------------------------------------------
   // DoDesignFlexure - If this is dtNoDesign, all flexure values are bogus.
   arFlexuralDesignType GetDoDesignFlexure() const;
   
   void SetNumStraightStrands(StrandIndexType Ns);
   StrandIndexType GetNumStraightStrands() const;
   
   void SetNumTempStrands(StrandIndexType Nt);
   StrandIndexType GetNumTempStrands() const;

   // Return the type of adjustable strands for the design. Base on design options
   pgsTypes::AdjustableStrandType GetAdjustableStrandType() const;

   // Algorithm can design harped or raised straight for adjustable strands
   // GetNumHarpedStrands will return the number of adjustable strands no matter what the design type
   void SetNumHarpedStrands(StrandIndexType Nh);
   StrandIndexType GetNumHarpedStrands() const;

   // Functions below are only valid for dtDesignFullyBondedRaised and dtDesignForDebondingRaised
   // and cannot be called for dtDesignForHarping, dtDesignForDebonding or dtDesignFullyBonded
   // Pjack used for these strands is the harped strand pjack value
   void SetRaisedAdjustableStrands(const ConfigStrandFillVector& strandFill);
   ConfigStrandFillVector GetRaisedAdjustableStrands() const;

   void SetPjackStraightStrands(Float64 Pj);
   Float64 GetPjackStraightStrands() const;

   void SetUsedMaxPjackStraightStrands(bool Pj); // pjack set was max possible
   bool GetUsedMaxPjackStraightStrands() const;

   void SetPjackTempStrands(Float64 Pj);
   Float64 GetPjackTempStrands() const;

   void SetUsedMaxPjackTempStrands(bool Pj); // pjack set was max possible
   bool GetUsedMaxPjackTempStrands() const;

   void SetPjackHarpedStrands(Float64 Pj);
   Float64 GetPjackHarpedStrands() const;

   void SetUsedMaxPjackHarpedStrands(bool Pj); // pjack set was max possible
   bool GetUsedMaxPjackHarpedStrands() const;

   // offsets are measured from original strand grid location from library
   void SetHarpStrandOffsetEnd(pgsTypes::MemberEndType endType,Float64 oe);
   Float64 GetHarpStrandOffsetEnd(pgsTypes::MemberEndType endType) const;

   void SetHarpStrandOffsetHp(pgsTypes::MemberEndType endType,Float64 ohp);
   Float64 GetHarpStrandOffsetHp(pgsTypes::MemberEndType endType) const;

   DebondConfigCollection GetStraightStrandDebondInfo() const;
   void SetStraightStrandDebondInfo(const DebondConfigCollection& dbinfo);
   void ClearDebondInfo();

   void SetReleaseStrength(Float64 fci);
   Float64 GetReleaseStrength() const;

   void SetConcrete(matConcreteEx concrete);
   const matConcreteEx& GetConcrete() const;
   void SetConcreteStrength(Float64 fc);
   Float64 GetConcreteStrength() const;

   void SetSlabOffset(pgsTypes::MemberEndType end,Float64 offset);
   Float64 GetSlabOffset(pgsTypes::MemberEndType end) const; // "A" dimension

   void SetFillet(Float64 f);
   Float64 GetFillet() const;

   void SetLiftingLocations(Float64 left,Float64 right);
   Float64 GetLeftLiftingLocation() const;
   Float64 GetRightLiftingLocation() const;

   void SetTruckSupportLocations(Float64 left,Float64 right);
   Float64 GetLeadingOverhang() const;
   Float64 GetTrailingOverhang() const;

   void SetHaulTruck(LPCTSTR lpszHaulTruck);
   LPCTSTR GetHaulTruck() const;

   // Set if elastic modulus is defined by user. Used by GetGirderConfiguration
   void SetUserEc(Float64 Ec);
   void SetUserEci(Float64 Eci);

   pgsTypes::TTSUsage GetTemporaryStrandUsage() const;

   // Functions to create primary segment data structures
   GDRCONFIG GetSegmentConfiguration() const;
   CPrecastSegmentData GetSegmentData() const;

   // design states for concrete strengths
   const ConcreteStrengthDesignState& GetReleaseDesignState() const;
   const ConcreteStrengthDesignState& GetFinalDesignState() const;

   void SetReleaseDesignState(const ConcreteStrengthDesignState& state);
   void SetFinalDesignState(const ConcreteStrengthDesignState& state);

   // ==== Shear (Stirrup)-Related Properties ======
   //------------------------------------------------------------------------
   // DoDesignShear - If this is false, all shear values are bogus.
   bool GetDoDesignShear() const;

   ZoneIndexType GetNumberOfStirrupZonesDesigned() const;
   void SetNumberOfStirrupZonesDesigned(ZoneIndexType num);
   const CShearData2* GetShearData() const;
   void SetShearData(const CShearData2& rdata);

   // Longitudinal rebar data is also used in shear design
   void SetWasLongitudinalRebarForShearDesigned(bool isTrue);
   bool GetWasLongitudinalRebarForShearDesigned() const;
   CLongitudinalRebarData& GetLongitudinalRebarData();
   const CLongitudinalRebarData& GetLongitudinalRebarData() const;
   void SetLongitudinalRebarData(const CLongitudinalRebarData& rdata);

protected:
   void MakeCopy(const pgsSegmentDesignArtifact& rOther);
   virtual void MakeAssignment(const pgsSegmentDesignArtifact& rOther);

private:
   Outcome m_Outcome;

   std::vector<DesignNote> m_DesignNotes; // may want to consider making this a set if things get complicated
   
   mutable std::vector<arFlexuralDesignType> m_PreviouslyFailedDesigns;

   CSegmentKey m_SegmentKey;

   arDesignOptions m_DesignOptions;

   StrandIndexType   m_Ns;
   StrandIndexType   m_Nh;
   StrandIndexType   m_Nt;
   Float64 m_PjS;
   bool    m_PjSUsedMax;
   Float64 m_PjH;
   bool    m_PjHUsedMax;
   Float64 m_PjT;
   bool    m_PjTUsedMax;
   Float64 m_HarpStrandOffsetEnd[2]; // use pgsTypes::MemberEndType to access array
   Float64 m_HarpStrandOffsetHp[2];  // use pgsTypes::MemberEndType to access array

   ConfigStrandFillVector m_RaisedAdjustableStrandFill;

   DebondConfigCollection m_SsDebondInfo;

   Float64 m_Fci;
   Float64 m_SlabOffset[2]; // "A" dimension at start and end of girder
   Float64 m_Fillet;
   Float64 m_LiftLocLeft;
   Float64 m_LiftLocRight;
   Float64 m_ShipLocLeft;
   Float64 m_ShipLocRight;
   std::_tstring m_strHaulTruck;

   bool  m_IsUserEc;
   Float64 m_UserEc;
   bool  m_IsUserEci;
   Float64 m_UserEci;

   matConcreteEx m_Concrete;

   ConcreteStrengthDesignState m_ConcreteReleaseDesignState;
   ConcreteStrengthDesignState m_ConcreteFinalDesignState;

   CShearData2 m_ShearData;
   ZoneIndexType  m_NumShearZones;

   bool m_bWasLongitudinalRebarForShearDesigned;
   CLongitudinalRebarData m_LongitudinalRebarData;

   void Init();

   void ModSegmentDataForFlexureDesign(IBroker* pBroker, CPrecastSegmentData* pSegmentData) const;
   void ModSegmentDataForShearDesign(IBroker* pBroker, CPrecastSegmentData* pSegmentData) const;
};

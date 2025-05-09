///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#include <PgsExt\ConcreteMaterial.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\Keys.h>
#include <Materials/Materials.h>
#include <LRFD\LRFD.h>
#include "StatusItems.h"
#include <map>

///////////////////////////////////////////////////////////////////////
// CConcreteManager
//
// Manages concrete material models for the various bridge components
// including precast segments, deck slab, and railing systems.

class CConcreteManager
{
public:
   CConcreteManager();
   ~CConcreteManager();

   void Init(IBroker* pBroker,StatusGroupIDType statusGroupID);
   void Reset();

   pgsTypes::ConcreteType GetSegmentConcreteType(const CSegmentKey& segmentKey) const;
   bool DoesSegmentConcreteHaveAggSplittingStrength(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentConcreteAggSplittingStrength(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentMaxAggrSize(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentConcreteFiberLength(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentStrengthDensity(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentWeightDensity(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentEccK1(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentEccK2(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentCreepK1(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentCreepK2(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentShrinkageK1(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentShrinkageK2(const CSegmentKey& segmentKey) const;

   pgsTypes::ConcreteType GetClosureJointConcreteType(const CSegmentKey& closureKey) const;
   bool DoesClosureJointConcreteHaveAggSplittingStrength(const CSegmentKey& closureKey) const;
   Float64 GetClosureJointConcreteAggSplittingStrength(const CSegmentKey& closureKey) const;
   Float64 GetClosureJointMaxAggrSize(const CSegmentKey& closureKey) const;
   Float64 GetClosureJointConcreteFiberLength(const CClosureKey& closureKey) const;
   Float64 GetClosureJointStrengthDensity(const CSegmentKey& closureKey) const;
   Float64 GetClosureJointWeightDensity(const CSegmentKey& closureKey) const;
   Float64 GetClosureJointEccK1(const CSegmentKey& closureKey) const;
   Float64 GetClosureJointEccK2(const CSegmentKey& closureKey) const;
   Float64 GetClosureJointCreepK1(const CSegmentKey& closureKey) const;
   Float64 GetClosureJointCreepK2(const CSegmentKey& closureKey) const;
   Float64 GetClosureJointShrinkageK1(const CSegmentKey& closureKey) const;
   Float64 GetClosureJointShrinkageK2(const CSegmentKey& closureKey) const;

   pgsTypes::ConcreteType GetDeckConcreteType() const;
   bool DoesDeckConcreteHaveAggSplittingStrength() const;
   Float64 GetDeckConcreteAggSplittingStrength() const;
   Float64 GetDeckStrengthDensity() const;
   Float64 GetDeckWeightDensity() const;
   Float64 GetDeckMaxAggrSize() const;
   Float64 GetDeckEccK1() const;
   Float64 GetDeckEccK2() const;
   Float64 GetDeckCreepK1() const;
   Float64 GetDeckCreepK2() const;
   Float64 GetDeckShrinkageK1() const;
   Float64 GetDeckShrinkageK2() const;

   pgsTypes::ConcreteType GetLongitudinalJointConcreteType() const;
   bool DoesLongitudinalJointConcreteHaveAggSplittingStrength() const;
   Float64 GetLongitudinalJointConcreteAggSplittingStrength() const;
   Float64 GetLongitudinalJointMaxAggrSize() const;
   Float64 GetLongitudinalJointStrengthDensity() const;
   Float64 GetLongitudinalJointWeightDensity() const;
   Float64 GetLongitudinalJointEccK1() const;
   Float64 GetLongitudinalJointEccK2() const;
   Float64 GetLongitudinalJointCreepK1() const;
   Float64 GetLongitudinalJointCreepK2() const;
   Float64 GetLongitudinalJointShrinkageK1() const;
   Float64 GetLongitudinalJointShrinkageK2() const;

   Float64 GetRailingSystemDensity(pgsTypes::TrafficBarrierOrientation orientation) const;

   Float64 GetNWCDensityLimit() const;
   Float64 GetLWCDensityLimit() const;

   Float64 GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type) const;
   Float64 GetFlexureFrCoefficient(pgsTypes::ConcreteType type) const;
   Float64 GetSegmentFlexureFrCoefficient(const CSegmentKey& segmentKey) const;
   Float64 GetClosureJointFlexureFrCoefficient(const CClosureKey& closureKey) const;
   Float64 GetShearModRupture(Float64 fc,pgsTypes::ConcreteType type) const;
   Float64 GetShearFrCoefficient(pgsTypes::ConcreteType type) const;
   Float64 GetSegmentShearFrCoefficient(const CSegmentKey& segmentKey) const;
   Float64 GetClosureJointShearFrCoefficient(const CClosureKey& closureKey) const;
   Float64 GetEconc(pgsTypes::ConcreteType type, Float64 fc,Float64 density,Float64 K1,Float64 K2) const;
   bool HasUHPC() const;

   // Time-dependent concrete properties
   // t is the absolute time in the time sequence. Time is automatically
   // convert to concrete age when computing properties. Zero is returned
   // if t is before the concrete has been cast or attained strength
   Float64 GetDeckCastingTime(IndexType castingRegionIdx) const;
   Float64 GetDeckFc(IndexType castingRegionIdx, Float64 t) const;
   Float64 GetDeckEc(IndexType castingRegionIdx, Float64 t) const;
   Float64 GetDeckFlexureFr(IndexType castingRegionIdx, Float64 t) const;
   Float64 GetDeckShearFr(IndexType castingRegionIdx, Float64 t) const;
   Float64 GetDeckFreeShrinkageStrain(IndexType castingRegionIdx, Float64 t) const;
   std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> GetDeckFreeShrinkageStrainDetails(IndexType castingRegionIdx, Float64 t) const;
   Float64 GetDeckCreepCoefficient(IndexType castingRegionIdx, Float64 t,Float64 tla) const;
   std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> GetDeckCreepCoefficientDetails(IndexType castingRegionIdx, Float64 t,Float64 tla) const;
   Float64 GetDeckAgingCoefficient(IndexType castingRegionIdx, Float64 timeOfLoading) const;
   Float64 GetDeckConcreteFirstCrackingStrength() const;
   Float64 GetDeckAutogenousShrinkage() const;
   const std::unique_ptr<WBFL::Materials::ConcreteBase>& GetDeckConcrete(IndexType castingRegionIdx) const;

   Float64 GetSegmentCastingTime(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentFc(const CSegmentKey& segmentKey,Float64 t) const;
   Float64 GetSegmentEc(const CSegmentKey& segmentKey,Float64 t) const;
   Float64 GetSegmentFlexureFr(const CSegmentKey& segmentKey,Float64 t) const;
   Float64 GetSegmentShearFr(const CSegmentKey& segmentKey,Float64 t) const;
   Float64 GetSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,Float64 t) const;
   std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> GetSegmentFreeShrinkageStrainDetails(const CSegmentKey& segmentKey,Float64 t) const;
   Float64 GetSegmentCreepCoefficient(const CSegmentKey& segmentKey,Float64 t,Float64 tla) const;
   std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> GetSegmentCreepCoefficientDetails(const CSegmentKey& segmentKey,Float64 t,Float64 tla) const;
   Float64 GetSegmentAgingCoefficient(const CSegmentKey& segmentKey,Float64 timeOfLoading) const;
   Float64 GetSegmentConcreteFirstCrackingStrength(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentAutogenousShrinkage(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentConcreteInitialEffectiveCrackingStrength(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentConcreteDesignEffectiveCrackingStrength(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentConcreteCrackLocalizationStrength(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentConcreteCrackLocalizationStrain(const CSegmentKey& segmentKey) const;
   Float64 GetSegmentConcreteFiberOrientationReductionFactor(const CSegmentKey& segmentKey) const;
   const std::unique_ptr<WBFL::Materials::ConcreteBase>& GetSegmentConcrete(const CSegmentKey& segmentKey) const;

   Float64 GetClosureJointCastingTime(const CClosureKey& closureKey) const;
   Float64 GetClosureJointFc(const CClosureKey& closureKey,Float64 t) const;
   Float64 GetClosureJointEc(const CClosureKey& closureKey,Float64 t) const;
   Float64 GetClosureJointFlexureFr(const CClosureKey& closureKey,Float64 t) const;
   Float64 GetClosureJointShearFr(const CClosureKey& closureKey,Float64 t) const;
   Float64 GetClosureJointFreeShrinkageStrain(const CClosureKey& closureKey,Float64 t) const;
   std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> GetClosureJointFreeShrinkageStrainDetails(const CClosureKey& closureKey,Float64 t) const;
   Float64 GetClosureJointCreepCoefficient(const CClosureKey& closureKey,Float64 t,Float64 tla) const;
   std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> GetClosureJointCreepCoefficientDetails(const CClosureKey& closureKey,Float64 t,Float64 tla) const;
   Float64 GetClosureJointAgingCoefficient(const CClosureKey& closureKey,Float64 timeOfLoading) const;
   const std::unique_ptr<WBFL::Materials::ConcreteBase>& GetClosureJointConcrete(const CClosureKey& closureKey) const;
   Float64 GetClosureJointAutogenousShrinkage(const CClosureKey& closureKey) const;

   Float64 GetRailingSystemCastingTime(pgsTypes::TrafficBarrierOrientation orientation) const;
   Float64 GetRailingSystemFc(pgsTypes::TrafficBarrierOrientation orientation,Float64 t) const;
   Float64 GetRailingSystemEc(pgsTypes::TrafficBarrierOrientation orientation,Float64 t) const;
   Float64 GetRailingSystemFreeShrinkageStrain(pgsTypes::TrafficBarrierOrientation orientation,Float64 t) const;
   std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> GetRailingSystemFreeShrinkageStrainDetails(pgsTypes::TrafficBarrierOrientation orientation,Float64 t) const;
   Float64 GetRailingSystemCreepCoefficient(pgsTypes::TrafficBarrierOrientation orientation,Float64 t,Float64 tla) const;
   std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> GetRailingSystemCreepCoefficientDetails(pgsTypes::TrafficBarrierOrientation orientation,Float64 t,Float64 tla) const;
   Float64 GetRailingSystemAgingCoefficient(pgsTypes::TrafficBarrierOrientation orientation,Float64 timeOfLoading) const;
   const std::unique_ptr<WBFL::Materials::ConcreteBase>& GetRailingSystemConcrete(pgsTypes::TrafficBarrierOrientation orientation) const;

   Float64 GetLongitudinalJointCastingTime() const;
   Float64 GetLongitudinalJointFc(Float64 t) const;
   Float64 GetLongitudinalJointEc(Float64 t) const;
   Float64 GetLongitudinalJointFlexureFr(Float64 t) const;
   Float64 GetLongitudinalJointShearFr(Float64 t) const;
   Float64 GetLongitudinalJointFreeShrinkageStrain(Float64 t) const;
   std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> GetLongitudinalJointFreeShrinkageStrainDetails(Float64 t) const;
   Float64 GetLongitudinalJointCreepCoefficient(Float64 t, Float64 tla) const;
   std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> GetLongitudinalJointCreepCoefficientDetails(Float64 t, Float64 tla) const;
   Float64 GetLongitudinalJointAgingCoefficient(Float64 timeOfLoading) const;
   const std::unique_ptr<WBFL::Materials::ConcreteBase>& GetLongitudinalJointConcrete() const;

   const std::unique_ptr<WBFL::Materials::SimpleConcrete>& GetPierConcrete(PierIndexType pierIdx) const;

   Float64 GetSegmentLambda(const CSegmentKey& segmentKey) const;
   Float64 GetClosureJointLambda(const CClosureKey& closureKey) const;
   Float64 GetDeckLambda() const;
   Float64 GetRailingSystemLambda(pgsTypes::TrafficBarrierOrientation orientation) const;
   Float64 GetLongitudinalJointLambda() const;

private:
   IBroker* m_pBroker; // weak reference
   StatusGroupIDType m_StatusGroupID;
   mutable bool m_bIsValidated; // Level 1 concrete definition is valid
   mutable bool m_bIsSegmentValidated; // Level 2 concrete definition is valid for segments/closure joints
   mutable bool m_bIsRailingSystemValidated; // Level 2 concrete definition is valid for railing system
   mutable bool m_bIsDeckValidated; // Level 2 concrete definition is valid for deck
   mutable bool m_bIsLongitudinalJointValidated; // Level 2 concrete definition is valid for longitudinal joints
   const CBridgeDescription2* m_pBridgeDesc;

   void ValidateConcrete() const;
   void ValidateSegmentConcrete() const;
   void ValidateRailingSystemConcrete() const;
   void ValidateDeckConcrete() const;
   void ValidateLongitudinalJointConcrete() const;
   void ValidateConcreteParameters(std::unique_ptr<WBFL::Materials::ConcreteBase>& pConcrete,pgsConcreteStrengthStatusItem::ConcreteType elementType,LPCTSTR strLabel,const CSegmentKey& segmentKey) const;
   bool IsConcreteDensityInRange(Float64 density,pgsTypes::ConcreteType type) const;

   // create new concrete material objects given the basic concrete input information
   void CreateConcrete(const CConcreteMaterial& concrete,LPCTSTR strName,WBFL::Materials::SimpleConcrete* pReleaseConc,WBFL::Materials::SimpleConcrete* pConcrete) const;

   // Factory method for creating time-dependent material models
   std::unique_ptr<WBFL::Materials::ConcreteBase> CreateConcreteModel(LPCTSTR strName,const CConcreteMaterial& concrete,Float64 timeAtCasting,Float64 cureTime,Float64 ageAtInitialLoading,Float64 stepTime) const;

   // factory method for LRFD concrete (non time-dependent version)
   std::unique_ptr<WBFL::LRFD::LRFDConcrete> CreateLRFDConcreteModel(const CConcreteMaterial& concrete,Float64 startTime,Float64 stepTime) const;

   // factory method for LRFD concrete (time-dependent version)
   std::unique_ptr<WBFL::LRFD::LRFDTimeDependentConcrete> CreateTimeDependentLRFDConcreteModel(const CConcreteMaterial& concrete,Float64 ageAtInitialLoading) const;

   // factory method for ACI209 concrete model
   std::unique_ptr<WBFL::Materials::ACI209Concrete> CreateACI209Model(const CConcreteMaterial& concrete,Float64 ageAtInitialLoading) const;

   // factory method for CEB-FIP concrete model
   std::unique_ptr<WBFL::Materials::CEBFIPConcrete> CreateCEBFIPModel(const CConcreteMaterial& concrete,Float64 ageAtInitialLoading) const;

   // Returns the concrete Aging coefficient, X
   Float64 GetConcreteAgingCoefficient(const std::unique_ptr<WBFL::Materials::ConcreteBase>& pConcrete,Float64 timeOfLoading) const;

   // Material model for precast girder segments
   mutable std::map< CSegmentKey, std::unique_ptr<WBFL::Materials::ConcreteBase> > m_pSegmentConcrete;

   // Material model for cast-in-place closure joint
   mutable std::map< CSegmentKey, std::unique_ptr<WBFL::Materials::ConcreteBase> > m_pClosureConcrete;

   // Material model for pier concrete
   mutable std::map<PierIndexType,std::unique_ptr<WBFL::Materials::SimpleConcrete>> m_pPierConcrete;

   // Material model for deck concrete
   mutable Float64 m_DeckEcK1;
   mutable Float64 m_DeckEcK2;
   mutable Float64 m_DeckCreepK1;
   mutable Float64 m_DeckCreepK2;
   mutable Float64 m_DeckShrinkageK1;
   mutable Float64 m_DeckShrinkageK2;
   mutable std::vector<std::unique_ptr<WBFL::Materials::ConcreteBase>> m_pvDeckConcrete; // time dependent Deck concrete model. vector index is the deck casting region index

   // Material model for longitudinal joint concrete
   mutable Float64 m_LongitudinalJointEcK1;
   mutable Float64 m_LongitudinalJointEcK2;
   mutable Float64 m_LongitudinalJointCreepK1;
   mutable Float64 m_LongitudinalJointCreepK2;
   mutable Float64 m_LongitudinalJointShrinkageK1;
   mutable Float64 m_LongitudinalJointShrinkageK2;
   mutable std::unique_ptr<WBFL::Materials::ConcreteBase> m_pLongitudinalJointConcrete; // time dependent Longitudinal Joint concrete model

   // Material model for railing system concrete
   mutable std::array<std::unique_ptr<WBFL::Materials::ConcreteBase>, 2> m_pRailingConcrete; // index is pgsTypes::TrafficBarrierOrientation

   // callback IDs for the status callbacks we register
   StatusCallbackIDType m_scidConcreteStrengthWarning;
   StatusCallbackIDType m_scidConcreteStrengthError;

   // Many methods return a reference to a unique_ptr. If the pointer is null
   // we can't return nullptr, we need to return a reference to a unique_ptr object.
   // These are unique_ptr objects that have a nullptr. Return these in place of nullptr
   std::unique_ptr<WBFL::Materials::SimpleConcrete> m_NullConcrete;
   std::unique_ptr<WBFL::Materials::ConcreteBase> m_NullConcreteBase;
};
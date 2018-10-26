///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <PgsExt\SegmentKey.h>
#include <Material\Material.h>
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

   pgsTypes::ConcreteType GetSegmentConcreteType(const CSegmentKey& segmentKey);
   bool DoesSegmentConcreteHaveAggSplittingStrength(const CSegmentKey& segmentKey);
   Float64 GetSegmentConcreteAggSplittingStrength(const CSegmentKey& segmentKey);
   Float64 GetSegmentMaxAggrSize(const CSegmentKey& segmentKey);
   Float64 GetSegmentStrengthDensity(const CSegmentKey& segmentKey);
   Float64 GetSegmentWeightDensity(const CSegmentKey& segmentKey);
   Float64 GetSegmentEccK1(const CSegmentKey& segmentKey);
   Float64 GetSegmentEccK2(const CSegmentKey& segmentKey);
   Float64 GetSegmentCreepK1(const CSegmentKey& segmentKey);
   Float64 GetSegmentCreepK2(const CSegmentKey& segmentKey);
   Float64 GetSegmentShrinkageK1(const CSegmentKey& segmentKey);
   Float64 GetSegmentShrinkageK2(const CSegmentKey& segmentKey);

   pgsTypes::ConcreteType GetClosureJointConcreteType(const CSegmentKey& closureKey);
   bool DoesClosureJointConcreteHaveAggSplittingStrength(const CSegmentKey& closureKey);
   Float64 GetClosureJointConcreteAggSplittingStrength(const CSegmentKey& closureKey);
   Float64 GetClosureJointMaxAggrSize(const CSegmentKey& closureKey);
   Float64 GetClosureJointStrengthDensity(const CSegmentKey& closureKey);
   Float64 GetClosureJointWeightDensity(const CSegmentKey& closureKey);
   Float64 GetClosureJointEccK1(const CSegmentKey& closureKey);
   Float64 GetClosureJointEccK2(const CSegmentKey& closureKey);
   Float64 GetClosureJointCreepK1(const CSegmentKey& closureKey);
   Float64 GetClosureJointCreepK2(const CSegmentKey& closureKey);
   Float64 GetClosureJointShrinkageK1(const CSegmentKey& closureKey);
   Float64 GetClosureJointShrinkageK2(const CSegmentKey& closureKey);

   pgsTypes::ConcreteType GetDeckConcreteType();
   bool DoesDeckConcreteHaveAggSplittingStrength();
   Float64 GetDeckConcreteAggSplittingStrength();
   Float64 GetDeckStrengthDensity();
   Float64 GetDeckWeightDensity();
   Float64 GetDeckMaxAggrSize();
   Float64 GetDeckEccK1();
   Float64 GetDeckEccK2();
   Float64 GetDeckCreepK1();
   Float64 GetDeckCreepK2();
   Float64 GetDeckShrinkageK1();
   Float64 GetDeckShrinkageK2();

   Float64 GetRailingSystemDensity(pgsTypes::TrafficBarrierOrientation orientation);

   Float64 GetNWCDensityLimit();
   Float64 GetLWCDensityLimit();

   Float64 GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type);
   Float64 GetFlexureFrCoefficient(pgsTypes::ConcreteType type);
   Float64 GetFlexureFrCoefficient(const CSegmentKey& segmentKey);
   Float64 GetShearModRupture(Float64 fc,pgsTypes::ConcreteType type);
   Float64 GetShearFrCoefficient(pgsTypes::ConcreteType type);
   Float64 GetShearFrCoefficient(const CSegmentKey& segmentKey);
   Float64 GetEconc(Float64 fc,Float64 density,Float64 K1,Float64 K2);

   // Time-dependent concrete properties
   // t is the absolute time in the time sequence. Time is automatically
   // convert to concrete age when computing properties. Zero is returned
   // if t is before the concrete has been cast or attained strength
   Float64 GetDeckCastingTime();
   Float64 GetDeckFc(Float64 t);
   Float64 GetDeckEc(Float64 t);
   Float64 GetDeckFlexureFr(Float64 t);
   Float64 GetDeckShearFr(Float64 t);
   Float64 GetDeckFreeShrinkageStrain(Float64 t);
   Float64 GetDeckCreepCoefficient(Float64 t,Float64 tla);
   matConcreteBase* GetDeckConcrete();

   Float64 GetSegmentCastingTime(const CSegmentKey& segmentKey);
   Float64 GetSegmentFc(const CSegmentKey& segmentKey,Float64 t);
   Float64 GetSegmentEc(const CSegmentKey& segmentKey,Float64 t);
   Float64 GetSegmentFlexureFr(const CSegmentKey& segmentKey,Float64 t);
   Float64 GetSegmentShearFr(const CSegmentKey& segmentKey,Float64 t);
   Float64 GetSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,Float64 t);
   Float64 GetSegmentCreepCoefficient(const CSegmentKey& segmentKey,Float64 t,Float64 tla);
   matConcreteBase* GetSegmentConcrete(const CSegmentKey& segmentKey);

   Float64 GetClosureJointCastingTime(const CClosureKey& closureKey);
   Float64 GetClosureJointFc(const CClosureKey& closureKey,Float64 t);
   Float64 GetClosureJointEc(const CClosureKey& closureKey,Float64 t);
   Float64 GetClosureJointFlexureFr(const CClosureKey& closureKey,Float64 t);
   Float64 GetClosureJointShearFr(const CClosureKey& closureKey,Float64 t);
   Float64 GetClosureJointFreeShrinkageStrain(const CClosureKey& closureKey,Float64 t);
   Float64 GetClosureJointCreepCoefficient(const CClosureKey& closureKey,Float64 t,Float64 tla);
   matConcreteBase* GetClosureJointConcrete(const CClosureKey& closureKey);

   Float64 GetRailingSystemCastingTime(pgsTypes::TrafficBarrierOrientation orientation);
   Float64 GetRailingSystemFc(pgsTypes::TrafficBarrierOrientation orientation,Float64 t);
   Float64 GetRailingSystemEc(pgsTypes::TrafficBarrierOrientation orientation,Float64 t);
   Float64 GetRailingSystemFreeShrinkageStrain(pgsTypes::TrafficBarrierOrientation orientation,Float64 t);
   Float64 GetRailingSystemCreepCoefficient(pgsTypes::TrafficBarrierOrientation orientation,Float64 t,Float64 tla);
   matConcreteBase* GetRailingSystemConcrete(pgsTypes::TrafficBarrierDistribution orientation);


private:
   CComPtr<IBroker> m_pBroker;
   StatusGroupIDType m_StatusGroupID;
   bool m_bIsValidated;
   const CBridgeDescription2* m_pBridgeDesc;

   void ValidateConcrete();
   void ValidateConcrete(boost::shared_ptr<matConcreteBase> pConcrete,pgsConcreteStrengthStatusItem::ConcreteType elementType,LPCTSTR strLabel,const CSegmentKey& segmentKey);
   bool IsConcreteDensityInRange(Float64 density,pgsTypes::ConcreteType type);

   // create new concrete material objects given the basic concrete input information
   void CreateConcrete(const CConcreteMaterial& concrete,LPCTSTR strName,matConcreteEx* pReleaseConc,matConcreteEx* pConcrete);

   // Factory method for creating time-dependent material models
   matConcreteBase* CreateConcreteModel(LPCTSTR strName,const CConcreteMaterial& concrete,Float64 timeAtCasting,Float64 cureTime,Float64 ageAtInitialLoading,Float64 stepTime,Float64 vs);

   // factory method for LRFD concrete (non time-dependent version)
   matLRFDConcrete* CreateLRFDConcreteModel(const CConcreteMaterial& concrete,Float64 stepTime);
   // matConcreteBase* CreateTimeDependentLRFDConcreteModel();

   // factory method for ACI209 concrete model
   matACI209Concrete* CreateACI209Model(const CConcreteMaterial& concrete,Float64 ageAtInitialLoading);

   // Material model for precast girder segments
   std::map< CSegmentKey, boost::shared_ptr<matConcreteBase> > m_pSegmentConcrete;

   // Material model for cast-in-place closure joint
   std::map< CSegmentKey, boost::shared_ptr<matConcreteBase> > m_pClosureConcrete;

   Float64 m_DeckEcK1;
   Float64 m_DeckEcK2;
   Float64 m_DeckCreepK1;
   Float64 m_DeckCreepK2;
   Float64 m_DeckShrinkageK1;
   Float64 m_DeckShrinkageK2;
   std::auto_ptr<matConcreteBase> m_pDeckConc; // time dependent Deck concrete model

   std::auto_ptr<matConcreteBase> m_pRailingConc[2]; // index is pgsTypes::TrafficBarrierOrientation

   // callback IDs for the status callbacks we register
   StatusCallbackIDType m_scidConcreteStrengthWarning;
   StatusCallbackIDType m_scidConcreteStrengthError;

};
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

#include "stdafx.h"
#include "ConcreteManager.h"
#include <EAF\EAFStatusCenter.h>
#include <EAF\EAFDisplayUnits.h>

#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Bridge.h>

#include <LRFD\ConcreteUtil.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>
#include <PgsExt\GirderLabel.h>

#include <psgLib/MomentCapacityCriteria.h>
#include <psgLib/ShearCapacityCriteria.h>
#include <psgLib/LimitStateConcreteStrengthCriteria.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CConcreteManager::CConcreteManager()
{
   m_bIsValidated              = false;
   m_bIsSegmentValidated       = false;
   m_bIsRailingSystemValidated = false;
   m_bIsDeckValidated          = false;
   m_bIsLongitudinalJointValidated = false;
}

CConcreteManager::~CConcreteManager()
{
}

void CConcreteManager::Init(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidConcreteStrengthWarning  = pStatusCenter->RegisterCallback(new pgsConcreteStrengthStatusCallback(m_pBroker,eafTypes::statusWarning));
   m_scidConcreteStrengthError    = pStatusCenter->RegisterCallback(new pgsConcreteStrengthStatusCallback(m_pBroker,eafTypes::statusError));

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   m_pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
}

void CConcreteManager::Reset()
{
   m_pLongitudinalJointConcrete = std::unique_ptr<WBFL::Materials::ConcreteBase>(nullptr);
   m_pvDeckConcrete.clear();
   m_pClosureConcrete.clear();
   m_pSegmentConcrete.clear();
   m_pRailingConcrete[pgsTypes::tboLeft]  = std::unique_ptr<WBFL::Materials::ConcreteBase>(nullptr);
   m_pRailingConcrete[pgsTypes::tboRight] = std::unique_ptr<WBFL::Materials::ConcreteBase>(nullptr);

   m_bIsValidated              = false;
   m_bIsSegmentValidated       = false;
   m_bIsRailingSystemValidated = false;
   m_bIsDeckValidated          = false;
   m_bIsLongitudinalJointValidated = false;
}

void CConcreteManager::ValidateConcrete() const
{
   if ( m_bIsValidated )
   {
      return;
   }

   const CTimelineManager* pTimelineMgr = m_pBridgeDesc->GetTimelineManager();

   //////////////////////////////////////////////////////////////////////////////
   //
   // Create Deck concrete
   //
   //////////////////////////////////////////////////////////////////////////////
   const CDeckDescription2* pDeck = m_pBridgeDesc->GetDeckDescription();
   if (pDeck->GetDeckType() != pgsTypes::sdtNone)
   {
      Float64 modE;
      if (pDeck->Concrete.bUserEc)
      {
         modE = pDeck->Concrete.Ec;
      }
      else
      {
         modE = WBFL::LRFD::ConcreteUtil::ModE((WBFL::Materials::ConcreteType)pDeck->Concrete.Type,pDeck->Concrete.Fc,
            pDeck->Concrete.StrengthDensity,
            false /* ignore LRFD range checks */);

         if (WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition())
         {
            modE *= (pDeck->Concrete.EcK1*pDeck->Concrete.EcK2);
         }
      }

      // save these parameters for later so we don't have to look them up every time they are needed
      m_DeckEcK1 = pDeck->Concrete.EcK1;
      m_DeckEcK2 = pDeck->Concrete.EcK2;
      m_DeckCreepK1 = pDeck->Concrete.CreepK1;
      m_DeckCreepK2 = pDeck->Concrete.CreepK2;
      m_DeckShrinkageK1 = pDeck->Concrete.ShrinkageK1;
      m_DeckShrinkageK2 = pDeck->Concrete.ShrinkageK2;

      // Time dependent model
      EventIndexType castDeckEventIdx = pTimelineMgr->GetCastDeckEventIndex();
      Float64   initial_time_at_casting = pTimelineMgr->GetStart(castDeckEventIdx);
      const auto* pTimelineEvent = pTimelineMgr->GetEventByIndex(castDeckEventIdx);
      ATLASSERT(pTimelineEvent && pTimelineEvent->GetCastDeckActivity().IsEnabled());
      const auto& castDeckActivity = pTimelineEvent->GetCastDeckActivity();

      Float64   age_at_initial_loading = castDeckActivity.GetTotalCuringDuration();
      Float64   cure_time = castDeckActivity.GetActiveCuringDuration();
      Float64   time_between_casting = castDeckActivity.GetTimeBetweenCasting();

      IndexType nRegions = castDeckActivity.GetCastingRegionCount();
      m_pvDeckConcrete.resize(nRegions);

      IndexType nCastings = castDeckActivity.GetCastingCount();
      for (IndexType castingIdx = 0; castingIdx < nCastings; castingIdx++)
      {
         Float64 time_at_casting = initial_time_at_casting + castingIdx*time_between_casting;
         Float64 time_step = time_at_casting + cure_time;

         auto vRegions = castDeckActivity.GetRegions(castingIdx);
         for (auto regionIdx : vRegions)
         {
            CString strName;
            strName.Format(_T("Deck Concrete: Region %d, Sequence %d"), LABEL_INDEX(regionIdx), LABEL_INDEX(castingIdx));
            m_pvDeckConcrete[regionIdx] = CreateConcreteModel(strName, pDeck->Concrete, time_at_casting, cure_time, age_at_initial_loading, time_step);
         }
      }
   }

   //////////////////////////////////////////////////////////////////////////////
   //
   // Create railing concrete
   //
   //////////////////////////////////////////////////////////////////////////////
   EventIndexType railingSystemEventIdx = pTimelineMgr->GetRailingSystemLoadEventIndex();
   Float64 time_at_casting        = pTimelineMgr->GetStart(railingSystemEventIdx);
   Float64 age_at_initial_loading = 0.0; // assume railing system load is at full strength immediately
   Float64 cure_time = 0.0;
   Float64 modE;

   const CRailingSystem* pLeftRailingSystem = m_pBridgeDesc->GetLeftRailingSystem();
   ATLASSERT(!IsUHPC(pLeftRailingSystem->Concrete.Type)); // UHPC not supported yet - if this fires, there is a bug in the UI somewhere

   if ( pLeftRailingSystem->Concrete.bUserEc )
   {
      modE = pLeftRailingSystem->Concrete.Ec;
   }
   else
   {
      modE = WBFL::LRFD::ConcreteUtil::ModE((WBFL::Materials::ConcreteType)pLeftRailingSystem->Concrete.Type,pLeftRailingSystem->Concrete.Fc, 
                                    pLeftRailingSystem->Concrete.StrengthDensity, 
                                    false /* ignore LRFD range checks */ );

      if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
      {
         modE *= pLeftRailingSystem->Concrete.EcK1*pLeftRailingSystem->Concrete.EcK2;
      }
   }

   Float64 time_step = time_at_casting + cure_time;
   m_pRailingConcrete[pgsTypes::tboLeft] = CreateConcreteModel(_T("Left Railing Concrete"), pLeftRailingSystem->Concrete,time_at_casting,cure_time,age_at_initial_loading,time_step);

   const CRailingSystem* pRightRailingSystem = m_pBridgeDesc->GetRightRailingSystem();
   ATLASSERT(!IsUHPC(pRightRailingSystem->Concrete.Type)); // UHPC not supported yet - if this fires, there is a bug in the UI somewhere
   if ( pRightRailingSystem->Concrete.bUserEc )
   {
      modE = pRightRailingSystem->Concrete.Ec;
   }
   else
   {
      modE = WBFL::LRFD::ConcreteUtil::ModE((WBFL::Materials::ConcreteType)pRightRailingSystem->Concrete.Type,pRightRailingSystem->Concrete.Fc, 
                                    pRightRailingSystem->Concrete.StrengthDensity, 
                                    false /* ignore LRFD range checks */ );

      if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
      {
         modE *= pRightRailingSystem->Concrete.EcK1*pRightRailingSystem->Concrete.EcK2;
      }
   }

   m_pRailingConcrete[pgsTypes::tboRight] = CreateConcreteModel(_T("Right Railing Concrete"), pRightRailingSystem->Concrete,time_at_casting,cure_time,age_at_initial_loading,time_step);
   
   //////////////////////////////////////////////////////////////////////////////
   //
   // Precast Segment and Closure Joint Concrete
   //
   //////////////////////////////////////////////////////////////////////////////
   bool bPCIUHPCGirder = false; // when deck concrete is valided below, we need to know if any of the girders are PCI UHPC
   GroupIndexType nGroups = m_pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            if (pSegment->Material.Concrete.Type == pgsTypes::PCI_UHPC)
            {
               bPCIUHPCGirder = true;
            }

            SegmentIDType segmentID = pSegment->GetID();

            EventIndexType segConstructEventIdx = pTimelineMgr->GetSegmentConstructionEventIndex(segmentID);
            Float64 segment_casting_time        = pTimelineMgr->GetStart(segConstructEventIdx);
            Float64 segment_age_at_release      = pTimelineMgr->GetEventByIndex(segConstructEventIdx)->GetConstructSegmentsActivity().GetTotalCuringDuration();
            Float64 segment_cure_time           = segment_age_at_release;


            // Time dependent concrete
            // for the LRFD stepped f'c concrete model, assume the jump from f'ci to f'c occurs
            // 28 days after the concrete is cast
            Float64 stepTime = segment_casting_time + 28;

            std::unique_ptr<WBFL::Materials::ConcreteBase> pSegmentConcrete = CreateConcreteModel(_T("Segment Concrete"),pSegment->Material.Concrete,segment_casting_time,segment_cure_time,segment_age_at_release,stepTime);
            m_pSegmentConcrete.insert( std::make_pair(segmentKey,std::move(pSegmentConcrete)) );

            if ( segIdx < nSegments-1 )
            {
               const CClosureJointData* pClosure  = pGirder->GetClosureJoint(segIdx);
               EventIndexType castClosureEventIdx = pTimelineMgr->GetCastClosureJointEventIndex(pClosure);
               Float64 closure_casting_time       = pTimelineMgr->GetStart(castClosureEventIdx);
               Float64 closure_age_at_continuity  = pTimelineMgr->GetEventByIndex(castClosureEventIdx)->GetCastClosureJointActivity().GetTotalCuringDuration();
               Float64 closure_cure_time          = closure_age_at_continuity;

               // this isn't really needed because closure joints are for spliced girders only and
               // spliced girders only use a time-dependent material model... but since we need 
               // to provide this parameters, we will provide it so that it is logical.
               //
               // For a pseudo time-dependent concrete model, the step from initial to release
               // occurs at continuity. The time this occurs is the time at continuity. The
               // time at continuity is the time at casting plus the age at continuity. Recall 
               // that age at continuity is the duration of time from casting to continuity.
               Float64 closure_step_time = closure_casting_time + closure_age_at_continuity;

               std::unique_ptr<WBFL::Materials::ConcreteBase> pClosureConcrete = CreateConcreteModel(_T("Closure Concrete"),pClosure->GetConcrete(),closure_casting_time,closure_cure_time,closure_age_at_continuity,closure_step_time);
               m_pClosureConcrete.insert( std::make_pair(segmentKey,std::move(pClosureConcrete)) );
            }
         }
      }
   }

   //////////////////////////////////////////////////////////////////////////////
   //
   // Create Longitudinal Joint concrete
   //
   //////////////////////////////////////////////////////////////////////////////
   if (m_pBridgeDesc->HasStructuralLongitudinalJoints())
   {
      const CConcreteMaterial& LJConcrete = m_pBridgeDesc->GetLongitudinalJointMaterial();
      if (LJConcrete.bUserEc)
      {
         modE = LJConcrete.Ec;
      }
      else
      {
         modE = WBFL::LRFD::ConcreteUtil::ModE((WBFL::Materials::ConcreteType)LJConcrete.Type,LJConcrete.Fc,
            LJConcrete.StrengthDensity,
            false /* ignore LRFD range checks */);

         if (WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition())
         {
            modE *= (LJConcrete.EcK1*LJConcrete.EcK2);
         }
      }

      // save these parameters for later so we don't have to look them up every time they are needed
      m_LongitudinalJointEcK1 = LJConcrete.EcK1;
      m_LongitudinalJointEcK2 = LJConcrete.EcK2;
      m_LongitudinalJointCreepK1 = LJConcrete.CreepK1;
      m_LongitudinalJointCreepK2 = LJConcrete.CreepK2;
      m_LongitudinalJointShrinkageK1 = LJConcrete.ShrinkageK1;
      m_LongitudinalJointShrinkageK2 = LJConcrete.ShrinkageK2;

      // Time dependent model
      EventIndexType castLongitudinalJointEventIdx = pTimelineMgr->GetCastLongitudinalJointEventIndex();
      time_at_casting = pTimelineMgr->GetStart(castLongitudinalJointEventIdx);
      age_at_initial_loading = pTimelineMgr->GetEventByIndex(castLongitudinalJointEventIdx)->GetCastLongitudinalJointActivity().GetTotalCuringDuration();
      cure_time = pTimelineMgr->GetEventByIndex(castLongitudinalJointEventIdx)->GetCastLongitudinalJointActivity().GetActiveCuringDuration();

      // modulus of rupture coefficients
      time_step = time_at_casting + cure_time;
      m_pLongitudinalJointConcrete = CreateConcreteModel(_T("Longitudinal Joint Concrete"), LJConcrete, time_at_casting, cure_time, age_at_initial_loading, time_step);
   }

   //////////////////////////////////////////////////////////////////////////////
   //
   // Pier Concrete
   //
   //////////////////////////////////////////////////////////////////////////////
   PierIndexType nPiers = m_pBridgeDesc->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData2* pPier = m_pBridgeDesc->GetPier(pierIdx);
      if ( pPier->GetPierModelType() == pgsTypes::pmtPhysical )
      {
         const CConcreteMaterial& concrete = pPier->GetConcrete();
         ATLASSERT(!IsUHPC(concrete.Type));

         Float64 modE;
         if ( concrete.bUserEci )
         {
            modE = concrete.Eci;
         }
         else
         {
            modE = WBFL::LRFD::ConcreteUtil::ModE( (WBFL::Materials::ConcreteType)concrete.Type, concrete.Fc, 
                                           concrete.StrengthDensity, 
                                           false /* ignore LRFD range checks */ );

            if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
            {
               modE *= (concrete.EcK1 * concrete.EcK2);
            }
         }

         std::unique_ptr<WBFL::Materials::SimpleConcrete> pPierConcrete = std::make_unique<WBFL::Materials::SimpleConcrete>(_T("Pier Concrete"),concrete.Fc,concrete.StrengthDensity, concrete.WeightDensity,modE,0.0/*dummy frShear*/, 0.0/*dummy frMoment*/);
         pPierConcrete->SetType((WBFL::Materials::ConcreteType)concrete.Type);
         m_pPierConcrete.insert( std::make_pair(pierIdx,std::move(pPierConcrete)) );
      }
   }

   //////////////////////////////////////////////////////////////////////////////
   //
   // Validate values   
   //
   //////////////////////////////////////////////////////////////////////////////

   GET_IFACE_NOCHECK(IEAFStatusCenter,pStatusCenter);

   // per 5.4.2.1 f'c must exceed 28 MPa (4 ksi)
   bool bSI = WBFL::LRFD::BDSManager::GetUnits() == WBFL::LRFD::BDSManager::Units::SI ? true : false;
   Float64 fcMin = bSI ? WBFL::Units::ConvertToSysUnits(28, WBFL::Units::Measure::MPa) : WBFL::Units::ConvertToSysUnits(4, WBFL::Units::Measure::KSI);

   Float64 fcMax = (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::EighthEdition2017 ? 10.0 : 15.0); // KSI... limit went from 10ksi to 15ksi in 8th edition
   fcMax = WBFL::Units::ConvertToSysUnits(fcMax, WBFL::Units::Measure::KSI);

   // check railing system
   if ( !IsConcreteDensityInRange(m_pRailingConcrete[pgsTypes::tboLeft]->GetStrengthDensity(),(pgsTypes::ConcreteType)m_pRailingConcrete[pgsTypes::tboLeft]->GetType()) )
   {
      std::_tostringstream os;
      if ( m_pRailingConcrete[pgsTypes::tboLeft]->GetType() == WBFL::Materials::ConcreteType::Normal )
      {
         os << _T("Left railing system concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
      }
      else
      {
         os << _T("Left railing system concrete density is out of range for Lightweight Concrete per LRFD 5.2.");
      }

      std::_tstring strMsg = os.str();

      CSegmentKey dummyKey;
      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::RailingSystem,pgsConcreteStrengthStatusItem::Density,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   if ( !IsConcreteDensityInRange(m_pRailingConcrete[pgsTypes::tboRight]->GetStrengthDensity(),(pgsTypes::ConcreteType)m_pRailingConcrete[pgsTypes::tboRight]->GetType()) )
   {
      std::_tostringstream os;
      if ( m_pRailingConcrete[pgsTypes::tboRight]->GetType() == WBFL::Materials::ConcreteType::Normal)
      {
         os << _T("Right railing system concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
      }
      else
      {
         os << _T("Right railing system concrete density is out of range for Lightweight Concrete per LRFD 5.2.");
      }

      std::_tstring strMsg = os.str();

      CSegmentKey dummyKey;
      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::RailingSystem,pgsConcreteStrengthStatusItem::Density,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   // Check Deck concrete
   if ( pDeck->GetDeckType() != pgsTypes::sdtNone )
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      GET_IFACE(ILimits,pLimits);

      IndexType regionIdx = 0;
      for (auto& pDeckConcrete : m_pvDeckConcrete)
      {
         // Post information item in the status center if these limits are not satisfied
         Float64 time = pDeckConcrete->GetTimeAtCasting() + pDeckConcrete->GetCureTime() + 28.0;
         Float64 fc28 = pDeckConcrete->GetFc(time);
         if (fc28 < fcMin && !IsEqual(fc28, fcMin))
         {
            CString strNote;
            strNote = bSI ? _T("Deck concrete cannot be less than 28 MPa per LRFD 5.4.2.1")
                          : _T("Deck concrete cannot be less than 4 KSI per LRFD 5.4.2.1");
            CString strMsg;
            strMsg.Format(_T("Casting Region %d: %s"), LABEL_INDEX(regionIdx), strNote);
            CSegmentKey dummyKey;
            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab, pgsConcreteStrengthStatusItem::FinalStrength, dummyKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg);
            pStatusCenter->Add(pStatusItem);
            //strMsg += std::_tstring(_T("\nSee Status Center for Details"));
            //THROW_UNWIND(strMsg.c_str(),-1);
         }

         pgsTypes::ConcreteType slabConcreteType = (pgsTypes::ConcreteType)pDeckConcrete->GetType();
         if (fcMax < fc28)
         {
            std::_tostringstream os;
            os << _T("Deck concrete strength (" << (LPCTSTR)::FormatDimension(fc28, pDisplayUnits->GetStressUnit()) << ") exceeds the ") << (LPCTSTR)::FormatDimension(fcMax, pDisplayUnits->GetStressUnit()) << _T(" concrete strength limit per LRFD 5.1");
            std::_tstring strMsg = os.str();

            CSegmentKey dummyKey;
            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab, pgsConcreteStrengthStatusItem::FinalStrength, dummyKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
            pStatusCenter->Add(pStatusItem);
         }

         Float64 max_slab_fc = pLimits->GetMaxSlabFc(slabConcreteType);
         if (  max_slab_fc < fc28 && !IsEqual(max_slab_fc,fc28))
         {
            std::_tostringstream os;
            os << _T("Deck concrete strength (" << (LPCTSTR)::FormatDimension(fc28,pDisplayUnits->GetStressUnit()) << ") exceeds the normal value of ") << (LPCTSTR)::FormatDimension(max_slab_fc,pDisplayUnits->GetStressUnit());
            std::_tstring strMsg = os.str();

            CSegmentKey dummyKey;
            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::FinalStrength,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
            pStatusCenter->Add(pStatusItem);
         }

         ATLASSERT(!IsUHPC(slabConcreteType)); // deck can't be UHPC
         if (bPCIUHPCGirder && (fc28 < WBFL::Units::ConvertToSysUnits(6.0, WBFL::Units::Measure::KSI) || slabConcreteType != pgsTypes::Normal))
         {
            // PCI UHPC SDG E.C4.2 recommends deck concrete by normal weight and have a minimum f'c of 6 ksi
            CString strMsg(_T("PCI UHPC SDG E.C4.2 recommends that PCI-UHPC girders with conventional concrete deck, the deck should be normal weight concrete and have a minimum specified compressive strength of 6.0 KSI"));
            CSegmentKey dummyKey;
            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab, pgsConcreteStrengthStatusItem::FinalStrength, dummyKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg);
            pStatusCenter->Add(pStatusItem);
         }

         Float64 max_wc = pLimits->GetMaxConcreteUnitWeight(slabConcreteType);
         Float64 MaxWc = WBFL::Units::ConvertFromSysUnits(max_wc,pDisplayUnits->GetDensityUnit().UnitOfMeasure);

         Float64 strength_density = pDeckConcrete->GetStrengthDensity();
         if ( max_wc < strength_density && !IsEqual(max_wc,strength_density,0.0001) )
         {
            std::_tostringstream os;
            os << _T("Deck concrete density for strength calculations exceeds the normal value of ") << MaxWc << _T(" ") << pDisplayUnits->GetDensityUnit().UnitOfMeasure.UnitTag();

            std::_tstring strMsg = os.str();

            CSegmentKey dummyKey;
            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::Density,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
            pStatusCenter->Add(pStatusItem);
         }

         Float64 weight_density = pDeckConcrete->GetWeightDensity();
         if ( max_wc < weight_density && !IsEqual(max_wc,weight_density,0.0001) )
         {
            std::_tostringstream os;
            os << _T("Deck concrete density for weight calculations exceeds the normal value of ") << MaxWc << _T(" ") << pDisplayUnits->GetDensityUnit().UnitOfMeasure.UnitTag();

            std::_tstring strMsg = os.str();

            CSegmentKey dummyKey;
            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::DensityForWeight,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
            pStatusCenter->Add(pStatusItem);
         }

         if ( !IsConcreteDensityInRange(strength_density, slabConcreteType) )
         {
            std::_tostringstream os;
            if (slabConcreteType == pgsTypes::Normal )
            {
               os << _T("Deck concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
            }
            else
            {
               os << _T("Deck concrete density is out of range for Lightweight Concrete per LRFD 5.2.");
            }

            std::_tstring strMsg = os.str();

            CSegmentKey dummyKey;
            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::Density,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
            pStatusCenter->Add(pStatusItem);
         }

         Float64 max_agg_size = pLimits->GetMaxConcreteAggSize(slabConcreteType);
         Float64 MaxAggSize = WBFL::Units::ConvertFromSysUnits(max_agg_size,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

         Float64 agg_size = pDeckConcrete->GetMaxAggregateSize();
         if ( max_agg_size < agg_size && !IsEqual(max_agg_size,agg_size))
         {
            std::_tostringstream os;
            os << _T("Deck concrete aggregate size exceeds the normal value of ") << MaxAggSize << _T(" ") << pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag();

            std::_tstring strMsg = os.str();

            CSegmentKey dummyKey;
            pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::AggSize,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
            pStatusCenter->Add(pStatusItem);
         }
      }
      regionIdx++;
   }

   // Check longitudinal joint concrete
   pgsTypes::ConcreteType jointConcreteType = m_pLongitudinalJointConcrete ? (pgsTypes::ConcreteType)m_pLongitudinalJointConcrete->GetType() : pgsTypes::Normal;
   if (m_pBridgeDesc->HasStructuralLongitudinalJoints() && !IsUHPC(jointConcreteType))
   {
      GET_IFACE(IEAFDisplayUnits, pDisplayUnits);
      GET_IFACE(ILimits, pLimits);

      Float64 time = m_pLongitudinalJointConcrete->GetTimeAtCasting() + m_pLongitudinalJointConcrete->GetCureTime() + 28.0;
      Float64 fc28 = m_pLongitudinalJointConcrete->GetFc(time);
      if (fc28 < fcMin && !IsEqual(fc28, fcMin))
      {
         std::_tstring strMsg;
         strMsg = bSI ? _T("Longitudinal joint concrete cannot be less than 28 MPa per LRFD 5.4.2.1")
            : _T("Longitudinal joint concrete cannot be less than 4 KSI per LRFD 5.4.2.1");
         CSegmentKey dummyKey;
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::LongitudinalJoint, pgsConcreteStrengthStatusItem::FinalStrength, dummyKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
         //strMsg += std::_tstring(_T("\nSee Status Center for Details"));
         //THROW_UNWIND(strMsg.c_str(),-1);
      }

      if (fcMax < fc28 )
      {
         std::_tostringstream os;
         os << _T("Longitudinal joint strength (" << (LPCTSTR)::FormatDimension(fc28, pDisplayUnits->GetStressUnit()) << ") exceeds the ") << (LPCTSTR)::FormatDimension(fcMax, pDisplayUnits->GetStressUnit()) << _T(" concrete strength limit per LRFD 5.1");
         std::_tstring strMsg = os.str();

         CSegmentKey dummyKey;
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::LongitudinalJoint, pgsConcreteStrengthStatusItem::FinalStrength, dummyKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      //Float64 max_slab_fc = pLimits->GetMaxSlabFc(slabConcreteType);
      //if (max_slab_fc < fc28 && !IsEqual(max_slab_fc, fc28))
      //{
      //   std::_tostringstream os;
      //   os << _T("LongitudinalJoint concrete strength (" << (LPCTSTR)::FormatDimension(fc28, pDisplayUnits->GetStressUnit()) << ") exceeds the normal value of ") << (LPCTSTR)::FormatDimension(max_slab_fc, pDisplayUnits->GetStressUnit());

      //   std::_tstring strMsg = os.str();

      //   CSegmentKey dummyKey;
      //   pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab, pgsConcreteStrengthStatusItem::FinalStrength, dummyKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
      //   pStatusCenter->Add(pStatusItem);
      //}

      Float64 max_wc = pLimits->GetMaxConcreteUnitWeight(jointConcreteType);
      Float64 MaxWc = WBFL::Units::ConvertFromSysUnits(max_wc, pDisplayUnits->GetDensityUnit().UnitOfMeasure);

      Float64 strength_density = m_pLongitudinalJointConcrete->GetStrengthDensity();
      if (max_wc < strength_density && !IsEqual(max_wc, strength_density, 0.0001))
      {
         std::_tostringstream os;
         os << _T("Longitudinal joint concrete density for strength calculations exceeds the normal value of ") << MaxWc << _T(" ") << pDisplayUnits->GetDensityUnit().UnitOfMeasure.UnitTag();

         std::_tstring strMsg = os.str();

         CSegmentKey dummyKey;
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::LongitudinalJoint, pgsConcreteStrengthStatusItem::Density, dummyKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 weight_density = m_pLongitudinalJointConcrete->GetWeightDensity();
      if (max_wc < weight_density && !IsEqual(max_wc, weight_density, 0.0001))
      {
         std::_tostringstream os;
         os << _T("Longitudinal joint concrete density for weight calculations exceeds the normal value of ") << MaxWc << _T(" ") << pDisplayUnits->GetDensityUnit().UnitOfMeasure.UnitTag();

         std::_tstring strMsg = os.str();

         CSegmentKey dummyKey;
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::LongitudinalJoint, pgsConcreteStrengthStatusItem::DensityForWeight, dummyKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      if (!IsConcreteDensityInRange(strength_density, jointConcreteType))
      {
         std::_tostringstream os;
         if (jointConcreteType == pgsTypes::Normal)
         {
            os << _T("Longitudinal joint concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
         }
         else
         {
            os << _T("Longitudinal joint concrete density is out of range for Lightweight Concrete per LRFD 5.2.");
         }

         std::_tstring strMsg = os.str();

         CSegmentKey dummyKey;
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::LongitudinalJoint, pgsConcreteStrengthStatusItem::Density, dummyKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 max_agg_size = pLimits->GetMaxConcreteAggSize(jointConcreteType);
      Float64 MaxAggSize = WBFL::Units::ConvertFromSysUnits(max_agg_size, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      Float64 agg_size = m_pLongitudinalJointConcrete->GetMaxAggregateSize();
      if (max_agg_size < agg_size && !IsEqual(max_agg_size, agg_size))
      {
         std::_tostringstream os;
         os << _T("Longitudinal joint concrete aggregate size exceeds the normal value of ") << MaxAggSize << _T(" ") << pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag();

         std::_tstring strMsg = os.str();

         CSegmentKey dummyKey;
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::LongitudinalJoint, pgsConcreteStrengthStatusItem::AggSize, dummyKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }
   }

   // Check girder concrete
   std::_tstring strMsg;
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);

            ValidateConcreteParameters(m_pSegmentConcrete[segmentKey],pgsConcreteStrengthStatusItem::GirderSegment,SEGMENT_LABEL(segmentKey),segmentKey);

            if ( segIdx < nSegments-1 )
            {
               ValidateConcreteParameters(m_pClosureConcrete[segmentKey],pgsConcreteStrengthStatusItem::ClosureJoint,CLOSURE_LABEL(segmentKey),segmentKey);
            }
         }
      }
   }

   m_bIsValidated = true;
}

void CConcreteManager::ValidateSegmentConcrete() const
{
   if ( m_bIsSegmentValidated )
   {
      return;
   }

   GET_IFACE(ISectionProperties,pSectProp);
   
   //////////////////////////////////////////////////////////////////////////////
   //
   // Precast Segment and Closure Joint Concrete
   //
   //////////////////////////////////////////////////////////////////////////////
   GroupIndexType nGroups = m_pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(grpIdx,gdrIdx,segIdx);
            Float64 V, S;
            pSectProp->GetSegmentVolumeAndSurfaceArea(segmentKey, &V, &S);
            Float64 vsSegment = IsZero(S) ? DBL_MAX : V/S;

            vsSegment = ::RoundOff(vsSegment,WBFL::Units::ConvertToSysUnits(0.5,WBFL::Units::Measure::Millimeter));

            m_pSegmentConcrete[segmentKey]->SetVSRatio(vsSegment);

            if ( segIdx < nSegments-1 )
            {
               CClosureKey closureKey(segmentKey);
               Float64 V, S;
               pSectProp->GetClosureJointVolumeAndSurfaceArea(closureKey, &V, &S);
               Float64 vsClosure = IsZero(S) ? DBL_MAX : V/S;

               vsClosure = ::RoundOff(vsClosure,WBFL::Units::ConvertToSysUnits(0.5,WBFL::Units::Measure::Millimeter));

               m_pClosureConcrete[closureKey]->SetVSRatio(vsClosure);
            }
         } // next segment
      } // next girder
   } // next group
   m_bIsSegmentValidated = true;
}

void CConcreteManager::ValidateRailingSystemConcrete() const
{
   if ( m_bIsRailingSystemValidated )
   {
      return;
   }

   //////////////////////////////////////////////////////////////////////////////
   //
   // Create railing concrete
   //
   //////////////////////////////////////////////////////////////////////////////
   // Railing system creep/shrinkage not considered to V/S of zero is fine
   m_pRailingConcrete[pgsTypes::tboLeft]->SetVSRatio(0.0);
   m_pRailingConcrete[pgsTypes::tboRight]->SetVSRatio(0.0);
   
   m_bIsRailingSystemValidated = true;
}

void CConcreteManager::ValidateDeckConcrete() const
{
   if ( m_bIsDeckValidated )
   {
      return;
   }

   for (auto& pDeckConcrete : m_pvDeckConcrete)
   {
      if (pDeckConcrete.get() != nullptr)
      {
         GET_IFACE(ISectionProperties, pSectProp);
         Float64 V, S;
         pSectProp->GetDeckVolumeAndSurfaceArea(&V, &S);
         Float64 vsDeck = IsZero(S) ? DBL_MAX : V / S;

         vsDeck = ::RoundOff(vsDeck, WBFL::Units::ConvertToSysUnits(0.5, WBFL::Units::Measure::Millimeter));

         pDeckConcrete->SetVSRatio(vsDeck);
      }
   }

   m_bIsDeckValidated = true;
}

void CConcreteManager::ValidateLongitudinalJointConcrete() const
{
   if (m_bIsLongitudinalJointValidated)
   {
      return;
   }

   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
#pragma Reminder("validate longitudinal joint concrete") // how do we get V/S for a joint?
      //GET_IFACE(ISectionProperties, pSectProp);
      //Float64 S = pSectProp->GetDeckSurfaceArea();
      //Float64 V = pSectProp->GetDeckVolume();
      //Float64 vsJoint = IsZero(S) ? DBL_MAX : V / S;

      //vsDeck = ::RoundOff(vsJoint, WBFL::Units::ConvertToSysUnits(0.5, WBFL::Units::Measure::Millimeter));

      //m_pLongitudinalJointConcrete->SetVSRatio(vsJoint);
      
      // Assume a 9" joint that is 6" thick
      // V/S = A*l/P*l = A/P A = 9*6, P = 9+9, V/S = 3
      m_pLongitudinalJointConcrete->SetVSRatio(WBFL::Units::ConvertToSysUnits(3.0, WBFL::Units::Measure::Inch));
   }

   m_bIsLongitudinalJointValidated = true;
}

void CConcreteManager::ValidateConcreteParameters(std::unique_ptr<WBFL::Materials::ConcreteBase>& pConcrete, pgsConcreteStrengthStatusItem::ConcreteType elementType, LPCTSTR strLabel, const CSegmentKey& segmentKey) const
{
   ATLASSERT(elementType == pgsConcreteStrengthStatusItem::GirderSegment || elementType == pgsConcreteStrengthStatusItem::ClosureJoint);

   // these interfaces not used unless there is a problem
   GET_IFACE_NOCHECK(IEAFDisplayUnits, pDisplayUnits);
   GET_IFACE_NOCHECK(IEAFStatusCenter, pStatusCenter);

   pgsTypes::ConcreteType concreteType = (pgsTypes::ConcreteType)pConcrete->GetType();

   std::_tstring strMsg;

   Float64 time_at_casting = pConcrete->GetTimeAtCasting();
   Float64 fci_time = time_at_casting + pConcrete->GetCureTime();
   Float64 fc_time = time_at_casting + 28.0;
   Float64 fci = pConcrete->GetFc(fci_time);
   Float64 fc28 = pConcrete->GetFc(fc_time);

   if (concreteType == pgsTypes::PCI_UHPC)
   {
      if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::NinthEdition2020)
      {
         strMsg = _T("The project criteria must be based on AASHTO LRFD 9th Edition 2020 or later when PCI UHPC materials are used.");
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::Specification, segmentKey, m_StatusGroupID, m_scidConcreteStrengthError, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 fci_min = WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::KSI);
      if (fci < fci_min)
      {
         std::_tostringstream os;
         os << strLabel << _T(": Initial concrete strength (") << (LPCTSTR)::FormatDimension(fci, pDisplayUnits->GetStressUnit()) << _T(") is less than the recommended minimum for PCI UHPC materials (") << (LPCTSTR)::FormatDimension(fci_min, pDisplayUnits->GetStressUnit()) << _T(")") << _T(", PCI UHPC SDG E.4.2");

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::ReleaseStrength, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 fcMin = WBFL::Units::ConvertToSysUnits(17.4, WBFL::Units::Measure::KSI);
      if (fc28 < fcMin)
      {
         std::_tostringstream os;
         os << strLabel << _T(": Concrete strength, f'c (") << (LPCTSTR)::FormatDimension(fc28, pDisplayUnits->GetStressUnit()) << _T(") is less that minimum performance criteria for PCI UHPC materials (") << (LPCTSTR)::FormatDimension(fcMin, pDisplayUnits->GetStressUnit()) << _T(")");
         strMsg = os.str();
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::FinalStrength, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 ffc;
      const auto* pLRFDConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(pConcrete.get());
      ffc = pLRFDConcrete->GetFirstCrackingStrength();

      Float64 ffcMin, fpeakMin, frrMin;
      WBFL::LRFD::ConcreteUtil::GetPCIUHPCMinProperties(&fcMin, &ffcMin, &fpeakMin, &frrMin);

      if (ffc < ffcMin)
      {
         std::_tostringstream os;
         os << strLabel << _T(": First peak (cracking) flexural strength (") << (LPCTSTR)::FormatDimension(ffc, pDisplayUnits->GetStressUnit()) << _T(") is less than the minimum performance criteria for PCI UHPC materials (") << (LPCTSTR)::FormatDimension(ffcMin, pDisplayUnits->GetStressUnit()) << _T(")") << _T(", PCI UHPC SDG E.4.2");

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::FirstPeakFlexuralStrength, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }
   }
   else if (concreteType == pgsTypes::UHPC)
   {
      if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::NinthEdition2020)
      {
         strMsg = _T("The project criteria must be based on AASHTO LRFD 9th Edition 2020 or later when UHPC materials are used.");
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::Specification, segmentKey, m_StatusGroupID, m_scidConcreteStrengthError, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 fc_min = WBFL::Units::ConvertToSysUnits(17.5, WBFL::Units::Measure::KSI); // GS 1.1.1
      if (fc28 < fc_min)
      {
         std::_tostringstream os;
         os << strLabel << _T(": Compressive strength, f'c (") << (LPCTSTR)::FormatDimension(fc28, pDisplayUnits->GetStressUnit()) << _T(") is less than the minimum (") << (LPCTSTR)::FormatDimension(fc_min, pDisplayUnits->GetStressUnit()) << _T(")") << _T(", GS 1.1.1");

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::FinalStrength, segmentKey, m_StatusGroupID, m_scidConcreteStrengthError, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      const auto* pLRFDConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(pConcrete.get());
      Float64 ftcr = pLRFDConcrete->GetDesignEffectiveCrackingStrength();
      Float64 ftcr_min = WBFL::Units::ConvertToSysUnits(0.75, WBFL::Units::Measure::KSI);
      if (ftcr < ftcr_min)
      {
         std::_tostringstream os;
         os << strLabel << _T(": Effective cracking strength, ft,cr (") << (LPCTSTR)::FormatDimension(ftcr, pDisplayUnits->GetStressUnit()) << _T(") is less than the minimum (") << (LPCTSTR)::FormatDimension(ftcr_min, pDisplayUnits->GetStressUnit()) << _T(")") << _T(", GS 1.1.1");

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::DesignEffectiveCrackingStrength, segmentKey, m_StatusGroupID, m_scidConcreteStrengthError, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 ftloc = pLRFDConcrete->GetCrackLocalizationStrength();
      if (ftloc < ftcr)
      {
         std::_tostringstream os;
         os << strLabel << _T(": Crack localization strength ft,loc (") << (LPCTSTR)::FormatDimension(ftloc, pDisplayUnits->GetStressUnit()) << _T(") is less than the effective cracking strength, ft,cr (") << (LPCTSTR)::FormatDimension(ftcr, pDisplayUnits->GetStressUnit()) << _T(")") << _T(", GS 1.1.1");

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::CrackLocalizationStrength, segmentKey, m_StatusGroupID, m_scidConcreteStrengthError, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 etloc = pLRFDConcrete->GetCrackLocalizationStrain();
      Float64 etloc_min = 0.0025;
      if (etloc < etloc_min)
      {
         std::_tostringstream os;
         os << strLabel << _T(": Crack localization strain et,loc (") << etloc << _T(") is less than ") << etloc_min << _T(", GS 1.1.1");

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::CrackLocalizationStrain, segmentKey, m_StatusGroupID, m_scidConcreteStrengthError, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 fci_min = WBFL::Units::ConvertToSysUnits(14.0, WBFL::Units::Measure::KSI);
      if (fci < fci_min)
      {
         std::_tostringstream os;
         os << strLabel << _T(": Initial concrete strength f'ci (") << (LPCTSTR)::FormatDimension(fci, pDisplayUnits->GetStressUnit()) << _T(") is less than the minimum (") << (LPCTSTR)::FormatDimension(fci_min, pDisplayUnits->GetStressUnit()) << _T(")") << _T(", GS 1.9.1.2");

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::ReleaseStrength, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }


      Float64 ftcri = pLRFDConcrete->GetInitialEffectiveCrackingStrength();
      if ((IsLE(fci, 0.9 * fc28)) && (ftcri < 0.75 * ftcr))
      {
         std::_tostringstream os;
         os << strLabel << _T(": f'ci <= 0.90f'c  and the initial effective cracking strength, ft,cri (") << (LPCTSTR)::FormatDimension(ftcri, pDisplayUnits->GetStressUnit()) << _T(") is less than 0.75ft,cr (") << (LPCTSTR)::FormatDimension(0.75 * ftcr, pDisplayUnits->GetStressUnit()) << _T(")") << _T(", GS 1.9.1.2");

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::InitialEffectiveCrackingStrength, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }
   }
   else
   {
      GET_IFACE(ILimits, pLimits);

      Float64 fcMin, fcMax;
      // per 5.4.2.1 f'c must exceed 28 MPa (4 ksi)
      bool bSI = WBFL::LRFD::BDSManager::GetUnits() == WBFL::LRFD::BDSManager::Units::SI ? true : false;
      fcMin = bSI ? WBFL::Units::ConvertToSysUnits(28, WBFL::Units::Measure::MPa) : WBFL::Units::ConvertToSysUnits(4, WBFL::Units::Measure::KSI);
      // the LRFD doesn't say that this specifically applies to closure joints,
      // but we are going to assume that it does.

      fcMax = (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::EighthEdition2017 ? 10.0 : 15.0); // KSI... limit went from 10ksi to 15ksi in 8th edition
      fcMax = WBFL::Units::ConvertToSysUnits(fcMax, WBFL::Units::Measure::KSI);

      Float64 max_fci, max_fc;
      if (elementType == pgsConcreteStrengthStatusItem::GirderSegment)
      {
         max_fci = pLimits->GetMaxSegmentFci(concreteType);
         max_fc = pLimits->GetMaxSegmentFc(concreteType);
      }
      else
      {
         max_fci = pLimits->GetMaxClosureFci(concreteType);
         max_fc = pLimits->GetMaxClosureFc(concreteType);
      }

      if (fc28 < fcMin)
      {
         std::_tostringstream os;
         os << strLabel << _T(": Concrete strength, f'c, is less that permitted by LRFD 5.4.2.1");
         strMsg = os.str();
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::FinalStrength, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }


      if (fcMax < fc28)
      {
         std::_tostringstream os;
         os << strLabel << _T(" strength (" << (LPCTSTR)::FormatDimension(fc28, pDisplayUnits->GetStressUnit()) << ") exceeds the ") << (LPCTSTR)::FormatDimension(fcMax, pDisplayUnits->GetStressUnit()) << _T(" concrete strength limit per LRFD 5.1");
         std::_tstring strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::FinalStrength, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      if (max_fci < fci && !IsEqual(max_fci, fci, WBFL::Units::ConvertToSysUnits(0.001, WBFL::Units::Measure::KSI)))
      {
         std::_tostringstream os;
         os << strLabel << _T(": Initial concrete strength (") << (LPCTSTR)::FormatDimension(fci, pDisplayUnits->GetStressUnit()) << _T(") exceeds the normal value of ") << (LPCTSTR)::FormatDimension(max_fci, pDisplayUnits->GetStressUnit());

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::ReleaseStrength, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      if (max_fc < fc28 && !IsEqual(max_fc, fc28, WBFL::Units::ConvertToSysUnits(0.001, WBFL::Units::Measure::KSI)))
      {
         std::_tostringstream os;
         os << strLabel << _T(": Concrete strength (") << (LPCTSTR)::FormatDimension(fc28, pDisplayUnits->GetStressUnit()) << _T(") exceeds the normal value of ") << (LPCTSTR)::FormatDimension(max_fc, pDisplayUnits->GetStressUnit());

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::FinalStrength, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 max_wc = pLimits->GetMaxConcreteUnitWeight(concreteType);
      Float64 wc = pConcrete->GetStrengthDensity();
      if (max_wc < wc && !IsEqual(max_wc, wc, 0.0001))
      {
         std::_tostringstream os;
         os << strLabel << _T(": Concrete density for strength calculations (") << (LPCTSTR)::FormatDimension(wc, pDisplayUnits->GetDensityUnit()) << _T(") exceeds the normal value of ") << (LPCTSTR)::FormatDimension(max_wc, pDisplayUnits->GetDensityUnit());

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::Density, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      wc = pConcrete->GetWeightDensity();
      if (max_wc < wc && !IsEqual(max_wc, wc, 0.0001))
      {
         std::_tostringstream os;
         os << strLabel << _T(": Concrete density for weight calculations (") << (LPCTSTR)::FormatDimension(wc, pDisplayUnits->GetDensityUnit()) << _T(") exceeds the normal value of ") << (LPCTSTR)::FormatDimension(max_wc, pDisplayUnits->GetDensityUnit());

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::DensityForWeight, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      if (!IsConcreteDensityInRange(pConcrete->GetStrengthDensity(), concreteType))
      {
         std::_tostringstream os;
         if (concreteType == pgsTypes::Normal)
         {
            os << strLabel << _T(": concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
         }
         else if (concreteType == pgsTypes::AllLightweight || concreteType == pgsTypes::SandLightweight)
         {
            os << strLabel << _T(": concrete density is out of range for Lightweight Concrete per LRFD 5.2.");
         }
         else
         {
            ATLASSERT(false); // is there a new concrete type
         }

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::DensityForWeight, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 max_agg_size = pLimits->GetMaxConcreteAggSize(concreteType);
      if (max_agg_size < pConcrete->GetMaxAggregateSize() && !IsEqual(max_agg_size, pConcrete->GetMaxAggregateSize()))
      {
         std::_tostringstream os;
         os << strLabel << _T(": Concrete aggregate size (") << (LPCTSTR)::FormatDimension(pConcrete->GetMaxAggregateSize(), pDisplayUnits->GetComponentDimUnit()) << _T(") exceeds the normal value of ") << (LPCTSTR)::FormatDimension(max_agg_size, pDisplayUnits->GetComponentDimUnit());

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::AggSize, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      // There are certain combinations of input, when Ec or Eci is user input and the other value
      // is computed where Ec could be less than Eci. Trap this error.
      Float64 Eci = pConcrete->GetEc(fci_time);
      Float64 Ec28 = pConcrete->GetEc(fc_time);
      if (Ec28 < Eci)
      {
         std::_tostringstream os;
         os << strLabel << _T(": Ec (")
            << (LPCTSTR)FormatDimension(Ec28, pDisplayUnits->GetModEUnit()) << _T(") is less than Eci (")
            << (LPCTSTR)FormatDimension(Eci, pDisplayUnits->GetModEUnit()) << _T(")");

         strMsg = os.str();

         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType, pgsConcreteStrengthStatusItem::Modulus, segmentKey, m_StatusGroupID, m_scidConcreteStrengthWarning, strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }
   }
}

bool CConcreteManager::IsConcreteDensityInRange(Float64 density,pgsTypes::ConcreteType type) const
{
   if (IsUHPC(type))
   {
      return true; // there isn't a density limit for UHPC
   }
   else if ( type == pgsTypes::Normal)
   {
      return ( GetNWCDensityLimit() <= density );
   }
   else
   {
      return (density <= GetLWCDensityLimit());
   }
}

std::unique_ptr<WBFL::Materials::ConcreteBase> CConcreteManager::CreateConcreteModel(LPCTSTR strName,const CConcreteMaterial& concrete,Float64 timeAtCasting,Float64 cureTime,Float64 ageAtInitialLoading,Float64 stepTime) const
{
   GET_IFACE(ILossParameters,pLossParameters);
   PrestressLossCriteria::LossMethodType loss_method = pLossParameters->GetLossMethod();

   GET_IFACE(IEnvironment,pEnvironment);
   Float64 rh = pEnvironment->GetRelHumidity();

   std::unique_ptr<WBFL::Materials::ConcreteBase> pConcrete;
   if ( loss_method == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      // for time step loss method, create concrete model based on time-dependent model type
      switch( pLossParameters->GetTimeDependentModel() )
      {
      case PrestressLossCriteria::TimeDependentConcreteModelType::AASHTO:
         pConcrete = CreateTimeDependentLRFDConcreteModel(concrete,ageAtInitialLoading);
         break;
      case PrestressLossCriteria::TimeDependentConcreteModelType::ACI209:
         pConcrete = CreateACI209Model(concrete,ageAtInitialLoading);
         break;
      case PrestressLossCriteria::TimeDependentConcreteModelType::CEBFIP:
         pConcrete = CreateCEBFIPModel(concrete,ageAtInitialLoading);
         break;
      default:
         ATLASSERT(false); // should never get here
         // is there a new model
      }
   }
   else
   {
      // for a non-time-step method, use a simple pseudo time-dependent model
      pConcrete = CreateLRFDConcreteModel(concrete,timeAtCasting+cureTime,stepTime);
   }

   // this is all on the base class so it can be done polymorphically
   pConcrete->SetName(strName);
   pConcrete->SetType((WBFL::Materials::ConcreteType)concrete.Type);
   pConcrete->SetStrengthDensity(concrete.StrengthDensity);
   pConcrete->SetWeightDensity(concrete.WeightDensity);
   pConcrete->HasAggSplittingStrength(concrete.bHasFct);
   pConcrete->SetAggSplittingStrength(concrete.Fct);
   pConcrete->SetMaxAggregateSize(concrete.MaxAggregateSize);
   pConcrete->SetFiberLength(concrete.FiberLength);
   pConcrete->SetRelativeHumidity(rh);
   pConcrete->SetTimeAtCasting(timeAtCasting);
   pConcrete->SetAgeAtInitialLoading(ageAtInitialLoading);
   pConcrete->SetCureTime(cureTime);
   pConcrete->SetCuringType((WBFL::Materials::CuringType)concrete.CureMethod);
   //pConcrete->SetVSRatio(vs); NOTE: volume to surface ratio is set during the level 2 validation 
   // of the concrete model. To get V/S we need to get section properties and section properties
   // need valid concrete models. This creates a circular dependency. However, the only part
   // of the concrete model that needs V/S is creep and shrinkage. Let V/S be invalid
   // on the concrete model until a creep or shrinkage parameter is requested... then, make
   // V/S correct

   return pConcrete;
}

void CConcreteManager::CreateConcrete(const CConcreteMaterial& concrete,LPCTSTR strName,WBFL::Materials::SimpleConcrete* pReleaseConc,WBFL::Materials::SimpleConcrete* pConcrete) const
{
   Float64 modE;
   if ( concrete.bUserEci )
   {
      modE = concrete.Eci;
   }
   else
   {
      modE = WBFL::LRFD::ConcreteUtil::ModE( (WBFL::Materials::ConcreteType)concrete.Type,concrete.Fci, 
                                     concrete.StrengthDensity, 
                                     false /* ignore LRFD range checks */ );

      if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
      {
         modE *= (concrete.EcK1 * concrete.EcK2);
      }
   }

   Float64 lambda = WBFL::LRFD::ConcreteUtil::ComputeConcreteDensityModificationFactor((WBFL::Materials::ConcreteType)concrete.Type,concrete.StrengthDensity,concrete.bHasFct,concrete.Fct,concrete.Fc);

   // get the modulus of rupture.
   Float64 frShear = WBFL::LRFD::ConcreteUtil::ModRupture(concrete.Fci, GetShearFrCoefficient(concrete.Type));
   Float64 frFlexure = WBFL::LRFD::ConcreteUtil::ModRupture(concrete.Fci, GetFlexureFrCoefficient(concrete.Type));

   pReleaseConc->SetName(strName);
   pReleaseConc->SetFc(concrete.Fci);
   pReleaseConc->SetDensity(concrete.StrengthDensity);
   pReleaseConc->SetDensityForWeight(concrete.WeightDensity);
   pReleaseConc->SetE(modE);
   pReleaseConc->SetMaxAggregateSize(concrete.MaxAggregateSize);
   pReleaseConc->SetFiberLength(concrete.FiberLength);
   pReleaseConc->SetType((WBFL::Materials::ConcreteType)concrete.Type);
   pReleaseConc->HasAggSplittingStrength(concrete.bHasFct);
   pReleaseConc->SetAggSplittingStrength(concrete.Fct);
   pReleaseConc->SetShearFr(lambda*frShear);
   pReleaseConc->SetFlexureFr(lambda*frFlexure);

   if ( concrete.bUserEc )
   {
      modE = concrete.Ec;
   }
   else
   {
      modE = WBFL::LRFD::ConcreteUtil::ModE( (WBFL::Materials::ConcreteType)concrete.Type,concrete.Fc, 
                                     concrete.StrengthDensity, 
                                     false /* ignore LRFD range checks */ );

      if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
      {
         modE *= (concrete.EcK1 * concrete.EcK2);
      }
   }

   // get the modulus of rupture.
   frShear   = WBFL::LRFD::ConcreteUtil::ModRupture(concrete.Fc,GetShearFrCoefficient(concrete.Type));
   frFlexure = (!IsUHPC(concrete.Type) ? WBFL::LRFD::ConcreteUtil::ModRupture(concrete.Fc,GetFlexureFrCoefficient(concrete.Type)) : 0);

   pConcrete->SetName(strName);
   pConcrete->SetFc(concrete.Fc);
   pConcrete->SetDensity(concrete.StrengthDensity);
   pConcrete->SetDensityForWeight(concrete.WeightDensity);
   pConcrete->SetE(modE);
   pConcrete->SetMaxAggregateSize(concrete.MaxAggregateSize);
   pConcrete->SetFiberLength(concrete.FiberLength);
   pConcrete->SetType((WBFL::Materials::ConcreteType)concrete.Type);
   pConcrete->HasAggSplittingStrength(concrete.bHasFct);
   pConcrete->SetAggSplittingStrength(concrete.Fct);
   pConcrete->SetShearFr(lambda*frShear);
   pConcrete->SetFlexureFr(lambda*frFlexure);
}

pgsTypes::ConcreteType CConcreteManager::GetSegmentConcreteType(const CSegmentKey& segmentKey) const
{
   ValidateConcrete();
   return (pgsTypes::ConcreteType)m_pSegmentConcrete[segmentKey]->GetType();
}

bool CConcreteManager::DoesSegmentConcreteHaveAggSplittingStrength(const CSegmentKey& segmentKey) const
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->HasAggSplittingStrength();
}

Float64 CConcreteManager::GetSegmentConcreteAggSplittingStrength(const CSegmentKey& segmentKey) const
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetAggSplittingStrength();
}

Float64 CConcreteManager::GetSegmentMaxAggrSize(const CSegmentKey& segmentKey) const
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetMaxAggregateSize();
}

Float64 CConcreteManager::GetSegmentConcreteFiberLength(const CSegmentKey& segmentKey) const
{
   ValidateConcrete();
   return IsUHPC(m_pSegmentConcrete[segmentKey]->GetType()) ? m_pSegmentConcrete[segmentKey]->GetFiberLength() : 0;
}

Float64 CConcreteManager::GetSegmentStrengthDensity(const CSegmentKey& segmentKey) const
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetStrengthDensity();
}

Float64 CConcreteManager::GetSegmentWeightDensity(const CSegmentKey& segmentKey) const
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetWeightDensity();
}

Float64 CConcreteManager::GetSegmentEccK1(const CSegmentKey& segmentKey) const
{
   Float64 K1 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      K1 = pMaterial->Concrete.EcK1;
   }

   return K1;
}

Float64 CConcreteManager::GetSegmentEccK2(const CSegmentKey& segmentKey) const
{
   Float64 K2 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      K2 = pMaterial->Concrete.EcK2;
   }

   return K2;
}

Float64 CConcreteManager::GetSegmentCreepK1(const CSegmentKey& segmentKey) const
{
   Float64 K1 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      K1 = pMaterial->Concrete.CreepK1;
   }

   return K1;
}

Float64 CConcreteManager::GetSegmentCreepK2(const CSegmentKey& segmentKey) const
{
   Float64 K2 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      K2 = pMaterial->Concrete.CreepK2;
   }

   return K2;
}

Float64 CConcreteManager::GetSegmentShrinkageK1(const CSegmentKey& segmentKey) const
{
   Float64 K1 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      K1 = pMaterial->Concrete.ShrinkageK1;
   }

   return K1;
}

Float64 CConcreteManager::GetSegmentShrinkageK2(const CSegmentKey& segmentKey) const
{
   Float64 K2 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      K2 = pMaterial->Concrete.ShrinkageK2;
   }

   return K2;
}

pgsTypes::ConcreteType CConcreteManager::GetClosureJointConcreteType(const CSegmentKey& closureKey) const
{
   ValidateConcrete();
   return (pgsTypes::ConcreteType)m_pClosureConcrete[closureKey]->GetType();
}

bool CConcreteManager::DoesClosureJointConcreteHaveAggSplittingStrength(const CSegmentKey& closureKey) const
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->HasAggSplittingStrength();
}

Float64 CConcreteManager::GetClosureJointConcreteAggSplittingStrength(const CSegmentKey& closureKey) const
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetAggSplittingStrength();
}

Float64 CConcreteManager::GetClosureJointMaxAggrSize(const CSegmentKey& closureKey) const
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetMaxAggregateSize();
}

Float64 CConcreteManager::GetClosureJointConcreteFiberLength(const CClosureKey& closureKey) const
{
   ValidateConcrete();
   return IsUHPC(m_pClosureConcrete[closureKey]->GetType()) ? m_pClosureConcrete[closureKey]->GetFiberLength() : 0;
}

Float64 CConcreteManager::GetClosureJointStrengthDensity(const CSegmentKey& closureKey) const
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetStrengthDensity();
}

Float64 CConcreteManager::GetClosureJointWeightDensity(const CSegmentKey& closureKey) const
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetWeightDensity();
}

Float64 CConcreteManager::GetClosureJointEccK1(const CSegmentKey& closureKey) const
{
   Float64 K1 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      const CClosureJointData* pClosure = m_pBridgeDesc->GetClosureJoint(closureKey);
      K1 = pClosure->GetConcrete().EcK1;
   }

   return K1;
}

Float64 CConcreteManager::GetClosureJointEccK2(const CSegmentKey& closureKey) const
{
   Float64 K2 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      const CClosureJointData* pClosure = m_pBridgeDesc->GetClosureJoint(closureKey);
      K2 = pClosure->GetConcrete().EcK2;
   }

   return K2;
}

Float64 CConcreteManager::GetClosureJointCreepK1(const CSegmentKey& closureKey) const
{
   Float64 K1 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      const CClosureJointData* pClosure = m_pBridgeDesc->GetClosureJoint(closureKey);
      K1 = pClosure->GetConcrete().CreepK1;
   }

   return K1;
}

Float64 CConcreteManager::GetClosureJointCreepK2(const CSegmentKey& closureKey) const
{
   Float64 K2 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      const CClosureJointData* pClosure = m_pBridgeDesc->GetClosureJoint(closureKey);
      K2 = pClosure->GetConcrete().CreepK2;
   }

   return K2;
}

Float64 CConcreteManager::GetClosureJointShrinkageK1(const CSegmentKey& closureKey) const
{
   Float64 K1 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      const CClosureJointData* pClosure = m_pBridgeDesc->GetClosureJoint(closureKey);
      K1 = pClosure->GetConcrete().ShrinkageK1;
   }

   return K1;
}

Float64 CConcreteManager::GetClosureJointShrinkageK2(const CSegmentKey& closureKey) const
{
   Float64 K2 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      const CClosureJointData* pClosure = m_pBridgeDesc->GetClosureJoint(closureKey);
      K2 = pClosure->GetConcrete().ShrinkageK2;
   }

   return K2;
}

pgsTypes::ConcreteType CConcreteManager::GetDeckConcreteType() const
{
   ValidateConcrete();
   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[0].get(); // use region 0 because the deck material in all casting regions is the same
   if ( pDeckConcrete != nullptr )
   {
      return (pgsTypes::ConcreteType)pDeckConcrete->GetType();
   }
   else
   {
      return pgsTypes::Normal;
   }
}

bool CConcreteManager::DoesDeckConcreteHaveAggSplittingStrength() const
{
   ValidateConcrete();
   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[0].get(); // use region 0 because the deck material in all casting regions is the same
   if ( pDeckConcrete != nullptr )
   {
      return pDeckConcrete->HasAggSplittingStrength();
   }
   else
   {
      return false;
   }
}

Float64 CConcreteManager::GetDeckConcreteAggSplittingStrength() const
{
   ValidateConcrete();
   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[0].get(); // use region 0 because the deck material in all casting regions is the same
   if ( pDeckConcrete != nullptr )
   {
      return pDeckConcrete->GetAggSplittingStrength();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckStrengthDensity() const
{
   ValidateConcrete();
   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[0].get(); // use region 0 because the deck material in all casting regions is the same

   if ( pDeckConcrete != nullptr )
   {
      return pDeckConcrete->GetStrengthDensity();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckWeightDensity() const
{
   ValidateConcrete();

   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[0].get(); // use region 0 because the deck material in all casting regions is the same

   if ( pDeckConcrete != nullptr )
   {
      return pDeckConcrete->GetWeightDensity();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckMaxAggrSize() const
{
   ValidateConcrete();
   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[0].get(); // use region 0 because the deck material in all casting regions is the same

   if ( pDeckConcrete != nullptr )
   {
      return pDeckConcrete->GetMaxAggregateSize();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckEccK1() const
{
   ValidateConcrete();
   Float64 K1 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      K1 = m_DeckEcK1;
   }
   return K1;
}

Float64 CConcreteManager::GetDeckEccK2() const
{
   ValidateConcrete();
   Float64 K2 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      K2 = m_DeckEcK2;
   }
   return K2;
}

Float64 CConcreteManager::GetDeckCreepK1() const
{
   ValidateConcrete();
   Float64 K1 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      K1 = m_DeckCreepK1;
   }
   return K1;
}

Float64 CConcreteManager::GetDeckCreepK2() const
{
   ValidateConcrete();
   Float64 K2 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      K2 = m_DeckCreepK2;
   }
   return K2;
};

Float64 CConcreteManager::GetDeckShrinkageK1() const
{
   ValidateConcrete();
   Float64 K1 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      K1 = m_DeckShrinkageK1;
   }
   return K1;
}

Float64 CConcreteManager::GetDeckShrinkageK2() const
{
   ValidateConcrete();
   Float64 K2 = 1.0;
   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      K2 = m_DeckShrinkageK2;
   }
   return K2;
};

Float64 CConcreteManager::GetRailingSystemDensity(pgsTypes::TrafficBarrierOrientation orientation) const
{
   ValidateConcrete();
   return m_pRailingConcrete[orientation]->GetWeightDensity();
}

Float64 CConcreteManager::GetRailingSystemCastingTime(pgsTypes::TrafficBarrierOrientation orientation) const
{
   ValidateConcrete();
   return m_pRailingConcrete[orientation]->GetTimeAtCasting();
}

Float64 CConcreteManager::GetRailingSystemFc(pgsTypes::TrafficBarrierOrientation orientation,Float64 t) const
{
   ValidateConcrete();
   return m_pRailingConcrete[orientation]->GetFc(t);
}

Float64 CConcreteManager::GetRailingSystemEc(pgsTypes::TrafficBarrierOrientation orientation,Float64 t) const
{
   ValidateConcrete();
   return m_pRailingConcrete[orientation]->GetEc(t);
}

Float64 CConcreteManager::GetRailingSystemFreeShrinkageStrain(pgsTypes::TrafficBarrierOrientation orientation,Float64 t) const
{
   return GetRailingSystemFreeShrinkageStrainDetails(orientation,t)->esh;
}

std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> CConcreteManager::GetRailingSystemFreeShrinkageStrainDetails(pgsTypes::TrafficBarrierOrientation orientation,Float64 t) const
{
   ValidateConcrete();
   ValidateRailingSystemConcrete();
   return m_pRailingConcrete[orientation]->GetFreeShrinkageStrainDetails(t);
}

Float64 CConcreteManager::GetRailingSystemCreepCoefficient(pgsTypes::TrafficBarrierOrientation orientation,Float64 t,Float64 tla) const
{
   ValidateConcrete();
   ValidateRailingSystemConcrete();
   return m_pRailingConcrete[orientation]->GetCreepCoefficient(t,tla);
}

std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> CConcreteManager::GetRailingSystemCreepCoefficientDetails(pgsTypes::TrafficBarrierOrientation orientation,Float64 t,Float64 tla) const
{
   ValidateConcrete();
   ValidateRailingSystemConcrete();
   return m_pRailingConcrete[orientation]->GetCreepCoefficientDetails(t,tla);
}

Float64 CConcreteManager::GetRailingSystemAgingCoefficient(pgsTypes::TrafficBarrierOrientation orientation,Float64 timeOfLoading) const
{
   ValidateConcrete();
   ValidateRailingSystemConcrete();
   return GetConcreteAgingCoefficient(m_pRailingConcrete[orientation],timeOfLoading);
}

const std::unique_ptr<WBFL::Materials::ConcreteBase>& CConcreteManager::GetRailingSystemConcrete(pgsTypes::TrafficBarrierOrientation orientation) const
{
   ValidateConcrete();
   ValidateRailingSystemConcrete();
   return m_pRailingConcrete[orientation];
}

pgsTypes::ConcreteType CConcreteManager::GetLongitudinalJointConcreteType() const
{
   ValidateConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return (pgsTypes::ConcreteType)m_pLongitudinalJointConcrete->GetType();
   }
   else
   {
      return pgsTypes::Normal;
   }
}

bool CConcreteManager::DoesLongitudinalJointConcreteHaveAggSplittingStrength() const
{
   ValidateConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return m_pLongitudinalJointConcrete->HasAggSplittingStrength();
   }
   else
   {
      return false;
   }
}

Float64 CConcreteManager::GetLongitudinalJointConcreteAggSplittingStrength() const
{
   ValidateConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return m_pLongitudinalJointConcrete->GetAggSplittingStrength();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetLongitudinalJointStrengthDensity() const
{
   ValidateConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return m_pLongitudinalJointConcrete->GetStrengthDensity();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetLongitudinalJointWeightDensity() const
{
   ValidateConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return m_pLongitudinalJointConcrete->GetWeightDensity();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetLongitudinalJointMaxAggrSize() const
{
   ValidateConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return m_pLongitudinalJointConcrete->GetMaxAggregateSize();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetLongitudinalJointEccK1() const
{
   ValidateConcrete();
   Float64 K1 = 1.0;
   if (WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition())
   {
      K1 = m_LongitudinalJointEcK1;
   }
   return K1;
}

Float64 CConcreteManager::GetLongitudinalJointEccK2() const
{
   ValidateConcrete();
   Float64 K2 = 1.0;
   if (WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition())
   {
      K2 = m_LongitudinalJointEcK2;
   }
   return K2;
}

Float64 CConcreteManager::GetLongitudinalJointCreepK1() const
{
   ValidateConcrete();
   Float64 K1 = 1.0;
   if (WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition())
   {
      K1 = m_LongitudinalJointCreepK1;
   }
   return K1;
}

Float64 CConcreteManager::GetLongitudinalJointCreepK2() const
{
   ValidateConcrete();
   Float64 K2 = 1.0;
   if (WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition())
   {
      K2 = m_LongitudinalJointCreepK2;
   }
   return K2;
};

Float64 CConcreteManager::GetLongitudinalJointShrinkageK1() const
{
   ValidateConcrete();
   Float64 K1 = 1.0;
   if (WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition())
   {
      K1 = m_LongitudinalJointShrinkageK1;
   }
   return K1;
}

Float64 CConcreteManager::GetLongitudinalJointShrinkageK2() const
{
   ValidateConcrete();
   Float64 K2 = 1.0;
   if (WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition())
   {
      K2 = m_LongitudinalJointShrinkageK2;
   }
   return K2;
};

const std::unique_ptr<WBFL::Materials::SimpleConcrete>& CConcreteManager::GetPierConcrete(PierIndexType pierIdx) const
{
   ValidateConcrete();
   std::map<PierIndexType,std::unique_ptr<WBFL::Materials::SimpleConcrete>>::iterator found(m_pPierConcrete.find(pierIdx));
   if ( found == m_pPierConcrete.end() )
   {
      // pier concrete models only exist for "physical" piers
      ATLASSERT(false);
      return m_NullConcrete;
   }

   return found->second;
}

Float64 CConcreteManager::GetSegmentLambda(const CSegmentKey& segmentKey) const
{
   ValidateConcrete();
   const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pSegmentConcrete[segmentKey].get());
   return (pConcrete ? pConcrete->GetLambda() : 1.0);
}

Float64 CConcreteManager::GetClosureJointLambda(const CClosureKey& closureKey) const
{
   ValidateConcrete();
   const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pClosureConcrete[closureKey].get());
   return pConcrete ? pConcrete->GetLambda() : 1.0;
}

Float64 CConcreteManager::GetDeckLambda() const
{
   auto* pDeckConcrete = m_pvDeckConcrete[0].get(); // use region 0 because the deck material in all casting regions is the same
   if ( pDeckConcrete != nullptr )
   {
      const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(pDeckConcrete);
      return pConcrete ? pConcrete->GetLambda() : 1.0;
   }
   else
   {
      return 1.0;
   }
}

Float64 CConcreteManager::GetRailingSystemLambda(pgsTypes::TrafficBarrierOrientation orientation) const
{
   ValidateConcrete();
   ValidateRailingSystemConcrete();

   const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pRailingConcrete[orientation].get());
   return pConcrete ? pConcrete->GetLambda() : 1.0;
}

Float64 CConcreteManager::GetLongitudinalJointLambda() const
{
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pLongitudinalJointConcrete.get());
      return pConcrete ? pConcrete->GetLambda() : 1.0;
   }
   else
   {
      return 1.0;
   }
}

Float64 CConcreteManager::GetNWCDensityLimit() const
{
   return WBFL::LRFD::ConcreteUtil::GetNWCDensityLimit();
}

Float64 CConcreteManager::GetLWCDensityLimit() const
{
   return WBFL::LRFD::ConcreteUtil::GetLWCDensityLimit();
}

Float64 CConcreteManager::GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type) const
{
   return WBFL::LRFD::ConcreteUtil::ModRupture( fc, GetFlexureFrCoefficient(type) );
}

Float64 CConcreteManager::GetFlexureFrCoefficient(pgsTypes::ConcreteType type) const
{
   if (IsUHPC(type))
      return 0;

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& moment_capacity_criteria = pSpecEntry->GetMomentCapacityCriteria();
   return moment_capacity_criteria.GetModulusOfRuptureCoefficient(type);
}

Float64 CConcreteManager::GetSegmentFlexureFrCoefficient(const CSegmentKey& segmentKey) const
{
   pgsTypes::ConcreteType type = GetSegmentConcreteType(segmentKey);
   return GetFlexureFrCoefficient(type);
}

Float64 CConcreteManager::GetClosureJointFlexureFrCoefficient(const CClosureKey& closureKey) const
{
   pgsTypes::ConcreteType type = GetClosureJointConcreteType(closureKey);
   return GetFlexureFrCoefficient(type);
}

Float64 CConcreteManager::GetShearModRupture(Float64 fc,pgsTypes::ConcreteType type) const
{
   return WBFL::LRFD::ConcreteUtil::ModRupture( fc, GetShearFrCoefficient(type) );
}

Float64 CConcreteManager::GetEconc(pgsTypes::ConcreteType type, Float64 fc,Float64 density,Float64 K1,Float64 K2) const
{
   return K1*K2*WBFL::LRFD::ConcreteUtil::ModE((WBFL::Materials::ConcreteType)type, fc,density, false ); // ignore LRFD limits
}

bool CConcreteManager::HasUHPC() const
{
   ValidateConcrete();
   ValidateSegmentConcrete();

   for (auto& item : m_pSegmentConcrete)
   {
      if (IsUHPC(item.second->GetType()))
      {
         return true;
      }
   }

   for (auto& item : m_pClosureConcrete)
   {
      if (IsUHPC(item.second->GetType()))
      {
         return true;
      }
   }

   //ValidateDeckConcrete();
   //for (auto& item : m_pvDeckConcrete)
   //{
   //   if (item->GetType() == WBFL::Materials::ConcreteType::PCI_UHPC || item->GetType() == WBFL::Materials::ConcreteType::UHPC)
   //   {
   //      return true;
   //   }
   //}

   ValidateLongitudinalJointConcrete();
   if (m_pLongitudinalJointConcrete != nullptr && (IsUHPC(m_pLongitudinalJointConcrete->GetType())))
   {
      return true;
   }

   //ValidateRailingSystemConcrete();
   //for (auto& item : m_pRailingConcrete)
   //{
   //   if (item->GetType() == WBFL::Materials::ConcreteType::PCI_UHPC || item->GetType() == WBFL::Materials::ConcreteType::UHPC)
   //   {
   //      ATLASSERT(false); // the UI should prevent UHPC for railings
   //      return true;
   //   }
   //}

   //for (auto& item : m_pPierConcrete)
   //{
   //   if (item.second->GetType() == WBFL::Materials::ConcreteType::PCI_UHPC || item.second->GetType() == WBFL::Materials::ConcreteType::UHPC)
   //   {
   //      ATLASSERT(false); // the UI should prevent UHPC for piers
   //      return true;
   //   }
   //}

   return false;
}


Float64 CConcreteManager::GetShearFrCoefficient(pgsTypes::ConcreteType type) const
{
   if (IsUHPC(type))
      return 0;

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& shear_capacity_criteria = pSpecEntry->GetShearCapacityCriteria();
   return shear_capacity_criteria.ModulusOfRuptureCoefficient[type];
}

Float64 CConcreteManager::GetSegmentShearFrCoefficient(const CSegmentKey& segmentKey) const
{
   pgsTypes::ConcreteType type = GetSegmentConcreteType(segmentKey);
   return GetShearFrCoefficient(type);
}

Float64 CConcreteManager::GetClosureJointShearFrCoefficient(const CClosureKey& closureKey) const
{
   pgsTypes::ConcreteType type = GetClosureJointConcreteType(closureKey);
   return GetShearFrCoefficient(type);
}

Float64 CConcreteManager::GetDeckCastingTime(IndexType castingRegionIdx) const
{
   ValidateConcrete();
   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[castingRegionIdx].get(); // use region 0 because the deck material in all casting regions is the same
   if ( pDeckConcrete != nullptr )
   {
      return pDeckConcrete->GetTimeAtCasting();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckFc(IndexType castingRegionIdx,Float64 t) const
{
   ValidateConcrete();
   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[castingRegionIdx].get(); // use region 0 because the deck material in all casting regions is the same

   if (pDeckConcrete != nullptr )
   {
      return pDeckConcrete->GetFc(t);
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckEc(IndexType castingRegionIdx,Float64 t) const
{
   ValidateConcrete();
   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[castingRegionIdx].get(); // use region 0 because the deck material in all casting regions is the same
   if (pDeckConcrete != nullptr )
   {
      return pDeckConcrete->GetEc(t);
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckFlexureFr(IndexType castingRegionIdx,Float64 t) const
{
   ValidateConcrete();
   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[castingRegionIdx].get(); // use region 0 because the deck material in all casting regions is the same
   if (pDeckConcrete != nullptr )
   {
      return pDeckConcrete->GetFlexureFr(t);
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckShearFr(IndexType castingRegionIdx,Float64 t) const
{
   ValidateConcrete();
   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[castingRegionIdx].get(); // use region 0 because the deck material in all casting regions is the same
   if (pDeckConcrete != nullptr )
   {
      return pDeckConcrete->GetShearFr(t);
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckFreeShrinkageStrain(IndexType castingRegionIdx,Float64 t) const
{
   const auto& pDetails = GetDeckFreeShrinkageStrainDetails(castingRegionIdx,t);
   if ( pDetails )
   {
      return pDetails->esh;
   }
   else
   {
      return 0;
   }
}

std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> CConcreteManager::GetDeckFreeShrinkageStrainDetails(IndexType castingRegionIdx,Float64 t) const
{
   ValidateConcrete();
   ValidateDeckConcrete();
   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[castingRegionIdx].get(); // use region 0 because the deck material in all casting regions is the same
   if (pDeckConcrete != nullptr )
   {
      return pDeckConcrete->GetFreeShrinkageStrainDetails(t);
   }
   else
   {
      return nullptr;
   }
}

Float64 CConcreteManager::GetDeckCreepCoefficient(IndexType castingRegionIdx,Float64 t,Float64 tla) const
{
   ValidateConcrete();
   ValidateDeckConcrete();
   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[castingRegionIdx].get(); // use region 0 because the deck material in all casting regions is the same
   if (pDeckConcrete != nullptr )
   {
      return pDeckConcrete->GetCreepCoefficient(t,tla);
   }
   else
   {
      return 0;
   }
}

std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> CConcreteManager::GetDeckCreepCoefficientDetails(IndexType castingRegionIdx,Float64 t,Float64 tla) const
{
   ValidateConcrete();
   ValidateDeckConcrete();
   auto* pDeckConcrete = m_pvDeckConcrete.empty() ? nullptr : m_pvDeckConcrete[castingRegionIdx].get(); // use region 0 because the deck material in all casting regions is the same
   if (pDeckConcrete != nullptr )
   {
      return pDeckConcrete->GetCreepCoefficientDetails(t,tla);
   }
   else
   {
      return nullptr;
   }
}

Float64 CConcreteManager::GetDeckAgingCoefficient(IndexType castingRegionIdx, Float64 timeOfLoading) const
{
   ValidateConcrete();
   ValidateDeckConcrete();
   if (m_pvDeckConcrete.empty() || m_pvDeckConcrete[castingRegionIdx] == nullptr)
   {
      return 0;
   }
   else
   {
      return GetConcreteAgingCoefficient(m_pvDeckConcrete[castingRegionIdx], timeOfLoading);
   }
}

Float64 CConcreteManager::GetDeckConcreteFirstCrackingStrength() const
{
   IndexType castingRegionIdx = 0;
   ValidateConcrete();
   ValidateDeckConcrete();
   if (m_pvDeckConcrete.empty() || m_pvDeckConcrete[castingRegionIdx] == nullptr)
   {
      return 0;
   }
   else
   {
      const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pvDeckConcrete[castingRegionIdx].get());
      return pConcrete->GetFirstCrackingStrength();
   }
}

Float64 CConcreteManager::GetDeckAutogenousShrinkage() const
{
   IndexType castingRegionIdx = 0;
   ValidateConcrete();
   ValidateDeckConcrete();
   if (m_pvDeckConcrete.empty() || m_pvDeckConcrete[castingRegionIdx] == nullptr)
   {
      return 0;
   }
   else
   {
      const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pvDeckConcrete[castingRegionIdx].get());
      return pConcrete->GetAutogenousShrinkage();
   }
}

const std::unique_ptr<WBFL::Materials::ConcreteBase>& CConcreteManager::GetDeckConcrete(IndexType castingRegionIdx) const
{
   ValidateConcrete();
   ValidateDeckConcrete();
   const auto& pDeckConcrete = m_pvDeckConcrete.empty() ? m_NullConcreteBase : m_pvDeckConcrete[castingRegionIdx]; // use region 0 because the deck material in all casting regions is the same
   return pDeckConcrete;
}

Float64 CConcreteManager::GetSegmentCastingTime(const CSegmentKey& segmentKey) const
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetTimeAtCasting();
}

Float64 CConcreteManager::GetSegmentFc(const CSegmentKey& segmentKey,Float64 t) const
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetFc(t);
}

Float64 CConcreteManager::GetSegmentEc(const CSegmentKey& segmentKey,Float64 t) const
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetEc(t);
}

Float64 CConcreteManager::GetSegmentFlexureFr(const CSegmentKey& segmentKey,Float64 t) const
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetFlexureFr(t);
}

Float64 CConcreteManager::GetSegmentShearFr(const CSegmentKey& segmentKey,Float64 t) const
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetShearFr(t);
}

Float64 CConcreteManager::GetSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,Float64 t) const
{
   return GetSegmentFreeShrinkageStrainDetails(segmentKey,t)->esh;
}

std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> CConcreteManager::GetSegmentFreeShrinkageStrainDetails(const CSegmentKey& segmentKey,Float64 t) const
{
   ValidateConcrete();
   ValidateSegmentConcrete();
   return m_pSegmentConcrete[segmentKey]->GetFreeShrinkageStrainDetails(t);
}

Float64 CConcreteManager::GetSegmentCreepCoefficient(const CSegmentKey& segmentKey,Float64 t,Float64 tla) const
{
   ValidateConcrete();
   ValidateSegmentConcrete();
   return m_pSegmentConcrete[segmentKey]->GetCreepCoefficient(t,tla);
}

std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> CConcreteManager::GetSegmentCreepCoefficientDetails(const CSegmentKey& segmentKey,Float64 t,Float64 tla) const
{
   ValidateConcrete();
   ValidateSegmentConcrete();
   return m_pSegmentConcrete[segmentKey]->GetCreepCoefficientDetails(t,tla);
}

Float64 CConcreteManager::GetSegmentAgingCoefficient(const CSegmentKey& segmentKey,Float64 timeOfLoading) const
{
   ValidateConcrete();
   ValidateSegmentConcrete();
   return GetConcreteAgingCoefficient(m_pSegmentConcrete[segmentKey],timeOfLoading);
}

Float64 CConcreteManager::GetSegmentConcreteFirstCrackingStrength(const CSegmentKey& segmentKey) const
{
   ValidateConcrete();
   ValidateSegmentConcrete();
   if (m_pSegmentConcrete[segmentKey]->GetType() == WBFL::Materials::ConcreteType::PCI_UHPC)
   {
      const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pSegmentConcrete[segmentKey].get());
      return pConcrete->GetFirstCrackingStrength();
   }
   else
   {
      ATLASSERT(false); // this is a call for PCI UHPC only so how did you get here?
      return 0;
   }
}

Float64 CConcreteManager::GetSegmentAutogenousShrinkage(const CSegmentKey& segmentKey) const
{
   ValidateConcrete();
   ValidateSegmentConcrete();
   if (m_pSegmentConcrete[segmentKey]->GetType() == WBFL::Materials::ConcreteType::PCI_UHPC)
   {
      const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pSegmentConcrete[segmentKey].get());
      return pConcrete->GetAutogenousShrinkage();
   }
   else
   {
      ATLASSERT(false); // this is a call for PCI UHPC only so how did you get here?
      return 0;
   }
}

Float64 CConcreteManager::GetSegmentConcreteInitialEffectiveCrackingStrength(const CSegmentKey& segmentKey) const
{
   // returns ft,cri
   ValidateConcrete();
   ValidateSegmentConcrete();
   if (m_pSegmentConcrete[segmentKey]->GetType() == WBFL::Materials::ConcreteType::UHPC)
   {
      const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pSegmentConcrete[segmentKey].get());
      return pConcrete->GetInitialEffectiveCrackingStrength();
   }
   else
   {
      ATLASSERT(false); // this is a call for UHPC only so how did you get here?
      return 0;
   }
}

Float64 CConcreteManager::GetSegmentConcreteDesignEffectiveCrackingStrength(const CSegmentKey& segmentKey) const
{
   // returns ft,cr
   ValidateConcrete();
   ValidateSegmentConcrete();
   if (m_pSegmentConcrete[segmentKey]->GetType() == WBFL::Materials::ConcreteType::UHPC)
   {
      const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pSegmentConcrete[segmentKey].get());
      return pConcrete->GetDesignEffectiveCrackingStrength();
   }
   else
   {
      ATLASSERT(false); // this is a call for UHPC only so how did you get here?
      return 0;
   }
}

Float64 CConcreteManager::GetSegmentConcreteCrackLocalizationStrength(const CSegmentKey& segmentKey) const
{
   // returns ft,loc
   ValidateConcrete();
   ValidateSegmentConcrete();
   if (m_pSegmentConcrete[segmentKey]->GetType() == WBFL::Materials::ConcreteType::UHPC)
   {
      const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pSegmentConcrete[segmentKey].get());
      return pConcrete->GetCrackLocalizationStrength();
   }
   else
   {
      ATLASSERT(false); // this is a call for PCI UHPC only so how did you get here?
      return 0;
   }
}

Float64 CConcreteManager::GetSegmentConcreteCrackLocalizationStrain(const CSegmentKey& segmentKey) const
{
   // returns et,loc
   ValidateConcrete();
   ValidateSegmentConcrete();
   if (m_pSegmentConcrete[segmentKey]->GetType() == WBFL::Materials::ConcreteType::UHPC)
   {
      const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pSegmentConcrete[segmentKey].get());
      return pConcrete->GetCrackLocalizationStrain();
   }
   else
   {
      ATLASSERT(false); // this is a call for PCI UHPC only so how did you get here?
      return 0;
   }
}

Float64 CConcreteManager::GetSegmentConcreteFiberOrientationReductionFactor(const CSegmentKey& segmentKey) const
{
   // returns gamma,u
   ValidateConcrete();
   ValidateSegmentConcrete();
   if (m_pSegmentConcrete[segmentKey]->GetType() == WBFL::Materials::ConcreteType::UHPC)
   {
      const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pSegmentConcrete[segmentKey].get());
      return pConcrete->GetFiberOrientationReductionFactor();
   }
   else
   {
      ATLASSERT(false); // this is a call for PCI UHPC only so how did you get here?
      return 0;
   }
}

const std::unique_ptr<WBFL::Materials::ConcreteBase>& CConcreteManager::GetSegmentConcrete(const CSegmentKey& segmentKey) const
{
   ValidateConcrete();
   ValidateSegmentConcrete();
   return m_pSegmentConcrete[segmentKey];
}

Float64 CConcreteManager::GetClosureJointCastingTime(const CClosureKey& closureKey) const
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetTimeAtCasting();
}

Float64 CConcreteManager::GetClosureJointFc(const CClosureKey& closureKey,Float64 t) const
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetFc(t);
}

Float64 CConcreteManager::GetClosureJointFlexureFr(const CClosureKey& closureKey,Float64 t) const
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetFlexureFr(t);
}

Float64 CConcreteManager::GetClosureJointShearFr(const CClosureKey& closureKey,Float64 t) const
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetShearFr(t);
}

Float64 CConcreteManager::GetClosureJointEc(const CClosureKey& closureKey,Float64 t) const
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetEc(t);
}

Float64 CConcreteManager::GetClosureJointFreeShrinkageStrain(const CClosureKey& closureKey,Float64 t) const
{
   return GetClosureJointFreeShrinkageStrainDetails(closureKey,t)->esh;
}

std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> CConcreteManager::GetClosureJointFreeShrinkageStrainDetails(const CClosureKey& closureKey,Float64 t) const
{
   ValidateConcrete();
   ValidateSegmentConcrete();
   return m_pClosureConcrete[closureKey]->GetFreeShrinkageStrainDetails(t);
}

Float64 CConcreteManager::GetClosureJointCreepCoefficient(const CClosureKey& closureKey,Float64 t,Float64 tla) const
{
   ValidateConcrete();
   ValidateSegmentConcrete();
   return m_pClosureConcrete[closureKey]->GetCreepCoefficient(t,tla);
}

std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> CConcreteManager::GetClosureJointCreepCoefficientDetails(const CClosureKey& closureKey,Float64 t,Float64 tla) const
{
   ValidateConcrete();
   ValidateSegmentConcrete();
   return m_pClosureConcrete[closureKey]->GetCreepCoefficientDetails(t,tla);
}

Float64 CConcreteManager::GetClosureJointAgingCoefficient(const CClosureKey& closureKey,Float64 timeOfLoading) const
{
   ValidateConcrete();
   ValidateSegmentConcrete();
   return GetConcreteAgingCoefficient(m_pClosureConcrete[closureKey],timeOfLoading);
}

const std::unique_ptr<WBFL::Materials::ConcreteBase>& CConcreteManager::GetClosureJointConcrete(const CClosureKey& closureKey) const
{
   ValidateConcrete();
   ValidateSegmentConcrete();
   return m_pClosureConcrete[closureKey];
}

Float64 CConcreteManager::GetClosureJointAutogenousShrinkage(const CClosureKey& closureKey) const
{
   ValidateConcrete();
   ValidateSegmentConcrete();
   if (m_pClosureConcrete[closureKey]->GetType() == WBFL::Materials::ConcreteType::PCI_UHPC)
   {
      const WBFL::LRFD::LRFDConcreteBase* pConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(m_pClosureConcrete[closureKey].get());
      return pConcrete->GetAutogenousShrinkage();
   }
   else
   {
      ATLASSERT(false); // this is a call for UHPC only so how did you get here?
      return 0;
   }
}

Float64 CConcreteManager::GetLongitudinalJointCastingTime() const
{
   ValidateConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return m_pLongitudinalJointConcrete->GetTimeAtCasting();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetLongitudinalJointFc(Float64 t) const
{
   ValidateConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return m_pLongitudinalJointConcrete->GetFc(t);
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetLongitudinalJointEc(Float64 t) const
{
   ValidateConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return m_pLongitudinalJointConcrete->GetEc(t);
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetLongitudinalJointFlexureFr(Float64 t) const
{
   ValidateConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return m_pLongitudinalJointConcrete->GetFlexureFr(t);
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetLongitudinalJointShearFr(Float64 t) const
{
   ValidateConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return m_pLongitudinalJointConcrete->GetShearFr(t);
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetLongitudinalJointFreeShrinkageStrain(Float64 t) const
{
   const auto& pDetails = GetLongitudinalJointFreeShrinkageStrainDetails(t);
   if (pDetails)
   {
      return pDetails->esh;
   }
   else
   {
      return 0;
   }
}

std::unique_ptr<WBFL::Materials::ConcreteBaseShrinkageDetails> CConcreteManager::GetLongitudinalJointFreeShrinkageStrainDetails(Float64 t) const
{
   ValidateConcrete();
   ValidateLongitudinalJointConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return m_pLongitudinalJointConcrete->GetFreeShrinkageStrainDetails(t);
   }
   else
   {
      return nullptr;
   }
}

Float64 CConcreteManager::GetLongitudinalJointCreepCoefficient(Float64 t, Float64 tla) const
{
   ValidateConcrete();
   ValidateLongitudinalJointConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return m_pLongitudinalJointConcrete->GetCreepCoefficient(t, tla);
   }
   else
   {
      return 0;
   }
}

std::unique_ptr<WBFL::Materials::ConcreteBaseCreepDetails> CConcreteManager::GetLongitudinalJointCreepCoefficientDetails(Float64 t, Float64 tla) const
{
   ValidateConcrete();
   ValidateLongitudinalJointConcrete();
   if (m_pLongitudinalJointConcrete.get() != nullptr)
   {
      return m_pLongitudinalJointConcrete->GetCreepCoefficientDetails(t, tla);
   }
   else
   {
      return nullptr;
   }
}

Float64 CConcreteManager::GetLongitudinalJointAgingCoefficient(Float64 timeOfLoading) const
{
   ValidateConcrete();
   ValidateLongitudinalJointConcrete();
   if (m_pLongitudinalJointConcrete != nullptr)
   {
      return GetConcreteAgingCoefficient(m_pLongitudinalJointConcrete, timeOfLoading);
   }
   else
   {
      return 0;
   }
}

const std::unique_ptr<WBFL::Materials::ConcreteBase>& CConcreteManager::GetLongitudinalJointConcrete() const
{
   ValidateConcrete();
   ValidateLongitudinalJointConcrete();
   return m_pLongitudinalJointConcrete;
}

std::unique_ptr<WBFL::LRFD::LRFDConcrete> CConcreteManager::CreateLRFDConcreteModel(const CConcreteMaterial& concrete,Float64 startTime,Float64 stepTime) const
{
   // this concrete model is simple step function. fc and E are constant at f'ci and Eci from t = 0 to
   // just before t = stepTime. From t = stepTime and beyond fc and E are f'c and Ec.
   //
   //     ^ f'c or Ec
   //     |
   //     |
   //     |                        +-------------------- f'c, Ec
   //     |                        |
   //     |                        |
   //     |                        |
   //     |                        |
   //     |                        |
   //     |       +----------------+  f'ci, Eci
   //     |       |
   //     +-------+----------------+-------------------------> t = time
   //             t = startTime    t = stepTime

   std::unique_ptr<WBFL::LRFD::LRFDConcrete> pLRFDConcrete(std::make_unique<WBFL::LRFD::LRFDConcrete>());

   WBFL::Materials::SimpleConcrete initialConcrete, finalConcrete;
   CreateConcrete(concrete,_T(""),&initialConcrete,&finalConcrete);
   pLRFDConcrete->SetConcreteModels(initialConcrete,finalConcrete);
   pLRFDConcrete->SetStartTime(startTime);
   pLRFDConcrete->SetStepTime(stepTime);

   pLRFDConcrete->SetEcCorrectionFactors(concrete.EcK1,concrete.EcK2);
   pLRFDConcrete->SetCreepCorrectionFactors(concrete.CreepK1,concrete.CreepK2);
   pLRFDConcrete->SetShrinkageCorrectionFactors(concrete.ShrinkageK1,concrete.ShrinkageK2);

   Float64 lambda = WBFL::LRFD::ConcreteUtil::ComputeConcreteDensityModificationFactor((WBFL::Materials::ConcreteType)concrete.Type,concrete.StrengthDensity,concrete.bHasFct,concrete.Fct,concrete.Fc);
   pLRFDConcrete->SetLambda(lambda);

   if (concrete.Type == pgsTypes::Normal && (stepTime-startTime) < 90)
   {
      GET_IFACE(ILibrary, pLib);
      GET_IFACE(ISpecification, pSpec);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
      const auto& limit_state_concrete_strength_criteria = pSpecEntry->GetLimitStateConcreteStrengthCriteria();
      if (limit_state_concrete_strength_criteria.bUse90DayConcreteStrength && limit_state_concrete_strength_criteria.SlowCuringConcreteStrengthFactor != 1.0)
      {
         CConcreteMaterial concrete90(concrete);
         concrete90.Fc *= limit_state_concrete_strength_criteria.SlowCuringConcreteStrengthFactor;
         WBFL::Materials::SimpleConcrete initialConcrete, finalConcrete90;
         CreateConcrete(concrete90, _T(""), &initialConcrete, &finalConcrete90);
         pLRFDConcrete->Use90DayStrength(finalConcrete90);
      }
   }

   // PCI UHPC
   pLRFDConcrete->SetFirstCrackingStrength(concrete.Ffc);
   pLRFDConcrete->SetPostCrackingTensileStrength(concrete.Frr);
   pLRFDConcrete->SetAutogenousShrinkage(concrete.AutogenousShrinkage);

   // UHPC
   pLRFDConcrete->SetCompressionResponseReductionFactor(concrete.alpha_u);
   if(concrete.bExperimental_ecu)  pLRFDConcrete->SetCompressiveStrainLimit(concrete.ecu);
   pLRFDConcrete->SetInitialEffectiveCrackingStrength(concrete.ftcri);
   pLRFDConcrete->SetDesignEffectiveCrackingStrength(concrete.ftcr);
   pLRFDConcrete->SetCrackLocalizationStrength(concrete.ftloc);
   pLRFDConcrete->SetCrackLocalizationStrain(concrete.etloc);
   pLRFDConcrete->SetFiberOrientationReductionFactor(concrete.gamma_u);

   return pLRFDConcrete;
}

std::unique_ptr<WBFL::LRFD::LRFDTimeDependentConcrete> CConcreteManager::CreateTimeDependentLRFDConcreteModel(const CConcreteMaterial& concrete,Float64 ageAtInitialLoading) const
{
   std::unique_ptr<WBFL::LRFD::LRFDTimeDependentConcrete> pConcrete(std::make_unique<WBFL::LRFD::LRFDTimeDependentConcrete>());

   Float64 A,B;
   if ( concrete.bACIUserParameters )
   {
      A = concrete.A;
      B = concrete.B;
   }
   else
   {
      WBFL::LRFD::LRFDTimeDependentConcrete::GetModelParameters((WBFL::Materials::CuringType)concrete.CureMethod,
                                                        (WBFL::Materials::CementType)concrete.ACI209CementType,
                                                       &A,&B);
   }

   pConcrete->SetA(A);
   pConcrete->SetBeta(B);

   if ( concrete.bBasePropertiesOnInitialValues )
   {
      // back out Fc28 and Ec28 based on properties at time of casting
      pConcrete->SetFc28(concrete.Fci,ageAtInitialLoading);
      pConcrete->UserEc28(concrete.bUserEci);
      pConcrete->SetEc28(concrete.Eci,ageAtInitialLoading);
   }
   else
   {
      pConcrete->SetFc28(concrete.Fc);
      pConcrete->UserEc28(concrete.bUserEc);
      pConcrete->SetEc28(concrete.Ec);
   }

   pConcrete->SetEcCorrectionFactors(concrete.EcK1,concrete.EcK2);
   pConcrete->SetCreepCorrectionFactors(concrete.CreepK1,concrete.CreepK2);
   pConcrete->SetShrinkageCorrectionFactors(concrete.ShrinkageK1,concrete.ShrinkageK2);

   pConcrete->SetShearModulusOfRuptureCoefficient(GetShearFrCoefficient(concrete.Type));
   pConcrete->SetFlexureModulusOfRuptureCoefficient(GetFlexureFrCoefficient(concrete.Type));

   Float64 lambda = WBFL::LRFD::ConcreteUtil::ComputeConcreteDensityModificationFactor((WBFL::Materials::ConcreteType)concrete.Type,concrete.StrengthDensity,concrete.bHasFct,concrete.Fct,concrete.Fc);
   pConcrete->SetLambda(lambda);

   // PCI UHPC
   pConcrete->SetFirstCrackingStrength(concrete.Ffc);
   pConcrete->SetPostCrackingTensileStrength(concrete.Frr);
   pConcrete->SetAutogenousShrinkage(concrete.AutogenousShrinkage);

   // UHPC
   pConcrete->SetCompressionResponseReductionFactor(concrete.alpha_u);
   if(concrete.bExperimental_ecu) pConcrete->SetCompressiveStrainLimit(concrete.ecu);
   pConcrete->SetInitialEffectiveCrackingStrength(concrete.ftcri);
   pConcrete->SetDesignEffectiveCrackingStrength(concrete.ftcr);
   pConcrete->SetCrackLocalizationStrength(concrete.ftloc);
   pConcrete->SetCrackLocalizationStrain(concrete.etloc);
   pConcrete->SetFiberOrientationReductionFactor(concrete.gamma_u);

   return pConcrete;
}

std::unique_ptr<WBFL::Materials::ACI209Concrete> CConcreteManager::CreateACI209Model(const CConcreteMaterial& concrete,Float64 ageAtInitialLoading) const
{
   std::unique_ptr<WBFL::Materials::ACI209Concrete> pConcrete(std::make_unique<WBFL::Materials::ACI209Concrete>());

   Float64 A,B;
   if ( concrete.bACIUserParameters )
   {
      A = concrete.A;
      B = concrete.B;
   }
   else
   {
      WBFL::Materials::ACI209Concrete::GetModelParameters((WBFL::Materials::CuringType)concrete.CureMethod,
                                            (WBFL::Materials::CementType)concrete.ACI209CementType,
                                            &A,&B);
   }

   pConcrete->SetA(A);
   pConcrete->SetBeta(B);

   if ( concrete.bBasePropertiesOnInitialValues )
   {
      // back out Fc28 and Ec28 based on properties at time of casting
      pConcrete->SetFc28(concrete.Fci,ageAtInitialLoading);
      pConcrete->UserEc28(concrete.bUserEci);
      pConcrete->SetEc28(concrete.Eci,ageAtInitialLoading);
   }
   else
   {
      pConcrete->SetFc28(concrete.Fc);
      pConcrete->UserEc28(concrete.bUserEc);
      pConcrete->SetEc28(concrete.Ec);
   }

   return pConcrete;
}

std::unique_ptr<WBFL::Materials::CEBFIPConcrete> CConcreteManager::CreateCEBFIPModel(const CConcreteMaterial& concrete,Float64 ageAtInitialLoading) const
{
   std::unique_ptr<WBFL::Materials::CEBFIPConcrete> pConcrete(std::make_unique<WBFL::Materials::CEBFIPConcrete>());

   Float64 S,BetaSc;
   if ( concrete.bCEBFIPUserParameters )
   {
      S = concrete.S;
      BetaSc = concrete.BetaSc;
   }
   else
   {
      WBFL::Materials::CEBFIPConcrete::GetModelParameters((WBFL::Materials::CEBFIPConcrete::CementType)concrete.CEBFIPCementType,
                                            &S,&BetaSc);
   }

   pConcrete->SetS(S);
   pConcrete->SetBetaSc(BetaSc);

   if ( concrete.bBasePropertiesOnInitialValues )
   {
      pConcrete->SetFc28(concrete.Fci,ageAtInitialLoading);
      pConcrete->UserEc28(concrete.bUserEci);
      pConcrete->SetEc28(concrete.Eci,ageAtInitialLoading);
   }
   else
   {
      pConcrete->SetFc28(concrete.Fc);
      pConcrete->UserEc28(concrete.bUserEc);
      pConcrete->SetEc28(concrete.Ec);
   }

   return pConcrete;
}

Float64 CConcreteManager::GetConcreteAgingCoefficient(const std::unique_ptr<WBFL::Materials::ConcreteBase>& pConcrete,Float64 timeOfLoading) const
{
   return 1.0;
   //// based on "Approximate expressions for the Aging coefficient and the relaxation function in the viscoelastic
   //// analysis of concrete structures", G. Lacidogna,, M. Tarantino. "Materials and Structures", Vol 29, April 1996, pp 131-140
   //Float64 age = pConcrete->GetAge(timeOfLoading);
   //if ( age < 0 )
   //{
   //   return 0;
   //}

   //Float64 sqrt_age = sqrt(age);
   //Float64 X = sqrt_age/(1 + sqrt_age);
   //return X;
}

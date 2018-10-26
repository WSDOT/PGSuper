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

#include "stdafx.h"
#include "ConcreteManager.h"
#include <EAF\EAFStatusCenter.h>
#include <EAF\EAFDisplayUnits.h>

#include <IFace\Project.h>
#include <IFace\Intervals.h>
#include <IFace\Bridge.h>

#include <Lrfd\ConcreteUtil.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>
#include <PgsExt\GirderLabel.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CConcreteManager::CConcreteManager()
{
   m_bIsValidated = false;
   m_bIsValidated2 = false;
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
   m_pDeckConc = std::auto_ptr<matConcreteBase>(0);
   m_pClosureConcrete.clear();
   m_pSegmentConcrete.clear();
   m_pRailingConc[pgsTypes::tboLeft]  = std::auto_ptr<matConcreteBase>(0);
   m_pRailingConc[pgsTypes::tboRight] = std::auto_ptr<matConcreteBase>(0);

   m_bIsValidated = false;
   m_bIsValidated2 = false;
}

void CConcreteManager::ValidateConcrete()
{
   if ( m_bIsValidated )
   {
      return;
   }

   GET_IFACE(IIntervals,pIntervals);

   const CTimelineManager* pTimelineMgr = m_pBridgeDesc->GetTimelineManager();

   //////////////////////////////////////////////////////////////////////////////
   //
   // Create Deck concrete
   //
   //////////////////////////////////////////////////////////////////////////////
   const CDeckDescription2* pDeck = m_pBridgeDesc->GetDeckDescription();
   if ( pDeck->DeckType != pgsTypes::sdtNone )
   {
      Float64 modE;
      if ( pDeck->Concrete.bUserEc )
      {
         modE = pDeck->Concrete.Ec;
      }
      else
      {
         modE = lrfdConcreteUtil::ModE(pDeck->Concrete.Fc, 
                                       pDeck->Concrete.StrengthDensity, 
                                       false /* ignore LRFD range checks */ );

         if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
         {
            modE *= (pDeck->Concrete.EcK1*pDeck->Concrete.EcK2);
         }
      }

      // save these parameters for later so we don't have to look them up every time they are needed
      m_DeckEcK1        = pDeck->Concrete.EcK1;
      m_DeckEcK2        = pDeck->Concrete.EcK2;
      m_DeckCreepK1     = pDeck->Concrete.CreepK1;
      m_DeckCreepK2     = pDeck->Concrete.CreepK2;
      m_DeckShrinkageK1 = pDeck->Concrete.ShrinkageK1;
      m_DeckShrinkageK2 = pDeck->Concrete.ShrinkageK2;

      // Time dependent model
      EventIndexType castDeckEventIdx  = pTimelineMgr->GetCastDeckEventIndex();
      Float64   time_at_casting        = pTimelineMgr->GetStart(castDeckEventIdx);
      Float64   age_at_initial_loading = pTimelineMgr->GetEventByIndex(castDeckEventIdx)->GetCastDeckActivity().GetConcreteAgeAtContinuity();
      Float64   cure_time              = age_at_initial_loading;

      // modulus of rupture coefficients
      Float64 time_step = time_at_casting + cure_time;
      m_pDeckConc.reset( CreateConcreteModel(_T("Deck Concrete"),pDeck->Concrete,time_at_casting,cure_time,age_at_initial_loading,time_step) );
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
   if ( pLeftRailingSystem->Concrete.bUserEc )
   {
      modE = pLeftRailingSystem->Concrete.Ec;
   }
   else
   {
      modE = lrfdConcreteUtil::ModE(pLeftRailingSystem->Concrete.Fc, 
                                    pLeftRailingSystem->Concrete.StrengthDensity, 
                                    false /* ignore LRFD range checks */ );

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         modE *= pLeftRailingSystem->Concrete.EcK1*pLeftRailingSystem->Concrete.EcK2;
      }
   }

   Float64 time_step = time_at_casting + cure_time;
   m_pRailingConc[pgsTypes::tboLeft].reset( CreateConcreteModel(_T("Left Railing Concrete"), pLeftRailingSystem->Concrete,time_at_casting,cure_time,age_at_initial_loading,time_step) );

   const CRailingSystem* pRightRailingSystem = m_pBridgeDesc->GetRightRailingSystem();
   if ( pRightRailingSystem->Concrete.bUserEc )
   {
      modE = pRightRailingSystem->Concrete.Ec;
   }
   else
   {
      modE = lrfdConcreteUtil::ModE(pRightRailingSystem->Concrete.Fc, 
                                    pRightRailingSystem->Concrete.StrengthDensity, 
                                    false /* ignore LRFD range checks */ );

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         modE *= pRightRailingSystem->Concrete.EcK1*pRightRailingSystem->Concrete.EcK2;
      }
   }

   m_pRailingConc[pgsTypes::tboRight].reset( CreateConcreteModel(_T("Right Railing Concrete"), pRightRailingSystem->Concrete,time_at_casting,cure_time,age_at_initial_loading,time_step) );
   
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

            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

            SegmentIDType segmentID = pSegment->GetID();

            EventIndexType segConstructEventIdx = pTimelineMgr->GetSegmentConstructionEventIndex(segmentID);
            Float64 segment_casting_time        = pTimelineMgr->GetStart(segConstructEventIdx);
            Float64 segment_age_at_release      = pTimelineMgr->GetEventByIndex(segConstructEventIdx)->GetConstructSegmentsActivity().GetAgeAtRelease();
            Float64 segment_cure_time = segment_age_at_release;


            // Time dependent concrete
            // for the LRFD stepped f'c concrete model, assume the jump from f'ci to f'c occurs
            // at hauling interval
            IntervalIndexType intervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
            Float64 stepTime = pIntervals->GetTime(segmentKey,intervalIdx,pgsTypes::Start);

            matConcreteBase* pSegmentConcrete = CreateConcreteModel(_T("Segment Concrete"),pSegment->Material.Concrete,segment_casting_time,segment_cure_time,segment_age_at_release,stepTime);
            m_pSegmentConcrete.insert( std::make_pair(segmentKey,boost::shared_ptr<matConcreteBase>(pSegmentConcrete)) );

            if ( segIdx < nSegments-1 )
            {
               const CClosureJointData* pClosure  = pGirder->GetClosureJoint(segIdx);
               EventIndexType castClosureEventIdx = pTimelineMgr->GetCastClosureJointEventIndex(pClosure->GetID());
               Float64 closure_casting_time       = pTimelineMgr->GetStart(castClosureEventIdx);
               Float64 closure_age_at_continuity  = pTimelineMgr->GetEventByIndex(castClosureEventIdx)->GetCastClosureJointActivity().GetConcreteAgeAtContinuity();
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

               matConcreteBase* pClosureConcrete = CreateConcreteModel(_T("Closure Concrete"),pClosure->GetConcrete(),closure_casting_time,closure_cure_time,closure_age_at_continuity,closure_step_time);
               m_pClosureConcrete.insert( std::make_pair(segmentKey,boost::shared_ptr<matConcreteBase>(pClosureConcrete)) );
            }
         }
      }
   }

   //////////////////////////////////////////////////////////////////////////////
   //
   // Validate values   
   //
   //////////////////////////////////////////////////////////////////////////////

   GET_IFACE_NOCHECK(IEAFStatusCenter,pStatusCenter);

   // per 5.4.2.1 f'c must exceed 28 MPa (4 ksi)
   bool bSI = lrfdVersionMgr::GetUnits() == lrfdVersionMgr::SI ? true : false;
   Float64 fcMin = bSI ? ::ConvertToSysUnits(28, unitMeasure::MPa) : ::ConvertToSysUnits(4, unitMeasure::KSI);

   // check railing system
   if ( !IsConcreteDensityInRange(m_pRailingConc[pgsTypes::tboLeft]->GetStrengthDensity(),(pgsTypes::ConcreteType)m_pRailingConc[pgsTypes::tboLeft]->GetType()) )
   {
      std::_tostringstream os;
      if ( m_pRailingConc[pgsTypes::tboLeft]->GetType() == pgsTypes::Normal )
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

   if ( !IsConcreteDensityInRange(m_pRailingConc[pgsTypes::tboRight]->GetStrengthDensity(),(pgsTypes::ConcreteType)m_pRailingConc[pgsTypes::tboRight]->GetType()) )
   {
      std::_tostringstream os;
      if ( m_pRailingConc[pgsTypes::tboRight]->GetType() == pgsTypes::Normal )
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
   if ( pDeck->DeckType != pgsTypes::sdtNone )
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      GET_IFACE(ILimits,pLimits);

      Float64 time = m_pDeckConc->GetTimeAtCasting() + m_pDeckConc->GetCureTime() + 28.0;
      Float64 fc28 = m_pDeckConc->GetFc(time);
      if ( fc28 < fcMin && !IsEqual(fc28,fcMin) )
      {
         std::_tstring strMsg;
         strMsg = bSI ? _T("Deck concrete cannot be less than 28 MPa per LRFD 5.4.2.1") 
                      : _T("Deck concrete cannot be less than 4 KSI per LRFD 5.4.2.1");
         CSegmentKey dummyKey;
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::FinalStrength,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
         //strMsg += std::_tstring(_T("\nSee Status Center for Details"));
         //THROW_UNWIND(strMsg.c_str(),-1);
      }

      pgsTypes::ConcreteType slabConcreteType = (pgsTypes::ConcreteType)m_pDeckConc->GetType();
      Float64 max_slab_fc = pLimits->GetMaxSlabFc(slabConcreteType);
      if (  max_slab_fc < fc28 && !IsEqual(max_slab_fc,fc28) )
      {
         Float64 fcMax = ::ConvertFromSysUnits(max_slab_fc,pDisplayUnits->GetStressUnit().UnitOfMeasure);
         std::_tostringstream os;
         os << _T("Slab concrete strength exceeds the normal value of ") << fcMax << _T(" ") << pDisplayUnits->GetStressUnit().UnitOfMeasure.UnitTag();

         std::_tstring strMsg = os.str();

         CSegmentKey dummyKey;
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::FinalStrength,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 max_wc = pLimits->GetMaxConcreteUnitWeight(slabConcreteType);
      Float64 MaxWc = ::ConvertFromSysUnits(max_wc,pDisplayUnits->GetDensityUnit().UnitOfMeasure);

      Float64 strength_density = m_pDeckConc->GetStrengthDensity();
      if ( max_wc < strength_density && !IsEqual(max_wc,strength_density,0.0001) )
      {
         std::_tostringstream os;
         os << _T("Slab concrete density for strength calculations exceeds the normal value of ") << MaxWc << _T(" ") << pDisplayUnits->GetDensityUnit().UnitOfMeasure.UnitTag();

         std::_tstring strMsg = os.str();

         CSegmentKey dummyKey;
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::Density,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 weight_density = m_pDeckConc->GetWeightDensity();
      if ( max_wc < weight_density && !IsEqual(max_wc,weight_density,0.0001) )
      {
         std::_tostringstream os;
         os << _T("Slab concrete density for weight calculations exceeds the normal value of ") << MaxWc << _T(" ") << pDisplayUnits->GetDensityUnit().UnitOfMeasure.UnitTag();

         std::_tstring strMsg = os.str();

         CSegmentKey dummyKey;
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::DensityForWeight,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      if ( !IsConcreteDensityInRange(strength_density, (pgsTypes::ConcreteType)m_pDeckConc->GetType()) )
      {
         std::_tostringstream os;
         if ( m_pDeckConc->GetType() == pgsTypes::Normal )
         {
            os << _T("Slab concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
         }
         else
         {
            os << _T("Slab concrete density is out of range for Lightweight Concrete per LRFD 5.2.");
         }

         std::_tstring strMsg = os.str();

         CSegmentKey dummyKey;
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::Density,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
         pStatusCenter->Add(pStatusItem);
      }

      Float64 max_agg_size = pLimits->GetMaxConcreteAggSize(slabConcreteType);
      Float64 MaxAggSize = ::ConvertFromSysUnits(max_agg_size,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      Float64 agg_size = m_pDeckConc->GetMaxAggregateSize();
      if ( max_agg_size < agg_size && !IsEqual(max_agg_size,agg_size))
      {
         std::_tostringstream os;
         os << _T("Slab concrete aggregate size exceeds the normal value of ") << MaxAggSize << _T(" ") << pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag();

         std::_tstring strMsg = os.str();

         CSegmentKey dummyKey;
         pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::Slab,pgsConcreteStrengthStatusItem::AggSize,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
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

            std::_tostringstream osLabel;
            osLabel << _T("Girder ") << LABEL_GIRDER(gdrIdx) << _T(" Segment ") << LABEL_SEGMENT(segIdx);
            ValidateConcreteParameters(m_pSegmentConcrete[segmentKey],pgsConcreteStrengthStatusItem::GirderSegment,osLabel.str().c_str(),segmentKey);

            if ( segIdx < nSegments-1 )
            {
               std::_tostringstream osLabel2;
               osLabel2 << _T("Girder ") << LABEL_GIRDER(gdrIdx) << _T(" Closure ") << LABEL_SEGMENT(segIdx);
               ValidateConcreteParameters(m_pClosureConcrete[segmentKey],pgsConcreteStrengthStatusItem::ClosureJoint,osLabel2.str().c_str(),segmentKey);
            }
         }
      }
   }

   m_bIsValidated = true;
}

void CConcreteManager::ValidateConcrete2()
{
   if ( m_bIsValidated2 )
   {
      return;
   }

   GET_IFACE(ISectionProperties,pSectProp);


   //////////////////////////////////////////////////////////////////////////////
   //
   // Create Deck concrete
   //
   //////////////////////////////////////////////////////////////////////////////
   if ( m_pDeckConc.get() != NULL )
   {
      Float64 S = pSectProp->GetDeckSurfaceArea();
      Float64 V = pSectProp->GetDeckVolume();
      Float64 vsDeck = IsZero(S) ? DBL_MAX : V/S;

      m_pDeckConc->SetVSRatio(vsDeck);
   }

   //////////////////////////////////////////////////////////////////////////////
   //
   // Create railing concrete
   //
   //////////////////////////////////////////////////////////////////////////////
   // Railing system creep/shrinkage not considered to V/S of zero is fine
   m_pRailingConc[pgsTypes::tboLeft]->SetVSRatio(0.0);
   m_pRailingConc[pgsTypes::tboRight]->SetVSRatio(0.0);
   
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
            Float64 S = pSectProp->GetSegmentSurfaceArea(segmentKey);
            Float64 V = pSectProp->GetSegmentVolume(segmentKey);

            Float64 vsSegment = IsZero(S) ? DBL_MAX : V/S;

            m_pSegmentConcrete[segmentKey]->SetVSRatio(vsSegment);

            if ( segIdx < nSegments-1 )
            {
               CClosureKey closureKey(segmentKey);
               Float64 S = pSectProp->GetClosureJointSurfaceArea(closureKey);
               Float64 V = pSectProp->GetClosureJointVolume(closureKey);

               Float64 vsClosure = IsZero(S) ? DBL_MAX : V/S;

               m_pClosureConcrete[closureKey]->SetVSRatio(vsClosure);
            }
         } // next segment
      } // next girder
   } // next group

   m_bIsValidated2 = true;
}


void CConcreteManager::ValidateConcreteParameters(boost::shared_ptr<matConcreteBase> pConcrete,pgsConcreteStrengthStatusItem::ConcreteType elementType,LPCTSTR strLabel,const CSegmentKey& segmentKey)
{
   ATLASSERT(elementType == pgsConcreteStrengthStatusItem::GirderSegment || elementType == pgsConcreteStrengthStatusItem::ClosureJoint);
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(ILimits,pLimits);

   GET_IFACE_NOCHECK(IEAFStatusCenter,pStatusCenter);

   pgsTypes::ConcreteType concreteType = (pgsTypes::ConcreteType)pConcrete->GetType();

   // per 5.4.2.1 f'c must exceed 28 MPa (4 ksi)
   bool bSI = lrfdVersionMgr::GetUnits() == lrfdVersionMgr::SI ? true : false;
   Float64 fcMin = bSI ? ::ConvertToSysUnits(28, unitMeasure::MPa) : ::ConvertToSysUnits(4, unitMeasure::KSI);
   // the LRFD doesn't say that this specifically applies to closure joints,
   // but we are going to assume that it does.

   Float64 max_wc = pLimits->GetMaxConcreteUnitWeight(concreteType);
   Float64 MaxWc  = ::ConvertFromSysUnits(max_wc,pDisplayUnits->GetDensityUnit().UnitOfMeasure);

   Float64 max_fci, max_fc;
   if ( elementType == pgsConcreteStrengthStatusItem::GirderSegment )
   {
      max_fci = pLimits->GetMaxSegmentFci(concreteType);
      max_fc  = pLimits->GetMaxSegmentFc(concreteType);
   }
   else
   {
      max_fci = pLimits->GetMaxClosureFci(concreteType);
      max_fc  = pLimits->GetMaxClosureFc(concreteType);
   }
   Float64 fciMax = ::ConvertFromSysUnits(max_fci,pDisplayUnits->GetStressUnit().UnitOfMeasure);
   Float64 fcMax  = ::ConvertFromSysUnits(max_fc, pDisplayUnits->GetStressUnit().UnitOfMeasure);

   Float64 max_agg_size = pLimits->GetMaxConcreteAggSize(concreteType);
   Float64 MaxAggSize   = ::ConvertFromSysUnits(max_agg_size,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

   std::_tstring strMsg;

   Float64 time = pConcrete->GetTimeAtCasting() + pConcrete->GetCureTime() + 28.0;
   Float64 fc28 = pConcrete->GetFc(time);
   Float64 fci  = pConcrete->GetFc(pConcrete->GetCureTime());
   if (fc28 < fcMin )
   {
      std::_tostringstream os;
      os << strLabel << _T(": Concrete strength is less that permitted by LRFD 5.4.2.1");
      strMsg = os.str();
      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType,pgsConcreteStrengthStatusItem::FinalStrength,segmentKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   if (  max_fci < fci )
   {
      std::_tostringstream os;
      os << strLabel << _T(": Initial concrete strength exceeds the normal value of ") << fciMax << _T(" ") << pDisplayUnits->GetStressUnit().UnitOfMeasure.UnitTag();

      strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType,pgsConcreteStrengthStatusItem::ReleaseStrength,segmentKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   if (  max_fc < fc28 )
   {
      std::_tostringstream os;
      os << strLabel << _T(": Concrete strength exceeds the normal value of ") << fcMax << _T(" ") << pDisplayUnits->GetStressUnit().UnitOfMeasure.UnitTag();

      strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType,pgsConcreteStrengthStatusItem::FinalStrength,segmentKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   Float64 wc = pConcrete->GetStrengthDensity();
   if ( max_wc < wc && !IsEqual(max_wc,wc,0.0001))
   {
      std::_tostringstream os;
      os << strLabel << _T(": Concrete density for strength calcuations exceeds the normal value of ") << MaxWc << _T(" ") << pDisplayUnits->GetDensityUnit().UnitOfMeasure.UnitTag();

      strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType,pgsConcreteStrengthStatusItem::Density,segmentKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   wc = pConcrete->GetWeightDensity();
   if ( max_wc < wc && !IsEqual(max_wc,wc,0.0001) )
   {
      std::_tostringstream os;
      os << strLabel << _T(": Concrete density for weight calcuations exceeds the normal value of ") << MaxWc << _T(" ") << pDisplayUnits->GetDensityUnit().UnitOfMeasure.UnitTag();

      strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType,pgsConcreteStrengthStatusItem::DensityForWeight,segmentKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   if ( !IsConcreteDensityInRange(pConcrete->GetStrengthDensity(), concreteType) )
   {
      std::_tostringstream os;
      if ( concreteType == pgsTypes::Normal )
      {
         os << strLabel << _T(": concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
      }
      else
      {
         os << strLabel << _T(": concrete density is out of range for Lightweight Concrete per LRFD 5.2.");
      }

      strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType,pgsConcreteStrengthStatusItem::DensityForWeight,segmentKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   if ( max_agg_size < pConcrete->GetMaxAggregateSize() && !IsEqual(max_agg_size,pConcrete->GetMaxAggregateSize()) )
   {
      std::_tostringstream os;
      os << strLabel << _T(": Concrete aggregate size exceeds the normal value of ") << MaxAggSize << _T(" ") << pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag();

      strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType,pgsConcreteStrengthStatusItem::AggSize,segmentKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   // There are certain combinations of input, when Ec or Eci is user input and the other value
   // is computed where Ec could be less than Eci. Trap this error.
   Float64 Ec28 = pConcrete->GetEc(time);
   Float64 Eci  = pConcrete->GetEc(pConcrete->GetCureTime());
   if ( Ec28 < Eci )
   {
      std::_tostringstream os;
      os << strLabel << _T(": Ec (") 
         << FormatDimension(Ec28, pDisplayUnits->GetModEUnit()).GetBuffer() << _T(") is less than Eci (")
         << FormatDimension(Eci,  pDisplayUnits->GetModEUnit()).GetBuffer() << _T(")");

      strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType,pgsConcreteStrengthStatusItem::Modulus,segmentKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }
}

bool CConcreteManager::IsConcreteDensityInRange(Float64 density,pgsTypes::ConcreteType type)
{
   if ( type == pgsTypes::Normal )
   {
      return ( GetNWCDensityLimit() <= density );
   }
   else
   {
      return (density <= GetLWCDensityLimit());
   }
}

matConcreteBase* CConcreteManager::CreateConcreteModel(LPCTSTR strName,const CConcreteMaterial& concrete,Float64 timeAtCasting,Float64 cureTime,Float64 ageAtInitialLoading,Float64 stepTime)
{
   GET_IFACE(ILossParameters,pLossParameters);
   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();

   GET_IFACE(IEnvironment,pEnvironment);
   Float64 rh = pEnvironment->GetRelHumidity();

   matConcreteBase* pConcrete;
   if ( loss_method == pgsTypes::TIME_STEP )
   {
      // for time step loss method, create concrete model based on time-dependent model type
      switch( pLossParameters->GetTimeDependentModel() )
      {
      case pgsTypes::tdmAASHTO:
         pConcrete = CreateTimeDependentLRFDConcreteModel(concrete,ageAtInitialLoading);
         break;
      case pgsTypes::tdmACI209:
         pConcrete = CreateACI209Model(concrete,ageAtInitialLoading);
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
   pConcrete->SetType((matConcreteBase::Type)concrete.Type);
   pConcrete->SetStrengthDensity(concrete.StrengthDensity);
   pConcrete->SetWeightDensity(concrete.WeightDensity);
   pConcrete->HasAggSplittingStrength(concrete.bHasFct);
   pConcrete->SetAggSplittingStrength(concrete.Fct);
   pConcrete->SetMaxAggregateSize(concrete.MaxAggregateSize);
   pConcrete->SetRelativeHumidity(rh);
   pConcrete->SetTimeAtCasting(timeAtCasting);
   pConcrete->SetCureTime(cureTime);
   pConcrete->SetCureMethod((matConcreteBase::CureMethod)concrete.CureMethod);
   //pConcrete->SetVSRatio(vs); NOTE: volume to surface ratio is set during the level 2 validation 
   // of the concrete model. To get V/S we need to get section properties and section properties
   // need valid concrete models. This creates a circular dependency. However, the only part
   // of the concrete model that needs V/S is creep and shrinkage. Let V/S be invalid
   // on the concrete model until a creep of shrinkage parameter is requested... then, make
   // V/S correct


   return pConcrete;
}

void CConcreteManager::CreateConcrete(const CConcreteMaterial& concrete,LPCTSTR strName,matConcreteEx* pReleaseConc,matConcreteEx* pConcrete)
{
   Float64 modE;
   if ( concrete.bUserEci )
   {
      modE = concrete.Eci;
   }
   else
   {
      modE = lrfdConcreteUtil::ModE( concrete.Fci, 
                                     concrete.StrengthDensity, 
                                     false /* ignore LRFD range checks */ );

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         modE *= (concrete.EcK1 * concrete.EcK2);
      }
   }

   // get the modulus of rupture.
   Float64 frShear   = lrfdConcreteUtil::ModRupture(concrete.Fci,GetShearFrCoefficient(concrete.Type));
   Float64 frFlexure = lrfdConcreteUtil::ModRupture(concrete.Fci,GetFlexureFrCoefficient(concrete.Type));

   pReleaseConc->SetName(strName);
   pReleaseConc->SetFc(concrete.Fci);
   pReleaseConc->SetDensity(concrete.StrengthDensity);
   pReleaseConc->SetDensityForWeight(concrete.WeightDensity);
   pReleaseConc->SetE(modE);
   pReleaseConc->SetMaxAggregateSize(concrete.MaxAggregateSize);
   pReleaseConc->SetType((matConcrete::Type)concrete.Type);
   pReleaseConc->HasAggSplittingStrength(concrete.bHasFct);
   pReleaseConc->SetAggSplittingStrength(concrete.Fct);
   pReleaseConc->SetShearFr(frShear);
   pReleaseConc->SetFlexureFr(frFlexure);

   if ( concrete.bUserEc )
   {
      modE = concrete.Ec;
   }
   else
   {
      modE = lrfdConcreteUtil::ModE( concrete.Fc, 
                                     concrete.StrengthDensity, 
                                     false /* ignore LRFD range checks */ );

      if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      {
         modE *= (concrete.EcK1 * concrete.EcK2);
      }
   }

   // get the modulus of rupture.
   frShear   = lrfdConcreteUtil::ModRupture(concrete.Fc,GetShearFrCoefficient(concrete.Type));
   frFlexure = lrfdConcreteUtil::ModRupture(concrete.Fc,GetFlexureFrCoefficient(concrete.Type));

   pConcrete->SetName(strName);
   pConcrete->SetFc(concrete.Fc);
   pConcrete->SetDensity(concrete.StrengthDensity);
   pConcrete->SetDensityForWeight(concrete.WeightDensity);
   pConcrete->SetE(modE);
   pConcrete->SetMaxAggregateSize(concrete.MaxAggregateSize);
   pConcrete->SetType((matConcrete::Type)concrete.Type);
   pConcrete->HasAggSplittingStrength(concrete.bHasFct);
   pConcrete->SetAggSplittingStrength(concrete.Fct);
   pConcrete->SetShearFr(frShear);
   pConcrete->SetFlexureFr(frFlexure);
}

pgsTypes::ConcreteType CConcreteManager::GetSegmentConcreteType(const CSegmentKey& segmentKey)
{
   ValidateConcrete();
   return (pgsTypes::ConcreteType)m_pSegmentConcrete[segmentKey]->GetType();
}

bool CConcreteManager::DoesSegmentConcreteHaveAggSplittingStrength(const CSegmentKey& segmentKey)
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->HasAggSplittingStrength();
}

Float64 CConcreteManager::GetSegmentConcreteAggSplittingStrength(const CSegmentKey& segmentKey)
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetAggSplittingStrength();
}

Float64 CConcreteManager::GetSegmentMaxAggrSize(const CSegmentKey& segmentKey)
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetMaxAggregateSize();
}

Float64 CConcreteManager::GetSegmentStrengthDensity(const CSegmentKey& segmentKey)
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetStrengthDensity();
}

Float64 CConcreteManager::GetSegmentWeightDensity(const CSegmentKey& segmentKey)
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetWeightDensity();
}

Float64 CConcreteManager::GetSegmentEccK1(const CSegmentKey& segmentKey)
{
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      K1 = pMaterial->Concrete.EcK1;
   }

   return K1;
}

Float64 CConcreteManager::GetSegmentEccK2(const CSegmentKey& segmentKey)
{
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      K2 = pMaterial->Concrete.EcK2;
   }

   return K2;
}

Float64 CConcreteManager::GetSegmentCreepK1(const CSegmentKey& segmentKey)
{
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      K1 = pMaterial->Concrete.CreepK1;
   }

   return K1;
}

Float64 CConcreteManager::GetSegmentCreepK2(const CSegmentKey& segmentKey)
{
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      K2 = pMaterial->Concrete.CreepK2;
   }

   return K2;
}

Float64 CConcreteManager::GetSegmentShrinkageK1(const CSegmentKey& segmentKey)
{
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      K1 = pMaterial->Concrete.ShrinkageK1;
   }

   return K1;
}

Float64 CConcreteManager::GetSegmentShrinkageK2(const CSegmentKey& segmentKey)
{
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pMaterial = pSegmentData->GetSegmentMaterial(segmentKey);
      K2 = pMaterial->Concrete.ShrinkageK1;
   }

   return K2;
}

pgsTypes::ConcreteType CConcreteManager::GetClosureJointConcreteType(const CSegmentKey& closureKey)
{
   ValidateConcrete();
   return (pgsTypes::ConcreteType)m_pClosureConcrete[closureKey]->GetType();
}

bool CConcreteManager::DoesClosureJointConcreteHaveAggSplittingStrength(const CSegmentKey& closureKey)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->HasAggSplittingStrength();
}

Float64 CConcreteManager::GetClosureJointConcreteAggSplittingStrength(const CSegmentKey& closureKey)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetAggSplittingStrength();
}

Float64 CConcreteManager::GetClosureJointMaxAggrSize(const CSegmentKey& closureKey)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetMaxAggregateSize();
}

Float64 CConcreteManager::GetClosureJointStrengthDensity(const CSegmentKey& closureKey)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetStrengthDensity();
}

Float64 CConcreteManager::GetClosureJointWeightDensity(const CSegmentKey& closureKey)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetWeightDensity();
}

Float64 CConcreteManager::GetClosureJointEccK1(const CSegmentKey& closureKey)
{
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CClosureJointData* pClosure = m_pBridgeDesc->GetClosureJoint(closureKey);
      K1 = pClosure->GetConcrete().EcK1;
   }

   return K1;
}

Float64 CConcreteManager::GetClosureJointEccK2(const CSegmentKey& closureKey)
{
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CClosureJointData* pClosure = m_pBridgeDesc->GetClosureJoint(closureKey);
      K2 = pClosure->GetConcrete().EcK2;
   }

   return K2;
}

Float64 CConcreteManager::GetClosureJointCreepK1(const CSegmentKey& closureKey)
{
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CClosureJointData* pClosure = m_pBridgeDesc->GetClosureJoint(closureKey);
      K1 = pClosure->GetConcrete().CreepK1;
   }

   return K1;
}

Float64 CConcreteManager::GetClosureJointCreepK2(const CSegmentKey& closureKey)
{
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CClosureJointData* pClosure = m_pBridgeDesc->GetClosureJoint(closureKey);
      K2 = pClosure->GetConcrete().CreepK2;
   }

   return K2;
}

Float64 CConcreteManager::GetClosureJointShrinkageK1(const CSegmentKey& closureKey)
{
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CClosureJointData* pClosure = m_pBridgeDesc->GetClosureJoint(closureKey);
      K1 = pClosure->GetConcrete().ShrinkageK1;
   }

   return K1;
}

Float64 CConcreteManager::GetClosureJointShrinkageK2(const CSegmentKey& closureKey)
{
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CClosureJointData* pClosure = m_pBridgeDesc->GetClosureJoint(closureKey);
      K2 = pClosure->GetConcrete().ShrinkageK2;
   }

   return K2;
}

pgsTypes::ConcreteType CConcreteManager::GetDeckConcreteType()
{
   ValidateConcrete();
   if ( m_pDeckConc.get() != NULL )
   {
      return (pgsTypes::ConcreteType)m_pDeckConc->GetType();
   }
   else
   {
      return pgsTypes::Normal;
   }
}

bool CConcreteManager::DoesDeckConcreteHaveAggSplittingStrength()
{
   ValidateConcrete();
   if ( m_pDeckConc.get() != NULL )
   {
      return m_pDeckConc->HasAggSplittingStrength();
   }
   else
   {
      return false;
   }
}

Float64 CConcreteManager::GetDeckConcreteAggSplittingStrength()
{
   ValidateConcrete();
   if ( m_pDeckConc.get() != NULL )
   {
      return m_pDeckConc->GetAggSplittingStrength();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckStrengthDensity()
{
   ValidateConcrete();
   if ( m_pDeckConc.get() != NULL )
   {
      return m_pDeckConc->GetStrengthDensity();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckWeightDensity()
{
   ValidateConcrete();
   if ( m_pDeckConc.get() != NULL )
   {
      return m_pDeckConc->GetWeightDensity();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckMaxAggrSize()
{
   ValidateConcrete();
   if ( m_pDeckConc.get() != NULL )
   {
      return m_pDeckConc->GetMaxAggregateSize();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckEccK1()
{
   ValidateConcrete();
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      K1 = m_DeckEcK1;
   }
   return K1;
}

Float64 CConcreteManager::GetDeckEccK2()
{
   ValidateConcrete();
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      K2 = m_DeckEcK2;
   }
   return K2;
}

Float64 CConcreteManager::GetDeckCreepK1()
{
   ValidateConcrete();
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      K1 = m_DeckCreepK1;
   }
   return K1;
}

Float64 CConcreteManager::GetDeckCreepK2()
{
   ValidateConcrete();
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      K2 = m_DeckCreepK2;
   }
   return K2;
};

Float64 CConcreteManager::GetDeckShrinkageK1()
{
   ValidateConcrete();
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      K1 = m_DeckShrinkageK1;
   }
   return K1;
}

Float64 CConcreteManager::GetDeckShrinkageK2()
{
   ValidateConcrete();
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      K2 = m_DeckShrinkageK2;
   }
   return K2;
};

Float64 CConcreteManager::GetRailingSystemDensity(pgsTypes::TrafficBarrierOrientation orientation)
{
   ValidateConcrete();
   return m_pRailingConc[orientation]->GetWeightDensity();
}

Float64 CConcreteManager::GetRailingSystemCastingTime(pgsTypes::TrafficBarrierOrientation orientation)
{
   ValidateConcrete();
   return m_pRailingConc[orientation]->GetTimeAtCasting();
}

Float64 CConcreteManager::GetRailingSystemFc(pgsTypes::TrafficBarrierOrientation orientation,Float64 t)
{
   ValidateConcrete();
   return m_pRailingConc[orientation]->GetFc(t);
}

Float64 CConcreteManager::GetRailingSystemEc(pgsTypes::TrafficBarrierOrientation orientation,Float64 t)
{
   ValidateConcrete();
   return m_pRailingConc[orientation]->GetEc(t);
}

Float64 CConcreteManager::GetRailingSystemFreeShrinkageStrain(pgsTypes::TrafficBarrierOrientation orientation,Float64 t)
{
   ValidateConcrete();
   ValidateConcrete2();
   return m_pRailingConc[orientation]->GetFreeShrinkageStrain(t);
}

Float64 CConcreteManager::GetRailingSystemCreepCoefficient(pgsTypes::TrafficBarrierOrientation orientation,Float64 t,Float64 tla)
{
   ValidateConcrete();
   ValidateConcrete2();
   return m_pRailingConc[orientation]->GetCreepCoefficient(t,tla);
}

matConcreteBase* CConcreteManager::GetRailingSystemConcrete(pgsTypes::TrafficBarrierDistribution orientation)
{
   ValidateConcrete();
   return m_pRailingConc[orientation].get();
}

Float64 CConcreteManager::GetNWCDensityLimit()
{
   Float64 limit;
   if ( lrfdVersionMgr::GetUnits() == lrfdVersionMgr::US )
   {
      limit = ::ConvertToSysUnits(135.0,unitMeasure::LbfPerFeet3);
   }
   else
   {
      limit = ::ConvertToSysUnits(2150.0,unitMeasure::KgPerMeter3);
   }

   return limit;
}

Float64 CConcreteManager::GetLWCDensityLimit()
{
   Float64 limit;
   if ( lrfdVersionMgr::GetUnits() == lrfdVersionMgr::US )
   {
      limit = ::ConvertToSysUnits(120.0,unitMeasure::LbfPerFeet3);
   }
   else
   {
      limit = ::ConvertToSysUnits(1925.0,unitMeasure::KgPerMeter3);
   }

   return limit;
}

Float64 CConcreteManager::GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type)
{
   return lrfdConcreteUtil::ModRupture( fc, GetFlexureFrCoefficient(type) );
}

Float64 CConcreteManager::GetFlexureFrCoefficient(pgsTypes::ConcreteType type)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   return pSpecEntry->GetFlexureModulusOfRuptureCoefficient(type);
}

Float64 CConcreteManager::GetFlexureFrCoefficient(const CSegmentKey& segmentKey)
{
   pgsTypes::ConcreteType type = GetSegmentConcreteType(segmentKey);
   return GetFlexureFrCoefficient(type);
}

Float64 CConcreteManager::GetShearModRupture(Float64 fc,pgsTypes::ConcreteType type)
{
   return lrfdConcreteUtil::ModRupture( fc, GetShearFrCoefficient(type) );
}

Float64 CConcreteManager::GetEconc(Float64 fc,Float64 density,Float64 K1,Float64 K2)
{
   return K1*K2*lrfdConcreteUtil::ModE(fc,density, false ); // ignore LRFD limits
}

Float64 CConcreteManager::GetShearFrCoefficient(pgsTypes::ConcreteType type)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   return pSpecEntry->GetShearModulusOfRuptureCoefficient(type);
}

Float64 CConcreteManager::GetShearFrCoefficient(const CSegmentKey& segmentKey)
{
   pgsTypes::ConcreteType type = GetSegmentConcreteType(segmentKey);
   return GetShearFrCoefficient(type);
}

Float64 CConcreteManager::GetDeckCastingTime()
{
   ValidateConcrete();
   if ( m_pDeckConc.get() != NULL )
   {
      return m_pDeckConc->GetTimeAtCasting();
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckFc(Float64 t)
{
   ValidateConcrete();
   if ( m_pDeckConc.get() != NULL )
   {
      return m_pDeckConc->GetFc(t);
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckEc(Float64 t)
{
   ValidateConcrete();
   if ( m_pDeckConc.get() != NULL )
   {
      return m_pDeckConc->GetEc(t);
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckFlexureFr(Float64 t)
{
   ValidateConcrete();
   if ( m_pDeckConc.get() != NULL )
   {
      return m_pDeckConc->GetFlexureFr(t);
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckShearFr(Float64 t)
{
   ValidateConcrete();
   if ( m_pDeckConc.get() != NULL )
   {
      return m_pDeckConc->GetShearFr(t);
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckFreeShrinkageStrain(Float64 t)
{
   ValidateConcrete();
   ValidateConcrete2();
   if ( m_pDeckConc.get() != NULL )
   {
      return m_pDeckConc->GetFreeShrinkageStrain(t);
   }
   else
   {
      return 0;
   }
}

Float64 CConcreteManager::GetDeckCreepCoefficient(Float64 t,Float64 tla)
{
   ValidateConcrete();
   ValidateConcrete2();
   if ( m_pDeckConc.get() != NULL )
   {
      return m_pDeckConc->GetCreepCoefficient(t,tla);
   }
   else
   {
      return 0;
   }
}

matConcreteBase* CConcreteManager::GetDeckConcrete()
{
   ValidateConcrete();
   return m_pDeckConc.get();
}

Float64 CConcreteManager::GetSegmentCastingTime(const CSegmentKey& segmentKey)
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetTimeAtCasting();
}

Float64 CConcreteManager::GetSegmentFc(const CSegmentKey& segmentKey,Float64 t)
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetFc(t);
}

Float64 CConcreteManager::GetSegmentEc(const CSegmentKey& segmentKey,Float64 t)
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetEc(t);
}

Float64 CConcreteManager::GetSegmentFlexureFr(const CSegmentKey& segmentKey,Float64 t)
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetFlexureFr(t);
}

Float64 CConcreteManager::GetSegmentShearFr(const CSegmentKey& segmentKey,Float64 t)
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetShearFr(t);
}

Float64 CConcreteManager::GetSegmentFreeShrinkageStrain(const CSegmentKey& segmentKey,Float64 t)
{
   ValidateConcrete();
   ValidateConcrete2();
   return m_pSegmentConcrete[segmentKey]->GetFreeShrinkageStrain(t);
}

Float64 CConcreteManager::GetSegmentCreepCoefficient(const CSegmentKey& segmentKey,Float64 t,Float64 tla)
{
   ValidateConcrete();
   ValidateConcrete2();
   return m_pSegmentConcrete[segmentKey]->GetCreepCoefficient(t,tla);
}

matConcreteBase* CConcreteManager::GetSegmentConcrete(const CSegmentKey& segmentKey)
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey].get();
}

Float64 CConcreteManager::GetClosureJointCastingTime(const CClosureKey& closureKey)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetTimeAtCasting();
}

Float64 CConcreteManager::GetClosureJointFc(const CClosureKey& closureKey,Float64 t)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetFc(t);
}

Float64 CConcreteManager::GetClosureJointFlexureFr(const CClosureKey& closureKey,Float64 t)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetFlexureFr(t);
}

Float64 CConcreteManager::GetClosureJointShearFr(const CClosureKey& closureKey,Float64 t)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetShearFr(t);
}

Float64 CConcreteManager::GetClosureJointEc(const CClosureKey& closureKey,Float64 t)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetEc(t);
}

Float64 CConcreteManager::GetClosureJointFreeShrinkageStrain(const CClosureKey& closureKey,Float64 t)
{
   ValidateConcrete();
   ValidateConcrete2();
   return m_pClosureConcrete[closureKey]->GetFreeShrinkageStrain(t);
}

Float64 CConcreteManager::GetClosureJointCreepCoefficient(const CClosureKey& closureKey,Float64 t,Float64 tla)
{
   ValidateConcrete();
   ValidateConcrete2();
   return m_pClosureConcrete[closureKey]->GetCreepCoefficient(t,tla);
}

matConcreteBase* CConcreteManager::GetClosureJointConcrete(const CClosureKey& closureKey)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey].get();
}

lrfdLRFDConcrete* CConcreteManager::CreateLRFDConcreteModel(const CConcreteMaterial& concrete,Float64 startTime,Float64 stepTime)
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

   lrfdLRFDConcrete* pLRFDConcrete = new lrfdLRFDConcrete();

   matConcreteEx initialConcrete, finalConcrete;
   CreateConcrete(concrete,_T(""),&initialConcrete,&finalConcrete);
   pLRFDConcrete->SetConcreteModels(initialConcrete,finalConcrete);
   pLRFDConcrete->SetStartTime(startTime);
   pLRFDConcrete->SetStepTime(stepTime);

   pLRFDConcrete->SetEcCorrectionFactors(concrete.EcK1,concrete.EcK2);
   pLRFDConcrete->SetCreepCorrectionFactors(concrete.CreepK1,concrete.CreepK2);
   pLRFDConcrete->SetShrinkageCorrectionFactors(concrete.ShrinkageK1,concrete.ShrinkageK2);

   return pLRFDConcrete;
}

lrfdLRFDTimeDependentConcrete* CConcreteManager::CreateTimeDependentLRFDConcreteModel(const CConcreteMaterial& concrete,Float64 ageAtInitialLoading)
{
   lrfdLRFDTimeDependentConcrete* pConcrete = new lrfdLRFDTimeDependentConcrete();

   Float64 A,B;
   if ( concrete.bACIUserParameters )
   {
      A = concrete.A;
      B = concrete.B;
   }
   else
   {
      lrfdLRFDTimeDependentConcrete::GetModelParameters((matConcreteBase::CureMethod)concrete.CureMethod,
                                                       (matConcreteBase::CementType)concrete.CementType,
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

   return pConcrete;
}

matACI209Concrete* CConcreteManager::CreateACI209Model(const CConcreteMaterial& concrete,Float64 ageAtInitialLoading)
{
   matACI209Concrete* pConcrete = new matACI209Concrete();

   Float64 A,B;
   if ( concrete.bACIUserParameters )
   {
      A = concrete.A;
      B = concrete.B;
   }
   else
   {
      matACI209Concrete::GetModelParameters((matConcreteBase::CureMethod)concrete.CureMethod,
                                            (matConcreteBase::CementType)concrete.CementType,
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

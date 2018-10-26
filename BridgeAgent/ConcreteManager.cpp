///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <PgsExt\ClosurePourData.h>
#include <PgsExt\GirderLabel.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CConcreteManager::CConcreteManager()
{
   m_bIsValidated = false;
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
}

void CConcreteManager::ValidateConcrete()
{
   if ( m_bIsValidated )
      return;

   GET_IFACE(IIntervals,pIntervals);

#pragma Reminder("UPDATE: using dummy POI to compute V/S for time-dependent properties")
   GET_IFACE(ISectionProperties,pSectProp);
   pgsPointOfInterest poi(CSegmentKey(0,0,0),0.0); // dummy POI

   //////////////////////////////////////////////////////////////////////////////
   //
   // Create Deck concrete
   //
   //////////////////////////////////////////////////////////////////////////////
   const CDeckDescription2* pDeck = m_pBridgeDesc->GetDeckDescription();
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
   EventIndexType castDeckEventIdx  = m_pBridgeDesc->GetTimelineManager()->GetCastDeckEventIndex();
   Float64   time_at_casting        = m_pBridgeDesc->GetTimelineManager()->GetStart(castDeckEventIdx);
   Float64   age_at_initial_loading = m_pBridgeDesc->GetTimelineManager()->GetEventByIndex(castDeckEventIdx)->GetCastDeckActivity().GetConcreteAgeAtContinuity();
   Float64   cure_time              = age_at_initial_loading;
#pragma Reminder("REVIEW: is this the correct V/S ratio for the deck?")
   // can't get deck properties until the model is build... and that is what we are doing here so there is recursion

   //Float64 vsDeck = pSectProp->GetTributaryDeckArea(poi)/pSectProp->GetTributaryFlangeWidth(poi);
   Float64 vsDeck = 1.5;
   // *NOTE* Only the top portion of the Deck is exposed to drying

   // modulus of rupture coefficients
   Float64 time_step = time_at_casting + cure_time;
   m_pDeckConc.reset( CreateConcreteModel(_T("Deck Concrete"),pDeck->Concrete,time_at_casting,cure_time,age_at_initial_loading,time_step,vsDeck) );

   //////////////////////////////////////////////////////////////////////////////
   //
   // Create railing concrete
   //
   //////////////////////////////////////////////////////////////////////////////
   EventIndexType railingSystemEventIdx = m_pBridgeDesc->GetTimelineManager()->GetRailingSystemLoadEventIndex();
   IntervalIndexType intervalIdx = pIntervals->GetInterval(railingSystemEventIdx);
   time_at_casting = pIntervals->GetStart(intervalIdx);
   age_at_initial_loading = 0.0; // assume railing system load is at full strength immediately
   cure_time = 0.0;

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

   time_step = time_at_casting + cure_time;
   m_pRailingConc[pgsTypes::tboLeft].reset( CreateConcreteModel(_T("Left Railing Concrete"), pLeftRailingSystem->Concrete,time_at_casting,cure_time,age_at_initial_loading,time_step,0.0) );

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

   m_pRailingConc[pgsTypes::tboRight].reset( CreateConcreteModel(_T("Right Railing Concrete"), pRightRailingSystem->Concrete,time_at_casting,cure_time,age_at_initial_loading,time_step,0.0) );
   
   //////////////////////////////////////////////////////////////////////////////
   //
   // Precast Segment and Closure Pour Concrete
   //
   //////////////////////////////////////////////////////////////////////////////
   EventIndexType segConstructEventIdx = m_pBridgeDesc->GetTimelineManager()->GetSegmentConstructionEventIndex();
   Float64 segment_casting_time        = m_pBridgeDesc->GetTimelineManager()->GetStart(segConstructEventIdx);
   Float64 segment_age_at_release      = m_pBridgeDesc->GetTimelineManager()->GetEventByIndex(segConstructEventIdx)->GetConstructSegmentsActivity().GetAgeAtRelease();
   Float64 segment_cure_time = segment_age_at_release;

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

            // Time dependent concrete
            //Float64 vsSegment = pSectProp->GetVolume(segmentKey)/pSectProp->GetSurfaceArea(segmentKey);
#pragma Reminder("UPDATE: using dummy value for segment V/S ratio")
            Float64 vsSegment = 3.0;

            //SegmentIDType segmentID = pSegment->GetID();
            //EventIndexType erectionEventIdx = m_pBridgeDesc->GetTimelineManager()->GetSegmentErectionEventIndex(segmentID);
            //IntervalIndexType intervalIdx   = pIntervals->GetInterval(erectionEventIdx); // hauling is the first interval in the erect segment activity
            //Float64 stepTime = pIntervals->GetMiddle(intervalIdx);
            IntervalIndexType intervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey); // hauling is the first interval in the erect segment activity
            Float64 stepTime = pIntervals->GetStart(intervalIdx);


            matConcreteBase* pSegmentConcrete = CreateConcreteModel(_T("Segment Concrete"),pSegment->Material.Concrete,segment_casting_time,segment_cure_time,segment_age_at_release,stepTime,vsSegment);
            m_pSegmentConcrete.insert( std::make_pair(segmentKey,boost::shared_ptr<matConcreteBase>(pSegmentConcrete)) );

            if ( segIdx < nSegments-1 )
            {
               const CClosurePourData* pClosure   = pGirder->GetClosurePour(segIdx);
               EventIndexType castClosureEventIdx      = m_pBridgeDesc->GetTimelineManager()->GetCastClosurePourEventIndex(pSegment->GetID());
               Float64 closure_casting_time       = m_pBridgeDesc->GetTimelineManager()->GetStart(castClosureEventIdx);
               Float64 closure_age_at_continuity  = m_pBridgeDesc->GetTimelineManager()->GetEventByIndex(castClosureEventIdx)->GetCastClosurePourActivity().GetConcreteAgeAtContinuity();
               Float64 closure_cure_time          = closure_age_at_continuity;

               // this isn't really needed because closure pours are for spliced girders only and
               // spliced girders only use a time-dependent material model... but since we need 
               // to provide this parameters, we will provide it so that it is logical.
               //
               // For a pseudo time-dependent concrete model, the step from initial to release
               // occurs at continuity. The time this occurs is the time at continuity. The
               // time at continuity is the time at casting plus the age at continuity. Recall 
               // that age at continuity is the duration of time from casting to continuity.
               Float64 closure_step_time = closure_casting_time + closure_age_at_continuity;

#pragma Reminder("UPDATE: using segment V/S ratio for closure - should compute for closure")
               matConcreteBase* pClosureConcrete = CreateConcreteModel(_T("Closure Concrete"),pClosure->GetConcrete(),closure_casting_time,closure_cure_time,closure_age_at_continuity,closure_step_time,vsSegment);
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

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE(ILimits,pLimits);

   // per 5.4.2.1 f'c must exceed 28 MPa (4 ksi)
   bool bSI = lrfdVersionMgr::GetUnits() == lrfdVersionMgr::SI ? true : false;
   Float64 fcMin = bSI ? ::ConvertToSysUnits(28, unitMeasure::MPa) : ::ConvertToSysUnits(4, unitMeasure::KSI);

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   // check railing system
   if ( !IsConcreteDensityInRange(m_pRailingConc[pgsTypes::tboLeft]->GetStrengthDensity(),(pgsTypes::ConcreteType)m_pRailingConc[pgsTypes::tboLeft]->GetType()) )
   {
      std::_tostringstream os;
      if ( m_pRailingConc[pgsTypes::tboLeft]->GetType() == pgsTypes::Normal )
         os << _T("Left railing system concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
      else
         os << _T("Left railing system concrete density is out of range for Lightweight Concrete per LRFD 5.2.");

      std::_tstring strMsg = os.str();

      CSegmentKey dummyKey;
      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::RailingSystem,pgsConcreteStrengthStatusItem::Density,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   if ( !IsConcreteDensityInRange(m_pRailingConc[pgsTypes::tboRight]->GetStrengthDensity(),(pgsTypes::ConcreteType)m_pRailingConc[pgsTypes::tboRight]->GetType()) )
   {
      std::_tostringstream os;
      if ( m_pRailingConc[pgsTypes::tboRight]->GetType() == pgsTypes::Normal )
         os << _T("Right railing system concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
      else
         os << _T("Right railing system concrete density is out of range for Lightweight Concrete per LRFD 5.2.");

      std::_tstring strMsg = os.str();

      CSegmentKey dummyKey;
      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(pgsConcreteStrengthStatusItem::RailingSystem,pgsConcreteStrengthStatusItem::Density,dummyKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   // Check Deck concrete
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
         os << _T("Slab concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
      else
         os << _T("Slab concrete density is out of range for Lightweight Concrete per LRFD 5.2.");

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
            //ValidateConcrete(m_pSegmentConcreteRelease[segmentKey],m_pSegmentConcreteFinal[segmentKey],pgsConcreteStrengthStatusItem::GirderSegment,osLabel.str().c_str(),segmentKey);
            ValidateConcrete(m_pSegmentConcrete[segmentKey],pgsConcreteStrengthStatusItem::GirderSegment,osLabel.str().c_str(),segmentKey);

            if ( segIdx < nSegments-1 )
            {
               std::_tostringstream osLabel2;
               osLabel2 << _T("Girder ") << LABEL_GIRDER(gdrIdx) << _T(" Closure ") << LABEL_SEGMENT(segIdx);
               ValidateConcrete(m_pClosureConcrete[segmentKey],pgsConcreteStrengthStatusItem::ClosurePour,osLabel2.str().c_str(),segmentKey);
            }
         }
      }
   }

   m_bIsValidated = true;
}

void CConcreteManager::ValidateConcrete(boost::shared_ptr<matConcreteBase> pConcrete,pgsConcreteStrengthStatusItem::ConcreteType elementType,LPCTSTR strLabel,const CSegmentKey& segmentKey)
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE(ILimits,pLimits);

   pgsTypes::ConcreteType concreteType = (pgsTypes::ConcreteType)pConcrete->GetType();

   // per 5.4.2.1 f'c must exceed 28 MPa (4 ksi)
   bool bSI = lrfdVersionMgr::GetUnits() == lrfdVersionMgr::SI ? true : false;
   Float64 fcMin = bSI ? ::ConvertToSysUnits(28, unitMeasure::MPa) : ::ConvertToSysUnits(4, unitMeasure::KSI);

   Float64 max_wc = pLimits->GetMaxConcreteUnitWeight(concreteType);
   Float64 MaxWc  = ::ConvertFromSysUnits(max_wc,pDisplayUnits->GetDensityUnit().UnitOfMeasure);

   Float64 max_girder_fci = pLimits->GetMaxSegmentFci(concreteType);
   Float64 fciGirderMax   = ::ConvertFromSysUnits(max_girder_fci,pDisplayUnits->GetStressUnit().UnitOfMeasure);
   Float64 max_girder_fc  = pLimits->GetMaxSegmentFc(concreteType);
   Float64 fcGirderMax    = ::ConvertFromSysUnits(max_girder_fc,pDisplayUnits->GetStressUnit().UnitOfMeasure);

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

   if (  max_girder_fci < fci )
   {
      std::_tostringstream os;
      os << strLabel << _T(": Initial concrete strength exceeds the normal value of ") << fciGirderMax << _T(" ") << pDisplayUnits->GetStressUnit().UnitOfMeasure.UnitTag();

      strMsg = os.str();

      pgsConcreteStrengthStatusItem* pStatusItem = new pgsConcreteStrengthStatusItem(elementType,pgsConcreteStrengthStatusItem::ReleaseStrength,segmentKey,m_StatusGroupID,m_scidConcreteStrengthWarning,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   if (  max_girder_fc < fc28 )
   {
      std::_tostringstream os;
      os << strLabel << _T(": Concrete strength exceeds the normal value of ") << fcGirderMax << _T(" ") << pDisplayUnits->GetStressUnit().UnitOfMeasure.UnitTag();

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
         os << strLabel << _T(": concrete density is out of range for Normal Weight Concrete per LRFD 5.2.");
      else
         os << strLabel << _T(": concrete density is out of range for Lightweight Concrete per LRFD 5.2.");

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

matConcreteBase* CConcreteManager::CreateConcreteModel(LPCTSTR strName,const CConcreteMaterial& concrete,Float64 timeAtCasting,Float64 cureTime,Float64 ageAtInitialLoading,Float64 stepTime,Float64 vs)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   GET_IFACE(ILossParameters,pLossParameters);
   pgsTypes::LossMethod loss_method = pLossParameters->GetLossMethod();

   GET_IFACE(IEnvironment,pEnvironment);
   Float64 rh = pEnvironment->GetRelHumidity();

   matConcreteBase* pConcrete;
   if ( loss_method == pgsTypes::TIME_STEP )
   {
      // for time step loss method, create concrete model based on time-dependent model type
      switch( pSpecEntry->GetTimeDependentModel() )
      {
      case TDM_ACI209:
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
      pConcrete = CreateLRFDConcreteModel(concrete,stepTime);
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
   pConcrete->SetVSRatio(vs);
   pConcrete->SetTimeAtCasting(timeAtCasting);
   pConcrete->SetCureTime(cureTime);
   pConcrete->SetCureMethod((matConcreteBase::CureMethod)concrete.CureMethod);


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

   // get the modulus of rupture. note that parameter 2 is NormalDensity in all cases. Since we are providing
   // the coefficient, this parameter is ignored so it doesn't matter what we use.
   Float64 frShear   = lrfdConcreteUtil::ModRupture(concrete.Fci,lrfdConcreteUtil::NormalDensity,GetShearFrCoefficient(concrete.Type));
   Float64 frFlexure = lrfdConcreteUtil::ModRupture(concrete.Fci,lrfdConcreteUtil::NormalDensity,GetFlexureFrCoefficient(concrete.Type));

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

   // get the modulus of rupture. note that parameter 2 is NormalDensity in all cases. Since we are providing
   // the coefficient, this parameter is ignored so it doesn't matter what we use.
   frShear   = lrfdConcreteUtil::ModRupture(concrete.Fc,lrfdConcreteUtil::NormalDensity,GetShearFrCoefficient(concrete.Type));
   frFlexure = lrfdConcreteUtil::ModRupture(concrete.Fc,lrfdConcreteUtil::NormalDensity,GetFlexureFrCoefficient(concrete.Type));

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

pgsTypes::ConcreteType CConcreteManager::GetClosurePourConcreteType(const CSegmentKey& closureKey)
{
   ValidateConcrete();
   return (pgsTypes::ConcreteType)m_pClosureConcrete[closureKey]->GetType();
}

bool CConcreteManager::DoesClosurePourConcreteHaveAggSplittingStrength(const CSegmentKey& closureKey)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->HasAggSplittingStrength();
}

Float64 CConcreteManager::GetClosurePourConcreteAggSplittingStrength(const CSegmentKey& closureKey)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetAggSplittingStrength();
}

Float64 CConcreteManager::GetClosurePourMaxAggrSize(const CSegmentKey& closureKey)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetMaxAggregateSize();
}

Float64 CConcreteManager::GetClosurePourStrengthDensity(const CSegmentKey& closureKey)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetStrengthDensity();
}

Float64 CConcreteManager::GetClosurePourWeightDensity(const CSegmentKey& closureKey)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetWeightDensity();
}

Float64 CConcreteManager::GetClosurePourEccK1(const CSegmentKey& closureKey)
{
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CClosurePourData* pClosure = m_pBridgeDesc->GetClosurePour(closureKey);
      K1 = pClosure->GetConcrete().EcK1;
   }

   return K1;
}

Float64 CConcreteManager::GetClosurePourEccK2(const CSegmentKey& closureKey)
{
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CClosurePourData* pClosure = m_pBridgeDesc->GetClosurePour(closureKey);
      K2 = pClosure->GetConcrete().EcK2;
   }

   return K2;
}

Float64 CConcreteManager::GetClosurePourCreepK1(const CSegmentKey& closureKey)
{
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CClosurePourData* pClosure = m_pBridgeDesc->GetClosurePour(closureKey);
      K1 = pClosure->GetConcrete().CreepK1;
   }

   return K1;
}

Float64 CConcreteManager::GetClosurePourCreepK2(const CSegmentKey& closureKey)
{
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CClosurePourData* pClosure = m_pBridgeDesc->GetClosurePour(closureKey);
      K2 = pClosure->GetConcrete().CreepK2;
   }

   return K2;
}

Float64 CConcreteManager::GetClosurePourShrinkageK1(const CSegmentKey& closureKey)
{
   Float64 K1 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CClosurePourData* pClosure = m_pBridgeDesc->GetClosurePour(closureKey);
      K1 = pClosure->GetConcrete().ShrinkageK1;
   }

   return K1;
}

Float64 CConcreteManager::GetClosurePourShrinkageK2(const CSegmentKey& closureKey)
{
   Float64 K2 = 1.0;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      const CClosurePourData* pClosure = m_pBridgeDesc->GetClosurePour(closureKey);
      K2 = pClosure->GetConcrete().ShrinkageK2;
   }

   return K2;
}

pgsTypes::ConcreteType CConcreteManager::GetDeckConcreteType()
{
   ValidateConcrete();
   return (pgsTypes::ConcreteType)m_pDeckConc->GetType();
}

bool CConcreteManager::DoesDeckConcreteHaveAggSplittingStrength()
{
   ValidateConcrete();
   return m_pDeckConc->HasAggSplittingStrength();
}

Float64 CConcreteManager::GetDeckConcreteAggSplittingStrength()
{
   ValidateConcrete();
   return m_pDeckConc->GetAggSplittingStrength();
}

Float64 CConcreteManager::GetDeckStrengthDensity()
{
   ValidateConcrete();
   return m_pDeckConc->GetStrengthDensity();
}

Float64 CConcreteManager::GetDeckWeightDensity()
{
   ValidateConcrete();
   return m_pDeckConc->GetWeightDensity();
}

Float64 CConcreteManager::GetDeckMaxAggrSize()
{
   ValidateConcrete();
   return m_pDeckConc->GetMaxAggregateSize();
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
   return m_pRailingConc[orientation]->GetFreeShrinkageStrain(t);
}

Float64 CConcreteManager::GetRailingSystemCreepCoefficient(pgsTypes::TrafficBarrierOrientation orientation,Float64 t,Float64 tla)
{
   ValidateConcrete();
   return m_pRailingConc[orientation]->GetCreepCoefficient(t,tla);
}

Float64 CConcreteManager::GetNWCDensityLimit()
{
   Float64 limit;
   if ( lrfdVersionMgr::GetUnits() == lrfdVersionMgr::US )
      limit = ::ConvertToSysUnits(135.0,unitMeasure::LbfPerFeet3);
   else
      limit = ::ConvertToSysUnits(2150.0,unitMeasure::KgPerMeter3);

   return limit;
}

Float64 CConcreteManager::GetLWCDensityLimit()
{
   Float64 limit;
   if ( lrfdVersionMgr::GetUnits() == lrfdVersionMgr::US )
      limit = ::ConvertToSysUnits(120.0,unitMeasure::LbfPerFeet3);
   else
      limit = ::ConvertToSysUnits(1925.0,unitMeasure::KgPerMeter3);

   return limit;
}

Float64 CConcreteManager::GetFlexureModRupture(Float64 fc,pgsTypes::ConcreteType type)
{
   return lrfdConcreteUtil::ModRupture( fc, lrfdConcreteUtil::DensityType(type), GetFlexureFrCoefficient(type) );
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
   return lrfdConcreteUtil::ModRupture( fc, lrfdConcreteUtil::DensityType(type), GetShearFrCoefficient(type) );
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
   return m_pDeckConc->GetTimeAtCasting();
}

Float64 CConcreteManager::GetDeckFc(Float64 t)
{
   ValidateConcrete();
   return m_pDeckConc->GetFc(t);
}

Float64 CConcreteManager::GetDeckEc(Float64 t)
{
   ValidateConcrete();
   return m_pDeckConc->GetEc(t);
}

Float64 CConcreteManager::GetDeckFlexureFr(Float64 t)
{
   ValidateConcrete();
   return m_pDeckConc->GetFlexureFr(t);
}

Float64 CConcreteManager::GetDeckShearFr(Float64 t)
{
   ValidateConcrete();
   return m_pDeckConc->GetShearFr(t);
}

Float64 CConcreteManager::GetDeckFreeShrinkageStrain(Float64 t)
{
   ValidateConcrete();
   return m_pDeckConc->GetFreeShrinkageStrain(t);
}

Float64 CConcreteManager::GetDeckCreepCoefficient(Float64 t,Float64 tla)
{
   ValidateConcrete();
   return m_pDeckConc->GetCreepCoefficient(t,tla);
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
   return m_pSegmentConcrete[segmentKey]->GetFreeShrinkageStrain(t);
}

Float64 CConcreteManager::GetSegmentCreepCoefficient(const CSegmentKey& segmentKey,Float64 t,Float64 tla)
{
   ValidateConcrete();
   return m_pSegmentConcrete[segmentKey]->GetCreepCoefficient(t,tla);
}

Float64 CConcreteManager::GetClosurePourCastingTime(const CSegmentKey& closureKey)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetTimeAtCasting();
}

Float64 CConcreteManager::GetClosurePourFc(const CSegmentKey& closureKey,Float64 t)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetFc(t);
}

Float64 CConcreteManager::GetClosurePourFlexureFr(const CSegmentKey& closureKey,Float64 t)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetFlexureFr(t);
}

Float64 CConcreteManager::GetClosurePourShearFr(const CSegmentKey& closureKey,Float64 t)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetShearFr(t);
}

Float64 CConcreteManager::GetClosurePourEc(const CSegmentKey& closureKey,Float64 t)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetEc(t);
}

Float64 CConcreteManager::GetClosurePourFreeShrinkageStrain(const CSegmentKey& closureKey,Float64 t)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetFreeShrinkageStrain(t);
}

Float64 CConcreteManager::GetClosurePourCreepCoefficient(const CSegmentKey& closureKey,Float64 t,Float64 tla)
{
   ValidateConcrete();
   return m_pClosureConcrete[closureKey]->GetCreepCoefficient(t,tla);
}

matLRFDConcrete* CConcreteManager::CreateLRFDConcreteModel(const CConcreteMaterial& concrete,Float64 stepTime)
{
   // this concrete model is simple step function. fc and E are constant at f'ci and Eci from t = 0 to
   // just before t = t_shipping. From t = t_shipping and beyond fc and E are f'c and Ec.
   //
   //
   //     |
   //     |
   //     |                        --------------------- f'c, Ec
   //     |                        :
   //     |                        :
   //     |                        :
   //     |                        :
   //     |                        :
   //     |-------------------------  f'ci, Eci
   //     |
   //     +------------------------+-------------------------
   //                              t = t_shipping

   matLRFDConcrete* pLRFDConcrete = new matLRFDConcrete();

   matConcreteEx initialConcrete, finalConcrete;
   CreateConcrete(concrete,_T(""),&initialConcrete,&finalConcrete);
   pLRFDConcrete->SetConcreteModels(initialConcrete,finalConcrete);
   pLRFDConcrete->SetStepTime(stepTime);

   return pLRFDConcrete;
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
      GET_IFACE(IIntervals,pIntervals);
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

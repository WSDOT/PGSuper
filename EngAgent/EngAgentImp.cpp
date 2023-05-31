
///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// EngAgentImp.cpp : Implementation of CEngAgentImp
#include "stdafx.h"
#include "EngAgent.h"
#include "EngAgentImp.h"
#include <PGSuperException.h>

#include "GirderHandlingChecker.h"
#include "GirderLiftingChecker.h"
#include "SummaryRatingArtifactImpl.h"

#include <IFace\BeamFactory.h>
#include <IFace\StatusCenter.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>

#include <PgsExt\StatusItem.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\LoadFactors.h>
#include <PgsExt\EngUtil.h>
#include <PgsExt\DevelopmentLength.h>

#include <PsgLib\TrafficBarrierEntry.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>

#include <Units\Convert.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define VALIDATE(x) {if ( !Validate((x)) ) THROW_SHUTDOWN("Fatal Error in Engineer Agent",XREASON_AGENTVALIDATIONFAILURE,true);}
#define INVALIDATE(x) Invalidate((x))
#define CLEAR_ALL       0
#define LOSSES          1
#define LLDF            2
#define ARTIFACTS       3

// NOTE: Critical Section for Shear
// In PGSuper 2.x and earlier, the critical section for shear was at each end of a span/girder.
// In PGSuper 3.x and later (PGSuper unified with PGSplice) the critical section for shear are
// located by pier. There is a critical section along a girder on each side of every support.
// Consider a two span spliced girder bridge made up of three segments. The center segment is 
// cantilevered over the intermediate pier. Segments 1 and 3 have only one critical section for 
// shear near abutment 1 and 3. Segment 2 has two critical sections for shear, near the center
// of the segment, on either side of Pier 2.
// For a regular prestressed girder bridge (PGSuper) there is only one segment per span and
// the critical sections are located with respect to the end piers.

//-----------------------------------------------------------------------------
Float64 GetHorizPsComponent(IBroker* pBroker, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig= nullptr)
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   Float64 ss = pStrandGeometry->GetAvgStrandSlope(poi, pConfig);
   Float64 hz = 1.0;

   if (ss < Float64_Max)
   {
      hz = ss/sqrt(1*1 + ss*ss);
   }

   return hz;
}

//-----------------------------------------------------------------------------
Float64 GetVertPsComponent(IBroker* pBroker, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig = nullptr)
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   Float64 ss = pStrandGeometry->GetAvgStrandSlope(poi,pConfig);

   Float64 vz = 0.00;
   
   if (ss < Float64_Max)
   {
      vz = ::BinarySign(ss)*1.0/sqrt(1*1 + ss*ss);
   }

   return vz;
}

//-----------------------------------------------------------------------------
CollectionIndexType LimitStateToShearIndex(pgsTypes::LimitState limitState)
{
   ATLASSERT(IsStrengthLimitState(limitState));
   CollectionIndexType idx;

   switch (limitState)
   {
   case pgsTypes::StrengthI:                         idx = 0;       break;
   case pgsTypes::StrengthII:                        idx = 1;       break;
   case pgsTypes::StrengthI_Inventory:               idx = 2;       break;
   case pgsTypes::StrengthI_Operating:               idx = 3;       break;
   case pgsTypes::StrengthI_LegalRoutine:            idx = 4;       break;
   case pgsTypes::StrengthI_LegalSpecial:            idx = 5;       break;
   case pgsTypes::StrengthI_LegalEmergency:          idx = 6;       break;
   case pgsTypes::StrengthII_PermitRoutine:          idx = 7;       break;
   case pgsTypes::StrengthII_PermitSpecial:          idx = 8;       break;
   default:
      ATLASSERT(false); // is there a new limit state type?
      idx = 0;
      break;
   }

   return idx;
}

//-----------------------------------------------------------------------------
// Simple exception safe class to override transformed and non-prismatic haunch properties during design 
// if needed and reset them back afterward
class DesignOverrider
{
public:
   DesignOverrider(bool overTransf, bool overHaunch, IBroker* pBroker) :
      m_bOverTransf(overTransf),
      m_bOverHaunch(overHaunch),
      m_pBroker(pBroker)
   {
      if (m_bOverTransf || m_bOverHaunch)
      {
         GET_IFACE(ILibrary, pLib );
         GET_IFACE(ISpecification,pSpec);
         const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

         SpecLibraryEntry clone(*pSpecEntry);

         // Designer does not handle transformed section properties well - set to gross
         if (m_bOverTransf)
         {
            clone.SetSectionPropertyMode(pgsTypes::spmGross);
         }

         if (m_bOverHaunch)
         {
            clone.SetHaunchAnalysisSectionPropertiesType(pgsTypes::hspConstFilletDepth);
         }

         // We've made changes to clone, now add it to library
         SpecLibrary* pLibrary = pLib->GetSpecLibrary();

         m_OriginalSpecEntryName = pSpecEntry->GetName();
         m_CloneSpecEntryName = pSpecEntry->GetName() + std::_tstring(_T("_clone"));
 
         while(!pLibrary->AddEntry(clone, m_CloneSpecEntryName.c_str()))
         {
            // Name not accepted - create a new one (not pretty)
            m_CloneSpecEntryName += _T("x");
         }

         // Set to new specification until we go out of scope
         pSpec->SetSpecification(m_CloneSpecEntryName);
      }
   }

   ~DesignOverrider()
   {
      if (m_bOverTransf || m_bOverHaunch)
      {
         GET_IFACE(ILibrary, pLib );
         GET_IFACE(ISpecification,pSpec);

         // Set things back to where we started
         pSpec->SetSpecification(m_OriginalSpecEntryName);

         SpecLibrary* pLibrary = pLib->GetSpecLibrary();
         libILibrary::EntryRemoveOutcome outcome = pLibrary->RemoveEntry(m_CloneSpecEntryName.c_str());
         ATLASSERT(libILibrary::RemOk == outcome);
      }
   }

private:
   DesignOverrider();

   CComPtr<IBroker> m_pBroker;
   bool m_bOverTransf;
   bool m_bOverHaunch;
   std::_tstring m_OriginalSpecEntryName;
   std::_tstring m_CloneSpecEntryName;
};


/////////////////////////////////////////////////////////////////////////////
// CEngAgentImp
CEngAgentImp::CEngAgentImp() :
m_ShearCapEngineer(nullptr,0),
m_bAreDistFactorEngineersValidated(false)
{
}

//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateAll()
{
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   pStatusCenter->RemoveByStatusGroupID(m_StatusGroupID);

   InvalidateHaunch();
   InvalidateLosses();
   InvalidateLiveLoadDistributionFactors();
   InvalidateArtifacts();
   InvalidateRatingArtifacts();
   InvalidateShearCapacity();
   InvalidateFpc();
   InvalidateShearCritSection();

   pgsMomentCapacityEngineer* pOldEng = m_pMomentCapacityEngineer.release();
   m_pMomentCapacityEngineer = std::make_unique<pgsMomentCapacityEngineer>(m_pBroker, m_StatusGroupID);
#if defined _USE_MULTITHREADING
   m_ThreadManager.CreateThread(CEngAgentImp::DeleteMomentCapacityEngineer, (LPVOID)(pOldEng));
#else
   CEngAgentImp::DeleteMomentCapacityEngineer((LPVOID)(pOldEng));
#endif

   pgsPrincipalWebStressEngineer* pOld = m_pPrincipalWebStressEngineer.release();
   m_pPrincipalWebStressEngineer = std::make_unique<pgsPrincipalWebStressEngineer>(m_pBroker, m_StatusGroupID);
#if defined _USE_MULTITHREADING
   m_ThreadManager.CreateThread(CEngAgentImp::DeletePrincipalWebStressEngineer, (LPVOID)(pOld));
#else
   CEngAgentImp::DeletePrincipalWebStressEngineer((LPVOID)(pOld));
#endif
}

UINT CEngAgentImp::DeleteMomentCapacityEngineer(LPVOID pParam)
{
   WATCH(_T("Begin: DeleteMomentCapacityEngineer"));
   pgsMomentCapacityEngineer* pEng = (pgsMomentCapacityEngineer*)pParam;
   delete pEng;
   WATCH(_T("End: DeleteMomentCapacityEngineer"));
   return 0;
}

UINT CEngAgentImp::DeletePrincipalWebStressEngineer(LPVOID pParam)
{
   WATCH(_T("Begin: DeletePrincipalWebStressEngineer"));
   pgsPrincipalWebStressEngineer* pEng = (pgsPrincipalWebStressEngineer*)pParam;
   delete pEng;
   WATCH(_T("End: DeletePrincipalWebStressEngineer"));
   return 0;
}

//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateHaunch()
{
   m_SlabOffsetDetails.clear();
}

//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateLosses()
{
   LOG("Invalidating losses");
   m_TransferLengthEngineer.Invalidate();
   m_DevelopmentLengthEngineer.Invalidate();
   m_PsForceEngineer.Invalidate();
   m_PsForce.clear(); // if losses are gone, so are forces
   m_PsForceWithLiveLoad.clear();
}

//-----------------------------------------------------------------------------
void CEngAgentImp::ValidateLiveLoadDistributionFactors(const CGirderKey& girderKey) const
{
   if (!m_bAreDistFactorEngineersValidated)
   {
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

      // Create dist factor engineer
      if ( m_pDistFactorEngineer == nullptr )
      {
         const CGirderGroupData* pGroup      = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
         const GirderLibraryEntry* pGdrEntry = pGroup->GetGirderLibraryEntry(girderKey.girderIndex);

         CComPtr<IBeamFactory> pFactory;
         pGdrEntry->GetBeamFactory(&pFactory);
         pFactory->CreateDistFactorEngineer(m_pBroker,m_StatusGroupID,nullptr,nullptr,nullptr,&m_pDistFactorEngineer);
      }

      // Issue warning status items if warranted
      pgsTypes::DistributionFactorMethod method = pBridgeDesc->GetDistributionFactorMethod();
      if ( method == pgsTypes::DirectlyInput )
      {
         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         std::_tstring str(_T("Live Load Distribution Factors were User-Input."));

         pgsLldfWarningStatusItem* pStatusItem = new pgsLldfWarningStatusItem(m_StatusGroupID,m_scidLldfWarning,str.c_str());
         pStatusCenter->Add(pStatusItem);
      }
      else
      {
         if ( method == pgsTypes::LeverRule )
         {
            GET_IFACE(IEAFStatusCenter,pStatusCenter);
            std::_tstring str(_T("All Live Load Distribution Factors are computed using the Lever Rule."));
            pgsLldfWarningStatusItem* pStatusItem = new pgsLldfWarningStatusItem(m_StatusGroupID,m_scidLldfWarning,str.c_str());
            pStatusCenter->Add(pStatusItem);
         }
         else
         {
            GET_IFACE(ILiveLoads,pLiveLoads);
            LldfRangeOfApplicabilityAction action = pLiveLoads->GetLldfRangeOfApplicabilityAction();
            if (action == roaIgnore)
            {
               GET_IFACE(IEAFStatusCenter,pStatusCenter);
               std::_tstring str(_T("Ranges of Applicability for Live Load Distribution Factor Equations have been ignored."));
               pgsLldfWarningStatusItem* pStatusItem = new pgsLldfWarningStatusItem(m_StatusGroupID,m_scidLldfWarning,str.c_str());
               pStatusCenter->Add(pStatusItem);
            }
            else if (action == roaIgnoreUseLeverRule)
            {
               GET_IFACE(IEAFStatusCenter,pStatusCenter);
               std::_tstring str(_T("The Lever Rule has been used for all cases where Ranges of Applicability for Live Load Distribution Factor Equations are exceeded. Otherwise, factors are computed using the Equations."));
               pgsLldfWarningStatusItem* pStatusItem = new pgsLldfWarningStatusItem(m_StatusGroupID,m_scidLldfWarning,str.c_str());
               pStatusCenter->Add(pStatusItem);
            }
         }
      }

      m_bAreDistFactorEngineersValidated = true;
   }
}

//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateLiveLoadDistributionFactors()
{
   LOG("Invalidating live load distribution factors");
   m_pDistFactorEngineer = 0;
   m_bAreDistFactorEngineersValidated = false;
}

//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateArtifacts()
{
   LOG("Invalidating artifacts");
   m_Designer.ClearArtifacts();

   m_LiftingArtifacts.clear();
   m_HaulingArtifacts.clear();
}

//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateRatingArtifacts()
{
   LOG("Invalidating rating artifacts");
   int size = sizeof(m_RatingArtifacts)/sizeof(std::map<RatingArtifactKey,pgsRatingArtifact>);
   for ( int i = 0; i < size; i++ )
   {
      m_RatingArtifacts[i].clear();
   }
}

//-----------------------------------------------------------------------------
void CEngAgentImp::ValidateArtifacts(const CGirderKey& girderKey) const
{
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   std::_tostringstream os;
   os << _T("Analyzing ") << GIRDER_LABEL(girderKey) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   const pgsGirderArtifact* pGdrArtifact = m_Designer.Check(girderKey);
}

//-----------------------------------------------------------------------------
void CEngAgentImp::ValidateRatingArtifacts(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const
{
   GET_IFACE(IRatingSpecification,pRatingSpec);
   if ( !pRatingSpec->IsRatingEnabled(ratingType) )
   {
      return; // this type isn't enabled, so leave
   }

   RatingArtifactKey key(girderKey,vehicleIdx);
   auto found = m_RatingArtifacts[ratingType].find(key);
   if ( found != m_RatingArtifacts[ratingType].end() )
   {
      return; // We already have an artifact for this girder
   }

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   std::_tostringstream os;
   os << "Load Rating Girder Line " << LABEL_GIRDER(girderKey.girderIndex) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   pgsRatingArtifact artifact = m_LoadRater.Rate(girderKey,ratingType,vehicleIdx);

   m_RatingArtifacts[ratingType].insert( std::make_pair(key,artifact) );
}

//-----------------------------------------------------------------------------
const LOSSDETAILS* CEngAgentImp::FindLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) const
{
   return m_PsForceEngineer.GetLosses(poi,intervalIdx);
}

//-----------------------------------------------------------------------------

const pgsRatingArtifact* CEngAgentImp::FindRatingArtifact(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const
{
    RatingArtifactKey key(girderKey,vehicleIdx);
    auto found = m_RatingArtifacts[ratingType].find( key );
    if ( found == m_RatingArtifacts[ratingType].end() )
    {
        return nullptr;
    }

    return &(*found).second;
}

//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateShearCapacity()
{
   LOG(_T("Invalidating shear capacities"));
#pragma Reminder("UPDATE: this should be done in a worker thread... see BridgeAgent")
   CollectionIndexType size = sizeof(m_ShearCapacity)/sizeof(ShearCapacityContainer);
   for (CollectionIndexType idx = 0; idx < size; idx++ )
   {
      m_ShearCapacity[idx].clear();
   }
}

//-----------------------------------------------------------------------------

const SHEARCAPACITYDETAILS* CEngAgentImp::ValidateShearCapacity(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const
{
   CollectionIndexType idx = LimitStateToShearIndex(limitState);

   if ( poi.GetID() != INVALID_ID )
   {
      auto found = m_ShearCapacity[idx].find( poi.GetID() );

      if ( found != m_ShearCapacity[idx].end() )
      {
         return &( (*found).second ); // capacities have already been computed
      }
   }

   SHEARCAPACITYDETAILS scd;
   m_ShearCapEngineer.ComputeShearCapacity(intervalIdx,limitState,poi,nullptr,&scd);

   ATLASSERT(poi.GetID() != INVALID_ID);

   auto retval = m_ShearCapacity[idx].insert( std::make_pair(poi.GetID(),scd) );
   return &((*(retval.first)).second);
}

//-----------------------------------------------------------------------------
const FPCDETAILS* CEngAgentImp::ValidateFpc(const pgsPointOfInterest& poi) const
{
   if ( poi.GetID() != INVALID_ID )
   {
      auto found = m_Fpc.find( poi.GetID() );
      if ( found != m_Fpc.end() )
      {
         return &( (*found).second ); // already been computed
      }
   }

   FPCDETAILS mcd;
   m_ShearCapEngineer.ComputeFpc(poi,nullptr,&mcd);

   ATLASSERT(poi.GetID() != INVALID_ID);

   auto retval = m_Fpc.insert( std::make_pair(poi.GetID(),mcd) );
   return &((*(retval.first)).second);
}

//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateFpc()
{
   LOG(_T("Invalidating Fpc"));
   m_Fpc.clear();
}


//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateShearCritSection()
{
   LOG(_T("Invalidating critical section for shear"));
   CollectionIndexType size = sizeof(m_CritSectionDetails)/sizeof(std::map<CGirderKey,std::vector<CRITSECTDETAILS>>);
   for (CollectionIndexType idx = 0; idx < size; idx++ )
   {
      m_CritSectionDetails[idx].clear();
   }
}

//-----------------------------------------------------------------------------
const std::vector<CRITSECTDETAILS>& CEngAgentImp::ValidateShearCritSection(pgsTypes::LimitState limitState,const CGirderKey& girderKey, const GDRCONFIG* pConfig) const
{
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   // LRFD 2004 and later, critical section is only a function of dv, which comes from the calculation of Mu,
   // so critical section is not a function of the limit state. We will work with the Strength I limit state
   if ( lrfdVersionMgr::ThirdEdition2004 <= pSpecEntry->GetSpecificationType() )
   {
      limitState = pgsTypes::StrengthI;
   }

   auto* pDetails = (pConfig ? &m_DesignCritSectionDetails : &m_CritSectionDetails);
   auto found = (*pDetails)[LimitStateToShearIndex(limitState)].find(girderKey);
   if ( found != (*pDetails)[LimitStateToShearIndex(limitState)].end() )
   {
      // We already have the value for this girder...
      return found->second;
   }

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   std::_tostringstream os;

   os << _T("Computing ") << GetLimitStateString(limitState) << _T(" critical section for shear for Girder ") << LABEL_GIRDER(girderKey.girderIndex) << std::ends;

   pProgress->UpdateMessage( os.str().c_str() );

   // calculations
   std::vector<CRITSECTDETAILS> vCSD(CalculateShearCritSection(limitState,girderKey,pConfig));

   auto retval = (*pDetails)[LimitStateToShearIndex(limitState)].insert( std::make_pair(girderKey,vCSD) );
   ATLASSERT(retval.second == true);
   return retval.first->second;
}

//-----------------------------------------------------------------------------
std::vector<CRITSECTDETAILS> CEngAgentImp::CalculateShearCritSection(pgsTypes::LimitState limitState, const CGirderKey& girderKey, const GDRCONFIG* pConfig) const
{
   // NOTE: Think spliced girder bridge when working in this function...
   // There are multiple critical sections for shear in a spliced girder. There is one on each
   // side of every pier. There could be zero, one, two, or more critical sections in a segment.
   // The critical sections do not necessarily occur near the ends of the segments (e.g. cantilever pier segment).
   //
   // For conventional pretensioned girders, there are two critical sections per girder (each girder with one segment)
   // and they occur near the ends of the girder.
   //
   //       Segment               Segment                Segment           Segment
   //  |<------------------->|<---------------->|<------------------>|<--------------->|
   //  ####*=================|=*##########*=====|========*#######*===|=============*####  
   //  o                            ^                        ^                         o  
   //  |             Span           |       Span             |        Span             |
   //  |<-------------------------->|<---------------------->|<----------------------->|
   //
   // * = critical section location
   // # = location between FOS and critical section (critical section zones)

   ASSERT_GIRDER_KEY(girderKey);

   std::vector<CRITSECTDETAILS> vcsDetails;

   PoiAttributeType attributes = (IsStrengthILimitState(limitState) ? POI_CRITSECTSHEAR1 : POI_CRITSECTSHEAR2);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bThirdEdition = ( lrfdVersionMgr::ThirdEdition2004 <= pSpecEntry->GetSpecificationType() ? true : false );

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;

   // Determine how many critical sections the girder will have.
   // Number of critical sections is equal to the number of face of supports

   GET_IFACE(IPointOfInterest, pIPoi);
   PoiList vFosPoi;
   pIPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), POI_FACEOFSUPPORT, &vFosPoi);

   // if there aren't any face of supports on this segment, leave now
   if ( vFosPoi.size() == 0 )
   {
      return vcsDetails;
   }

   // need to check reactions... if uplift then CS is at face of support
   // we want to use the bridge analysis type that minimizes reaction
   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   // get the piers that go with the face of supports
   std::vector<std::pair<const CPierData2*,pgsTypes::PierFaceType>> vPiers;
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(girderKey.groupIndex);

   // pier at start of group
   const CPierData2* pPier = pGroup->GetPier(pgsTypes::metStart);
   vPiers.emplace_back(pPier,pgsTypes::Ahead);

   // intermediate piers
   pPier = pPier->GetNextSpan()->GetNextPier();
   while ( pPier != pGroup->GetPier(pgsTypes::metEnd) )
   {
      vPiers.emplace_back(pPier,pgsTypes::Back); // left FOS
      vPiers.emplace_back(pPier,pgsTypes::Ahead); // right FOS

      pPier = pPier->GetNextSpan()->GetNextPier();
   }

   // Pier at end of group
   vPiers.emplace_back(pPier,pgsTypes::Back);

   // should be one pier for each FOS poi
   ATLASSERT(vFosPoi.size() == vPiers.size());

   GET_IFACE(ILimitStateForces, pLSForces);
   GET_IFACE_NOCHECK(IGirder,   pGirder);
   GET_IFACE_NOCHECK(IBridge,   pBridge); // not always used, but don't want to get it every time through the loops below

   // find the critical section associated with each FOS
   auto fosIter(vFosPoi.begin());
   auto fosEnd(vFosPoi.end());
   std::vector<std::pair<const CPierData2*,pgsTypes::PierFaceType>>::iterator pierIter(vPiers.begin());
   for ( ; fosIter != fosEnd; fosIter++, pierIter++ )
   {
      CRITSECTDETAILS csDetails;
      csDetails.pCriticalSection = nullptr;
      csDetails.bAtFaceOfSupport = false;
      const pgsPointOfInterest& poiFaceOfSupport(*fosIter);

      // need to get pier index that goes with this FOS
      const CPierData2* pPier = pierIter->first;
      pgsTypes::PierFaceType face = pierIter->second;
      PierIndexType pierIdx = pPier->GetIndex();

      csDetails.PierIdx = pierIdx;
      csDetails.PierFace = face;

      // get reactions at the pier
      Float64 Rmin,Rmax;
      pLSForces->GetLSReaction(intervalIdx,limitState,pierIdx,girderKey,bat,true,&Rmin, &Rmax);

      Rmin = IsZero(Rmin,0.001) ? 0 : Rmin;
      Rmax = IsZero(Rmax,0.001) ? 0 : Rmax;

      if ( Rmin < 0 )
      {
         // this is an uplift reaction... use the face of support for the critical section
         csDetails.bAtFaceOfSupport = true;
         csDetails.poiFaceOfSupport = poiFaceOfSupport;
         csDetails.poiFaceOfSupport.SetNonReferencedAttributes(csDetails.poiFaceOfSupport.GetNonReferencedAttributes() | attributes);
         csDetails.Start = poiFaceOfSupport.GetDistFromStart();
         csDetails.End   = csDetails.Start;
         vcsDetails.push_back(csDetails);
         continue;
      }

      // reaction causes compression in the end of the girder... need to locate CS 
      // We will look at POI that are with 2.5H from the face of support

      Float64 Hg = pGirder->GetHeight(poiFaceOfSupport);

      // Get points of interest
      // only use POI's within 2.5H from the face of the supports (2.5H on either side)
      Float64 left  = 2.5*Hg;
      Float64 right = 2.5*Hg;
      if ( pPier->IsInteriorPier() )
      {
         if ( face == pgsTypes::Ahead )
         {
            left = 0;
         }

         if ( face == pgsTypes::Back )
         {
            right = 0;
         }
      }

#pragma Reminder("Can Optimize here by reducing the number adjacent pois. The function below returns way too many.")
      PoiList vPoi;
      pIPoi->GetPointsOfInterestInRange(left, poiFaceOfSupport, right, &vPoi);

      WBFL::Math::PiecewiseFunction theta;
      WBFL::Math::PiecewiseFunction dv;
      WBFL::Math::PiecewiseFunction dv_cos_theta;
      WBFL::Math::PiecewiseFunction unity;  // 45deg line from face of support

      // create a graph for dv and 0.5d*dv*cot(theta)
      // create intercept lines as well since we are looping on poi.
      LOG(WBFL::Debug::endl<<_T("Critical Section Intercept graph"));
      LOG(_T("Location , Dv, Theta, 0.5*Dv*cotan(theta), Y"));
      for(const pgsPointOfInterest& poi : vPoi)
      {
         CRITSECTIONDETAILSATPOI csdp;
         csdp.Intersection = CRITSECTIONDETAILSATPOI::NoIntersection;

         // distance from face of support to POI
         Float64 x;
         if ( face == pgsTypes::Ahead )
         {
            x = poi.GetDistFromStart() - poiFaceOfSupport.GetDistFromStart();
         }
         else
         {
            x = poiFaceOfSupport.GetDistFromStart() - poi.GetDistFromStart();
         }

         if ( x < 0 ) // poi is between CL Bearing and FOS... skip it
         {
            continue;
         }

         SHEARCAPACITYDETAILS scd;
         GetRawShearCapacityDetails(limitState,intervalIdx,poi,pConfig,&scd);

         // dv
         dv.AddPoint( x, scd.dv );
         csdp.Poi = poi;
         csdp.DistFromFOS = x;
         csdp.Dv  = scd.dv;

         // 0.5*dv*cot(theta)
         // theta is valid only if shear stress is in range
         if (scd.ShearInRange)
         {
            Float64 dvt   = 0.5*scd.dv/tan(scd.Theta);
            dv_cos_theta.AddPoint( x, dvt );
            theta.AddPoint( x, scd.Theta );

            csdp.InRange          = true;
            csdp.Theta            = scd.Theta;
            csdp.CotanThetaDv05   = dvt;
         }
         else
         {
            csdp.InRange          = false;
            csdp.Theta            = 0.0;
            csdp.CotanThetaDv05   = 0.0;
         }

         // save details
         csDetails.PoiData.push_back(csdp);

         // intercept functions make a 45 degree angle upward from supports
         // the intercept line is the line of unity
         unity.AddPoint( x, x );

         LOG(poi.GetDistFromStart()<<_T(", ")<<csdp.Dv<<_T(", ")<<csdp.Theta<<_T(", ")<<csdp.CotanThetaDv05<<_T(", ")<<x);
      }

      LOG(_T("End of intercept values")<< WBFL::Debug::endl);

      // now that the graphs are created, find the intersection of the unity line with the dv and 0.5dvcot(theta) lines
      // determine intersections
      WBFL::Geometry::Point2d p;
      Float64 x1;
      WBFL::Math::Range range = dv.GetRange();  // range is same for all

      try
      {
         // dv
         Int16 nIntersections = dv.Intersect(unity,range,&p);
         ATLASSERT( nIntersections == 1 );
         x1 = p.X(); // distance from face of support where the intersection occurs

         // set the critical section poi data... need the segment key and the
         // distance from the start of the segment. Distance from start of segment is
         // distance from face of support (x1) + distance from start of segment to the FOS
         csDetails.CsDv.Poi.SetSegmentKey(poiFaceOfSupport.GetSegmentKey());
         csDetails.CsDv.Poi.SetDistFromStart(face == pgsTypes::Ahead ? x1 + poiFaceOfSupport.GetDistFromStart() : poiFaceOfSupport.GetDistFromStart() - x1);

         csDetails.CsDv.DistFromFOS = x1;
         csDetails.CsDv.Dv          = dv.Evaluate(x1);
         csDetails.CsDv.InRange     = theta.GetRange().IsInRange(x1);
         if (csDetails.CsDv.InRange)
         {
            csDetails.CsDv.Intersection   = CRITSECTIONDETAILSATPOI::DvIntersection;
            csDetails.CsDv.Theta          = theta.Evaluate(x1);
            csDetails.CsDv.CotanThetaDv05 = dv_cos_theta.Evaluate(x1);
         }
         else
         {
            csDetails.CsDv.Intersection   = CRITSECTIONDETAILSATPOI::NoIntersection;
            csDetails.CsDv.Theta          = 0.0;
            csDetails.CsDv.CotanThetaDv05 = 0.0;
         }

         LOG(_T("Dv Intersection = (") << p.X() << _T(", ") << p.Y() << _T(")"));

         if (!bThirdEdition)
         {
            // dv cot(theta)
            nIntersections = dv_cos_theta.Intersect(unity, range, &p);
            if (nIntersections == 1)
            {
               x1 = p.X(); // distance from face of support where the intersection occurs

               csDetails.CsDvt.InRange = true;
               csDetails.CsDvt.Intersection = CRITSECTIONDETAILSATPOI::ThetaIntersection;
               csDetails.CsDvt.DistFromFOS = x1;

               // set the critical section poi data... need the segment key and the
               // distance from the start of the segment. Distance from start of segment is
               // distance from face of support (x1) + distance from start of segment to the FOS

               csDetails.CsDvt.Poi.SetSegmentKey(poiFaceOfSupport.GetSegmentKey());
               csDetails.CsDvt.Poi.SetDistFromStart(face == pgsTypes::Ahead ? x1 + poiFaceOfSupport.GetDistFromStart() : poiFaceOfSupport.GetDistFromStart() - x1);

               csDetails.CsDvt.Dv = dv.Evaluate(x1);
               csDetails.CsDvt.Theta = theta.Evaluate(x1);
               csDetails.CsDvt.CotanThetaDv05 = dv_cos_theta.Evaluate(x1);

               LOG(_T(".5*Dv*cot(theta) Intersection = (") << p.X() << _T(", ") << p.Y() << _T(")"));
            }
            else
            {
               ATLASSERT(nIntersections == 0);
               csDetails.CsDvt.InRange = false;
               csDetails.CsDvt.Intersection = CRITSECTIONDETAILSATPOI::NoIntersection;
               LOG(_T(".5*Dv*cot(theta) on Left Intersection = No Intersection"));
            }
         }

         // Determine the critical section. The critical section is the intersection
         // point that is furthest from the face of support
         if ( bThirdEdition ) // 4th edition and later
         {
            // Critical section is at dv from face of support
            csDetails.pCriticalSection = &csDetails.CsDv;
            csDetails.PoiData.push_back(csDetails.CsDv);
         }
         else
         {
            // 3rd edition and earlier
            if ( face == pgsTypes::Ahead )
            {
               if ( csDetails.CsDvt.Poi.GetDistFromStart() < csDetails.CsDv.Poi.GetDistFromStart() )
               {
                  csDetails.pCriticalSection = &csDetails.CsDv;
               }
               else
               {
                  csDetails.pCriticalSection = &csDetails.CsDvt;
               }
            }
            else
            {
               if ( csDetails.CsDv.Poi.GetDistFromStart() < csDetails.CsDvt.Poi.GetDistFromStart() )
               {
                  csDetails.pCriticalSection = &csDetails.CsDv;
               }
               else
               {
                  csDetails.pCriticalSection = &csDetails.CsDvt;
               }
            }

            csDetails.PoiData.push_back(csDetails.CsDv);
            csDetails.PoiData.push_back(csDetails.CsDvt);
         }

         std::sort(csDetails.PoiData.begin(),csDetails.PoiData.end());

         // critical section zone
         if ( face == pgsTypes::Ahead )
         {
            // CS/FOS is on ahead side of pier so zone goes from CL Bearing at start to CS location
            // (If this is the first segment of the girder, CS-zone starts at start face of girder)
            if ( poiFaceOfSupport.GetSegmentKey().segmentIndex == 0 )
            {
               csDetails.Start = pBridge->GetSegmentStartEndDistance(poiFaceOfSupport.GetSegmentKey());
            }
            else
            {
               // CS-zone starts where the CL Pier intersects the segment
               Float64 Xpoi;
               bool bResult = pBridge->GetPierLocation(pierIdx,csDetails.pCriticalSection->Poi.GetSegmentKey(),&Xpoi);
               ATLASSERT(bResult == true);

               csDetails.Start = Xpoi;
            }

            csDetails.End = csDetails.pCriticalSection->Poi.GetDistFromStart();
         }
         else
         {
            // CS/FOS is on back side of pier so zone goes from CS to CL Bearing at end
            csDetails.Start = csDetails.pCriticalSection->Poi.GetDistFromStart();

            // if this is the last segment in the girder, end the CS-zone at the end face of the girder
            // otherwise end it at the intersection of the CL Pier and segment
            SegmentIndexType nSegments = pGroup->GetGirder(girderKey.girderIndex)->GetSegmentCount();
            if( poiFaceOfSupport.GetSegmentKey().segmentIndex == nSegments-1 )
            {
               csDetails.End = pBridge->GetSegmentLength(poiFaceOfSupport.GetSegmentKey()) - pBridge->GetSegmentEndEndDistance(poiFaceOfSupport.GetSegmentKey());
            }
            else
            {
               Float64 Xpoi;
               bool bResult = pBridge->GetPierLocation(pierIdx,csDetails.pCriticalSection->Poi.GetSegmentKey(),&Xpoi);
               ATLASSERT(bResult == true);

               csDetails.End = Xpoi;
            }
         }

         csDetails.pCriticalSection->Poi.SetNonReferencedAttributes(attributes);
         csDetails.pCriticalSection->Poi.SetReferencedAttributes(0);

         vcsDetails.push_back(csDetails);
      }
      catch (const WBFL::Math::XFunction&)
      {
         GET_IFACE(IEAFStatusCenter,pStatusCenter);

         std::_tstring msg(_T("An error occurred while locating the critical section for shear"));
         pgsUnknownErrorStatusItem* pStatusItem = new pgsUnknownErrorStatusItem(m_StatusGroupID,m_scidUnknown,_T(__FILE__),__LINE__,msg.c_str());
         pStatusCenter->Add(pStatusItem);

         msg += std::_tstring(_T("\nSee Status Center for Details"));
         THROW_UNWIND(msg.c_str(),-1);
      }
   } // next face of support

   return vcsDetails;
}

/////////////////////////////////////////////////////////////////////////////
// IAgent
STDMETHODIMP CEngAgentImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);

   return S_OK;
}

STDMETHODIMP CEngAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface(IID_ILosses,                      this);
   pBrokerInit->RegInterface(IID_IPretensionForce,             this);
   pBrokerInit->RegInterface(IID_IPosttensionForce,            this);
   pBrokerInit->RegInterface(IID_ILiveLoadDistributionFactors, this);
   pBrokerInit->RegInterface(IID_IMomentCapacity,              this);
   pBrokerInit->RegInterface(IID_IShearCapacity,               this);
   pBrokerInit->RegInterface(IID_IPrincipalWebStress,          this);
   pBrokerInit->RegInterface(IID_IGirderHaunch,                this);
   pBrokerInit->RegInterface(IID_IFabricationOptimization,     this);
   pBrokerInit->RegInterface(IID_IArtifact,                    this);
   pBrokerInit->RegInterface(IID_ICrackedSection,              this);

    return S_OK;
}

STDMETHODIMP CEngAgentImp::Init()
{
   CREATE_LOGFILE("EngAgent");
   EAF_AGENT_INIT;

   m_pMomentCapacityEngineer = std::make_unique<pgsMomentCapacityEngineer>(m_pBroker,m_StatusGroupID);
   m_pPrincipalWebStressEngineer = std::make_unique<pgsPrincipalWebStressEngineer>(m_pBroker, m_StatusGroupID);

   m_TransferLengthEngineer.SetBroker(m_pBroker);
   m_DevelopmentLengthEngineer.SetBroker(m_pBroker);

   m_PsForceEngineer.SetBroker(m_pBroker);
   m_ShearCapEngineer.SetBroker(m_pBroker);
   m_Designer.SetBroker(m_pBroker);
   m_LoadRater.SetBroker(m_pBroker);

   m_Designer.SetStatusGroupID(m_StatusGroupID);
   m_PsForceEngineer.SetStatusGroupID(m_StatusGroupID);
   m_ShearCapEngineer.SetStatusGroupID(m_StatusGroupID);

   // regiter the callback ID's we will be using
   m_scidUnknown                = pStatusCenter->RegisterCallback( new pgsUnknownErrorStatusCallback() );
   m_scidRefinedAnalysis        = pStatusCenter->RegisterCallback( new pgsRefinedAnalysisStatusCallback(m_pBroker) );
   m_scidBridgeDescriptionError = pStatusCenter->RegisterCallback( new pgsBridgeDescriptionStatusCallback(m_pBroker,eafTypes::statusError));
   m_scidLldfWarning            = pStatusCenter->RegisterCallback( new pgsLldfWarningStatusCallback(m_pBroker) );

   return AGENT_S_SECONDPASSINIT;
}

STDMETHODIMP CEngAgentImp::Init2()
{

   //
   // Attach to connection points for interfaces this agent depends on
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   // Connection Point for the bridge description input
   hr = pBrokerInit->FindConnectionPoint(IID_IBridgeDescriptionEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwBridgeDescCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ISpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwSpecificationCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_IRatingSpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwRatingSpecificationCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ILoadModifiersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwLoadModifiersCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_IEnvironmentEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwEnvironmentCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ILossParametersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwLossParametersCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   return S_OK;
}

STDMETHODIMP CEngAgentImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_EngAgent;
   return S_OK;
}

STDMETHODIMP CEngAgentImp::Reset()
{
   InvalidateAll();
   return S_OK;
}

STDMETHODIMP CEngAgentImp::ShutDown()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr;
   
   hr = pBrokerInit->FindConnectionPoint(IID_IBridgeDescriptionEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwBridgeDescCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ISpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwSpecificationCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_IRatingSpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwRatingSpecificationCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ILoadModifiersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwLoadModifiersCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_IEnvironmentEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwEnvironmentCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ILossParametersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwLossParametersCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   EAF_AGENT_CLEAR_INTERFACE_CACHE;
   CLOSE_LOGFILE;

   return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// ILosses
Float64 CEngAgentImp::GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType) const
{
   const LOSSDETAILS* pDetails = FindLosses(poi,INVALID_INDEX);
   ATLASSERT(pDetails != 0);

   Float64 val;
   if ( pDetails->LossMethod == pgsTypes::TIME_STEP )
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(poi.GetSegmentKey());
      if ( strandType == pgsTypes::Permanent )
      {
         Float64 ApsStraight = pDetails->TimeStepDetails[releaseIntervalIdx].Strands[pgsTypes::Straight].As;
         Float64 ApsHarped   = pDetails->TimeStepDetails[releaseIntervalIdx].Strands[pgsTypes::Harped].As;
         Float64 As = ApsStraight + ApsHarped;
         if ( IsZero(As) )
         {
            val = 0;
         }
         else
         {
            // -1 because dP is less than zero for a reduction in tension and we want the loss as a positive value
            Float64 dPStraight = -1*pDetails->TimeStepDetails[releaseIntervalIdx].Strands[pgsTypes::Straight].dP;
            Float64 dPHarped   = -1*pDetails->TimeStepDetails[releaseIntervalIdx].Strands[pgsTypes::Harped].dP;
            val = (dPStraight + dPHarped)/As;
         }
      }
      else
      {
         val = -1*pDetails->TimeStepDetails[releaseIntervalIdx].Strands[strandType].dfpe;
         // -1 because dfpe is less than zero for a reduction in tension and we want the loss as a positive value
      }
   }
   else
   {
      if ( strandType == pgsTypes::Temporary )
      {
         val = pDetails->pLosses->TemporaryStrand_ElasticShorteningLosses();
      }
      else
      {
         val = pDetails->pLosses->PermanentStrand_ElasticShorteningLosses();
      }
   }

   return val;
}

const LOSSDETAILS* CEngAgentImp::GetLossDetails(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) const
{
   return FindLosses(poi,intervalIdx);
}

CString CEngAgentImp::GetRestrainingLoadName(IntervalIndexType intervalIdx,int loadType) const
{
   CString strLoadName;
   if ( loadType == TIMESTEP_CR )
   {
      strLoadName.Format(_T("Restrained_Creep_%d"),LABEL_INTERVAL(intervalIdx));
   }
   else if ( loadType == TIMESTEP_SH )
   {
      strLoadName.Format(_T("Restrained_Shrinkage_%d"),LABEL_INTERVAL(intervalIdx));
   }
   else if ( loadType == TIMESTEP_RE )
   {
      strLoadName.Format(_T("Restrained_Relaxation_%d"),LABEL_INTERVAL(intervalIdx));
   }

   return strLoadName;
}

void CEngAgentImp::ReportLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const
{
   m_PsForceEngineer.ReportLosses(girderKey,pChapter,pDisplayUnits);
}

void CEngAgentImp::ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const
{
   m_PsForceEngineer.ReportFinalLosses(girderKey,pChapter,pDisplayUnits);
}

Float64 CEngAgentImp::GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config) const
{
   const LOSSDETAILS* pDetails = GetLossDetails(poi,config,INVALID_INDEX);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
   {
      val = pDetails->pLosses->TemporaryStrand_ElasticShorteningLosses();
   }
   else
   {
      val = pDetails->pLosses->PermanentStrand_ElasticShorteningLosses();
   }

   return val;
}

const LOSSDETAILS* CEngAgentImp::GetLossDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx) const
{
   return m_PsForceEngineer.GetLosses(poi,config,intervalIdx);
}

void CEngAgentImp::ClearDesignLosses()
{
   m_PsForceEngineer.ClearDesignLosses();
}

Float64 CEngAgentImp::GetEffectivePrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const
{
   return m_PsForceEngineer.GetEffectivePrestressLoss(poi,strandType,intervalIdx,intervalTime,true/*apply elastic gain reduction*/,pConfig);
}

Float64 CEngAgentImp::GetEffectivePrestressLossWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState,VehicleIndexType vehicleIdx,bool bIncludeElasticEffects, bool bApplyElasticGainReduction, const GDRCONFIG* pConfig) const
{
   return m_PsForceEngineer.GetEffectivePrestressLossWithLiveLoad(poi,strandType,limitState,vehicleIdx, bIncludeElasticEffects, bApplyElasticGainReduction, pConfig);
}

Float64 CEngAgentImp::GetTimeDependentLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const
{
   return m_PsForceEngineer.GetTimeDependentLosses(poi,strandType,intervalIdx,intervalTime,pConfig);
}

Float64 CEngAgentImp::GetInstantaneousEffects(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const
{
   return m_PsForceEngineer.GetInstantaneousEffects(poi,strandType,intervalIdx,intervalTime,true/*apply elastic gain reduction*/, pConfig);
}

Float64 CEngAgentImp::GetInstantaneousEffectsWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, const GDRCONFIG* pConfig) const
{
   return m_PsForceEngineer.GetInstantaneousEffectsWithLiveLoad(poi,strandType,limitState,true/*apply elastic gain reduction*/,vehicleIdx, pConfig);
}

Float64 CEngAgentImp::GetGirderTendonFrictionLoss(const pgsPointOfInterest& poi,DuctIndexType ductIdx) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOnGirder(poi) )
   {
      const LOSSDETAILS* pLossDetails = GetLossDetails(poi,0); // friction losses are always computed in the first interval
      return pLossDetails->GirderFrictionLossDetails[ductIdx].dfpF;
   }
   else
   {
      return 0;
   }
}

Float64 CEngAgentImp::GetSegmentTendonFrictionLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   if (pPoi->IsOnSegment(poi))
   {
      const LOSSDETAILS* pLossDetails = GetLossDetails(poi, 0); // friction losses are always computed in the first interval
      return pLossDetails->SegmentFrictionLossDetails[ductIdx].dfpF;
   }
   else
   {
      return 0;
   }
}

Float64 CEngAgentImp::GetGirderTendonAnchorSetZoneLength(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType) const
{
   const ANCHORSETDETAILS* pDetails = m_PsForceEngineer.GetGirderTendonAnchorSetDetails(girderKey,ductIdx);
   return pDetails->Lset[endType];
}

Float64 CEngAgentImp::GetSegmentTendonAnchorSetZoneLength(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const
{
   const ANCHORSETDETAILS* pDetails = m_PsForceEngineer.GetSegmentTendonAnchorSetDetails(segmentKey, ductIdx);
   return pDetails->Lset[endType];
}

Float64 CEngAgentImp::GetGirderTendonAnchorSetLoss(const pgsPointOfInterest& poi,DuctIndexType ductIdx) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOnGirder(poi) )
   {
      const LOSSDETAILS* pLossDetails = GetLossDetails(poi,0); // anchor set losses are always computed in the first interval
      return pLossDetails->GirderFrictionLossDetails[ductIdx].dfpA;
   }
   else
   {
      return 0;
   }
}

Float64 CEngAgentImp::GetSegmentTendonAnchorSetLoss(const pgsPointOfInterest& poi, DuctIndexType ductIdx) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   if (pPoi->IsOnSegment(poi))
   {
      const LOSSDETAILS* pLossDetails = GetLossDetails(poi, 0); // anchor set losses are always computed in the first interval
      return pLossDetails->SegmentFrictionLossDetails[ductIdx].dfpA;
   }
   else
   {
      return 0;
   }
}

Float64 CEngAgentImp::GetGirderTendonElongation(const CGirderKey& girderKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const
{
   return m_PsForceEngineer.GetGirderTendonElongation(girderKey, ductIdx, endType);
}

Float64 CEngAgentImp::GetSegmentTendonElongation(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType) const
{
   return m_PsForceEngineer.GetSegmentTendonElongation(segmentKey, ductIdx, endType);
}

Float64 CEngAgentImp::GetGirderTendonAverageFrictionLoss(const CGirderKey& girderKey, DuctIndexType ductIdx) const
{
   return m_PsForceEngineer.GetGirderTendonAverageFrictionLoss(girderKey, ductIdx);
}

Float64 CEngAgentImp::GetSegmentTendonAverageFrictionLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const
{
   return m_PsForceEngineer.GetSegmentTendonAverageFrictionLoss(segmentKey, ductIdx);
}

Float64 CEngAgentImp::GetGirderTendonAverageAnchorSetLoss(const CGirderKey& girderKey, DuctIndexType ductIdx) const
{
   return m_PsForceEngineer.GetGirderTendonAverageAnchorSetLoss(girderKey, ductIdx);
}

Float64 CEngAgentImp::GetSegmentTendonAverageAnchorSetLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const
{
   return m_PsForceEngineer.GetSegmentTendonAverageAnchorSetLoss(segmentKey, ductIdx);
}

bool CEngAgentImp::AreElasticGainsApplicable() const
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   return pSpecEntry->AreElasticGainsApplicable();
}

bool CEngAgentImp::IsDeckShrinkageApplicable() const
{
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   if (IsNonstructuralDeck(pIBridgeDesc->GetDeckDescription()->GetDeckType()))
   {
      // no deck, no deck shrinkage
      return false;
   }

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   return pSpecEntry->IsDeckShrinkageApplicable();
}

bool CEngAgentImp::LossesIncludeInitialRelaxation() const
{
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(spec_name.c_str());

   GET_IFACE_NOCHECK(ILossParameters, pLossParams); // not used if spec is before 3rd Edition 2004
   bool bInitialRelaxation = (pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ||
      pLossParams->GetLossMethod() == pgsTypes::WSDOT_REFINED ||
      pLossParams->GetLossMethod() == pgsTypes::TXDOT_REFINED_2004 ||
      pLossParams->GetLossMethod() == pgsTypes::WSDOT_LUMPSUM ||
      pLossParams->GetLossMethod() == pgsTypes::TIME_STEP ? true : false);

   return bInitialRelaxation;
}


/////////////////////////////////////////////////////////////////////////////
// IPretensionForce

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetPjackMax(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,StrandIndexType nStrands) const
{
   return m_PsForceEngineer.GetPjackMax(segmentKey,strandType,nStrands);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetPjackMax(const CSegmentKey& segmentKey,const WBFL::Materials::PsStrand& strand,StrandIndexType nStrands) const
{
   return m_PsForceEngineer.GetPjackMax(segmentKey,strand,nStrands);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetTransferLength(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType, const GDRCONFIG* pConfig) const
{
   return m_TransferLengthEngineer.GetTransferLength(segmentKey,strandType,xferType,pConfig);
}

const std::shared_ptr<pgsTransferLength> CEngAgentImp::GetTransferLengthDetails(const CSegmentKey& segmentKey, pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType, const GDRCONFIG* pConfig) const
{
   return m_TransferLengthEngineer.GetTransferLengthDetails(segmentKey, strandType, xferType, pConfig);
}

Float64 CEngAgentImp::GetTransferLengthAdjustment(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType, const GDRCONFIG* pConfig) const
{
   return m_TransferLengthEngineer.GetTransferLengthAdjustment(poi,strandType, xferType,pConfig);
}

Float64 CEngAgentImp::GetTransferLengthAdjustment(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::TransferLengthType xferType, StrandIndexType strandIdx, const GDRCONFIG* pConfig) const
{
   return m_TransferLengthEngineer.GetTransferLengthAdjustment(poi, strandType, xferType, strandIdx, pConfig);
}

void CEngAgentImp::ReportTransferLengthDetails(const CSegmentKey& segmentKey, pgsTypes::TransferLengthType xferType, rptChapter* pChapter) const
{
   return m_TransferLengthEngineer.ReportTransferLengthDetails(segmentKey, xferType, pChapter);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetDevelopmentLength(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, bool bDebonded,const GDRCONFIG* pConfig) const
{
   return m_DevelopmentLengthEngineer.GetDevelopmentLength(poi,strandType, bDebonded,pConfig);
}

//-----------------------------------------------------------------------------
const std::shared_ptr<pgsDevelopmentLength> CEngAgentImp::GetDevelopmentLengthDetails(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, bool bDebonded,const GDRCONFIG* pConfig) const
{
    return m_DevelopmentLengthEngineer.GetDevelopmentLengthDetails(poi,strandType,bDebonded,pConfig);
}

void CEngAgentImp::ReportDevelopmentLengthDetails(const CSegmentKey& segmentKey, rptChapter* pChapter) const
{
   m_DevelopmentLengthEngineer.ReportDevelopmentLengthDetails(segmentKey, pChapter);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetDevelopmentLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType,bool bDebonded,const GDRCONFIG* pConfig) const
{
   return m_DevelopmentLengthEngineer.GetDevelopmentLengthAdjustment(poi,strandIdx,strandType,bDebonded,pConfig);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetDevelopmentLengthAdjustment(const pgsPointOfInterest& poi,StrandIndexType strandIdx,pgsTypes::StrandType strandType, Float64 fps, Float64 fpe,const GDRCONFIG* pConfig) const
{
   return m_DevelopmentLengthEngineer.GetDevelopmentLengthAdjustment(poi,strandIdx,strandType,fps,fpe,pConfig);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetHoldDownForce(const CSegmentKey& segmentKey,bool bTotal,Float64* pSlope,pgsPointOfInterest* pPoi,const GDRCONFIG* pConfig) const
{
   return m_PsForceEngineer.GetHoldDownForce(segmentKey,bTotal,pSlope,pPoi,pConfig);
}

Float64 CEngAgentImp::GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const
{
   Float64 cos = GetHorizPsComponent(m_pBroker,poi,pConfig);
   Float64 P = GetPrestressForce(poi,pgsTypes::Harped,intervalIdx,intervalTime,pgsTypes::tltMaximum,pConfig);
   Float64 Hp = fabs(cos*P); // this should always be positive
   return Hp;
}

Float64 CEngAgentImp::GetVertHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const
{
   Float64 sin = GetVertPsComponent(m_pBroker,poi);
   Float64 P = GetPrestressForce(poi,pgsTypes::Harped,intervalIdx,intervalTime,pgsTypes::tltMaximum,pConfig);
   Float64 Vp = sin*P;

   // determine sign of Vp. If Vp has the opposite sign as the shear due to the externally applied
   // loads, it resists shear and it is taken as a positive value (See LRFD 5.2 and 5.7.3.3 (pre2017: 5.8.3.3))
   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType batMax = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType batMin = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE(ILimitStateForces,pLsForces);
   WBFL::System::SectionValue Vmin, Vmax, dummy;
   pLsForces->GetShear(intervalIdx,pgsTypes::StrengthI,poi,batMax,&dummy,&Vmax);
   pLsForces->GetShear(intervalIdx,pgsTypes::StrengthI,poi,batMin,&Vmin,&dummy);

   Float64 max = Max(Vmax.Left(),Vmax.Right());
   Float64 min = Min(Vmin.Left(),Vmin.Right());
   max = IsZero(max, 0.001) ? 0 : max;
   min = IsZero(min, 0.001) ? 0 : min;

   Float64 sign;
   if ( fabs(min) < fabs(max) )
   {
      sign = ::Sign(max); // returns -1,0,1
   }
   else
   {
      sign = ::Sign(min);
   }

   sign *= -1; // sign of Vp is opposite sign of Vu

   if ( IsZero(sign) ) // if Vu is zero, sign is zero... Vp is just Vp and it should be a positive value
   {
      Vp = fabs(Vp);
   }
   else
   {
      Vp *= sign;
   }

   return Vp;
}

Float64 CEngAgentImp::GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime, pgsTypes::TransferLengthType xferLengthType,const GDRCONFIG* pConfig) const
{
   if (pConfig == nullptr)
   {
#pragma Reminder("UPDATE - move caching into the PsForceEngineer")
      PrestressPoiKey key(poi, PrestressSubKey(strandType, intervalIdx, intervalTime));
      std::map<PrestressPoiKey, Float64>::iterator found = m_PsForce.find(key);
      if (found != m_PsForce.end())
      {
         return (*found).second;
      }
      else
      {
         Float64 F = m_PsForceEngineer.GetPrestressForce(poi, strandType, intervalIdx, intervalTime, xferLengthType);
         m_PsForce.insert(std::make_pair(key, F));
         return F;
      }
   }
   else
   {
      return m_PsForceEngineer.GetPrestressForce(poi, strandType, intervalIdx, intervalTime, xferLengthType, pConfig);
   }
}

Float64 CEngAgentImp::GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,bool bIncludeElasticEffects,pgsTypes::TransferLengthType xferLengthType) const
{
   return m_PsForceEngineer.GetPrestressForce(poi,strandType,intervalIdx,intervalTime,bIncludeElasticEffects,xferLengthType);
}

Float64 CEngAgentImp::GetPrestressForcePerStrand(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, bool bIncludeElasticEffects) const
{
   return GetPrestressForcePerStrand(poi, strandType, intervalIdx, intervalTime, bIncludeElasticEffects, nullptr);
}

Float64 CEngAgentImp::GetPrestressForcePerStrand(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, const GDRCONFIG* pConfig) const
{
   return GetPrestressForcePerStrand(poi, strandType, intervalIdx, intervalTime, true /*include elastic effects*/, pConfig);
}

Float64 CEngAgentImp::GetPrestressForcePerStrand(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,bool bIncludeElasticEffects,const GDRCONFIG* pConfig) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IStrandGeometry, pStrandGeom);
   Float64 Ps;
   if (pConfig)
   {
      Ps = GetPrestressForce(poi, strandType, intervalIdx, intervalTime,pgsTypes::tltMinimum, pConfig);
   }
   else
   {
      Ps = GetPrestressForce(poi, strandType, intervalIdx, intervalTime, bIncludeElasticEffects,pgsTypes::tltMinimum);
   }

   StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey,strandType,pConfig);
   if ( nStrands == 0 )
   {
      return 0;
   }

   GET_IFACE(IBridge,pBridge);
   Float64 gdr_length = pBridge->GetSegmentLength(segmentKey);

   const GDRCONFIG* pTheConfig;
   if (pConfig)
   {
      pTheConfig = pConfig;
   }
   else
   {
      GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);
      pTheConfig = &config;
   }

   // permanent option must cycle through for both straight and harped strands
   IndexType ntypes = 1;
   pgsTypes::StrandType strandTypeList[2];
   if (strandType == pgsTypes::Permanent)
   {
      ntypes = 2;
      strandTypeList[0] = pgsTypes::Straight;
      strandTypeList[1] = pgsTypes::Harped;
   }
   else
   {
      strandTypeList[0] = strandType;
   }

   for (IndexType in = 0; in < ntypes; in++)
   {
      for (const auto& debond_info : pTheConfig->PrestressConfig.Debond[strandTypeList[in]])
      {
         if (InRange(0.0, poi.GetDistFromStart(), debond_info.DebondLength[pgsTypes::metStart]) ||
            InRange(gdr_length - debond_info.DebondLength[pgsTypes::metEnd], poi.GetDistFromStart(), gdr_length))
         {
            nStrands--;
         }
      }
   }

   return Ps/nStrands;
}

Float64 CEngAgentImp::GetEffectivePrestress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,const GDRCONFIG* pConfig) const
{
   return m_PsForceEngineer.GetEffectivePrestress(poi,strandType,intervalIdx,intervalTime,true/*include elastic effects*/,true/*include elastic gain reductions*/, pConfig);
}

Float64 CEngAgentImp::GetEffectivePrestress(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, bool bIncludeElasticEffects) const
{
   return m_PsForceEngineer.GetEffectivePrestress(poi, strandType, intervalIdx, intervalTime, bIncludeElasticEffects, true/*include elastic gain reductions*/, nullptr);
}

Float64 CEngAgentImp::GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, bool bIncludeElasticEffects, VehicleIndexType vehicleIndex) const
{
#pragma Reminder("UPDATE - moving caching into the PsForceEngineer")
   PrestressWithLiveLoadPoiKey key(poi, PrestressWithLiveLoadSubKey(strandType, limitState,vehicleIndex));
   std::map<PrestressWithLiveLoadPoiKey, Float64>::iterator found = m_PsForceWithLiveLoad.find(key);
   if (found != m_PsForceWithLiveLoad.end())
   {
      return (*found).second;
   }
   else
   {
      Float64 F = m_PsForceEngineer.GetPrestressForceWithLiveLoad(poi, strandType, limitState, vehicleIndex, bIncludeElasticEffects, nullptr);
      m_PsForceWithLiveLoad.insert(std::make_pair(key, F));
      return F;
   }
}

Float64 CEngAgentImp::GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex, const GDRCONFIG* pConfig) const
{
   return m_PsForceEngineer.GetPrestressForceWithLiveLoad(poi, strandType, limitState, vehicleIndex, true /*include elastic effects*/, pConfig);
}

Float64 CEngAgentImp::GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi, pgsTypes::StrandType strandType, pgsTypes::LimitState limitState, bool bIncludeElasticEffects, bool bApplyElasticGainReduction, VehicleIndexType vehicleIndex) const
{
   return m_PsForceEngineer.GetEffectivePrestressWithLiveLoad(poi, strandType, limitState, vehicleIndex, bIncludeElasticEffects, bApplyElasticGainReduction, nullptr);
}

Float64 CEngAgentImp::GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::LimitState limitState, VehicleIndexType vehicleIndex, const GDRCONFIG* pConfig) const
{
   return m_PsForceEngineer.GetEffectivePrestressWithLiveLoad(poi,strandType,limitState,vehicleIndex,true /*include elastic effects*/, true/*apply elastic gain reductino*/, pConfig);
}

void CEngAgentImp::GetEccentricityEnvelope(const pgsPointOfInterest& rpoi,const GDRCONFIG& config, Float64* pLowerBound, Float64* pUpperBound) const
{
   // Strip context data for IPrestressForce interface version
   pgsEccEnvelope env = GetEccentricityEnvelope(rpoi, config);
   *pLowerBound = env.m_LbEcc;
   *pUpperBound = env.m_UbEcc;
}

pgsEccEnvelope CEngAgentImp::GetEccentricityEnvelope(const pgsPointOfInterest& rpoi,const GDRCONFIG& config) const
{
   return m_Designer.GetEccentricityEnvelope(rpoi, config);
}

/////////////////////////////////////////////////////////////////////////////
// IPosttensionForce
Float64 CEngAgentImp::GetGirderTendonPjackMax(const CGirderKey& girderKey,StrandIndexType nStrands) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(girderKey.girderIndex);
   const CPTData*             pPTData     = pGirder->GetPostTensioning();

   return GetGirderTendonPjackMax(girderKey,*pPTData->pStrand,nStrands);
}

Float64 CEngAgentImp::GetGirderTendonPjackMax(const CGirderKey& girderKey,const WBFL::Materials::PsStrand& strand,StrandIndexType nStrands) const
{
   GET_IFACE( IAllowableTendonStress, pAllowable);
   Float64 fpj = (pAllowable->CheckTendonStressAtJacking() ? pAllowable->GetGirderTendonAllowableAtJacking(girderKey) : pAllowable->GetGirderTendonAllowablePriorToSeating(girderKey));
   Float64 aps = strand.GetNominalArea();
   Float64 Fpj = fpj*aps*nStrands;

   return Fpj;
}

Float64 CEngAgentImp::GetSegmentTendonPjackMax(const CSegmentKey& segmentKey, StrandIndexType nStrands) const
{
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   const CSegmentPTData* pPTData = &(pSegment->Tendons);

   return GetSegmentTendonPjackMax(segmentKey, *pPTData->m_pStrand, nStrands);
}

Float64 CEngAgentImp::GetSegmentTendonPjackMax(const CSegmentKey& segmentKey, const WBFL::Materials::PsStrand& strand, StrandIndexType nStrands) const
{
   GET_IFACE(IAllowableTendonStress, pAllowable);
   Float64 fpj = (pAllowable->CheckTendonStressAtJacking() ? pAllowable->GetSegmentTendonAllowableAtJacking(segmentKey) : pAllowable->GetSegmentTendonAllowablePriorToSeating(segmentKey));
   Float64 aps = strand.GetNominalArea();
   Float64 Fpj = fpj*aps*nStrands;

   return Fpj;
}

Float64 CEngAgentImp::GetGirderTendonForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx,bool bIncludeMinLiveLoad,bool bIncludeMaxLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( !pPoi->IsOnGirder(poi) )
   {
      return 0;
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   CGirderKey girderKey(segmentKey);

   GET_IFACE(IGirderTendonGeometry,pTendonGeom);

   Float64 Fpe = 0;
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   DuctIndexType firstTendonIdx = (ductIdx == ALL_DUCTS ? 0 : ductIdx);
   DuctIndexType lastTendonIdx  = (ductIdx == ALL_DUCTS ? nDucts-1 : firstTendonIdx);
   for ( DuctIndexType tendonIdx = firstTendonIdx; tendonIdx <= lastTendonIdx; tendonIdx++ )
   {
      Float64 fpe = GetGirderTendonStress(poi,intervalIdx,time,tendonIdx,bIncludeMinLiveLoad,bIncludeMaxLiveLoad,limitState,vehicleIdx);
      Float64 Apt = pTendonGeom->GetGirderTendonArea(girderKey,intervalIdx,tendonIdx);

      Fpe += fpe*Apt;
   }

   return Fpe;
}

Float64 CEngAgentImp::GetSegmentTendonForce(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time, DuctIndexType ductIdx, bool bIncludeMinLiveLoad, bool bIncludeMaxLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   if (!pPoi->IsOnSegment(poi))
   {
      return 0;
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(ISegmentTendonGeometry, pTendonGeom);

   Float64 Fpe = 0;
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(segmentKey);
   DuctIndexType firstTendonIdx = (ductIdx == ALL_DUCTS ? 0 : ductIdx);
   DuctIndexType lastTendonIdx = (ductIdx == ALL_DUCTS ? nDucts - 1 : firstTendonIdx);
   for (DuctIndexType tendonIdx = firstTendonIdx; tendonIdx <= lastTendonIdx; tendonIdx++)
   {
      Float64 fpe = GetSegmentTendonStress(poi, intervalIdx, time, tendonIdx, bIncludeMinLiveLoad, bIncludeMaxLiveLoad, limitState, vehicleIdx);
      Float64 Apt = pTendonGeom->GetSegmentTendonArea(segmentKey, intervalIdx, tendonIdx);

      Fpe += fpe*Apt;
   }

   return Fpe;
}

Float64 CEngAgentImp::GetGirderTendonAverageInitialForce(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   ASSERT_GIRDER_KEY(girderKey);
   ATLASSERT(ductIdx != ALL_DUCTS);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey,ductIdx);

   GET_IFACE(IGirderTendonGeometry,pTendonGeom);
   Float64 Apt = pTendonGeom->GetGirderTendonArea(girderKey,stressTendonIntervalIdx,ductIdx);

   Float64 fpe = GetGirderTendonAverageInitialStress(girderKey,ductIdx);

   Float64 Fpe = Apt*fpe;
   return Fpe;
}

Float64 CEngAgentImp::GetSegmentTendonAverageInitialForce(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const
{
   ASSERT_SEGMENT_KEY(segmentKey);
   ATLASSERT(ductIdx != ALL_DUCTS);

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);

   GET_IFACE(ISegmentTendonGeometry, pTendonGeom);
   Float64 Apt = pTendonGeom->GetSegmentTendonArea(segmentKey, stressTendonIntervalIdx, ductIdx);

   Float64 fpe = GetSegmentTendonAverageInitialStress(segmentKey, ductIdx);

   Float64 Fpe = Apt*fpe;
   return Fpe;
}

Float64 CEngAgentImp::GetGirderTendonAverageInitialStress(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   ASSERT_GIRDER_KEY(girderKey);
   ATLASSERT(ductIdx != ALL_DUCTS);

   GET_IFACE(ILosses,pLosses);
   Float64 dfpF = pLosses->GetGirderTendonAverageFrictionLoss(girderKey,ductIdx);
   Float64 dfpA = pLosses->GetGirderTendonAverageAnchorSetLoss(girderKey,ductIdx);

   GET_IFACE(IGirderTendonGeometry,pTendonGeom);
   Float64 fpj = pTendonGeom->GetFpj(girderKey,ductIdx);

   Float64 fpe = fpj - dfpF - dfpA;
   return fpe;
}

Float64 CEngAgentImp::GetSegmentTendonAverageInitialStress(const CSegmentKey& segmentKey, DuctIndexType ductIdx) const
{
   ASSERT_SEGMENT_KEY(segmentKey);
   ATLASSERT(ductIdx != ALL_DUCTS);

   GET_IFACE(ILosses, pLosses);
   Float64 dfpF = pLosses->GetSegmentTendonAverageFrictionLoss(segmentKey, ductIdx);
   Float64 dfpA = pLosses->GetSegmentTendonAverageAnchorSetLoss(segmentKey, ductIdx);

   GET_IFACE(ISegmentTendonGeometry, pTendonGeom);
   Float64 fpj = pTendonGeom->GetFpj(segmentKey, ductIdx);

   Float64 fpe = fpj - dfpF - dfpA;
   return fpe;
}

Float64 CEngAgentImp::GetGirderTendonInitialStress(const pgsPointOfInterest& poi,DuctIndexType ductIdx,bool bIncludeAnchorSet) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( !pPoi->IsOnGirder(poi) )
   {
      return 0;
   }

   ATLASSERT(ductIdx != ALL_DUCTS);

   CGirderKey girderKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey,ductIdx);

   GET_IFACE(ILosses,pLosses);
   const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,stressTendonIntervalIdx);

   Float64 dfpF = pDetails->GirderFrictionLossDetails[ductIdx].dfpF;
   Float64 dfpA = (bIncludeAnchorSet ? pDetails->GirderFrictionLossDetails[ductIdx].dfpA : 0);

   GET_IFACE(IGirderTendonGeometry,pTendonGeom);
   Float64 fpj = pTendonGeom->GetFpj(girderKey,ductIdx);

   Float64 fpe = fpj - dfpF - dfpA;
   return fpe;
}

Float64 CEngAgentImp::GetSegmentTendonInitialStress(const pgsPointOfInterest& poi, DuctIndexType ductIdx, bool bIncludeAnchorSet) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   if (!pPoi->IsOnSegment(poi))
   {
      return 0;
   }

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   ATLASSERT(ductIdx != ALL_DUCTS);

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);

   GET_IFACE(ILosses, pLosses);
   const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi, stressTendonIntervalIdx);

   Float64 dfpF = pDetails->SegmentFrictionLossDetails[ductIdx].dfpF;
   Float64 dfpA = (bIncludeAnchorSet ? pDetails->SegmentFrictionLossDetails[ductIdx].dfpA : 0);

   GET_IFACE(ISegmentTendonGeometry, pTendonGeom);
   Float64 fpj = pTendonGeom->GetFpj(segmentKey, ductIdx);

   Float64 fpe = fpj - dfpF - dfpA;
   return fpe;
}

Float64 CEngAgentImp::GetGirderTendonInitialForce(const pgsPointOfInterest& poi,DuctIndexType ductIdx,bool bIncludeAnchorSet) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( !pPoi->IsOnGirder(poi) )
   {
      return 0;
   }

   ATLASSERT(ductIdx != ALL_DUCTS);

   CGirderKey girderKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey,ductIdx);

   GET_IFACE(IGirderTendonGeometry,pTendonGeom);
   Float64 Apt = pTendonGeom->GetGirderTendonArea(girderKey,stressTendonIntervalIdx,ductIdx);

   Float64 fpe = GetGirderTendonInitialStress(poi,ductIdx,bIncludeAnchorSet);

   Float64 Fpe = Apt*fpe;
   return Fpe;
}

Float64 CEngAgentImp::GetSegmentTendonInitialForce(const pgsPointOfInterest& poi, DuctIndexType ductIdx, bool bIncludeAnchorSet) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   if (!pPoi->IsOnSegment(poi))
   {
      return 0;
   }

   ATLASSERT(ductIdx != ALL_DUCTS);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);

   GET_IFACE(ISegmentTendonGeometry, pTendonGeom);
   Float64 Apt = pTendonGeom->GetSegmentTendonArea(segmentKey, stressTendonIntervalIdx, ductIdx);

   Float64 fpe = GetSegmentTendonInitialStress(poi, ductIdx, bIncludeAnchorSet);

   Float64 Fpe = Apt*fpe;
   return Fpe;
}

Float64 CEngAgentImp::GetGirderTendonStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx,bool bIncludeMinLiveLoad,bool bIncludeMaxLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( !pPoi->IsOnGirder(poi) )
   {
      return 0;
   }

   ATLASSERT(ductIdx != ALL_DUCTS);
   ATLASSERT(time != pgsTypes::Middle); // can only get tendon stress at start or end of interval

   const CGirderKey& girderKey(poi.GetSegmentKey());
   GET_IFACE(IGirderTendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   if ( nDucts == 0 )
   {
      // no ducts... get the heck outta here
      return 0;
   }


   GET_IFACE(ILosses,pLosses);
   const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey,ductIdx);

   // the Time Step Loss details has the tendon stress at the end of an interval
   // If we want the tendon stress at the start of the interval, get the stress at the end of the previous interval
   // However, if this is the interval when the tendon is stressed the stress at the end of the previous interval is zero so
   // want we want is the jacking stress in this interval plus the anchor set loss.
   if ( intervalIdx != stressTendonIntervalIdx )
   {
      if ( time == pgsTypes::Start )
      {
         // start of this interval is at the end of the previous interval
         if ( intervalIdx == 0 )
         {
            return 0;
         }

         intervalIdx--;
      }
   }
#if defined _DEBUG
   else
   {
      ATLASSERT(pIntervals->GetDuration(intervalIdx) == 0);
   }
#endif

   Float64 fpe = 0;
   if ( intervalIdx == stressTendonIntervalIdx && time == pgsTypes::Start )
   {
      fpe = pDetails->TimeStepDetails[intervalIdx].GirderTendons[ductIdx].fpj + pDetails->GirderFrictionLossDetails[ductIdx].dfpA;
   }
   else
   {
      fpe = pDetails->TimeStepDetails[intervalIdx].GirderTendons[ductIdx].fpe;

      if (bIncludeMinLiveLoad || bIncludeMaxLiveLoad)
      {
         GET_IFACE(IProductForces, pProductForces);
         pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);
         pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);
         Float64 Mmin, Mmax;

         if (vehicleIdx == INVALID_INDEX)
         {
            pProductForces->GetLiveLoadMoment(intervalIdx, llType, poi, bat, true/*include impact*/, true/*include LLDF*/, &Mmin, &Mmax);
         }
         else
         {
            pProductForces->GetVehicularLiveLoadMoment(intervalIdx, llType, vehicleIdx, poi, bat, true/*include impact*/, true/*include LLDF*/, &Mmin, &Mmax);
         }

         Float64 gLL;
         if (IsRatingLimitState(limitState))
         {
            GET_IFACE(IRatingSpecification, pRatingSpec);
            gLL = pRatingSpec->GetLiveLoadFactor(limitState, true);
         }
         else
         {
            GET_IFACE(ILoadFactors, pLoadFactors);
            const CLoadFactors* pLF = pLoadFactors->GetLoadFactors();
            gLL = pLF->GetLLIMMax(limitState);
         }

         GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);
         Float64 eccX, eccY;
         pGirderTendonGeometry->GetGirderTendonEccentricity(intervalIdx, poi, ductIdx, &eccX, &eccY);

         GET_IFACE(ISectionProperties, pSectProps);
         Float64 Ixx = pSectProps->GetIxx(intervalIdx, poi);
         
         Float64 M;
         if (bIncludeMinLiveLoad && bIncludeMaxLiveLoad)
         {
            M = Max(Mmin, Mmax);
         }
         else if (bIncludeMinLiveLoad)
         {
            M = Mmin;
         }
         else if (bIncludeMaxLiveLoad)
         {
            M = Mmax;
         }

         Float64 dfLL = gLL * M * eccY / Ixx;
         fpe += dfLL;
      }
   }

   return fpe;
}

Float64 CEngAgentImp::GetSegmentTendonStress(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType time, DuctIndexType ductIdx, bool bIncludeMinLiveLoad, bool bIncludeMaxLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   if (!pPoi->IsOnSegment(poi))
   {
      return 0;
   }

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   ATLASSERT(ductIdx != ALL_DUCTS);
   ATLASSERT(time != pgsTypes::Middle); // can only get tendon stress at start or end of interval

   GET_IFACE(ISegmentTendonGeometry, pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(segmentKey);
   if (nDucts == 0)
   {
      // no ducts... get the heck outta here
      return 0;
   }


   GET_IFACE(ILosses, pLosses);
   const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi, intervalIdx);

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);

   // the Time Step Loss details has the tendon stress at the end of an interval
   // If we want the tendon stress at the start of the interval, get the stress at the end of the previous interval
   // However, if this is the interval when the tendon is stressed the stress at the end of the previous interval is zero so
   // want we want is the jacking stress in this interval plus the anchor set loss.
   if (intervalIdx != stressTendonIntervalIdx)
   {
      if (time == pgsTypes::Start)
      {
         // start of this interval is at the end of the previous interval
         if (intervalIdx == 0)
         {
            return 0;
         }

         intervalIdx--;
      }
   }
#if defined _DEBUG
   else
   {
      ATLASSERT(pIntervals->GetDuration(intervalIdx) == 0);
   }
#endif

   Float64 fpe = 0;
   if (intervalIdx == stressTendonIntervalIdx && time == pgsTypes::Start)
   {
      fpe = pDetails->TimeStepDetails[intervalIdx].SegmentTendons[ductIdx].fpj + pDetails->SegmentFrictionLossDetails[ductIdx].dfpA;
   }
   else
   {
      fpe = pDetails->TimeStepDetails[intervalIdx].SegmentTendons[ductIdx].fpe;

      if (bIncludeMinLiveLoad || bIncludeMaxLiveLoad)
      {
         GET_IFACE(IProductForces, pProductForces);
         pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);
         pgsTypes::LiveLoadType llType = LiveLoadTypeFromLimitState(limitState);
         Float64 Mmin, Mmax;

         if (vehicleIdx == INVALID_INDEX)
         {
            pProductForces->GetLiveLoadMoment(intervalIdx, llType, poi, bat, true/*include impact*/, true/*include LLDF*/, &Mmin, &Mmax);
         }
         else
         {
            pProductForces->GetVehicularLiveLoadMoment(intervalIdx, llType, vehicleIdx, poi, bat, true/*include impact*/, true/*include LLDF*/, &Mmin, &Mmax);
         }

         Float64 gLL;
         if (IsRatingLimitState(limitState))
         {
            GET_IFACE(IRatingSpecification, pRatingSpec);
            gLL = pRatingSpec->GetLiveLoadFactor(limitState, true);
         }
         else
         {
            GET_IFACE(ILoadFactors, pLoadFactors);
            const CLoadFactors* pLF = pLoadFactors->GetLoadFactors();
            gLL = pLF->GetLLIMMax(limitState);
         }

         GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeomGeometry);
         Float64 eccX, eccY;
         pSegmentTendonGeomGeometry->GetSegmentTendonEccentricity(intervalIdx, poi, ductIdx, &eccX, &eccY);

         GET_IFACE(ISectionProperties, pSectProps);
         Float64 Ixx = pSectProps->GetIxx(intervalIdx, poi);

         Float64 M;
         if (bIncludeMinLiveLoad && bIncludeMaxLiveLoad)
         {
            M = Max(Mmin, Mmax);
         }
         else if (bIncludeMinLiveLoad)
         {
            M = Mmin;
         }
         else if (bIncludeMaxLiveLoad)
         {
            M = Mmax;
         }

         Float64 dfLL = gLL * M * eccY / Ixx;
         fpe += dfLL;
      }
   }

   return fpe;
}

Float64 CEngAgentImp::GetGirderTendonVerticalForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime,DuctIndexType ductIdx) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( !pPoi->IsOnGirder(poi) )
   {
      return 0;
   }

   CGirderKey girderKey(poi.GetSegmentKey());
   GET_IFACE(IGirderTendonGeometry,pTendonGeom);
   DuctIndexType nTendons = pTendonGeom->GetDuctCount(girderKey);
   if ( nTendons == 0 )
   {
      return 0.0;
   }

   DuctIndexType firstTendonIdx = (ductIdx == ALL_DUCTS ? 0 : ductIdx);
   DuctIndexType lastTendonIdx  = (ductIdx == ALL_DUCTS ? nTendons-1 : firstTendonIdx);

   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::BridgeAnalysisType batMax = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType batMin = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE(ILimitStateForces,pLsForces);
   WBFL::System::SectionValue Vmin, Vmax, dummy;
   pLsForces->GetShear(intervalIdx,pgsTypes::StrengthI,poi,batMax,&dummy,&Vmax);
   pLsForces->GetShear(intervalIdx,pgsTypes::StrengthI,poi,batMin,&Vmin,&dummy);

   Float64 max = Max(Vmax.Left(),Vmax.Right());
   Float64 min = Min(Vmin.Left(),Vmin.Right());
   max = IsZero(max) ? 0 : max;
   min = IsZero(min) ? 0 : min;

   Float64 sign;
   if ( fabs(min) < fabs(max) )
   {
      sign = ::Sign(max);
   }
   else
   {
      sign = ::Sign(min);
   }

   sign *= -1;

   Float64 Vp = 0;
   for ( DuctIndexType tendonIdx = firstTendonIdx; tendonIdx <= lastTendonIdx; tendonIdx++ )
   {
      if (pTendonGeom->IsOnDuct(poi, tendonIdx))
      {
         Float64 Fpt = GetGirderTendonForce(poi, intervalIdx, intervalTime, tendonIdx, true, true, pgsTypes::StrengthI, INVALID_INDEX);

         CComPtr<IVector3d> slope;
         pTendonGeom->GetGirderTendonSlope(poi, tendonIdx, &slope);

         Float64 Y, Z;
         slope->get_Y(&Y);
         slope->get_Z(&Z);

         // for the case of zero shear due to external loads,
         // we want Vp to always be positive. 
         if (IsZero(sign))
         {
            Y = fabs(Y);
            sign = 1;
         }

         Vp += sign*Fpt*Y / sqrt(Y*Y + Z*Z);
      }
   }

   return Vp;
}

Float64 CEngAgentImp::GetSegmentTendonVerticalForce(const pgsPointOfInterest& poi, IntervalIndexType intervalIdx, pgsTypes::IntervalTimeType intervalTime, DuctIndexType ductIdx) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   if (!pPoi->IsOnSegment(poi))
   {
      return 0;
   }

   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   GET_IFACE(ISegmentTendonGeometry, pTendonGeom);
   DuctIndexType nTendons = pTendonGeom->GetDuctCount(segmentKey);
   if (nTendons == 0)
   {
      return 0.0;
   }

   DuctIndexType firstTendonIdx = (ductIdx == ALL_DUCTS ? 0 : ductIdx);
   DuctIndexType lastTendonIdx = (ductIdx == ALL_DUCTS ? nTendons - 1 : firstTendonIdx);

   GET_IFACE(IProductForces, pProductForces);
   pgsTypes::BridgeAnalysisType batMax = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType batMin = pProductForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE(ILimitStateForces, pLsForces);
   WBFL::System::SectionValue Vmin, Vmax, dummy;
   pLsForces->GetShear(intervalIdx, pgsTypes::StrengthI, poi, batMax, &dummy, &Vmax);
   pLsForces->GetShear(intervalIdx, pgsTypes::StrengthI, poi, batMin, &Vmin, &dummy);

   Float64 max = Max(Vmax.Left(), Vmax.Right());
   Float64 min = Min(Vmin.Left(), Vmin.Right());
   max = IsZero(max) ? 0 : max;
   min = IsZero(min) ? 0 : min;

   Float64 sign;
   if (fabs(min) < fabs(max))
   {
      sign = ::Sign(max);
   }
   else
   {
      sign = ::Sign(min);
   }

   sign *= -1;

   Float64 Vp = 0;
   for (DuctIndexType tendonIdx = firstTendonIdx; tendonIdx <= lastTendonIdx; tendonIdx++)
   {
      if (pTendonGeom->IsOnDuct(poi))
      {
         Float64 Fpt = GetSegmentTendonForce(poi, intervalIdx, intervalTime, tendonIdx, true, true, pgsTypes::StrengthI, INVALID_INDEX);

         CComPtr<IVector3d> slope;
         pTendonGeom->GetSegmentTendonSlope(poi, tendonIdx, &slope);

         Float64 Y, Z;
         slope->get_Y(&Y);
         slope->get_Z(&Z);

         // for the case of zero shear due to external loads,
         // we want Vp to always be positive. 
         if (IsZero(sign))
         {
            Y = fabs(Y);
            sign = 1;
         }

         Vp += sign*Fpt*Y / sqrt(Y*Y + Z*Z);
      }
   }

   return Vp;
}

/////////////////////////////////////////////////////////////////////////////
// IArtifact
void CEngAgentImp::VerifyDistributionFactorRequirements(const pgsPointOfInterest& poi) const
{
   CheckCurvatureRequirements(poi);
   CheckGirderStiffnessRequirements(poi);
   CheckParallelGirderRequirements(poi);
}

void CEngAgentImp::TestRangeOfApplicability(const CSpanKey& spanKey) const
{
   // let computation check ROA and throw if necessary
   GetMomentDistFactor(spanKey, pgsTypes::ServiceI);
}

void CEngAgentImp::CheckCurvatureRequirements(const pgsPointOfInterest& poi) const
{
   //
   // Check the curvature requirements (4.6.1.2.1)
   // throws exception if requirements not met
   //

   GET_IFACE(IRoadwayData,pRoadway);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pIPoi);

   CSpanKey spanKey;
   Float64 Xspan;
   pIPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   SpanIndexType nSpans = pBridge->GetSpanCount();

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   const AlignmentData2& alignment_data = pRoadway->GetAlignmentData2();
   IndexType nCurves = alignment_data.CompoundCurves.size();
   if ( nCurves == 0 )
   {
      return; // no curves
   }

   GET_IFACE(ILiveLoads,pLiveLoads);
   if ( pLiveLoads->IgnoreLLDFRangeOfApplicability() ||
        lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() // no longer required with 2005 specs
      ) 
   {
      // this criteria is not applicable because the user has chosen to ignore the range of applicability requirements
      // or we are using the LRFD 2005 or later (curvature limits were removed in 2005)
      return;
   }

   // Ok, there are curves in the alignment
   // The angle subtended by a span is equal to the change in tangent bearings between the
   // start and end of the span.
   PierIndexType start_pier = PierIndexType(spanKey.spanIndex); // start pier has same id as span
   PierIndexType end_pier = start_pier+1;
   Float64 start_station = pBridge->GetPierStation(start_pier); 
   Float64 end_station   = pBridge->GetPierStation(end_pier);

   GET_IFACE(IRoadway,pAlignment);

   CComPtr<IDirection> start_brg, end_brg;
   pAlignment->GetBearing(start_station,&start_brg);
   pAlignment->GetBearing(end_station,  &end_brg);

   CComPtr<IAngle> subtended_angle;
   end_brg->AngleBetween(start_brg,&subtended_angle);

   Float64 delta;
   subtended_angle->get_Value(&delta);
   delta = ToDegrees(fabs(delta));

   if ( 180 < delta )
   {
      delta = 360 - delta;
   }
   
   GirderIndexType nBeams = pBridge->GetGirderCount(segmentKey.groupIndex);
   Float64 delta_limit;
   bool bIsOK = true;
   if ( nBeams == 0 || nBeams == 1 )
   {
      LPCTSTR msg = _T("The bridge must have at least two beams per span");
      pgsBridgeDescriptionStatusItem* pStatusItem = new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,pgsBridgeDescriptionStatusItem::Framing,msg);
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      pStatusCenter->Add(pStatusItem);

      std::_tstring strMsg(msg);
      strMsg += _T("\nSee Status Center for Details");
      THROW_UNWIND(strMsg.c_str(),-3);
   }
   else if ( nBeams == 2 )
   {
      delta_limit = (nSpans == 1 ? 2.0 : 3.0);
      bIsOK = ( delta < delta_limit ) ? true : false;
   }
   else if ( nBeams == 3 || nBeams == 4 )
   {
      delta_limit = (nSpans == 1 ? 3.0 : 4.0);
      bIsOK = ( delta < delta_limit ) ? true : false;
   }
   else 
   {
      ATLASSERT( 5 <= nBeams );
      delta_limit = (nSpans == 1 ? 4.0 : 5.0);
      bIsOK = ( delta < delta_limit ) ? true : false;
   }

   if ( !bIsOK )
   {
      std::_tostringstream os;
      os << _T("Live Load Distribution Factors could not be calculated for the following reason") << std::endl;
      os << _T("Per 4.6.1.2.1, the limiting central angle for neglecting curvature has been exceeded") << std::endl;
      os << _T("Computed value = ") << delta << _T(" deg") << std::endl;
      os << _T("Limiting value = ") << delta_limit << _T(" deg") << std::endl;
      os << _T("A refined method of analysis is required for this bridge") << std::endl;

      pgsRefinedAnalysisStatusItem* pStatusItem = new pgsRefinedAnalysisStatusItem(m_StatusGroupID,m_scidRefinedAnalysis,os.str().c_str());
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      pStatusCenter->Add(pStatusItem);

      os << _T("See Status Center for Details") << std::endl;
      THROW_UNWIND(os.str().c_str(),XREASON_REFINEDANALYSISREQUIRED);
   }
}

void CEngAgentImp::CheckGirderStiffnessRequirements(const pgsPointOfInterest& poi) const
{
   GET_IFACE(ILiveLoads,pLiveLoads);
   if ( pLiveLoads->IgnoreLLDFRangeOfApplicability() )
   {
      return; // nothing to do here
   }

   // get difference in stiffness between all girders in this span
   // if < than limit then status center + exception
   GET_IFACE_NOCHECK(IEAFStatusCenter,pStatusCenter);
   GET_IFACE(IBridge,pBridge);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   Float64 minStiffnessRatio = pSpecEntry->GetMinGirderStiffnessRatio();

   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // want to get stiffness at same relative location for each girder
   Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);
   Float64 fractionalLength = poi.GetDistFromStart() / segmentLength;

   GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);
  
   // look at non-composite moment of inertia
   // we want girders that are basically the same
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
  
   GET_IFACE(ISectionProperties,pSectProp);
   Float64 Imin = pSectProp->GetIxx(intervalIdx,poi);
   Float64 Imax = Imin;

   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CSegmentKey current_segment_key(segmentKey.groupIndex,gdrIdx,segmentKey.segmentIndex);
      Float64 current_loc = pBridge->GetSegmentLength(current_segment_key);
      current_loc *= fractionalLength;

      pgsPointOfInterest current_poi(current_segment_key, current_loc );
      
      IntervalIndexType intervalIdx = pIntervals->GetPrestressReleaseInterval(current_segment_key);

      Float64 I = pSectProp->GetIxx(intervalIdx,current_poi);
      Imin = Min(Imin,I);
      Imax = Max(Imax,I);
   }

   Float64 ratio = Imin/Imax;
   if ( ratio < minStiffnessRatio )
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      std::_tostringstream os;
      os << _T("Live Load Distribution Factors could not be calculated for the following reason:") << std::endl;
      os << _T("The girders in Span ") << LABEL_SPAN(spanKey.spanIndex) << _T(" do not have approximately the same stiffness as required by LRFD 4.6.2.2.1.") << std::endl;
      os << _T("Minimum I = ") << (LPCTSTR)FormatDimension(Imin,pDisplayUnits->GetMomentOfInertiaUnit(),true) << std::endl;
      os << _T("Maximum I = ") << (LPCTSTR)FormatDimension(Imax,pDisplayUnits->GetMomentOfInertiaUnit(),true) << std::endl;
      os << _T("Stiffness Ratio (I min / I max) = ") << (LPCTSTR)FormatScalar(ratio,pDisplayUnits->GetScalarFormat()) << std::endl;
      os << _T("Minimum stiffness ratio permitted by the Project Criteria = ") << (LPCTSTR)FormatScalar(minStiffnessRatio,pDisplayUnits->GetScalarFormat()) << std::endl;
      os << _T("A refined method of analysis is required for this bridge") << std::endl;

      pgsRefinedAnalysisStatusItem* pStatusItem = new pgsRefinedAnalysisStatusItem(m_StatusGroupID,m_scidRefinedAnalysis,os.str().c_str());
      pStatusCenter->Add(pStatusItem);

      os << _T("See Status Center for Details") << std::endl;
      THROW_UNWIND(os.str().c_str(),XREASON_REFINEDANALYSISREQUIRED);
   }
}

void CEngAgentImp::CheckParallelGirderRequirements(const pgsPointOfInterest& poi) const
{
   GET_IFACE(ILiveLoads,pLiveLoads);
   if ( pLiveLoads->IgnoreLLDFRangeOfApplicability() )
   {
      return; // nothing to do here
   }

   // get angle between all girders in this span
   // if < than limit then status center + exception
   GET_IFACE_NOCHECK(IEAFStatusCenter,pStatusCenter);
   GET_IFACE(IBridge,pBridge);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   Float64 maxAllowableAngle = pSpecEntry->GetMaxAngularDeviationBetweenGirders();

   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
   
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      // direction is measured 0 to 2pi, we want it measured between 0 and +/- pi.
      // if angle is greater than pi, convert it to 2pi - angle
      CComPtr<IDirection> objDirection;
      pBridge->GetSegmentBearing(CSegmentKey(segmentKey.groupIndex,0,segIdx),&objDirection);
      Float64 dir_of_prev_girder;
      objDirection->get_Value(&dir_of_prev_girder);

      if ( M_PI < dir_of_prev_girder )
      {
         dir_of_prev_girder = TWO_PI - dir_of_prev_girder;
      }

      Float64 maxAngularDifference = -DBL_MAX;
      for ( GirderIndexType gdrIdx = 1; gdrIdx < nGirders; gdrIdx++ )
      {
         objDirection.Release();
         pBridge->GetSegmentBearing(CSegmentKey(segmentKey.groupIndex,gdrIdx,segIdx),&objDirection);
         Float64 dir_of_this_girder;
         objDirection->get_Value(&dir_of_this_girder);

         if ( M_PI < dir_of_this_girder )
         {
            dir_of_this_girder = TWO_PI - dir_of_this_girder;
         }

         Float64 angular_diff = fabs(dir_of_this_girder - dir_of_prev_girder);
         maxAngularDifference = Max(angular_diff,maxAngularDifference);
      } // next girder

      if ( maxAllowableAngle < maxAngularDifference )
      {
         GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
         std::_tostringstream os;
         os << _T("Live Load Distribution Factors could not be calculated for the following reason:") << std::endl;
         os << _T("The girders in this span are not parallel as required by LRFD 4.6.2.2.1.") << std::endl;
         os << _T("Greatest angular difference between girders in Span ") << LABEL_SPAN(spanKey.spanIndex) << _T(" = ") << (LPCTSTR)FormatDimension(maxAngularDifference,pDisplayUnits->GetAngleUnit(),true) << std::endl;
         os << _T("Maximum angular difference permitted the Project Criteria = ") << (LPCTSTR)FormatDimension(maxAllowableAngle,pDisplayUnits->GetAngleUnit(),true) << std::endl;
         os << _T("A refined method of analysis is required for this bridge.") << std::endl;

         pgsRefinedAnalysisStatusItem* pStatusItem = new pgsRefinedAnalysisStatusItem(m_StatusGroupID,m_scidRefinedAnalysis,os.str().c_str());
         pStatusCenter->Add(pStatusItem);

         os << _T("See Status Center for Details") << std::endl;
         THROW_UNWIND(os.str().c_str(),XREASON_REFINEDANALYSISREQUIRED);
      }
   } // next segment
}

Float64 CEngAgentImp::GetMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pSpan->GetLLDFPosMoment(spanKey.girderIndex,limitState);
   }
   else
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      CGirderKey girderKey(pGroup->GetIndex(),spanKey.girderIndex);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetMomentDF(spanKey,limitState);
   }
}

Float64 CEngAgentImp::GetMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState,Float64 fcgdr) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pSpan->GetLLDFPosMoment(spanKey.girderIndex,limitState);
   }
   else
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      CGirderKey girderKey(pGroup->GetIndex(),spanKey.girderIndex);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetMomentDF(spanKey,limitState,fcgdr);
   }
}
   
Float64 CEngAgentImp::GetNegMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pSpan->GetLLDFNegMoment(spanKey.girderIndex,limitState);
   }
   else
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      CGirderKey girderKey(pGroup->GetIndex(),spanKey.girderIndex);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetMomentDF(spanKey,limitState);
   }
}
   
Float64 CEngAgentImp::GetNegMomentDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState,Float64 fcgdr) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pSpan->GetLLDFNegMoment(spanKey.girderIndex,limitState);
   }
   else
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      CGirderKey girderKey(pGroup->GetIndex(),spanKey.girderIndex);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetMomentDF(spanKey,limitState,fcgdr);
   }
}
   
Float64 CEngAgentImp::GetNegMomentDistFactorAtPier(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState limitState,pgsTypes::PierFaceType pierFace) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pPier->GetLLDFNegMoment(gdrIdx, limitState);
   }
   else
   {
      const CSpanData2* pSpan = (pierFace == pgsTypes::Back) ? pPier->GetPrevSpan() : pPier->GetNextSpan();
      GroupIndexType grpIdx;
      if (pSpan == nullptr)
      {
         grpIdx = 0;
      }
      else
      {
         const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
         grpIdx = pGroup->GetIndex();
      }
      CGirderKey girderKey(grpIdx,gdrIdx);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetNegMomentDF(pierIdx,gdrIdx,limitState,pierFace);
   }
}
   
Float64 CEngAgentImp::GetNegMomentDistFactorAtPier(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState limitState,pgsTypes::PierFaceType pierFace,Float64 fcgdr) const
{
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);

   if (pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput)
   {
      return pPier->GetLLDFNegMoment(gdrIdx, limitState);
   }
   else
   {
      const CSpanData2* pSpan = (pierFace == pgsTypes::Back) ? pPier->GetPrevSpan() : pPier->GetNextSpan();
      GroupIndexType grpIdx;
      if (pSpan == nullptr)
      {
         grpIdx = 0;
      }
      else
      {
         const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
         grpIdx = pGroup->GetIndex();
      }
      CGirderKey girderKey(grpIdx, gdrIdx);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetNegMomentDF(pierIdx,gdrIdx,limitState,pierFace,fcgdr);
   }
}

Float64 CEngAgentImp::GetShearDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      pgsTypes::GirderLocation gl = pGroup->IsExteriorGirder(spanKey.girderIndex) ? pgsTypes::Exterior : pgsTypes::Interior;
      return pSpan->GetLLDFShear(spanKey.girderIndex,limitState);
   }
   else
   {
      CGirderKey girderKey(pGroup->GetIndex(),spanKey.girderIndex);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetShearDF(spanKey,limitState);
   }
}

Float64 CEngAgentImp::GetShearDistFactor(const CSpanKey& spanKey,pgsTypes::LimitState limitState,Float64 fcgdr) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      pgsTypes::GirderLocation gl = pGroup->IsExteriorGirder(spanKey.girderIndex) ? pgsTypes::Exterior : pgsTypes::Interior;
      return pSpan->GetLLDFShear(spanKey.girderIndex,limitState);
   }
   else
   {
      CGirderKey girderKey(pGroup->GetIndex(),spanKey.girderIndex);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetShearDF(spanKey,limitState,fcgdr);
   }
}


Float64 CEngAgentImp::GetSkewCorrectionFactorForMoment(const CSpanKey& spanKey,pgsTypes::LimitState ls) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GroupIndexType grpIdx = pGroup->GetIndex();
   CGirderKey girderKey(grpIdx,Min(spanKey.girderIndex,pGroup->GetGirderCount()-1));

   ValidateLiveLoadDistributionFactors(girderKey);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return 1.0;
   }
   else
   {
      return m_pDistFactorEngineer->GetSkewCorrectionFactorForMoment(spanKey,ls);
   }
}

Float64 CEngAgentImp::GetSkewCorrectionFactorForShear(const CSpanKey& spanKey,pgsTypes::LimitState ls) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GroupIndexType grpIdx = pGroup->GetIndex();
   CGirderKey girderKey(grpIdx,Min(spanKey.girderIndex,pGroup->GetGirderCount()-1));

   ValidateLiveLoadDistributionFactors(girderKey);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return 1.0;
   }
   else
   {
      return m_pDistFactorEngineer->GetSkewCorrectionFactorForShear(spanKey,ls);
   }
}

void CEngAgentImp::GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,Float64* pM,Float64* nM,Float64* V) const
{
   GET_IFACE(IBridge,pBridge);
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IPointOfInterest,pPOI);
   CSpanKey spanKey;
   Float64 Xspan;
   pPOI->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   PierIndexType prev_pier = PierIndexType(spanKey.spanIndex);
   PierIndexType next_pier = prev_pier + 1;

   SpanIndexType nSpans = pBridge->GetSpanCount();

   Float64 dfPoints[2];
   IndexType nPoints;
   GetNegMomentDistFactorPoints(spanKey,&dfPoints[0],&nPoints);

   *V  = GetShearDistFactor(spanKey,limitState);


   if ( lrfdVersionMgr::SeventhEdition2014 <= lrfdVersionMgr::GetVersion() )
   {
      // LRFD 7th Edition, 2014 added variable skew correction factor for shear
      Float64 skewFactor = GetSkewCorrectionFactorForShear(spanKey,limitState);
      if ( !IsEqual(skewFactor,1.0) )
      {
         Float64 span_length = pBridge->GetSpanLength(spanKey);
         Float64 L = span_length/2;

         // (*V) contains the skew correct factor.... divide it out so
         // we are working with the base LLDF
         Float64 gV = (*V)/skewFactor;

         bool bObtuseStart = pBridge->IsObtuseCorner(spanKey,pgsTypes::metStart);
         bool bObtuseEnd   = pBridge->IsObtuseCorner(spanKey,pgsTypes::metEnd);
         if ( bObtuseStart && !bObtuseEnd )
         {
            // obtuse corner is at the start of the span...
            if ( Xspan <= L )
            {
               // ... and this poi is in the first half of the span so 
               // the skew factor needs to vary from its full value to 1.0 at mid-span
               Float64 adjustedSkewFactor = (L - Xspan)*(skewFactor - 1.0)/L + 1.0;
               (*V) = gV*adjustedSkewFactor;
            }
            else
            {
               // ... and this poi is past the first half of the span so
               // the skew correction factor isn't used
               (*V) = gV;
            }
         }
         else if ( !bObtuseStart && bObtuseEnd )
         {
            ATLASSERT(pBridge->IsObtuseCorner(spanKey,pgsTypes::metEnd) == true);
            // obtuse corner is at the end of the span...
            if ( Xspan <= L )
            {
               // ... and this poi is in the first half of the span so
               // the skew correction factor isn't used
               (*V) = gV;
            }
            else
            {
               // ... and this poi is past the first half of the span so
               // the skew factor needs vary from 1.0 at mid-span to its full value
               // at the end of the span
               Float64 adjustedSkewFactor = (Xspan - L)*(skewFactor - 1.0)/(span_length - L) + 1.0;
               (*V) = gV*adjustedSkewFactor;
            }
         }
         else if ( bObtuseStart && bObtuseEnd )
         {
            // obtuse on both ends
            if ( Xspan <= L )
            {
               Float64 adjustedSkewFactor = (L - Xspan)*(skewFactor - 1.0)/L + 1.0;
               (*V) = gV*adjustedSkewFactor;
            }
            else
            {
               Float64 adjustedSkewFactor = (Xspan - L)*(skewFactor - 1.0)/(span_length - L) + 1.0;
               (*V) = gV*adjustedSkewFactor;
            }
         }
         else
         {
            // There is a skew correct factor and neither end is obtuse... that means one end is in an acute corner
            // and the other is a right angle. The skew "spanning" effect is still applicable. Shear spans from
            // the obtuse corner to the right angle. Figure out which end has the right angle
            CComPtr<IAngle> objSkewAngle;
            pBridge->GetPierSkew((PierIndexType)spanKey.spanIndex,&objSkewAngle);
            Float64 skewAngle;
            objSkewAngle->get_Value(&skewAngle);
            if ( IsZero(skewAngle) )
            {
               // right angle is at the start, treat is as the obtuse corner...
               if ( Xspan <= L )
               {
                  // ... and this poi is in the first half of the span so 
                  // the skew factor needs to vary from its full value to 1.0 at mid-span
                  Float64 adjustedSkewFactor = (L - Xspan)*(skewFactor - 1.0)/L + 1.0;
                  (*V) = gV*adjustedSkewFactor;
               }
               else
               {
                  // ... and this poi is past the first half of the span so
                  // the skew correction factor isn't used
                  (*V) = gV;
               }
            }
            else
            {
               // right angle is at the end, treat it as the obtuse corner...
               if ( Xspan <= L )
               {
                  // ... and this poi is in the first half of the span so
                  // the skew correction factor isn't used
                  (*V) = gV;
               }
               else
               {
                  // ... and this poi is past the first half of the span so
                  // the skew factor needs vary from 1.0 at mid-span to its full value
                  // at the end of the span
                  Float64 adjustedSkewFactor = (Xspan - L)*(skewFactor - 1.0)/(span_length - L) + 1.0;
                  (*V) = gV*adjustedSkewFactor;
               }
            }
         }
      }
   }

   *pM = GetMomentDistFactor(spanKey,limitState);

   if ( nPoints == 0 )
   {
      *nM = GetNegMomentDistFactor(spanKey,limitState);
   }
   else if ( nPoints == 1 )
   {
      if ( Xspan < dfPoints[0] )
      {  
         // right of contraflexure point
         bool bContinuousOnLeft, bContinuousOnRight;
         pBridge->IsContinuousAtPier(prev_pier,&bContinuousOnLeft,&bContinuousOnRight);

         bool bIntegralOnLeft, bIntegralOnRight;
         pBridge->IsIntegralAtPier(prev_pier,&bIntegralOnLeft,&bIntegralOnRight);

         if ( bContinuousOnLeft || bContinuousOnRight || bIntegralOnRight )
         {
            //Integral to the left of this point... use DF at prev pier
            *nM = GetNegMomentDistFactorAtPier(prev_pier,spanKey.girderIndex,limitState,pgsTypes::Ahead);
         }
         else
         {
            // hinged to the left of this point... use DF for the span
            *nM = GetNegMomentDistFactor(spanKey,limitState);
         }
      }
      else
      {
         // left of contraflexure point
         bool bContinuousOnLeft, bContinuousOnRight;
         pBridge->IsContinuousAtPier(next_pier,&bContinuousOnLeft,&bContinuousOnRight);

         bool bIntegralOnLeft, bIntegralOnRight;
         pBridge->IsIntegralAtPier(next_pier,&bIntegralOnLeft,&bIntegralOnRight);

         if ( bContinuousOnLeft || bContinuousOnRight || bIntegralOnLeft )
         {
            // hinged to the left of this point... use DF at the next pier
            *nM = GetNegMomentDistFactorAtPier(next_pier,spanKey.girderIndex,limitState,pgsTypes::Back);
         }
         else
         {
            // Integral to the left of this point... use DF for the span
            *nM = GetNegMomentDistFactor(spanKey,limitState);
         }
      }
   }
   else
   {
      if ( Xspan < dfPoints[0] )
      {
         *nM = GetNegMomentDistFactorAtPier(prev_pier,spanKey.girderIndex,limitState,pgsTypes::Ahead);
      }
      else if ( ::InRange(dfPoints[0],Xspan,dfPoints[1]) )
      {
         *nM = GetNegMomentDistFactor(spanKey,limitState);
      }
      else
      {
         *nM = GetNegMomentDistFactorAtPier(next_pier,spanKey.girderIndex,limitState,pgsTypes::Back);
      }
   }
}

void CEngAgentImp::GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,Float64 fcgdr,Float64* pM,Float64* nM,Float64* V) const
{
   GET_IFACE(IBridge,pBridge);
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IPointOfInterest,pPOI);
   CSpanKey spanKey;
   Float64 Xspan;
   pPOI->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   PierIndexType prev_pier = PierIndexType(spanKey.spanIndex);
   PierIndexType next_pier = prev_pier + 1;

   SpanIndexType nSpans = pBridge->GetSpanCount();

   Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 dist_from_start = poi.GetDistFromStart() - end_size;

   Float64 dfPoints[2];
   IndexType nPoints;
   GetNegMomentDistFactorPoints(spanKey,&dfPoints[0],&nPoints);

   *V  = GetShearDistFactor(spanKey,limitState,fcgdr);
   *pM = GetMomentDistFactor(spanKey,limitState,fcgdr);

   if ( nPoints == 0 )
   {
      *nM = GetNegMomentDistFactor(spanKey,limitState,fcgdr);
   }
   else if ( nPoints == 1 )
   {
      if ( dist_from_start < dfPoints[0] )
      {  
         // right of contraflexure point
         bool bContinuousOnLeft, bContinuousOnRight;
         pBridge->IsContinuousAtPier(prev_pier,&bContinuousOnLeft,&bContinuousOnRight);

         bool bIntegralOnLeft, bIntegralOnRight;
         pBridge->IsIntegralAtPier(prev_pier,&bIntegralOnLeft,&bIntegralOnRight);

         if ( bContinuousOnLeft || bContinuousOnRight || bIntegralOnLeft || bIntegralOnRight )
         {
            //Integral to the left of this point... use DF at prev pier
            *nM = GetNegMomentDistFactorAtPier(prev_pier,spanKey.girderIndex,limitState,pgsTypes::Ahead,fcgdr);
         }
         else
         {
            // hinged to the left of this point... use DF for the span
            *nM = GetNegMomentDistFactor(spanKey,limitState,fcgdr);
         }
      }
      else
      {
         // left of contraflexure point
         bool bContinuousOnLeft, bContinuousOnRight;
         pBridge->IsContinuousAtPier(next_pier,&bContinuousOnLeft,&bContinuousOnRight);

         bool bIntegralOnLeft, bIntegralOnRight;
         pBridge->IsIntegralAtPier(next_pier,&bIntegralOnLeft,&bIntegralOnRight);

         if ( bContinuousOnLeft || bContinuousOnRight || bIntegralOnLeft || bIntegralOnRight )
         {
            // Integral to the left of this point... use DF for the span
            *nM = GetNegMomentDistFactor(spanKey,limitState,fcgdr);
         }
         else
         {
            // hinged to the left of this point... use DF at the next pier
            *nM = GetNegMomentDistFactorAtPier(next_pier,spanKey.girderIndex,limitState,pgsTypes::Back,fcgdr);
         }
      }
   }
   else
   {
      if ( dist_from_start < dfPoints[0] )
      {
         *nM = GetNegMomentDistFactorAtPier(prev_pier,spanKey.girderIndex,limitState,pgsTypes::Ahead,fcgdr);
      }
      else if ( dfPoints[0] <= dist_from_start && dist_from_start <= dfPoints[1] )
      {
         *nM = GetNegMomentDistFactor(spanKey,limitState,fcgdr);
      }
      else
      {
         *nM = GetNegMomentDistFactorAtPier(next_pier,spanKey.girderIndex,limitState,pgsTypes::Back,fcgdr);
      }
   }
}

void CEngAgentImp::GetNegMomentDistFactorPoints(const CSpanKey& spanKey,Float64* dfPoints,IndexType* nPoints) const
{
   GET_IFACE(IContraflexurePoints,pCP);
   pCP->GetContraflexurePoints(spanKey,dfPoints,nPoints);
}

void CEngAgentImp::ReportDistributionFactors(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   ValidateLiveLoadDistributionFactors(girderKey);

   if ( pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::DirectlyInput )
   {
      m_pDistFactorEngineer->BuildReport(girderKey,pChapter,pDisplayUnits);
   }
   else
   {
      rptRcScalar scalar;
      scalar.SetFormat( WBFL::System::NumericFormatTool::Format::Fixed );
      scalar.SetWidth(6);
      scalar.SetPrecision(3);
      scalar.SetTolerance(1.0e-6);

      rptParagraph* pPara;
      pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
      (*pChapter) << pPara;
      (*pPara) << _T("Live Load Distribution Factors - Method of Computation: Directly Input") << rptNewLine;

      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
      pgsTypes::GirderLocation gl = pGroup->IsInteriorGirder(girderKey.girderIndex) ? pgsTypes::Interior : pgsTypes::Exterior;

      ColumnIndexType nCols = 4;
      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         nCols += 3;
      }

      rptRcTable* table = rptStyleManager::CreateDefaultTable(nCols,_T(""));

      (*pPara) << table;


      // Set up table headings
      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         table->SetNumberOfHeaderRows(1);
         (*table)(0,0) << _T("");
         (*table)(0,1) << _T("+M");
         (*table)(0,2) << _T("-M");
         (*table)(0,3) << _T("V");
      }
      else
      {
         table->SetNumberOfHeaderRows(2);

         table->SetRowSpan(0,0,2);
         table->SetRowSpan(1,0,-1);
         (*table)(0,0) << _T("");

         table->SetColumnSpan(0,1,3);
         (*table)(0,1) << _T("Strength/Service");

         table->SetColumnSpan(0,4,3);
         (*table)(0,4) << _T("Fatigue/Special Permit Rating");

         (*table)(1,1) << _T("+M");
         (*table)(1,2) << _T("-M");
         (*table)(1,3) << _T("V");
         (*table)(1,4) << _T("+M");
         (*table)(1,5) << _T("-M");
         (*table)(1,6) << _T("V");
      }

      const CPierData2* pPier = pBridgeDesc->GetPier(0);
      const CSpanData2* pSpan = nullptr;

      RowIndexType row = table->GetNumberOfHeaderRows();
      do
      {
         PierIndexType pierIdx = pPier->GetIndex();

         (*table)(row,0) << _T("Pier ") << LABEL_PIER(pierIdx);
         (*table)(row,1) << _T("");

         bool bContinuous = pPier->IsContinuous();

         bool bIntegralOnLeft, bIntegralOnRight;
         pPier->IsIntegral(&bIntegralOnLeft,&bIntegralOnRight);

         if ( bContinuous || bIntegralOnLeft || bIntegralOnRight)
         {
            (*table)(row,2) << scalar.SetValue( pPier->GetLLDFNegMoment(girderKey.girderIndex, pgsTypes::StrengthI) );
         }
         else
         {
            (*table)(row,2) << _T("");
         }

         (*table)(row,3) << _T("");

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*table)(row,4) << _T("");

            if ( bContinuous || bIntegralOnLeft || bIntegralOnRight)
            {
               (*table)(row,5) << scalar.SetValue( pPier->GetLLDFNegMoment(girderKey.girderIndex, pgsTypes::FatigueI) );
            }
            else
            {
               (*table)(row,5) << _T("");
            }
            (*table)(row,6) << _T("");
         }

         row++;

         pSpan = pPier->GetNextSpan();
         if ( pSpan )
         {
            SpanIndexType spanIdx = pSpan->GetIndex();

            bool bContinuousStart = pSpan->GetPrevPier()->IsContinuous();
            bool bContinuousEnd   = pSpan->GetNextPier()->IsContinuous();
            
            pSpan->GetPrevPier()->IsIntegral(&bIntegralOnLeft,&bIntegralOnRight);
            bool bIntegralStart = bIntegralOnRight;
            
            pSpan->GetNextPier()->IsIntegral(&bIntegralOnLeft,&bIntegralOnRight);
            bool bIntegralEnd = bIntegralOnLeft;

            (*table)(row,0) << _T("Span ") << LABEL_SPAN(spanIdx);
            (*table)(row,1) << scalar.SetValue( pSpan->GetLLDFPosMoment(girderKey.girderIndex,pgsTypes::StrengthI) );
            
            if ( bContinuousStart || bContinuousEnd || bIntegralStart || bIntegralEnd )
            {
               (*table)(row,2) << scalar.SetValue( pSpan->GetLLDFNegMoment(girderKey.girderIndex,pgsTypes::StrengthI) );
            }
            else
            {
               (*table)(row,2) << _T("");
            }

            (*table)(row,3) << scalar.SetValue( pSpan->GetLLDFShear(girderKey.girderIndex,pgsTypes::StrengthI) );

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*table)(row,4) << scalar.SetValue( pSpan->GetLLDFPosMoment(girderKey.girderIndex,pgsTypes::FatigueI) );
               
               if ( bContinuousStart || bContinuousEnd || bIntegralStart || bIntegralEnd )
               {
                  (*table)(row,5) << scalar.SetValue( pSpan->GetLLDFNegMoment(girderKey.girderIndex,pgsTypes::FatigueI) );
               }
               else
               {
                  (*table)(row,5) << _T("");
               }

               (*table)(row,6) << scalar.SetValue( pSpan->GetLLDFShear(girderKey.girderIndex,pgsTypes::FatigueI) );
            }

            row++;

            pPier = pSpan->GetNextPier();
         }
      } while ( pSpan );
   }

   ReportReactionDistributionFactors(girderKey, pChapter, true/*subheading style*/);
}

void CEngAgentImp::ReportReactionDistributionFactors(const CGirderKey& girderKey, rptChapter* pChapter,bool bSubHeading) const
{
   rptParagraph* pPara = new rptParagraph(bSubHeading ? rptStyleManager::GetSubheadingStyle() : rptStyleManager::GetHeadingStyle());
   (*pChapter) << pPara;
   (*pPara) << _T("Live Load Distribution Factors - Reactions, Deflections, and Rotations") << rptNewLine;

   pPara = new rptParagraph;
   (*pChapter) << pPara;

   Float64 mpf;
   Uint32 nLanes;
   GirderIndexType nGirders;
   GET_IFACE(IBridge, pBridge);
   SpanIndexType startSpanIdx = pBridge->GetGirderGroupStartSpan(girderKey.groupIndex);
   SpanIndexType endSpanIdx = pBridge->GetGirderGroupEndSpan(girderKey.groupIndex);

   for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
   {
      CSpanKey spanKey(spanIdx, girderKey.girderIndex);
      Float64 lldf = GetDeflectionDistFactorEx(spanKey, &mpf, &nLanes, &nGirders);

      if (startSpanIdx != endSpanIdx)
      {
         *pPara << _T("Span ") << LABEL_SPAN(spanIdx) << rptNewLine;
      }
      (*pPara) << _T("Number of Design Lanes (nLanes) = ") << nLanes << rptNewLine;
      (*pPara) << _T("Number of Girders (nGirders) = ") << nGirders << rptNewLine;
      (*pPara) << _T("Multiple Presence Factor (mpf) = ") << mpf << rptNewLine;
      (*pPara) << _T("Distribution Factor = (mpf)*(nLanes)/(nGirders) = ") << lldf << rptNewLine;
   }
}

bool CEngAgentImp::Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState limitState,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

   CGirderKey girderKey(pGroup->GetIndex(),spanKey.girderIndex);
   ValidateLiveLoadDistributionFactors(girderKey);

   return m_pDistFactorEngineer->Run1250Tests(spanKey,limitState,pid,bridgeId,resultsFile,poiFile);
}

bool CEngAgentImp::GetDFResultsEx(const CSpanKey& spanKey,pgsTypes::LimitState limitState,
                               Float64* gpM, Float64* gpM1, Float64* gpM2,
                               Float64* gnM, Float64* gnM1, Float64* gnM2,
                               Float64* gV,  Float64* gV1,  Float64* gV2 )  const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

   CGirderKey girderKey(pGroup->GetIndex(),spanKey.girderIndex);
   ValidateLiveLoadDistributionFactors(girderKey);

   return m_pDistFactorEngineer->GetDFResultsEx(spanKey, limitState,
                               gpM, gpM1, gpM2, gnM, gnM1, gnM2,
                               gV,  gV1, gV2 ); 
}

Float64 CEngAgentImp::GetDeflectionDistFactor(const CSpanKey& spanKey) const
{
   Float64 mpf;
   Uint32 nLanes;
   GirderIndexType nGirders;
   return GetDeflectionDistFactorEx(spanKey, &mpf, &nLanes, &nGirders);
}

Float64 CEngAgentImp::GetDeflectionDistFactorEx(const CSpanKey& spanKey,Float64* pMPF,Uint32* pnLanes,GirderIndexType* pnGirders) const
{
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

   GirderIndexType nGirders = pGroup->GetGirderCount();
   Uint32 nLanes = GetNumberOfDesignLanes(spanKey.spanIndex);
   Float64 mpf = lrfdUtility::GetMultiplePresenceFactor(nLanes);
   Float64 gD = mpf*nLanes / nGirders;

   *pMPF = mpf;
   *pnLanes = nLanes;
   *pnGirders = nGirders;

   return gD;
}

Uint32 CEngAgentImp::GetNumberOfDesignLanes(SpanIndexType spanIdx) const
{
   Float64 dist_to_section, curb_to_curb;
   return GetNumberOfDesignLanesEx(spanIdx,&dist_to_section,&curb_to_curb);
}

Uint32 CEngAgentImp::GetNumberOfDesignLanesEx(SpanIndexType spanIdx,Float64* pDistToSection,Float64* pCurbToCurb) const
{
   // Base number of design lanes on the width of the bridge at the LLDF spacing location
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   GET_IFACE(IBridge,pBridge);
   PierIndexType prev_pier_idx = spanIdx;

   Float64 start_bridge_station = pBridge->GetPierStation(0);
   Float64 prev_pier_station    = pBridge->GetPierStation(prev_pier_idx);

   Float64 span_length = pBridge->GetSpanLength(spanIdx);

   Float64 span_fraction_for_girder_spacing = pSpecEntry->GetLLDFGirderSpacingLocation();
   Float64 loc1 = span_fraction_for_girder_spacing*span_length;
   Float64 loc2 = (1.0-span_fraction_for_girder_spacing)*span_length;

   Float64 width1 = pBridge->GetCurbToCurbWidth( (prev_pier_station - start_bridge_station) + loc1 );
   Float64 width2 = pBridge->GetCurbToCurbWidth( (prev_pier_station - start_bridge_station) + loc2 );

   Float64 curb_to_curb_width;

   if ( ::IsLE(width2,width1) )
   {
      curb_to_curb_width = width1;
      *pDistToSection = loc1;
   }
   else
   {
      curb_to_curb_width = width2;
      *pDistToSection = loc2;
   }

   *pCurbToCurb = curb_to_curb_width;

   return lrfdUtility::GetNumDesignLanes(curb_to_curb_width);
}

/////////////////////////////////////////////////////////////////////////////
// IMomentCapacity
Float64 CEngAgentImp::GetMomentCapacity(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment) const
{
   return m_pMomentCapacityEngineer->GetMomentCapacity(intervalIdx, poi, bPositiveMoment);
}

std::vector<Float64> CEngAgentImp::GetMomentCapacity(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const
{
   return m_pMomentCapacityEngineer->GetMomentCapacity(intervalIdx, vPoi, bPositiveMoment);
}

const MOMENTCAPACITYDETAILS* CEngAgentImp::GetMomentCapacityDetails(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, bool bPositiveMoment, const GDRCONFIG* pConfig) const
{
   return m_pMomentCapacityEngineer->GetMomentCapacityDetails(intervalIdx, poi, bPositiveMoment, pConfig);
}

std::vector<const MOMENTCAPACITYDETAILS*> CEngAgentImp::GetMomentCapacityDetails(IntervalIndexType intervalIdx, const PoiList& vPoi, bool bPositiveMoment, const GDRCONFIG* pConfig) const
{
   return m_pMomentCapacityEngineer->GetMomentCapacityDetails(intervalIdx, vPoi, bPositiveMoment, pConfig);
}

std::vector<Float64> CEngAgentImp::GetCrackingMoment(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const
{
   return m_pMomentCapacityEngineer->GetCrackingMoment(intervalIdx, vPoi, bPositiveMoment);
}

Float64 CEngAgentImp::GetCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const
{
   return m_pMomentCapacityEngineer->GetCrackingMoment(intervalIdx, poi, bPositiveMoment);
}

const CRACKINGMOMENTDETAILS* CEngAgentImp::GetCrackingMomentDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const
{
   return m_pMomentCapacityEngineer->GetCrackingMomentDetails(intervalIdx, poi, bPositiveMoment);
}

void CEngAgentImp::GetCrackingMomentDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd) const
{
   m_pMomentCapacityEngineer->GetCrackingMomentDetails(intervalIdx, poi, config, bPositiveMoment, pcmd);
}

std::vector<Float64> CEngAgentImp::GetMinMomentCapacity(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const
{
   return m_pMomentCapacityEngineer->GetMinMomentCapacity(intervalIdx, vPoi, bPositiveMoment);
}

Float64 CEngAgentImp::GetMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const
{
   return m_pMomentCapacityEngineer->GetMinMomentCapacity(intervalIdx, poi, bPositiveMoment);
}

const MINMOMENTCAPDETAILS* CEngAgentImp::GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment) const
{
   return m_pMomentCapacityEngineer->GetMinMomentCapacityDetails(intervalIdx, poi, bPositiveMoment);
}

void CEngAgentImp::GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd) const
{
   m_pMomentCapacityEngineer->GetMinMomentCapacityDetails(intervalIdx, poi, config, bPositiveMoment, pmmcd);
}

std::vector<const MINMOMENTCAPDETAILS*> CEngAgentImp::GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const
{
   return m_pMomentCapacityEngineer->GetMinMomentCapacityDetails(intervalIdx, vPoi, bPositiveMoment);
}

std::vector<const CRACKINGMOMENTDETAILS*> CEngAgentImp::GetCrackingMomentDetails(IntervalIndexType intervalIdx,const PoiList& vPoi,bool bPositiveMoment) const
{
   return m_pMomentCapacityEngineer->GetCrackingMomentDetails(intervalIdx, vPoi, bPositiveMoment);
}

/////////////////////////////////////////////////////////////////////////////
// IShearCapacity
pgsTypes::FaceType CEngAgentImp::GetFlexuralTensionSide(pgsTypes::LimitState limitState,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const
{
   // determine the "flexural tension side". See LRFD Figure C5.7.3.4.2-2 (pre2017: 5.8.3.4.2-2)

   GET_IFACE(ISpecification,pSpec);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE(ILimitStateForces,pLsForces);

   Float64 Mu_max, Mu_min;

   if ( analysisType == pgsTypes::Envelope )
   {
      Float64 Mmin,Mmax;

      pLsForces->GetMoment( intervalIdx, limitState, poi, pgsTypes::MaxSimpleContinuousEnvelope, &Mmin, &Mmax );
      Mu_max = Mmax;

      pLsForces->GetMoment( intervalIdx, limitState, poi, pgsTypes::MinSimpleContinuousEnvelope, &Mmin, &Mmax );
      Mu_min = Mmin;
   }
   else
   {
      pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
      pLsForces->GetMoment( intervalIdx, limitState, poi, bat, &Mu_min, &Mu_max );
   }

   Mu_max = IsZero(Mu_max) ? 0 : Mu_max;
   Mu_min = IsZero(Mu_min) ? 0 : Mu_min;

   // Determine if the tension side is on the top half or bottom half of the girder
   // The flexural tension side is on the bottom when the maximum (positive) bending moment
   // exceeds the minimum (negative) bending moment
   pgsTypes::FaceType tensionSide = (fabs(Mu_min) <= fabs(Mu_max) ? pgsTypes::BottomFace : pgsTypes::TopFace);
   return tensionSide;
}

Float64 CEngAgentImp::GetShearCapacity(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig) const
{
   SHEARCAPACITYDETAILS scd;
   GetShearCapacityDetails( limitState,intervalIdx,poi,pConfig,&scd );
   return scd.pVn; 
}

std::vector<Float64> CEngAgentImp::GetShearCapacity(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const PoiList& vPoi) const
{
   std::vector<Float64> Vn;
   Vn.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      Vn.push_back(GetShearCapacity(limitState,intervalIdx,poi));
   }

   return Vn;
}

void CEngAgentImp::GetShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,SHEARCAPACITYDETAILS* pscd) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   const CGirderKey& girderKey(segmentKey);
   
   GetRawShearCapacityDetails(limitState, intervalIdx, poi, pConfig, pscd);

   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   pgsTypes::ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   if ( shear_capacity_method == pgsTypes::scmBTEquations || shear_capacity_method == pgsTypes::scmWSDOT2007 )
   {
      ZoneIndexType csZoneIdx = GetCriticalSectionZoneIndex(limitState,poi);
      if ( csZoneIdx != INVALID_INDEX )
      {
         GET_IFACE(IPointOfInterest, pPOI);

         PoiList vCSPoi;
         pPOI->GetCriticalSections(limitState, girderKey, &vCSPoi);
         ATLASSERT(0 < vCSPoi.size());

         const pgsPointOfInterest& csPoi(vCSPoi[csZoneIdx]);
         ATLASSERT(csPoi.HasAttribute(POI_CRITSECTSHEAR1 | POI_CRITSECTSHEAR2));

         SHEARCAPACITYDETAILS cs_scd;
         GetRawShearCapacityDetails(limitState,intervalIdx,csPoi,pConfig,&cs_scd);
         m_ShearCapEngineer.TweakShearCapacityOutboardOfCriticalSection(csPoi,pscd,&cs_scd);
      }
   }
}

void CEngAgentImp::GetRawShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, SHEARCAPACITYDETAILS* pscd) const
{
#if defined _DEBUG
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   ATLASSERT(liveLoadIntervalIdx <= intervalIdx); // shear is only evaluated when live load is present
#endif

   if (pConfig)
   {
      m_ShearCapEngineer.ComputeShearCapacity(intervalIdx, limitState, poi, pConfig, pscd);
   }
   else
   {
      *pscd = *ValidateShearCapacity(limitState, intervalIdx, poi);
   }
}

Float64 CEngAgentImp::GetFpc(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const
{
   FPCDETAILS fd;
   GetFpcDetails(poi, pConfig, &fd);
   return fd.fpc;
}

void CEngAgentImp::GetFpcDetails(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, FPCDETAILS* pfpc) const
{
   if (pConfig)
   {
      m_ShearCapEngineer.ComputeFpc(poi, pConfig, pfpc);
   }
   else
   {
      *pfpc = *ValidateFpc(poi);
   }
}

ZoneIndexType CEngAgentImp::GetCriticalSectionZoneIndex(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi) const
{
   const auto& vCSDetails(ValidateShearCritSection(limitState,poi.GetSegmentKey()));
   Float64 x = poi.GetDistFromStart();
 
   auto& iter = std::cbegin(vCSDetails);
   auto& end = std::cend(vCSDetails);
   for ( ; iter != end; iter++)
   {
      const auto& csDetails(*iter);
      if ( csDetails.bAtFaceOfSupport )
      {
         if ( csDetails.poiFaceOfSupport.GetSegmentKey() == poi.GetSegmentKey() && ::InRange(csDetails.Start,x,csDetails.End) )
         {
            return (ZoneIndexType)std::distance(std::begin(vCSDetails), iter);
         }
      }
      else 
      {
         if ( csDetails.pCriticalSection->Poi.GetSegmentKey() == poi.GetSegmentKey() && ::InRange(csDetails.Start,x,csDetails.End) )
         {
            return (ZoneIndexType)std::distance(std::begin(vCSDetails), iter);
         }
      }
   }

   return INVALID_INDEX;
}

void CEngAgentImp::GetCriticalSectionZoneBoundary(pgsTypes::LimitState limitState,const CGirderKey& girderKey,ZoneIndexType csZoneIdx,Float64* pStart,Float64* pEnd) const
{
   const std::vector<CRITSECTDETAILS>& vCSDetails(ValidateShearCritSection(limitState,girderKey));
   const CRITSECTDETAILS& csDetails = vCSDetails[csZoneIdx];

   // start and end of the critical seciton are measured in POI coordinates. 
   // this method is supposed to return them in girder coordinates.
   // do the coordinate coversion.
   GET_IFACE(IPointOfInterest,pPoi);
   const pgsPointOfInterest& csPoi = csDetails.GetPointOfInterest();

   *pStart = pPoi->ConvertPoiToGirderCoordinate(pgsPointOfInterest(csPoi.GetSegmentKey(),csDetails.Start));
   *pEnd   = pPoi->ConvertPoiToGirderCoordinate(pgsPointOfInterest(csPoi.GetSegmentKey(),csDetails.End));
}

std::vector<Float64> CEngAgentImp::GetCriticalSections(pgsTypes::LimitState limitState, const CGirderKey& girderKey, const GDRCONFIG* pConfig) const
{
   const std::vector<CRITSECTDETAILS>& csDetails(GetCriticalSectionDetails(limitState, girderKey, pConfig));
   return GetCriticalSectionFromDetails(csDetails);
}

std::vector<Float64> CEngAgentImp::GetCriticalSectionFromDetails(const std::vector<CRITSECTDETAILS>& vCSDetails) const
{
   std::vector<Float64> csLoc;

   GET_IFACE(IPointOfInterest,pPoi);

   for( const auto& csDetails : vCSDetails)
   {
      const pgsPointOfInterest& csPoi = csDetails.GetPointOfInterest();
      Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(csPoi);
      csLoc.push_back(Xg);
   }

   return csLoc;
}

const std::vector<CRITSECTDETAILS>& CEngAgentImp::GetCriticalSectionDetails(pgsTypes::LimitState limitState,const CGirderKey& girderKey,const GDRCONFIG* pConfig) const
{
   return ValidateShearCritSection( limitState, girderKey, pConfig );
}

std::vector<SHEARCAPACITYDETAILS> CEngAgentImp::GetShearCapacityDetails(pgsTypes::LimitState limitState, IntervalIndexType intervalIdx,const PoiList& vPoi) const
{
   std::vector<SHEARCAPACITYDETAILS> details;
   details.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      SHEARCAPACITYDETAILS scd;
      GetShearCapacityDetails(limitState,intervalIdx,poi,nullptr,&scd);
      details.push_back(scd);
   }

   return details;
}

void CEngAgentImp::ClearDesignCriticalSections() const
{
   CollectionIndexType size = sizeof(m_DesignCritSectionDetails)/sizeof(std::map<CGirderKey,std::vector<CRITSECTDETAILS>>);
   for (CollectionIndexType idx = 0; idx < size; idx++ )
   {
      m_DesignCritSectionDetails[idx].clear();
   }
}

/////////////////////////////////////////////////////////////////////////////
// IPrincipalWebStress
void CEngAgentImp::GetPrincipalWebStressPointsOfInterest(const CSegmentKey& segmentKey, IntervalIndexType interval, PoiList* pPoiList) const
{
   m_Designer.GetPrincipalWebStressPointsOfInterest(segmentKey, interval, pPoiList);
}

const PRINCIPALSTRESSINWEBDETAILS* CEngAgentImp::GetPrincipalWebStressDetails(const pgsPointOfInterest& poi) const
{
   return m_pPrincipalWebStressEngineer->GetPrincipalStressInWeb(poi);
}

const std::vector<TimeStepCombinedPrincipalWebStressDetailsAtWebSection>* CEngAgentImp::GetTimeStepPrincipalWebStressDetails(const pgsPointOfInterest& poi, IntervalIndexType interval) const
{
   return m_pPrincipalWebStressEngineer->GetTimeStepPrincipalWebStressDetails(poi, interval);
}

/////////////////////////////////////////////////////////////////////////////
// IGirderHaunch
Float64 CEngAgentImp::GetRequiredSlabOffset(const CSegmentKey& segmentKey) const
{
   const auto& details = GetSlabOffsetDetails(segmentKey);
   Float64 slab_offset_round = details.RequiredMaxSlabOffsetRounded;

   return slab_offset_round;
}

const SLABOFFSETDETAILS& CEngAgentImp::GetSlabOffsetDetails(const CSegmentKey& segmentKey) const
{
   auto found = m_SlabOffsetDetails.find(segmentKey);

   if ( found == m_SlabOffsetDetails.end() )
   {
      // not found
      SLABOFFSETDETAILS details;
      m_Designer.GetSlabOffsetDetails(segmentKey,nullptr,&details);

      auto result = m_SlabOffsetDetails.insert( std::make_pair(segmentKey,details) );
      ATLASSERT(result.second == true);
      found = result.first;
   }

   return (*found).second;
}

Float64 CEngAgentImp::GetSectionGirderOrientationEffect(const pgsPointOfInterest& poi) const
{
   return m_Designer.GetSectionGirderOrientationEffect(poi);
}


/////////////////////////////////////////////////////////////////////////////
// IFabricationOptimization
bool CEngAgentImp::GetFabricationOptimizationDetails(const CSegmentKey& segmentKey,FABRICATIONOPTIMIZATIONDETAILS* pDetails) const
{
   GET_IFACE(ILossParameters, pLossParams);
   if (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP)
   {
      // not doing this for time-step analysis
      return false;
   }

   GET_IFACE(IBridgeDescription, pBridgeDesc);
   if (pBridgeDesc->GetPrecastSegmentData(segmentKey)->Strands.GetStrandDefinitionType() == pgsTypes::sdtDirectStrandInput)
   {
      // Can't do a fab optimization analysis with direct strand input
      return false;
   }

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);

   // there is nothing to do if there aren't any strands
   if ( config.PrestressConfig.GetStrandCount(pgsTypes::Straight) == 0 && config.PrestressConfig.GetStrandCount(pgsTypes::Harped) == 0 )
   {
      return true;
   }

   pDetails->Nt = config.PrestressConfig.GetStrandCount(pgsTypes::Temporary);
   pDetails->Pjack = config.PrestressConfig.Pjack[pgsTypes::Temporary];

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   pDetails->TempStrandUsage = pStrands->GetTemporaryStrandUsage();

   pgsGirderLiftingChecker lifting_checker(m_pBroker,m_StatusGroupID);

   GET_IFACE(IStrandGeometry,pStrandGeom);
   if ( 0 <  pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary) && 0 < pDetails->Nt )
   {
      // there are temp strands

      /////////////////////////////////////////////////////////////
      // lifting analysis
      /////////////////////////////////////////////////////////////

      // pretensioned TTS
      config.PrestressConfig.TempStrandUsage = pgsTypes::ttsPretensioned;

      GET_IFACE(ISegmentLiftingPointsOfInterest,pSegmentLiftingPointsOfInterest);

      HANDLINGCONFIG lift_config;
      lift_config.bIgnoreGirderConfig = false;
      lift_config.GdrConfig = config;
      WBFL::Stability::LiftingCheckArtifact artifact1;
      const WBFL::Stability::LiftingStabilityProblem* pStabilityProblem;
      lifting_checker.DesignLifting(segmentKey,lift_config,pSegmentLiftingPointsOfInterest,&artifact1,&pStabilityProblem,LOGGER);
      pDetails->L[PS_TTS] = lift_config.LeftOverhang;
   
      Float64 fci;

      Float64 fci_comp = artifact1.RequiredFcCompression();
      Float64 fci_tens = artifact1.RequiredFcTensionWithoutRebar();
      Float64 fci_tens_wrebar = artifact1.RequiredFcTensionWithRebar();

      bool minRebarRequired = fci_tens<0;
      fci = Max(fci_tens, fci_comp, fci_tens_wrebar);
      pDetails->Fci[PS_TTS] = fci;

      // without TTS
      config.PrestressConfig.TempStrandUsage = pgsTypes::ttsPretensioned;
      StrandIndexType Nt = config.PrestressConfig.GetStrandCount(pgsTypes::Temporary);
      Float64 Pjt = config.PrestressConfig.Pjack[pgsTypes::Temporary];

      config.PrestressConfig.ClearStrandFill(pgsTypes::Temporary);
      config.PrestressConfig.Pjack[pgsTypes::Temporary] = 0;

      lift_config.GdrConfig = config;
      WBFL::Stability::LiftingCheckArtifact artifact2;
      lifting_checker.DesignLifting(segmentKey,lift_config,pSegmentLiftingPointsOfInterest,&artifact2,&pStabilityProblem,LOGGER);
      pDetails->L[NO_TTS] = lift_config.LeftOverhang;
   
      fci_comp = artifact2.RequiredFcCompression();
      fci_tens = artifact2.RequiredFcTensionWithoutRebar();
      fci_tens_wrebar = artifact2.RequiredFcTensionWithRebar();

      minRebarRequired = fci_tens<0;
      fci = Max(fci_tens, fci_comp, fci_tens_wrebar);
      pDetails->Fci[NO_TTS] = fci;


      // post-tensioned TTS

      // lifting at location for NO_TTS (optional TTS)
      config.PrestressConfig.TempStrandUsage = pgsTypes::ttsPTBeforeLifting;
      config.PrestressConfig.Pjack[pgsTypes::Temporary] = Pjt;
      ConfigStrandFillVector rfillvec = pStrandGeom->ComputeStrandFill(segmentKey, pgsTypes::Temporary, Nt);
      config.PrestressConfig.SetStrandFill(pgsTypes::Temporary, rfillvec);

      lift_config.GdrConfig = config;
      lift_config.LeftOverhang = pDetails->L[NO_TTS];
      lift_config.RightOverhang = pDetails->L[NO_TTS];

      WBFL::Stability::LiftingCheckArtifact artifact3;
      lifting_checker.AnalyzeLifting(segmentKey,lift_config,pSegmentLiftingPointsOfInterest,&artifact3);
      pDetails->L[PT_TTS_OPTIONAL] = lift_config.LeftOverhang;

      fci_comp = artifact3.RequiredFcCompression();
      fci_tens = artifact3.RequiredFcTensionWithoutRebar();
      fci_tens_wrebar = artifact3.RequiredFcTensionWithRebar();

      minRebarRequired = fci_tens < 0 ? true : false;
      fci = Max(fci_tens, fci_comp, fci_tens_wrebar);
      pDetails->Fci[PT_TTS_OPTIONAL] = fci;

      // lifting at location for PS_TTS (required TTS)
      config.PrestressConfig.TempStrandUsage = pgsTypes::ttsPTBeforeLifting;
      config.PrestressConfig.Pjack[pgsTypes::Temporary] = Pjt;
      config.PrestressConfig.SetStrandFill(pgsTypes::Temporary, rfillvec);

      lift_config.GdrConfig = config;
      lift_config.LeftOverhang = pDetails->L[PS_TTS];
      lift_config.RightOverhang = pDetails->L[PS_TTS];

      WBFL::Stability::LiftingCheckArtifact artifact4;
      lifting_checker.AnalyzeLifting(segmentKey,lift_config,pSegmentLiftingPointsOfInterest,&artifact4);
      pDetails->L[PT_TTS_REQUIRED] = lift_config.LeftOverhang;
   
      fci_comp = artifact4.RequiredFcCompression();
      fci_tens = artifact4.RequiredFcTensionWithoutRebar();
      fci_tens_wrebar = artifact4.RequiredFcTensionWithRebar();

      minRebarRequired = fci_tens < 0 ? true : false;
      fci = Max(fci_tens, fci_comp, fci_tens_wrebar);
      pDetails->Fci[PT_TTS_REQUIRED] = fci;
   }

   /////////////////////////////////////////////////////////////
   // form stripping without TTS strength
   /////////////////////////////////////////////////////////////
   GET_IFACE(ILimitStateForces,pLS);
   GET_IFACE(IPretensionStresses,pPS);
   GET_IFACE(IPointOfInterest,pPOI);
   GET_IFACE(IProductForces,pProdForces);
   GET_IFACE(IIntervals,pIntervals);
   
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   PoiList vPoi;
   pPOI->GetPointsOfInterest(segmentKey, &vPoi);
   GDRCONFIG config_WithoutTTS;
   config_WithoutTTS = config;
   config_WithoutTTS.PrestressConfig.Pjack[pgsTypes::Temporary] = 0;
   config_WithoutTTS.PrestressConfig.ClearStrandFill(pgsTypes::Temporary);

   Float64 min_stress_WithoutTTS = DBL_MAX;
   Float64 max_stress_WithoutTTS = -DBL_MAX;
   Float64 min_stress_WithTTS = DBL_MAX;
   Float64 max_stress_WithTTS = -DBL_MAX;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      Float64 fTopLimitStateMin,fTopLimitStateMax;
      pLS->GetStress(releaseIntervalIdx,pgsTypes::ServiceI,poi,bat,false,pgsTypes::TopGirder,&fTopLimitStateMin,&fTopLimitStateMax);

      Float64 fBotLimitStateMin,fBotLimitStateMax;
      pLS->GetStress(releaseIntervalIdx,pgsTypes::ServiceI,poi,bat,false,pgsTypes::BottomGirder,&fBotLimitStateMin,&fBotLimitStateMax);

      Float64 fTopPre_WithoutTTS = pPS->GetDesignStress(releaseIntervalIdx,poi,pgsTypes::TopGirder,config_WithoutTTS,false, pgsTypes::ServiceI);
      Float64 fBotPre_WithoutTTS = pPS->GetDesignStress(releaseIntervalIdx,poi,pgsTypes::BottomGirder,config_WithoutTTS,false, pgsTypes::ServiceI);

      Float64 fTopMin_WithoutTTS = fTopLimitStateMin + fTopPre_WithoutTTS;
      Float64 fTopMax_WithoutTTS = fTopLimitStateMax + fTopPre_WithoutTTS;

      Float64 fBotMin_WithoutTTS = fBotLimitStateMin + fBotPre_WithoutTTS;
      Float64 fBotMax_WithoutTTS = fBotLimitStateMax + fBotPre_WithoutTTS;

      min_stress_WithoutTTS = Min(fBotMin_WithoutTTS,fTopMin_WithoutTTS,min_stress_WithoutTTS);
      max_stress_WithoutTTS = Max(fBotMax_WithoutTTS,fTopMax_WithoutTTS,max_stress_WithoutTTS);
   }

   GET_IFACE(IAllowableConcreteStress,pAllowStress);
   pgsPointOfInterest dummyPOI(segmentKey,0.0);
   Float64 c = -pAllowStress->GetAllowableCompressionStressCoefficient(dummyPOI,pgsTypes::TopGirder,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression));
   Float64 t, fmax;
   bool bfMax;
   pAllowStress->GetAllowableTensionStressCoefficient(dummyPOI,pgsTypes::TopGirder,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),false/*without rebar*/,false,&t,&bfMax,&fmax);

   Float64 fc_reqd_compression = min_stress_WithoutTTS/c;
   Float64 fc_reqd_tension = 0;
   if ( 0 < max_stress_WithoutTTS )
   {
      if (0 < t)
      {
         fc_reqd_tension = pow(max_stress_WithoutTTS/t,2);

         if ( bfMax && fmax < max_stress_WithoutTTS) 
         {
            // allowable stress is limited to value lower than needed
            // look at the with rebar case
            bool bCheckMaxAlt;
            Float64 fMaxAlt;
            Float64 talt;
            pAllowStress->GetAllowableTensionStressCoefficient(dummyPOI,pgsTypes::TopGirder,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),true/*with rebar*/,false/*in other than precompressed tensile zone*/,&talt,&bCheckMaxAlt,&fMaxAlt);
            fc_reqd_tension = pow(max_stress_WithoutTTS/talt,2);
         }
      }
   }

   if ( fc_reqd_tension < 0 )
   {
      pDetails->Fci_FormStripping_WithoutTTS = -1;
   }
   else
   {
      pDetails->Fci_FormStripping_WithoutTTS = Max(fc_reqd_tension,fc_reqd_compression);
   }

   /////////////////////////////////////////////////////////////
   // Shipping with equal cantilevers
   /////////////////////////////////////////////////////////////
   GET_IFACE(ISegmentHaulingPointsOfInterest,pSegmentHaulingPointsOfInterest);

   // Use factory to create appropriate hauling checker
   pgsGirderHandlingChecker checker_factory(m_pBroker,m_StatusGroupID);
   std::unique_ptr<pgsGirderHaulingChecker> hauling_checker( checker_factory.CreateGirderHaulingChecker() );

   config = pBridge->GetSegmentConfiguration(segmentKey);
   config.PrestressConfig.TempStrandUsage = pgsTypes::ttsPretensioned;

   HANDLINGCONFIG haulConfig;
   haulConfig.bIgnoreGirderConfig = false;
   haulConfig.GdrConfig = config;
   bool bResult;
   std::unique_ptr<pgsHaulingAnalysisArtifact> hauling_artifact_base ( hauling_checker->DesignHauling(segmentKey,haulConfig,true,pSegmentHaulingPointsOfInterest,&bResult,LOGGER));

   // Constructibility is wsdot-based. Cast artifact
   pgsWsdotHaulingAnalysisArtifact* hauling_artifact = dynamic_cast<pgsWsdotHaulingAnalysisArtifact*>(hauling_artifact_base.get());
   if (hauling_artifact == nullptr)
   {
      ATLASSERT(false); // Should check that hauling analysis is WSDOT before we get here
      return false;
   }

   if ( !bResult )
   {
      pDetails->bTempStrandsRequiredForShipping = true;
      return true;
   }

   GET_IFACE(IMaterials,pMaterial);
   Float64 fcMax = pMaterial->GetSegmentDesignFc(segmentKey,haulSegmentIntervalIdx);

   Float64 fcReqd = -1;

   ATLASSERT( IsEqual(haulConfig.LeftOverhang,haulConfig.RightOverhang) );

   GET_IFACE(ISegmentHaulingSpecCriteria,pCriteria);
   Float64 min_location = Max(pCriteria->GetMinimumHaulingSupportLocation(segmentKey,pgsTypes::metStart),
                              pCriteria->GetMinimumHaulingSupportLocation(segmentKey,pgsTypes::metEnd));

   Float64 location_accuracy = pCriteria->GetHaulingSupportLocationAccuracy();

   bool bDone = false;
   bool bUsingBigInc = true;
   Float64 bigInc = 4*location_accuracy;
   Float64 smallInc = location_accuracy;
   Float64 inc = bigInc;
   Float64 L = haulConfig.RightOverhang;
   while ( !bDone )
   {
      L += inc;

      HANDLINGCONFIG hauling_config;
      hauling_config.bIgnoreGirderConfig = false;
      hauling_config.GdrConfig = config;
      hauling_config.LeftOverhang = L;
      hauling_config.RightOverhang = L;

      std::unique_ptr<pgsHaulingAnalysisArtifact> hauling_artifact2( hauling_checker->AnalyzeHauling(segmentKey,hauling_config,pSegmentHaulingPointsOfInterest) );
   
      Float64 fc;
      Float64 fc_tens1, fc_comp1, fc_tens_wrebar1;
      hauling_artifact2->GetRequiredConcreteStrength(pgsTypes::CrownSlope, &fc_comp1, &fc_tens1, &fc_tens_wrebar1);
      Float64 fc_tens2, fc_comp2, fc_tens_wrebar2;
      hauling_artifact2->GetRequiredConcreteStrength(pgsTypes::Superelevation, &fc_comp2, &fc_tens2, &fc_tens_wrebar2);

      Float64 fc_tens = Max(fc_tens1, fc_tens2);
      Float64 fc_comp = Max(fc_comp1, fc_comp2);
      Float64 fc_tens_wrebar = Max(fc_tens_wrebar1, fc_tens_wrebar2);

      bool minRebarRequired = fc_tens < 0;
      fc = Max(fc_tens, fc_comp, fc_tens_wrebar);

      if ( fcMax < fc )
      {
         // went too far
         L -= inc; // back up

         if ( !bUsingBigInc )
         {
            // using the small increment... done
            bDone = true;
         }
         else
         {
            // jump to smaller increment and keep going
            bUsingBigInc = false;
            inc = smallInc;
         }
      }

      fcReqd = Max(fc,fcReqd);
   }

   pDetails->Lmin = haulConfig.RightOverhang;
   pDetails->Lmax = L;

   /////////////////////////////////////////////////////////////
   // Shipping with unequal cantilevers
   /////////////////////////////////////////////////////////////
   Float64 FScrMin = pCriteria->GetHaulingCrackingFs();
   Float64 FSrMin  = pCriteria->GetHaulingRolloverFs();

   Float64 overhang_sum = 2*pDetails->Lmin;
   bDone = false;
   bUsingBigInc = true;
   inc = bigInc;
   Float64 leading_overhang = min_location;
   Float64 trailing_overhang = -99999;
   while ( !bDone )
   {
      trailing_overhang = overhang_sum - leading_overhang;

      HANDLINGCONFIG hauling_config;
      hauling_config.bIgnoreGirderConfig = false;
      hauling_config.GdrConfig = config;
      hauling_config.LeftOverhang = trailing_overhang;
      hauling_config.RightOverhang = leading_overhang;

      std::unique_ptr<pgsHaulingAnalysisArtifact> hauling_artifact2_base( hauling_checker->AnalyzeHauling(segmentKey,hauling_config,pSegmentHaulingPointsOfInterest) );

      pgsWsdotHaulingAnalysisArtifact* hauling_artifact2 = dynamic_cast<pgsWsdotHaulingAnalysisArtifact*>(hauling_artifact2_base.get());
      if (hauling_artifact2==nullptr)
      {
         ATLASSERT(false); // Should check that hauling analysis is WSDOT before we get here
         return false;
      }

      Float64 fc;
      Float64 fc_tens1, fc_comp1, fc_tens_wrebar1;
      hauling_artifact2->GetRequiredConcreteStrength(pgsTypes::CrownSlope, &fc_comp1, &fc_tens1, &fc_tens_wrebar1);
      Float64 fc_tens2, fc_comp2, fc_tens_wrebar2;
      hauling_artifact2->GetRequiredConcreteStrength(pgsTypes::Superelevation, &fc_comp2, &fc_tens2, &fc_tens_wrebar2);

      Float64 fc_tens = Max(fc_tens1, fc_tens2);
      Float64 fc_comp = Max(fc_comp1, fc_comp2);
      Float64 fc_tens_wrebar = Max(fc_tens_wrebar1, fc_tens_wrebar2);

      bool minRebarRequired = fc_tens < 0;
      fc = Max(fc_tens, fc_comp, fc_tens_wrebar);

      // check factors of safety
      Float64 FScr = Min(hauling_artifact2->GetMinFsForCracking(pgsTypes::CrownSlope),hauling_artifact2->GetMinFsForCracking(pgsTypes::Superelevation));
      Float64 FSr  = Min(hauling_artifact2->GetFsRollover(pgsTypes::CrownSlope),hauling_artifact2->GetFsRollover(pgsTypes::Superelevation));

      bool bFS = ( FSrMin <= FSr && FScrMin <= FScr );

      // check concrete stress
      bool bFc = (fc <= fcMax );


      if ( bFS && bFc )
      {
         // girder is stable and stresses are fine

         if ( bUsingBigInc )
         {
            // the big increment is being used, so back up and
            // then keep going with the smaller increment
            leading_overhang -= inc;
            bUsingBigInc = false;
            inc = smallInc;
         }
         else
         {
            bDone = true;
         }
      }
      else if ( !bFS || !bFc )
      {
         // girder is not stable or stresses are to high
         // shift the overhangs
         leading_overhang += inc;
      }

      if ( pDetails->Lmin < leading_overhang )
      {
         // there isn't an unequal combination that is going to work
         // set the results to match the min overhang case and quit
         leading_overhang  = pDetails->Lmin;
         trailing_overhang = pDetails->Lmin;
         bDone = true;
      }

      fcReqd = Max(fc,fcReqd);
   }

   pDetails->LUmin = leading_overhang;
   pDetails->LUmax = trailing_overhang;
   pDetails->LUsum = overhang_sum;

   pDetails->Fc = fcReqd;

   return true;
}

/////////////////////////////////////////////////////////////////////////////
// IArtifact
const pgsGirderArtifact* CEngAgentImp::GetGirderArtifact(const CGirderKey& girderKey) const
{
   ValidateArtifacts(girderKey);
   return m_Designer.GetGirderArtifact(girderKey);
}

const pgsSegmentArtifact* CEngAgentImp::GetSegmentArtifact(const CSegmentKey& segmentKey) const
{
   const pgsGirderArtifact* pArtifact = GetGirderArtifact(segmentKey);
   return pArtifact->GetSegmentArtifact(segmentKey.segmentIndex);
}

const WBFL::Stability::LiftingCheckArtifact* CEngAgentImp::GetLiftingCheckArtifact(const CSegmentKey& segmentKey) const
{
   return m_Designer.CheckLifting(segmentKey);
}

const pgsHaulingAnalysisArtifact* CEngAgentImp::GetHaulingAnalysisArtifact(const CSegmentKey& segmentKey) const
{
   return m_Designer.CheckHauling(segmentKey);
}

const pgsRatingArtifact* CEngAgentImp::GetRatingArtifact(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const
{
   ValidateRatingArtifacts(girderKey,ratingType,vehicleIdx);
   return FindRatingArtifact(girderKey,ratingType,vehicleIdx);
}

std::shared_ptr<const pgsISummaryRatingArtifact> CEngAgentImp::GetSummaryRatingArtifact(const std::vector<CGirderKey>& girderKeys, pgsTypes::LoadRatingType ratingType, VehicleIndexType vehicleIdx) const
{
   std::shared_ptr<const pgsISummaryRatingArtifact> ptr(std::make_shared<pgsSummaryRatingArtifactImpl>(girderKeys, ratingType, vehicleIdx, m_pBroker, this));

   return ptr;
}

const pgsGirderDesignArtifact* CEngAgentImp::CreateDesignArtifact(const CGirderKey& girderKey, bool bDesignFlexure, arSlabOffsetDesignType haunchDesignType, arConcreteDesignType concreteDesignType, arShearDesignType shearDesignType) const
{
   if (bDesignFlexure || shearDesignType != sdtNoDesign)
   {
      std::map<CGirderKey, pgsGirderDesignArtifact>::size_type cRemove = m_DesignArtifacts.erase(girderKey);
      ATLASSERT(cRemove == 0 || cRemove == 1);


      // A girder can have multiple design strategies, but only one strategy is needed if we are designing for shear only
      GET_IFACE(ISpecification, pSpecification);
      std::vector<arDesignOptions> designOptions = pSpecification->GetDesignOptions(girderKey); // gets the design options and parameters for the specific type of girder (generally from the Project Criteria and Girder Library Entry)
      if (bDesignFlexure == false && shearDesignType != sdtNoDesign && 1 < designOptions.size())
      {
         designOptions.erase(designOptions.begin() + 1, designOptions.end());
         ATLASSERT(designOptions.size() == 1);
      }

      // Modify the options based on the overrides provided in the function arguements
      for (auto& options : designOptions)
      {
         if (bDesignFlexure)
         {
            if (haunchDesignType != sodDefault)
            {
               options.doDesignSlabOffset = haunchDesignType;
            }
            options.doDesignConcreteStrength = concreteDesignType;
         }
         else
         {
            options.doDesignForFlexure = dtNoDesign;
         }

         options.doDesignForShear = shearDesignType;
      }


      // Tricky: Set to gross sections and prismatic haunch if needed
      GET_IFACE_NOCHECK(IBridge, pBridge);
      GET_IFACE(ISectionProperties, pSectProp);

      bool doTrans = pSectProp->GetSectionPropertiesMode() == pgsTypes::spmTransformed;
      bool doHaunch = pSectProp->GetHaunchAnalysisSectionPropertiesType() == pgsTypes::hspDetailedDescription && IsStructuralDeck(pBridge->GetDeckType());

      // Use the class below to temporarly set transformed and prismatic haunch properties in library and then reset them back
      DesignOverrider overrider(doTrans, doHaunch, m_pBroker);

      pgsGirderDesignArtifact gdrDesignArtifact = m_Designer.Design(girderKey, designOptions);

      // NOTE: girder artifact should have an overall outcome for the design of all segments
      // we want to know if design was cancelled... all segment artifacts will have the DesignCancelled
      // outcome so we just have to check the first artifact
      SegmentIndexType segIdx = 0;
      pgsSegmentDesignArtifact* pSegmentDesignArtifact = gdrDesignArtifact.GetSegmentDesignArtifact(segIdx);
      if (pSegmentDesignArtifact->GetOutcome() == pgsSegmentDesignArtifact::DesignCancelled)
      {
         return nullptr;
      }

      // Write notes if section properties were tweaked for design
      if (doTrans)
      {
         pSegmentDesignArtifact->AddDesignNote(pgsSegmentDesignArtifact::dnTransformedSectionsSetToGross);
      }

      if (doHaunch)
      {
         pSegmentDesignArtifact->AddDesignNote(pgsSegmentDesignArtifact::dnParabolicHaunchSetToConstant);
      }

      auto retval = m_DesignArtifacts.insert(std::make_pair(girderKey, gdrDesignArtifact));
      return &((*retval.first).second);
   }
   else
   {
      ATLASSERT(false); // design wasn't specified (bDesignFlexure and bDesignShear are both false)
      return nullptr;
   }
}

const pgsGirderDesignArtifact* CEngAgentImp::GetDesignArtifact(const CGirderKey& girderKey) const
{
   auto found = m_DesignArtifacts.find(girderKey);
   if ( found == m_DesignArtifacts.end() )
   {
      return nullptr;
   }

   return &((*found).second);
}

void CEngAgentImp::CreateLiftingCheckArtifact(const CSegmentKey& segmentKey,Float64 supportLoc,WBFL::Stability::LiftingCheckArtifact* pArtifact) const
{
   bool bCreate = false;

   typedef std::map<CSegmentKey, std::map<Float64,WBFL::Stability::LiftingCheckArtifact,Float64_less> >::iterator iter_type;
   iter_type found_gdr;
   found_gdr = m_LiftingArtifacts.find(segmentKey);
   if ( found_gdr != m_LiftingArtifacts.end() )
   {
      std::map<Float64,WBFL::Stability::LiftingCheckArtifact,Float64_less>::iterator found;
      found = (*found_gdr).second.find(supportLoc);
      if ( found != (*found_gdr).second.end() )
      {
         *pArtifact = (*found).second;
      }
      else
      {
         bCreate = true;
      }
   }
   else
   {
      std::map<Float64,WBFL::Stability::LiftingCheckArtifact,Float64_less> artifacts;
      std::pair<iter_type,bool> iter = m_LiftingArtifacts.insert( std::make_pair(segmentKey, artifacts) );
      found_gdr = iter.first;
      bCreate = true;
   }


   if ( bCreate )
   {
      HANDLINGCONFIG config;

      config.bIgnoreGirderConfig = true;
      config.LeftOverhang  = supportLoc;
      config.RightOverhang = supportLoc;

      pgsGirderLiftingChecker checker(m_pBroker,m_StatusGroupID);
      GET_IFACE(ISegmentLiftingPointsOfInterest,pSegmentLiftingPointsOfInterest);
      checker.AnalyzeLifting(segmentKey,config,pSegmentLiftingPointsOfInterest,pArtifact);

      (*found_gdr).second.insert( std::make_pair(supportLoc,*pArtifact) );
   }
}

const pgsHaulingAnalysisArtifact* CEngAgentImp::CreateHaulingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 leftSupportLoc,Float64 rightSupportLoc) const
{
   const pgsHaulingAnalysisArtifact* pArtifact(nullptr);

   bool bCreate = false;

   typedef std::map<CSegmentKey, std::map<Float64,std::shared_ptr<pgsHaulingAnalysisArtifact>,Float64_less> >::iterator iter_type;
   iter_type found_gdr;
   found_gdr = m_HaulingArtifacts.find(segmentKey);
   if ( found_gdr != m_HaulingArtifacts.end() )
   {
      std::map<Float64,std::shared_ptr<pgsHaulingAnalysisArtifact>,Float64_less>::iterator found;
      found = (*found_gdr).second.find(leftSupportLoc);
      if ( found != (*found_gdr).second.end() )
      {
         pArtifact = (*found).second.get();
      }
      else
      {
         bCreate = true;
      }
   }
   else
   {
      std::map<Float64,std::shared_ptr<pgsHaulingAnalysisArtifact>,Float64_less> artifacts;
      std::pair<iter_type,bool> iter = m_HaulingArtifacts.insert( std::make_pair(segmentKey, artifacts) );
      found_gdr = iter.first;
      bCreate = true;
   }

   if ( bCreate )
   {
      HANDLINGCONFIG config;
      GET_IFACE(ISegmentHaulingPointsOfInterest,pSegmentHaulingPointsOfInterest);

      config.bIgnoreGirderConfig = true;
      config.LeftOverhang  = leftSupportLoc;
      config.RightOverhang = rightSupportLoc;

      // Use factory to create appropriate hauling checker
      pgsGirderHandlingChecker checker_factory(m_pBroker,m_StatusGroupID);
      std::unique_ptr<pgsGirderHaulingChecker> hauling_checker( checker_factory.CreateGirderHaulingChecker() );

      std::shared_ptr<pgsHaulingAnalysisArtifact> my_art (hauling_checker->AnalyzeHauling(segmentKey,config,pSegmentHaulingPointsOfInterest));

      // Get const, uncounted pointer
      pArtifact = my_art.get();

      (*found_gdr).second.insert( std::make_pair(leftSupportLoc,my_art) );
   }

   return pArtifact;
}

/////////////////////////////////////////////////////////////////////////////
// ICrackedSection
Float64 CEngAgentImp::GetIcr(const pgsPointOfInterest& poi, bool bPositiveMoment) const
{
   return m_pMomentCapacityEngineer->GetIcr(poi, bPositiveMoment);
}

const CRACKEDSECTIONDETAILS* CEngAgentImp::GetCrackedSectionDetails(const pgsPointOfInterest& poi, bool bPositiveMoment) const
{
   return m_pMomentCapacityEngineer->GetCrackedSectionDetails(poi, bPositiveMoment);
}

std::vector<const CRACKEDSECTIONDETAILS*> CEngAgentImp::GetCrackedSectionDetails(const PoiList& vPoi, bool bPositiveMoment) const
{
   return m_pMomentCapacityEngineer->GetCrackedSectionDetails(vPoi, bPositiveMoment);
}

/////////////////////////////////////////////////////////////////////////////
// IBridgeDescriptionEventSink
HRESULT CEngAgentImp::OnBridgeChanged(CBridgeChangedHint* pHint)
{
   LOG(_T("OnBridgeChanged Event Received"));
   InvalidateAll();
   return S_OK;
}

HRESULT CEngAgentImp::OnGirderFamilyChanged()
{
   LOG(_T("OnGirderFamilyChanged Event Received"));
   InvalidateAll();
   return S_OK;
}

HRESULT CEngAgentImp::OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint)
{
   LOG(_T("OnGirderChanged Event Received"));
   InvalidateAll();
   return S_OK;
}

HRESULT CEngAgentImp::OnLiveLoadChanged()
{
   LOG(_T("OnLiveLoadChanged Event Received"));
   InvalidateAll();
//   InvalidateArtifacts();
//   InvalidateShearCapacity();
//   InvalidateFpc();
//   InvalidateShearCritSection();
   return S_OK;
}

HRESULT CEngAgentImp::OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName)
{
   LOG(_T("OnLiveLoadNameChanged Event Received"));
   return S_OK;
}

HRESULT CEngAgentImp::OnConstructionLoadChanged()
{
   InvalidateAll();
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ISpecificationEventSink
HRESULT CEngAgentImp::OnSpecificationChanged()
{
   LOG(_T("OnSpecificationChanged Event Received"));
   InvalidateAll();

   return S_OK;
}

HRESULT CEngAgentImp::OnAnalysisTypeChanged()
{
   InvalidateAll();
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IRatingSpecificationEventSink
HRESULT CEngAgentImp::OnRatingSpecificationChanged()
{
   LOG(_T("OnRatingSpecificationChanged Event Received"));
   InvalidateRatingArtifacts();

   // invalidate shear capacities and critical sections associated with rating limit states
   for (int i = 0; i < (int)pgsTypes::LimitStateCount; i++)
   {
      pgsTypes::LimitState limitState = (pgsTypes::LimitState)i;
      if (IsRatingLimitState(limitState) && IsStrengthLimitState(limitState) )
      {
         IndexType idx = LimitStateToShearIndex(limitState);
         m_ShearCapacity[idx].clear();
         m_CritSectionDetails[idx].clear();
      }
   }

   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ILoadModifiersEventSink
HRESULT CEngAgentImp::OnLoadModifiersChanged()
{
   // Invalidate everything that depends on limit state results
   InvalidateHaunch();
   InvalidateArtifacts();
   InvalidateShearCapacity();
   InvalidateFpc();
   InvalidateShearCritSection();

   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IEnvironmentEventSink
HRESULT CEngAgentImp::OnExposureConditionChanged()
{
   InvalidateAll();
   return S_OK;
}

HRESULT CEngAgentImp::OnRelHumidityChanged()
{
   InvalidateAll();
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ILossParametersEventSink
HRESULT CEngAgentImp::OnLossParametersChanged()
{
   InvalidateAll();
   return S_OK;
}

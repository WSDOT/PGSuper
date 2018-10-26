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

// EngAgentImp.cpp : Implementation of CEngAgentImp
#include "stdafx.h"
#include "EngAgent.h"
#include "EngAgent_i.h"
#include "EngAgentImp.h"
#include "..\PGSuperException.h"

#include "GirderHandlingChecker.h"
#include "GirderLiftingChecker.h"

#include <IFace\BeamFactory.h>
#include <IFace\StatusCenter.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>

#include <PgsExt\StatusItem.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderLabel.h>

#include <PsgLib\TrafficBarrierEntry.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>

#include <PgsExt\ReportStyleHolder.h>

#include <Units\SysUnits.h>

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

#pragma Reminder("UPDATE: Deal with critical sections along a superstructure member")
// OLD PGSUPER - a critical section at each end of a span/girder
// NEW PGSUPER - critical section near both sides of every support along a superstructure member

//-----------------------------------------------------------------------------
Float64 GetHorizPsComponent(IBroker* pBroker, 
                            const pgsPointOfInterest& poi,
                            const GDRCONFIG& config)
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   Float64 ss = pStrandGeometry->GetAvgStrandSlope( poi,
                                                 config.PrestressConfig.GetNStrands(pgsTypes::Harped),
                                                 config.PrestressConfig.EndOffset,
                                                 config.PrestressConfig.HpOffset
                                               );
   Float64 hz = 1.0;

   if (ss < Float64_Max)
      hz = ss/sqrt(1*1 + ss*ss);

   return hz;
}

//-----------------------------------------------------------------------------
Float64 GetHorizPsComponent(IBroker* pBroker, 
                            const pgsPointOfInterest& poi)
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   Float64 ss = pStrandGeometry->GetAvgStrandSlope(poi);

   Float64 hz = 1.0;

   if (ss < Float64_Max)
      hz = ss/sqrt(1*1 + ss*ss);

   return hz;
}

//-----------------------------------------------------------------------------
Float64 GetVertPsComponent(IBroker* pBroker, 
                           const pgsPointOfInterest& poi)
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   Float64 ss = pStrandGeometry->GetAvgStrandSlope(poi);

   Float64 vz = 0.00;
   
   if (ss < Float64_Max)
      vz = 1.0/sqrt(1*1 + ss*ss);

   return vz;
}

//-----------------------------------------------------------------------------
CollectionIndexType LimitStateToShearIndex(pgsTypes::LimitState ls)
{
   CollectionIndexType idx;

   switch (ls)
   {
   case pgsTypes::StrengthI:                         idx = 0;       break;
   case pgsTypes::StrengthII:                        idx = 1;       break;
   case pgsTypes::StrengthI_Inventory:               idx = 2;       break;
   case pgsTypes::StrengthI_Operating:               idx = 3;       break;
   case pgsTypes::StrengthI_LegalRoutine:            idx = 4;       break;
   case pgsTypes::StrengthI_LegalSpecial:            idx = 5;       break;
   case pgsTypes::StrengthII_PermitRoutine:          idx = 6;       break;
   case pgsTypes::StrengthII_PermitSpecial:          idx = 7;       break;
   default:
      ATLASSERT(false); // is there a new limit state type?
      idx = 0;
      break;
   }

   return idx;
}

/////////////////////////////////////////////////////////////////////////////
// CEngAgentImp
CEngAgentImp::CEngAgentImp() :
m_MomentCapEngineer(0,0),
m_ShearCapEngineer(0,0),
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
   InvalidateMomentCapacity();
   InvalidateCrackedSectionDetails();
}

//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateHaunch()
{
   m_HaunchDetails.clear();
}

//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateLosses()
{
   LOG("Invalidating losses");

   m_PsForceEngineer.Invalidate();

#pragma Reminder("UPDATE: need to cache prestress force")
   //m_PsForce.clear(); // if losses are gone, so are forces
}

//-----------------------------------------------------------------------------
void CEngAgentImp::ValidateLiveLoadDistributionFactors(const CGirderKey& girderKey)
{
   if (!m_bAreDistFactorEngineersValidated)
   {
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

      // Create dist factor engineer
      if ( m_pDistFactorEngineer == NULL )
      {
         const CGirderGroupData* pGroup      = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
         const GirderLibraryEntry* pGdrEntry = pGroup->GetGirderLibraryEntry(girderKey.girderIndex);

         CComPtr<IBeamFactory> pFactory;
         pGdrEntry->GetBeamFactory(&pFactory);
         pFactory->CreateDistFactorEngineer(m_pBroker,m_StatusGroupID,NULL,NULL,&m_pDistFactorEngineer);
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
               std::_tstring str(_T("Ranges of Applicability for Load Distribution Factor Equations are to be Ignored."));
               pgsLldfWarningStatusItem* pStatusItem = new pgsLldfWarningStatusItem(m_StatusGroupID,m_scidLldfWarning,str.c_str());
               pStatusCenter->Add(pStatusItem);
            }
            else if (action == roaIgnoreUseLeverRule)
            {
               GET_IFACE(IEAFStatusCenter,pStatusCenter);
               std::_tstring str(_T("The Lever Rule is to be used for all cases where Ranges of Applicability for Load Distribution Factor Equations are Exceeded. Otherwise, factors are computed using the Equations."));
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
   m_CheckArtifacts.clear();

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
void CEngAgentImp::ValidateArtifacts(const CGirderKey& girderKey)
{
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IDocumentType,pDocType);

   std::_tostringstream os;
   if ( pDocType->IsPGSuperDocument() )
   {
      os << "Analyzing Span " << LABEL_SPAN(girderKey.groupIndex) << " Girder " << LABEL_GIRDER(girderKey.girderIndex) << std::ends;
   }
   else
   {
      os << "Analyzing Group " << LABEL_GROUP(girderKey.groupIndex) << " Girder " << LABEL_GIRDER(girderKey.girderIndex) << std::ends;
   }
   pProgress->UpdateMessage( os.str().c_str() );

   std::map<CGirderKey,pgsGirderArtifact>::iterator found;
   found = m_CheckArtifacts.find(girderKey);
   if ( found != m_CheckArtifacts.end() )
      return; // We already have an artifact for this girder


   pgsGirderArtifact gdr_artifact = m_Designer.Check(girderKey);

   m_CheckArtifacts.insert( std::make_pair(girderKey,gdr_artifact) );
}

//-----------------------------------------------------------------------------
void CEngAgentImp::ValidateRatingArtifacts(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIndex)
{
   GET_IFACE(IRatingSpecification,pRatingSpec);
   if ( !pRatingSpec->IsRatingEnabled(ratingType) )
      return; // this type isn't enabled, so leave

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   std::_tostringstream os;
   os << "Load Rating Girder Line " << LABEL_GIRDER(girderKey.girderIndex) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   std::map<RatingArtifactKey,pgsRatingArtifact>::iterator found;
   RatingArtifactKey key(girderKey,vehicleIndex);
   found = m_RatingArtifacts[ratingType].find(key);
   if ( found != m_RatingArtifacts[ratingType].end() )
      return; // We already have an artifact for this girder


   pgsRatingArtifact artifact = m_LoadRater.Rate(girderKey,ratingType,vehicleIndex);

   m_RatingArtifacts[ratingType].insert( std::make_pair(key,artifact) );
}

//-----------------------------------------------------------------------------
const LOSSDETAILS* CEngAgentImp::FindLosses(const pgsPointOfInterest& poi)
{
   return m_PsForceEngineer.GetLosses(poi);
}

//-----------------------------------------------------------------------------
pgsGirderArtifact* CEngAgentImp::FindArtifact(const CGirderKey& girderKey)
{
   std::map<CGirderKey,pgsGirderArtifact>::iterator found;
   found = m_CheckArtifacts.find( girderKey );
   if ( found == m_CheckArtifacts.end() )
       return 0;

   return &(*found).second;
}

pgsRatingArtifact* CEngAgentImp::FindRatingArtifact(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIndex)
{
    std::map<RatingArtifactKey,pgsRatingArtifact>::iterator found;
    RatingArtifactKey key(girderKey,vehicleIndex);
    found = m_RatingArtifacts[ratingType].find( key );
    if ( found == m_RatingArtifacts[ratingType].end() )
        return 0;

    return &(*found).second;
}

//-----------------------------------------------------------------------------
const MINMOMENTCAPDETAILS* CEngAgentImp::ValidateMinMomentCapacity(IntervalIndexType intervalIdx,
                                                                   const pgsPointOfInterest& poi,
                                                                   bool bPositiveMoment)
{
   std::map<PoiIDKey,MINMOMENTCAPDETAILS>::iterator found;
   std::map<PoiIDKey,MINMOMENTCAPDETAILS>* pMap;

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

   if ( poi.GetID() != INVALID_ID )
   {
      pMap = ( intervalIdx < compositeDeckIntervalIdx ) ? &m_NonCompositeMinMomentCapacity[bPositiveMoment]
                                                        : &m_CompositeMinMomentCapacity[bPositiveMoment];

      PoiIDKey key(poi.GetID(),poi);
      found = pMap->find( key );
      if ( found != pMap->end() )
         return &((*found).second); // capacities have already been computed
   }

   MINMOMENTCAPDETAILS mmcd;
   m_MomentCapEngineer.ComputeMinMomentCapacity(intervalIdx,poi,bPositiveMoment,&mmcd);

   PoiIDKey key(poi.GetID(),poi);
   std::pair<std::map<PoiIDKey,MINMOMENTCAPDETAILS>::iterator,bool> retval;
   retval = pMap->insert( std::make_pair(key,mmcd) );
   return &((*(retval.first)).second);
}

//-----------------------------------------------------------------------------
const CRACKINGMOMENTDETAILS* CEngAgentImp::ValidateCrackingMoments(IntervalIndexType intervalIdx,
                                                                   const pgsPointOfInterest& poi,
                                                                   bool bPositiveMoment)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

   std::map<PoiIDKey,CRACKINGMOMENTDETAILS>::iterator found;
   std::map<PoiIDKey,CRACKINGMOMENTDETAILS>* pMap;

   if ( poi.GetID() != INVALID_ID )
   {
      pMap = ( intervalIdx < compositeDeckIntervalIdx ) ? &m_NonCompositeCrackingMoment[bPositiveMoment] 
                                                        : &m_CompositeCrackingMoment[bPositiveMoment];

      PoiIDKey key(poi.GetID(),poi);
      found = pMap->find( key );
      if ( found != pMap->end() )
         return &( (*found).second ); // capacities have already been computed
   }

   CRACKINGMOMENTDETAILS cmd;
   m_MomentCapEngineer.ComputeCrackingMoment(intervalIdx,poi,bPositiveMoment,&cmd);

   PoiIDKey key(poi.GetID(),poi);
   std::pair<std::map<PoiIDKey,CRACKINGMOMENTDETAILS>::iterator,bool> retval;
   retval = pMap->insert( std::make_pair(key,cmd) );
   return &((*(retval.first)).second);
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* CEngAgentImp::ValidateMomentCapacity(IntervalIndexType intervalIdx,
                                                                  const pgsPointOfInterest& poi,
                                                                  bool bPositiveMoment)
{
   MOMENTCAPACITYDETAILS mcd = ComputeMomentCapacity(intervalIdx,poi,bPositiveMoment);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

   return StoreMomentCapacityDetails(intervalIdx,poi,bPositiveMoment,mcd,intervalIdx < compositeDeckIntervalIdx ? m_NonCompositeMomentCapacity[bPositiveMoment] : m_CompositeMomentCapacity[bPositiveMoment]);
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* CEngAgentImp::ValidateMomentCapacity(IntervalIndexType intervalIdx,
                                                                  const pgsPointOfInterest& poi,
                                                                  const GDRCONFIG& config,
                                                                  bool bPositiveMoment)
{
   MOMENTCAPACITYDETAILS mcd = ComputeMomentCapacity(intervalIdx,poi,config,bPositiveMoment);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

   return StoreMomentCapacityDetails(intervalIdx,poi,bPositiveMoment,mcd,intervalIdx < compositeDeckIntervalIdx ? m_TempNonCompositeMomentCapacity[bPositiveMoment] : m_TempCompositeMomentCapacity[bPositiveMoment]);
}

//-----------------------------------------------------------------------------
pgsPointOfInterest CEngAgentImp::GetEquivalentPointOfInterest(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 dist_from_start = poi.GetDistFromStart();

   pgsPointOfInterest search_poi( poi );

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,pGirder);

   // check for symmetry
   if ( pGirder->IsSymmetric(intervalIdx,segmentKey) )
   {
      Float64 girder_length = pBridge->GetSegmentLength(segmentKey);

      if ( girder_length/2 < dist_from_start )
      {
         // we are past mid-point of a symmetric girder
         // get the poi that is a mirror about the centerline of the girder

         GET_IFACE(IPointOfInterest,pPOI);
         Float64 dist_from_end = girder_length - dist_from_start;
         search_poi = pPOI->GetPointOfInterest(segmentKey,dist_from_end);

         if ( search_poi.GetID() == INVALID_ID ) // a symmetric POI was not actually found
            search_poi = poi;
      }
   }

   return search_poi;
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* CEngAgentImp::GetCachedMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

   return GetCachedMomentCapacity(intervalIdx,poi,bPositiveMoment,intervalIdx < compositeDeckIntervalIdx ? m_NonCompositeMomentCapacity[bPositiveMoment] : m_CompositeMomentCapacity[bPositiveMoment]);
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* CEngAgentImp::GetCachedMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment)
{
   // if the stored config is not equal to the requesting config, flush all the cached results
   if ( !config.IsFlexuralDataEqual(m_TempGirderConfig) )
   {
      m_TempNonCompositeMomentCapacity[0].clear();
      m_TempNonCompositeMomentCapacity[1].clear();

      m_TempCompositeMomentCapacity[0].clear();
      m_TempCompositeMomentCapacity[1].clear();

      m_TempGirderConfig = config;

      return NULL;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

   return GetCachedMomentCapacity(intervalIdx,poi,bPositiveMoment,intervalIdx < compositeDeckIntervalIdx ? m_TempNonCompositeMomentCapacity[bPositiveMoment] : m_TempCompositeMomentCapacity[bPositiveMoment]);
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* CEngAgentImp::GetCachedMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,const MomentCapacityDetailsContainer& container)
{
   // if the beam has some symmetry, we can use the results for another poi...
   // get the equivalent, mirrored POI

   // don't do this for negative moment... the symmetry check just isn't working right


   pgsPointOfInterest search_poi( (bPositiveMoment ? GetEquivalentPointOfInterest(intervalIdx,poi) : poi) );

   MomentCapacityDetailsContainer::const_iterator found;

   // if this is a real POI, then see if we've already computed results
   if ( search_poi.GetID() != INVALID_ID )
   {
      PoiIDKey key(search_poi.GetID(),search_poi);
      found = container.find( key );
      if ( found != container.end() )
         return &( (*found).second ); // capacities have already been computed
   }

   return NULL;
}

//-----------------------------------------------------------------------------
MOMENTCAPACITYDETAILS CEngAgentImp::ComputeMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   MOMENTCAPACITYDETAILS mcd;
   m_MomentCapEngineer.ComputeMomentCapacity(intervalIdx,poi,bPositiveMoment,&mcd);

   return mcd;
}

//-----------------------------------------------------------------------------
MOMENTCAPACITYDETAILS CEngAgentImp::ComputeMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment)
{
   MOMENTCAPACITYDETAILS mcd;
   m_MomentCapEngineer.ComputeMomentCapacity(intervalIdx,poi,&config,bPositiveMoment,&mcd);

   return mcd;
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* CEngAgentImp::StoreMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,const MOMENTCAPACITYDETAILS& mcd,MomentCapacityDetailsContainer& container)
{
   // if the beam has some symmetry, we can use the results for another poi...
   // get the equivalent, mirrored POI

   pgsPointOfInterest search_poi( (bPositiveMoment ? GetEquivalentPointOfInterest(intervalIdx,poi) : poi) );

   PoiIDKey key(search_poi.GetID(),search_poi);
   std::pair<MomentCapacityDetailsContainer::iterator,bool> retval;
   retval = container.insert( std::make_pair(key,mcd) );

   // insert failed
   if ( !retval.second )
   {
      // this shouldn't be happening unless we are out of memory or something really bad like that
      // if there is already something stored with 'key' the insert will fail and that is
      // bad because it indicates a bug elsewhere
      ATLASSERT(false); 
      return NULL;
   }


   return &((*(retval.first)).second);
}

//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateMomentCapacity()
{
   LOG(_T("Invalidating moment capacities"));

   for ( int i = 0; i < 2; i++ )
   {
      m_NonCompositeMomentCapacity[i].clear();
      m_CompositeMomentCapacity[i].clear();
      m_NonCompositeCrackingMoment[i].clear();
      m_CompositeCrackingMoment[i].clear();
      m_NonCompositeMinMomentCapacity[i].clear();
      m_CompositeMinMomentCapacity[i].clear();
   }
}


//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateShearCapacity()
{
   LOG(_T("Invalidating shear capacities"));
   CollectionIndexType size = sizeof(m_ShearCapacity)/sizeof(std::map<PoiIDKey,SHEARCAPACITYDETAILS>);
   for (CollectionIndexType idx = 0; idx < size; idx++ )
   {
      m_ShearCapacity[idx].clear();
   }
}

//-----------------------------------------------------------------------------
void CEngAgentImp::InvalidateCrackedSectionDetails()
{
   LOG(_T("Invalidating cracked section details"));

   for ( int i = 0; i < 2; i++ )
   {
      m_CrackedSectionDetails[i].clear();
   }
}

//-----------------------------------------------------------------------------

const SHEARCAPACITYDETAILS* CEngAgentImp::ValidateShearCapacity(pgsTypes::LimitState ls, 
                                                                IntervalIndexType intervalIdx, 
                                                                const pgsPointOfInterest& poi)
{
   std::map<PoiIDKey,SHEARCAPACITYDETAILS>::iterator found;

   CollectionIndexType idx = LimitStateToShearIndex(ls);

   if ( poi.GetID() != INVALID_ID )
   {
      PoiIDKey key(poi.GetID(),poi);
      found = m_ShearCapacity[idx].find( key );

      if ( found != m_ShearCapacity[idx].end() )
         return &( (*found).second ); // capacities have already been computed
   }

   SHEARCAPACITYDETAILS scd;
   m_ShearCapEngineer.ComputeShearCapacity(ls,intervalIdx,poi,&scd);

   PoiIDKey key(poi.GetID(),poi);
   std::pair<std::map<PoiIDKey,SHEARCAPACITYDETAILS>::iterator,bool> retval;
   retval = m_ShearCapacity[idx].insert( std::make_pair(key,scd) );
   return &((*(retval.first)).second);
}

//-----------------------------------------------------------------------------
const FPCDETAILS* CEngAgentImp::ValidateFpc(const pgsPointOfInterest& poi)
{
   std::map<PoiIDKey,FPCDETAILS>::iterator found;

   if ( poi.GetID() != INVALID_ID )
   {
      PoiIDKey key(poi.GetID(),poi);
      found = m_Fpc.find( key );
      if ( found != m_Fpc.end() )
         return &( (*found).second ); // already been computed
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   Float64 slabOffset = pBridge->GetSlabOffset(poi);

   FPCDETAILS mcd;
   m_ShearCapEngineer.ComputeFpc(poi,NULL,&mcd);

   PoiIDKey key(poi.GetID(),poi);
   std::pair<std::map<PoiIDKey,FPCDETAILS>::iterator,bool> retval;
   retval = m_Fpc.insert( std::make_pair(key,mcd) );
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
   CollectionIndexType size = sizeof(m_CritSectionDetails)/sizeof(std::map<CSegmentKey,std::vector<CRITSECTDETAILS>>);
   for (CollectionIndexType idx = 0; idx < size; idx++ )
   {
      m_CritSectionDetails[idx].clear();
   }
}

//-----------------------------------------------------------------------------
const std::vector<CRITSECTDETAILS>& CEngAgentImp::ValidateShearCritSection(pgsTypes::LimitState limitState,const CGirderKey& girderKey)
{
   USES_CONVERSION;
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   // LRFD 2004 and later, critical section is only a function of dv, which comes from the calculation of Mu,
   // so critical section is not a function of the limit state. We will work with the Strength I limit state
   if ( lrfdVersionMgr::ThirdEdition2004 <= pSpecEntry->GetSpecificationType() )
      limitState = pgsTypes::StrengthI;

   std::map<CGirderKey,std::vector<CRITSECTDETAILS>>::iterator found;
   found = m_CritSectionDetails[LimitStateToShearIndex(limitState)].find(girderKey);
   if ( found != m_CritSectionDetails[LimitStateToShearIndex(limitState)].end() )
   {
      // We already have the value for this girder...
      return found->second;
   }

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IEventMap,pEventMap);
   std::_tostringstream os;

   os << _T("Computing ") << OLE2T(pEventMap->GetLimitStateName(limitState)) << _T(" critical section for shear for Girder ")
      << LABEL_GIRDER(girderKey.girderIndex) << std::ends;

   pProgress->UpdateMessage( os.str().c_str() );

   // calculations
   std::vector<CRITSECTDETAILS> vCSD(CalculateShearCritSection(limitState,girderKey));

   std::pair<std::map<CGirderKey,std::vector<CRITSECTDETAILS>>::iterator,bool> retval;
   retval = m_CritSectionDetails[LimitStateToShearIndex(limitState)].insert( std::make_pair(girderKey,vCSD) );
   ATLASSERT(retval.second == true);
   return retval.first->second;
}

//-----------------------------------------------------------------------------
std::vector<CRITSECTDETAILS> CEngAgentImp::CalculateShearCritSection(pgsTypes::LimitState limitState,
                                             const CGirderKey& girderKey)
{
   GDRCONFIG config;
   return CalculateShearCritSection(limitState,girderKey,false,config);
}

//-----------------------------------------------------------------------------
std::vector<CRITSECTDETAILS> CEngAgentImp::CalculateShearCritSection(pgsTypes::LimitState limitState,
                                             const CGirderKey& girderKey,
                                             const GDRCONFIG& config)
{
   return CalculateShearCritSection(limitState,girderKey,true,config);
}

//-----------------------------------------------------------------------------
std::vector<CRITSECTDETAILS> CEngAgentImp::CalculateShearCritSection(pgsTypes::LimitState limitState,
                                             const CGirderKey& girderKey,
                                             bool bUseConfig,
                                             const GDRCONFIG& config)
{
   std::vector<CRITSECTDETAILS> vcsDetails;

   PoiAttributeType attributes = (limitState == pgsTypes::StrengthI ? POI_CRITSECTSHEAR1 : POI_CRITSECTSHEAR2);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bThirdEdition = ( lrfdVersionMgr::ThirdEdition2004 <= pSpecEntry->GetSpecificationType() ? true : false );

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   // Determine how many critical sections the girder will have.
   // Number of critical sections is equal to the number of face of supports

   GET_IFACE(IPointOfInterest, pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS),POI_FACEOFSUPPORT,POIFIND_OR) );

   // if there aren't any face of supports on this segment, leave now
   if ( vPoi.size() == 0 )
      return vcsDetails;

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
   vPiers.push_back(std::make_pair(pPier,pgsTypes::Ahead));

   // intermediate piers
   pPier = pPier->GetNextSpan()->GetNextPier();
   while ( pPier != pGroup->GetPier(pgsTypes::metEnd) )
   {
      vPiers.push_back(std::make_pair(pPier,pgsTypes::Back)); // left FOS
      vPiers.push_back(std::make_pair(pPier,pgsTypes::Ahead)); // right FOS

      pPier = pPier->GetNextSpan()->GetNextPier();
   }

   // Pier at end of group
   vPiers.push_back(std::make_pair(pPier,pgsTypes::Back));

   // should be one pier for each FOS poi
   ATLASSERT(vPoi.size() == vPiers.size());

   GET_IFACE(ILimitStateForces, pLSForces);
   GET_IFACE(IGirder,           pGirder);
   GET_IFACE(IBridge,           pBridge);

   // find the critical section associated with each FOS
   std::vector<pgsPointOfInterest>::iterator fosIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator fosEnd(vPoi.end());
   std::vector<std::pair<const CPierData2*,pgsTypes::PierFaceType>>::iterator pierIter(vPiers.begin());
   for ( ; fosIter != fosEnd; fosIter++, pierIter++ )
   {
      CRITSECTDETAILS csDetails;
      csDetails.pCriticalSection = NULL;
      csDetails.bAtFaceOfSupport = false;
      pgsPointOfInterest poiFaceOfSupport(*fosIter);

      // need to get pier index that goes with this FOS
      const CPierData2* pPier = pierIter->first;
      pgsTypes::PierFaceType face = pierIter->second;
      PierIndexType pierIdx = pPier->GetIndex();

      csDetails.PierIdx = pierIdx;
      csDetails.PierFace = face;

      // get reactions at the pier
      Float64 Rmin,Rmax;
      pLSForces->GetReaction(limitState,liveLoadIntervalIdx,pierIdx,girderKey,bat,true,&Rmin, &Rmax);

      if ( Rmin <= 0 )
      {
         // this is an uplift reaction... use the face of support for the critical section
         csDetails.bAtFaceOfSupport = true;
         csDetails.poiFaceOfSupport = poiFaceOfSupport;
         csDetails.poiFaceOfSupport.SetAttributes(csDetails.poiFaceOfSupport.GetAttributes() | attributes);
         csDetails.Start = poiFaceOfSupport.GetDistFromStart();
         csDetails.End   = csDetails.Start;
         vcsDetails.push_back(csDetails);
         continue;
      }

      // reaction causes compression in the end of the girder... need to location CS 
      // We will look at POI that are with 2.5H from the face of support

      Float64 Hg = pGirder->GetHeight(poiFaceOfSupport);

      // Get points of interest
      // only use POI's within 2.5H from the face of the supports (2.5H on either side)
      Float64 left  = 2.5*Hg;
      Float64 right = 2.5*Hg;
      if ( pPier->IsInteriorPier() )
      {
         if ( face == pgsTypes::Ahead )
            left = 0;

         if ( face == pgsTypes::Back )
            right = 0;
      }

      std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterestInRange(left,poiFaceOfSupport,right) );

      mathPwLinearFunction2dUsingPoints theta;
      mathPwLinearFunction2dUsingPoints dv;
      mathPwLinearFunction2dUsingPoints dv_cos_theta;
      mathPwLinearFunction2dUsingPoints unity;  // 45deg line from face of support

      // create a graph for dv and 0.5d*dv*cot(theta)
      // create intercept lines as well since we are looping on poi.
      LOG(endl<<_T("Critical Section Intercept graph"));
      LOG(_T("Location , Dv, Theta, 0.5*Dv*cotan(theta), Y"));
      std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
      std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
      for ( ; iter != end; iter++ )
      {
         CRITSECTIONDETAILSATPOI csdp;
         csdp.Intersection = CRITSECTIONDETAILSATPOI::NoIntersection;
         const pgsPointOfInterest& poi = *iter;

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
            continue;

         SHEARCAPACITYDETAILS scd;
         if ( bUseConfig )
            GetRawShearCapacityDetails(limitState,liveLoadIntervalIdx,poi,config,&scd);
         else
            GetRawShearCapacityDetails(limitState,liveLoadIntervalIdx,poi,&scd);

         // dv
         dv.AddPoint( gpPoint2d(x, scd.dv) );
         csdp.Poi = poi;
         csdp.DistFromFOS = x;
         csdp.Dv  = scd.dv;

         // 0.5*dv*cot(theta)
         // theta is valid only if shear stress is in range
         if (scd.ShearInRange)
         {
            Float64 dvt   = 0.5*scd.dv/tan(scd.Theta);
            dv_cos_theta.AddPoint( gpPoint2d(x, dvt) );
            theta.AddPoint( gpPoint2d(x, scd.Theta) );

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
         unity.AddPoint( gpPoint2d(x,x) );

         LOG(poi.GetDistFromStart()<<_T(", ")<<csdp.Dv<<_T(", ")<<csdp.Theta<<_T(", ")<<csdp.CotanThetaDv05<<_T(", ")<<x);
      }

      LOG(_T("End of intercept values")<<endl);

      // now that the graphs are created, find the intsection of the unity line with the dv and 0.5dvcot(theta) lines
      // determine intersections
      gpPoint2d p;
      Float64 x1;
      math1dRange range = dv.GetRange();  // range is same for all

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
         csDetails.CsDv.Dv             = dv.Evaluate(x1);
         csDetails.CsDv.InRange        = theta.GetRange().IsInRange(x1);
         if (csDetails.CsDv.InRange)
         {
            csDetails.CsDv.Intersection = CRITSECTIONDETAILSATPOI::DvIntersection;
            csDetails.CsDv.Theta          = theta.Evaluate(x1);
            csDetails.CsDv.CotanThetaDv05 = dv_cos_theta.Evaluate(x1);
         }
         else
         {
            csDetails.CsDv.Intersection = CRITSECTIONDETAILSATPOI::NoIntersection;
            csDetails.CsDv.Theta          = 0.0;
            csDetails.CsDv.CotanThetaDv05 = 0.0;
         }

         LOG(_T("Dv Intersection = (")<<p.X()<<_T(", ")<<p.Y()<<_T(")"));

         // dv cot(theta)
         nIntersections = dv_cos_theta.Intersect(unity,range,&p);
         if( nIntersections == 1 )
         {
            x1 = p.X(); // distance from face of support where the intersection occurs

            csDetails.CsDvt.InRange        = true;
            csDetails.CsDvt.Intersection = CRITSECTIONDETAILSATPOI::ThetaIntersection;

            csDetails.CsDvt.DistFromFOS = x1;

            // set the critical section poi data... need the segment key and the
            // distance from the start of the segment. Distance from start of segment is
            // distance from face of support (x1) + distance from start of segment to the FOS
	         
            csDetails.CsDvt.Poi.SetSegmentKey(poiFaceOfSupport.GetSegmentKey());
            csDetails.CsDvt.Poi.SetDistFromStart(face == pgsTypes::Ahead ? x1 + poiFaceOfSupport.GetDistFromStart() : poiFaceOfSupport.GetDistFromStart() - x1);

            csDetails.CsDvt.Dv             = dv.Evaluate(x1);
            csDetails.CsDvt.Theta          = theta.Evaluate(x1);
            csDetails.CsDvt.CotanThetaDv05 = dv_cos_theta.Evaluate(x1);

            LOG(_T(".5*Dv*cot(theta) Intersection = (")<<p.X()<<_T(", ")<<p.Y()<<_T(")"));
         }
         else
         {
            ATLASSERT(nIntersections == 0);
            csDetails.CsDvt.InRange        = false;
            csDetails.CsDvt.Intersection = CRITSECTIONDETAILSATPOI::NoIntersection;
            LOG(_T(".5*Dv*cot(theta) on Left Intersection = No Intersection"));
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
            // 3rd edition and earilier
            if ( csDetails.CsDvt.CotanThetaDv05 < csDetails.CsDv.Dv )
               csDetails.pCriticalSection = &csDetails.CsDv;
            else
               csDetails.pCriticalSection = &csDetails.CsDvt;

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
               csDetails.Start = 0.0;
            else
               csDetails.Start = pBridge->GetSegmentStartEndDistance(poiFaceOfSupport.GetSegmentKey());

            csDetails.End = csDetails.pCriticalSection->Poi.GetDistFromStart();
         }
         else
         {
            // CS/FOS is on back side of pier so zone goes from CS to CL Bearing at end
            csDetails.Start = csDetails.pCriticalSection->Poi.GetDistFromStart();

            // if this is the last segment in the girder, end the CS-zone at the end face of the girder
            // otherwise end it at the CL Bearing
            SegmentIndexType nSegments = pGroup->GetGirder(girderKey.girderIndex)->GetSegmentCount();
            if( poiFaceOfSupport.GetSegmentKey().segmentIndex == nSegments-1 )
               csDetails.End = pBridge->GetSegmentLength(poiFaceOfSupport.GetSegmentKey());
            else
               csDetails.End = pBridge->GetSegmentLength(poiFaceOfSupport.GetSegmentKey()) - pBridge->GetSegmentEndEndDistance(poiFaceOfSupport.GetSegmentKey());
         }

         csDetails.pCriticalSection->Poi.SetAttributes(attributes);

         vcsDetails.push_back(csDetails);
      }
      catch (const mathXEvalError&)
      {
         GET_IFACE(IEAFStatusCenter,pStatusCenter);

         std::_tstring msg(_T("An error occured while locating the critical section for shear"));
         pgsUnknownErrorStatusItem* pStatusItem = new pgsUnknownErrorStatusItem(m_StatusGroupID,m_scidUnknown,_T(__FILE__),__LINE__,msg.c_str());
         pStatusCenter->Add(pStatusItem);

         msg += std::_tstring(_T("\nSee Status Center for Details"));
         THROW_UNWIND(msg.c_str(),-1);
      }
   }

   return vcsDetails;
}

const CRACKEDSECTIONDETAILS* CEngAgentImp::ValidateCrackedSectionDetails(const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   std::map<PoiIDKey,CRACKEDSECTIONDETAILS>::iterator found;

   int idx = (bPositiveMoment ? 0 : 1);
   if ( poi.GetID() != INVALID_ID )
   {
      PoiIDKey key(poi.GetID(),poi);
      found = m_CrackedSectionDetails[idx].find( key );

      if ( found != m_CrackedSectionDetails[idx].end() )
         return &( (*found).second ); // cracked section has already been computed
   }

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE(IDocumentType,pDocType);
   std::_tostringstream os;
   if ( pDocType->IsPGSuperDocument() )
   {
      os << _T("Analyzing cracked section for Span ")
         << LABEL_SPAN(segmentKey.groupIndex) << _T(" Girder ")
         << LABEL_GIRDER(segmentKey.girderIndex) << _T(" at ") << (LPCTSTR)FormatDimension(poi.GetDistFromStart(),pDisplayUnits->GetSpanLengthUnit())
         << _T(" from start of girder") << std::ends;
   }
   else
   {
      os << _T("Analyzing cracked section for Group ")
         << LABEL_SPAN(segmentKey.groupIndex) << _T(" Girder ")
         << LABEL_GIRDER(segmentKey.girderIndex) << _T(" Segment ")
         << LABEL_SEGMENT(segmentKey.segmentIndex) 
         << _T(" at ") << (LPCTSTR)FormatDimension(poi.GetDistFromStart(),pDisplayUnits->GetSpanLengthUnit())
         << _T(" from start of segment") << std::ends;
   }

   pProgress->UpdateMessage( os.str().c_str() );

   CRACKEDSECTIONDETAILS csd;
   m_MomentCapEngineer.AnalyzeCrackedSection(poi,bPositiveMoment,&csd);

   PoiIDKey key(poi.GetID(),poi);
   std::pair<std::map<PoiIDKey,CRACKEDSECTIONDETAILS>::iterator,bool> retval;
   retval = m_CrackedSectionDetails[idx].insert( std::make_pair(key,csd) );
   return &((*(retval.first)).second);
}

/////////////////////////////////////////////////////////////////////////////
// IAgent
STDMETHODIMP CEngAgentImp::SetBroker(IBroker* pBroker)
{
   AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CEngAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface( IID_ILosses,                      this );
   pBrokerInit->RegInterface( IID_IPretensionForce,             this );
   pBrokerInit->RegInterface( IID_IPosttensionForce,            this );
   pBrokerInit->RegInterface( IID_ILiveLoadDistributionFactors, this );
   pBrokerInit->RegInterface( IID_IMomentCapacity,              this );
   pBrokerInit->RegInterface( IID_IShearCapacity,               this );
   pBrokerInit->RegInterface( IID_IGirderHaunch,                this );
   pBrokerInit->RegInterface( IID_IFabricationOptimization,     this );
   pBrokerInit->RegInterface( IID_IArtifact,                    this );
   pBrokerInit->RegInterface( IID_ICrackedSection,              this );

    return S_OK;
}

STDMETHODIMP CEngAgentImp::Init()
{
   CREATE_LOGFILE("EngAgent");
   AGENT_INIT;

   m_PsForceEngineer.SetBroker(m_pBroker);
   m_MomentCapEngineer.SetBroker(m_pBroker);
   m_ShearCapEngineer.SetBroker(m_pBroker);
   m_Designer.SetBroker(m_pBroker);
   m_LoadRater.SetBroker(m_pBroker);

   m_Designer.SetStatusGroupID(m_StatusGroupID);
   m_PsForceEngineer.SetStatusGroupID(m_StatusGroupID);
   m_MomentCapEngineer.SetStatusGroupID(m_StatusGroupID);
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

   return S_OK;
}

STDMETHODIMP CEngAgentImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_EngAgent;
   return S_OK;
}

STDMETHODIMP CEngAgentImp::Reset()
{
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

   CLOSE_LOGFILE;

   AGENT_CLEAR_INTERFACE_CACHE;

   return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// ILosses
Float64 CEngAgentImp::GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   const LOSSDETAILS* pDetails = FindLosses(poi);
   ATLASSERT(pDetails != 0);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = pDetails->pLosses->TemporaryStrand_ElasticShorteningLosses();
   else
      val = pDetails->pLosses->PermanentStrand_ElasticShorteningLosses();

   return val;
}

const LOSSDETAILS* CEngAgentImp::GetLossDetails(const pgsPointOfInterest& poi)
{
   return FindLosses(poi);
}

void CEngAgentImp::ReportLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   m_PsForceEngineer.ReportLosses(girderKey,pChapter,pDisplayUnits);
}

void CEngAgentImp::ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   m_PsForceEngineer.ReportFinalLosses(girderKey,pChapter,pDisplayUnits);
}

Float64 CEngAgentImp::GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   const LOSSDETAILS* pDetails = GetLossDetails(poi,config);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = pDetails->pLosses->TemporaryStrand_ElasticShorteningLosses();
   else
      val = pDetails->pLosses->PermanentStrand_ElasticShorteningLosses();

   return val;
}

const LOSSDETAILS* CEngAgentImp::GetLossDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   return m_PsForceEngineer.GetLosses(poi,config);
}

void CEngAgentImp::ClearDesignLosses()
{
   m_PsForceEngineer.ClearDesignLosses();
}

Float64 CEngAgentImp::GetPrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime)
{
   return m_PsForceEngineer.GetPrestressLoss(poi,strandType,intervalIdx,intervalTime);
}

Float64 CEngAgentImp::GetPrestressLoss(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime)
{
   return m_PsForceEngineer.GetPrestressLoss(poi,strandType,intervalIdx,intervalTime,&config);
}

Float64 CEngAgentImp::GetPrestressLossWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   return m_PsForceEngineer.GetPrestressLossWithLiveLoad(poi,strandType,NULL);
}

Float64 CEngAgentImp::GetFrictionLoss(const pgsPointOfInterest& poi,DuctIndexType ductIdx)
{
   const LOSSDETAILS* pLossDetails = GetLossDetails(poi);
   return pLossDetails->FrictionLossDetails[ductIdx].dfpF;
}

Float64 CEngAgentImp::GetAnchorSetZoneLength(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType)
{
   const ANCHORSETDETAILS* pDetails = m_PsForceEngineer.GetAnchorSetDetails(girderKey,ductIdx);
   return pDetails->Lset[endType];
}

Float64 CEngAgentImp::GetAnchorSetLoss(const pgsPointOfInterest& poi,DuctIndexType ductIdx)
{
   const LOSSDETAILS* pLossDetails = GetLossDetails(poi);
   return pLossDetails->FrictionLossDetails[ductIdx].dfpA;
}

/////////////////////////////////////////////////////////////////////////////
// IPretensionForce

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetPjackMax(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,StrandIndexType nStrands)
{
   return m_PsForceEngineer.GetPjackMax(segmentKey,strandType,nStrands);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetPjackMax(const CSegmentKey& segmentKey,const matPsStrand& strand,StrandIndexType nStrands)
{
   return m_PsForceEngineer.GetPjackMax(segmentKey,strand,nStrands);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetXferLength(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType)
{
   return m_PsForceEngineer.GetXferLength(segmentKey,strandType);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetDevLength(const pgsPointOfInterest& poi,bool bDebonded)
{
   return m_PsForceEngineer.GetDevLength(poi,bDebonded);
}

//-----------------------------------------------------------------------------
STRANDDEVLENGTHDETAILS CEngAgentImp::GetDevLengthDetails(const pgsPointOfInterest& poi,bool bDebonded)
{
    return m_PsForceEngineer.GetDevLengthDetails(poi,bDebonded);
}

STRANDDEVLENGTHDETAILS CEngAgentImp::GetDevLengthDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bDebonded)
{
    return m_PsForceEngineer.GetDevLengthDetails(poi,bDebonded,config);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetStrandBondFactor(const pgsPointOfInterest& poi,
                                          StrandIndexType strandIdx,
                                          pgsTypes::StrandType strandType,
                                          Float64 fps,Float64 fpe)
{
   return m_PsForceEngineer.GetDevLengthAdjustment(poi,strandIdx,strandType,fps,fpe);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetStrandBondFactor(const pgsPointOfInterest& poi,
                                          StrandIndexType strandIdx,
                                          pgsTypes::StrandType strandType)
{
   return m_PsForceEngineer.GetDevLengthAdjustment(poi,strandIdx,strandType);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetStrandBondFactor(const pgsPointOfInterest& poi,
                             const GDRCONFIG& config,
                             StrandIndexType strandIdx,
                             pgsTypes::StrandType strandType)
{
   return m_PsForceEngineer.GetDevLengthAdjustment(poi,strandIdx,strandType,config);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetStrandBondFactor(const pgsPointOfInterest& poi,
                             const GDRCONFIG& config,
                             StrandIndexType strandIdx,
                             pgsTypes::StrandType strandType,
                             Float64 fps,Float64 fpe)
{
   return m_PsForceEngineer.GetDevLengthAdjustment(poi,strandIdx,strandType,config,fps,fpe);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetHoldDownForce(const CSegmentKey& segmentKey)
{
   return m_PsForceEngineer.GetHoldDownForce(segmentKey);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetHoldDownForce(const CSegmentKey& segmentKey,const GDRCONFIG& config)
{
   return m_PsForceEngineer.GetHoldDownForce(segmentKey,config);
}

Float64 CEngAgentImp::GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime)
{
   Float64 cos = GetHorizPsComponent(m_pBroker,poi);
   return cos * GetPrestressForce(poi,pgsTypes::Harped,intervalIdx,intervalTime);
}

Float64 CEngAgentImp::GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime)
{
   Float64 cos = GetHorizPsComponent(m_pBroker,poi,config);
   return cos * GetPrestressForce(poi,config,pgsTypes::Harped,intervalIdx,intervalTime);
}

Float64 CEngAgentImp::GetVertHarpedStrandForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime)
{
   Float64 sin = GetVertPsComponent(m_pBroker,poi);
   return sin * GetPrestressForce(poi,pgsTypes::Harped,intervalIdx,intervalTime);
}

Float64 CEngAgentImp::GetVertHarpedStrandForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime)
{
   Float64 sin = GetVertPsComponent(m_pBroker,poi);
   return sin * GetPrestressForce(poi,config,pgsTypes::Harped,intervalIdx,intervalTime);
}

Float64 CEngAgentImp::GetPrestressForce(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime)
{
   return m_PsForceEngineer.GetPrestressForce(poi,strandType,intervalIdx,intervalTime);
}

Float64 CEngAgentImp::GetPrestressForce(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime)
{
   return m_PsForceEngineer.GetPrestressForce(poi,strandType,intervalIdx,intervalTime,config);
}

Float64 CEngAgentImp::GetPrestressForcePerStrand(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);
   Float64 slabOffset = pBridge->GetSlabOffset(poi);
   return GetPrestressForcePerStrand(poi,config,strandType,intervalIdx,intervalTime);
}

Float64 CEngAgentImp::GetPrestressForcePerStrand(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 Ps = GetPrestressForce(poi,config,strandType,intervalIdx,intervalTime);
   StrandIndexType nStrands = config.PrestressConfig.GetNStrands(strandType);

   GET_IFACE(IBridge,pBridge);
   Float64 gdr_length = pBridge->GetSegmentLength(segmentKey);

   std::vector<DEBONDCONFIG>::const_iterator iter;
   for ( iter = config.PrestressConfig.Debond[strandType].begin(); iter != config.PrestressConfig.Debond[strandType].end(); iter++ )
   {
      const DEBONDCONFIG& debond_info = *iter;
      if ( InRange(0.0,poi.GetDistFromStart(),debond_info.LeftDebondLength) ||
           InRange(gdr_length - debond_info.RightDebondLength, poi.GetDistFromStart(), gdr_length) )
      {
         nStrands--;
      }
   }

   return Ps/nStrands;
}

Float64 CEngAgentImp::GetEffectivePrestress(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime)
{
   return m_PsForceEngineer.GetEffectivePrestress(poi,strandType,intervalIdx,intervalTime);
}

Float64 CEngAgentImp::GetEffectivePrestress(const pgsPointOfInterest& poi,const GDRCONFIG& config,pgsTypes::StrandType strandType,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType intervalTime)
{
   return m_PsForceEngineer.GetEffectivePrestress(poi,strandType,intervalIdx,intervalTime,&config);
}

Float64 CEngAgentImp::GetPrestressForceWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   return m_PsForceEngineer.GetPrestressForceWithLiveLoad(poi,strandType,NULL);
}

Float64 CEngAgentImp::GetEffectivePrestressWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   return m_PsForceEngineer.GetEffectivePrestressWithLiveLoad(poi,strandType,NULL);
}

/////////////////////////////////////////////////////////////////////////////
// IPosttensionForce
Float64 CEngAgentImp::GetPjackMax(const CGirderKey& girderKey,StrandIndexType nStrands)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(girderKey.girderIndex);
   const CPTData*             pPTData     = pGirder->GetPostTensioning();

   return GetPjackMax(girderKey,*pPTData->pStrand,nStrands);
}

Float64 CEngAgentImp::GetPjackMax(const CGirderKey& girderKey,const matPsStrand& strand,StrandIndexType nStrands)
{
   GET_IFACE( IAllowableTendonStress, pAllowable);
   Float64 fpj = pAllowable->GetAllowablePriorToSeating(girderKey);
   Float64 aps = strand.GetNominalArea();
   Float64 Fpj = fpj*aps*nStrands;

   return Fpj;
}

Float64 CEngAgentImp::GetTendonForce(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CSplicedGirderData*  pGirder = pIBridgeDesc->GetGirder(segmentKey);
   const CPTData*             pPTData = pGirder->GetPostTensioning();

   GET_IFACE(IGirder,pIGirder);
   WebIndexType nWebs = pIGirder->GetWebCount(poi.GetSegmentKey());

   Float64 aps = pPTData->pStrand->GetNominalArea();

   Float64 Fpe = 0;
   DuctIndexType nDucts = pPTData->GetDuctCount();
   DuctIndexType firstDuctIdx = (ductIdx == ALL_DUCTS ? 0 : ductIdx);
   DuctIndexType lastDuctIdx  = (ductIdx == ALL_DUCTS ? nDucts-1 : firstDuctIdx);
   for ( DuctIndexType idx = firstDuctIdx; idx <= lastDuctIdx; idx++ )
   {
      Float64 fpe = GetTendonStress(poi,intervalIdx,time,idx); // get the tendon stress

      // get number of strands in the tendon
      const CDuctData* pDuct = pPTData->GetDuct(idx/nWebs);
      StrandIndexType nStrands = pDuct->nStrands;

      Fpe += fpe*aps*nStrands;
   }

   return Fpe;
}

Float64 CEngAgentImp::GetTendonStress(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::IntervalTimeType time,DuctIndexType ductIdx)
{
   ATLASSERT(ductIdx != ALL_DUCTS);
   ATLASSERT(time != pgsTypes::Middle); // can only get tendon stress at start or end of interval

   GET_IFACE(ILosses,pLosses);
   const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi);

   GET_IFACE(IIntervals,pIntervals);
   const CGirderKey& girderKey(poi.GetSegmentKey());
   IntervalIndexType ptIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);

   // the Time Step Loss details has the tendon stress at the end of an interval
   // If we want the tendon stress at the start of the interval, get the stress at the end of the previous interval
   // However, if this is the interval when the tendon is stressed the stress at the end of the previous interval is zero so
   // want we want is the stress at the end of this interval. The duration of the interval should be zero.
   if ( intervalIdx != ptIntervalIdx )
   {
      if ( time == pgsTypes::Start )
      {
         // start of this interval is at the end of the previous interval
         if ( intervalIdx == 0 )
            return 0;

         intervalIdx--;
      }
   }
#if defined _DEBUG
   else
   {
      ATLASSERT(pIntervals->GetDuration(intervalIdx) == 0);
   }
#endif

   Float64 fpe = pDetails->TimeStepDetails[intervalIdx].Tendons[ductIdx].fpe; // effective stress at end of this interval
   return fpe;
}

/////////////////////////////////////////////////////////////////////////////
// IArtifact
void CEngAgentImp::VerifyDistributionFactorRequirements(const pgsPointOfInterest& poi)
{
   CheckCurvatureRequirements(poi);
   CheckGirderStiffnessRequirements(poi);
   CheckParallelGirderRequirements(poi);
}

void CEngAgentImp::CheckCurvatureRequirements(const pgsPointOfInterest& poi)
{
   //
   // Check the curvature requirements (4.6.1.2.1)
   // throws exception if requirements not met
   //

   GET_IFACE(IRoadwayData,pRoadway);
   GET_IFACE(IRoadway,pAlignment);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE(ILiveLoads,pLiveLoads);
   GET_IFACE(IPointOfInterest,pIPoi);

   SpanIndexType spanIdx = pIPoi->GetSpan(poi);

   SpanIndexType nSpans = pBridge->GetSpanCount();

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   AlignmentData2 alignment_data = pRoadway->GetAlignmentData2();
   IndexType nCurves = alignment_data.HorzCurves.size();
   if ( nCurves == 0 )
      return; // no curves

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
   PierIndexType start_pier = PierIndexType(spanIdx); // start pier has same id as span
   PierIndexType end_pier = start_pier+1;
   Float64 start_station = pBridge->GetPierStation(start_pier); 
   Float64 end_station   = pBridge->GetPierStation(end_pier);

   CComPtr<IDirection> start_brg, end_brg;
   pAlignment->GetBearing(start_station,&start_brg);
   pAlignment->GetBearing(end_station,  &end_brg);

   CComPtr<IAngle> subtended_angle;
   end_brg->AngleBetween(start_brg,&subtended_angle);

   Float64 delta;
   subtended_angle->get_Value(&delta);
   delta = ToDegrees(fabs(delta));

   if ( 180 < delta )
      delta = 360 - delta;

   
   GirderIndexType nBeams = pBridge->GetGirderCount(segmentKey.groupIndex);
   Float64 delta_limit;
   bool bIsOK = true;
   if ( nBeams == 0 || nBeams == 1 )
   {
      LPCTSTR msg = _T("The bridge must have at least two beams per span");
      pgsBridgeDescriptionStatusItem* pStatusItem = new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,pgsBridgeDescriptionStatusItem::Framing,msg);
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
      pStatusCenter->Add(pStatusItem);

      os << _T("See Status Center for Details") << std::endl;
      THROW_UNWIND(os.str().c_str(),XREASON_REFINEDANALYSISREQUIRED);
   }
}

void CEngAgentImp::CheckGirderStiffnessRequirements(const pgsPointOfInterest& poi)
{
   GET_IFACE(ILiveLoads,pLiveLoads);
   if ( pLiveLoads->IgnoreLLDFRangeOfApplicability() )
      return; // nothing to do here

   // get angle between all girders in this span
   // if < than limit then status center + exception
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE(IBridge,pBridge);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   Float64 minStiffnessRatio = pSpecEntry->GetMinGirderStiffnessRatio();

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GirderIndexType nGirders = pBridge->GetGirderCount(segmentKey.groupIndex);
  
   // look at non-composite moment of inertia
   // we want girders that are basically the same
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetCastDeckInterval();
  
   GET_IFACE(ISectionProperties,pSectProp);
   Float64 Imin = pSectProp->GetIx(intervalIdx,poi);
   Float64 Imax = Imin;

   for ( GirderIndexType gdrIdx = 1; gdrIdx < nGirders; gdrIdx++ )
   {
      CSegmentKey current_segment_key(segmentKey.groupIndex,gdrIdx,segmentKey.segmentIndex);
      pgsPointOfInterest current_poi(current_segment_key,poi.GetDistFromStart());
      
      Float64 I = pSectProp->GetIx(intervalIdx,current_poi);
      Imin = _cpp_min(Imin,I);
      Imax = _cpp_max(Imax,I);
   }

   Float64 ratio = Imin/Imax;
   if ( ratio < minStiffnessRatio )
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      std::_tostringstream os;
      os << _T("Live Load Distribution Factors could not be calculated for the following reason") << std::endl;
      os << _T("Per 4.6.2.2.1, the girders in this span do not have approximately the same stiffness.") << std::endl;
      os << _T("Minimum I = ") << (LPCTSTR)FormatDimension(Imin,pDisplayUnits->GetMomentOfInertiaUnit(),true) << std::endl;
      os << _T("Maximum I = ") << (LPCTSTR)FormatDimension(Imax,pDisplayUnits->GetMomentOfInertiaUnit(),true) << std::endl;
      os << _T("Stiffness Ratio (I min / I max) = ") << (LPCTSTR)FormatScalar(ratio,pDisplayUnits->GetScalarFormat()) << std::endl;
      os << _T("Minimum stiffness ratio permitted by ") << pSpecEntry->GetName() << _T(" = ") << (LPCTSTR)FormatScalar(minStiffnessRatio,pDisplayUnits->GetScalarFormat()) << std::endl;
      os << _T("A refined method of analysis is required for this bridge") << std::endl;

      pgsRefinedAnalysisStatusItem* pStatusItem = new pgsRefinedAnalysisStatusItem(m_StatusGroupID,m_scidRefinedAnalysis,os.str().c_str());
      pStatusCenter->Add(pStatusItem);

      os << _T("See Status Center for Details") << std::endl;
      THROW_UNWIND(os.str().c_str(),XREASON_REFINEDANALYSISREQUIRED);
   }
}

void CEngAgentImp::CheckParallelGirderRequirements(const pgsPointOfInterest& poi)
{
   GET_IFACE(ILiveLoads,pLiveLoads);
   if ( pLiveLoads->IgnoreLLDFRangeOfApplicability() )
      return; // nothing to do here

   // get angle between all girders in this span
   // if < than limit then status center + exception
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE(IBridge,pBridge);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   Float64 maxAllowableAngle = pSpecEntry->GetMaxAngularDeviationBetweenGirders();

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
         dir_of_prev_girder = TWO_PI - dir_of_prev_girder;

      Float64 maxAngularDifference = -DBL_MAX;
      for ( GirderIndexType gdrIdx = 1; gdrIdx < nGirders; gdrIdx++ )
      {
         objDirection.Release();
         pBridge->GetSegmentBearing(CSegmentKey(segmentKey.groupIndex,gdrIdx,segIdx),&objDirection);
         Float64 dir_of_this_girder;
         objDirection->get_Value(&dir_of_this_girder);

         if ( M_PI < dir_of_this_girder )
            dir_of_this_girder = TWO_PI - dir_of_this_girder;

         Float64 angular_diff = fabs(dir_of_this_girder - dir_of_prev_girder);
         maxAngularDifference = _cpp_max(angular_diff,maxAngularDifference);
      } // next girder

      if ( maxAllowableAngle < maxAngularDifference )
      {
         GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
         std::_tostringstream os;
         os << _T("Live Load Distribution Factors could not be calculated for the following reason") << std::endl;
         os << _T("Per 4.6.2.2.1, the girders in this span are not parallel.") << std::endl;
         os << _T("Greatest angular difference between girders in this span = ") << (LPCTSTR)FormatDimension(maxAngularDifference,pDisplayUnits->GetAngleUnit(),true) << std::endl;
         os << _T("Maximum angular difference permitted by ") << pSpecEntry->GetName() << _T(" = ") << (LPCTSTR)FormatDimension(maxAllowableAngle,pDisplayUnits->GetAngleUnit(),true) << std::endl;
         os << _T("A refined method of analysis is required for this bridge") << std::endl;

         pgsRefinedAnalysisStatusItem* pStatusItem = new pgsRefinedAnalysisStatusItem(m_StatusGroupID,m_scidRefinedAnalysis,os.str().c_str());
         pStatusCenter->Add(pStatusItem);

         os << _T("See Status Center for Details") << std::endl;
         THROW_UNWIND(os.str().c_str(),XREASON_REFINEDANALYSISREQUIRED);
      }
   } // next segment
}

Float64 CEngAgentImp::GetMomentDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pSpan->GetLLDFPosMoment(gdrIdx,ls);
   }
   else
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetMomentDF(spanIdx,gdrIdx,ls);
   }
}

Float64 CEngAgentImp::GetMomentDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,Float64 fcgdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pSpan->GetLLDFPosMoment(gdrIdx,ls);
   }
   else
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetMomentDF(spanIdx,gdrIdx,ls,fcgdr);
   }
}
   
Float64 CEngAgentImp::GetNegMomentDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pSpan->GetLLDFNegMoment(gdrIdx,ls);
   }
   else
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetMomentDF(spanIdx,gdrIdx,ls);
   }
}
   
Float64 CEngAgentImp::GetNegMomentDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,Float64 fcgdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pSpan->GetLLDFNegMoment(gdrIdx,ls);
   }
   else
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetMomentDF(spanIdx,gdrIdx,ls,fcgdr);
   }
}
   
Float64 CEngAgentImp::GetNegMomentDistFactorAtPier(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
   const CSpanData2* pSpan = (pierFace == pgsTypes::Back) ? pPier->GetPrevSpan() : pPier->GetNextSpan();

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pPier->GetLLDFNegMoment(gdrIdx, ls);
   }
   else
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetNegMomentDF(pierIdx,gdrIdx,ls,pierFace);
   }
}
   
Float64 CEngAgentImp::GetNegMomentDistFactorAtPier(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
   const CSpanData2* pSpan = (pierFace == pgsTypes::Back) ? pPier->GetPrevSpan() : pPier->GetNextSpan();

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pPier->GetLLDFNegMoment(gdrIdx, ls);
   }
   else
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetNegMomentDF(pierIdx,gdrIdx,ls,pierFace,fcgdr);
   }
}

Float64 CEngAgentImp::GetShearDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      pgsTypes::GirderLocation gl = pGroup->IsExteriorGirder(gdrIdx) ? pgsTypes::Exterior : pgsTypes::Interior;
      return pSpan->GetLLDFShear(gdrIdx,ls);
   }
   else
   {
      CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetShearDF(spanIdx,gdrIdx,ls);
   }
}

Float64 CEngAgentImp::GetShearDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,Float64 fcgdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      pgsTypes::GirderLocation gl = pGroup->IsExteriorGirder(gdrIdx) ? pgsTypes::Exterior : pgsTypes::Interior;
      return pSpan->GetLLDFShear(gdrIdx,ls);
   }
   else
   {
      CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetShearDF(spanIdx,gdrIdx,ls,fcgdr);
   }
}

Float64 CEngAgentImp::GetReactionDistFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
   const CSpanData2* pSpan = (pierIdx == pBridgeDesc->GetPierCount()-1 ? pPier->GetPrevSpan() : pPier->GetNextSpan());

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pPier->GetLLDFReaction(gdrIdx, ls);
   }
   else
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetReactionDF(pierIdx,gdrIdx,ls);
   }
}

Float64 CEngAgentImp::GetReactionDistFactor(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,Float64 fcgdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
   const CSpanData2* pSpan = (pierIdx == pBridgeDesc->GetPierCount()-1 ? pPier->GetPrevSpan() : pPier->GetNextSpan());

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pPier->GetLLDFReaction(gdrIdx, ls);
   }
   else
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
      ValidateLiveLoadDistributionFactors(girderKey);

      return m_pDistFactorEngineer->GetReactionDF(pierIdx,gdrIdx,ls,fcgdr);
   }
}

void CEngAgentImp::GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState ls,Float64* pM,Float64* nM,Float64* V)
{
   GET_IFACE(IBridge,pBridge);
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IPointOfInterest,pPOI);
   SpanIndexType spanIdx = pPOI->GetSpan(poi);

   GirderIndexType gdrIdx = segmentKey.girderIndex;

   PierIndexType prev_pier = PierIndexType(spanIdx);
   PierIndexType next_pier = prev_pier + 1;

   SpanIndexType nSpans = pBridge->GetSpanCount();

   Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 dist_from_start = poi.GetDistFromStart() - end_size;

   Float64 dfPoints[2];
   Uint32 nPoints;
   GetNegMomentDistFactorPoints(spanIdx,gdrIdx,&dfPoints[0],&nPoints);

   *V  = GetShearDistFactor(spanIdx,gdrIdx,ls);
   *pM = GetMomentDistFactor(spanIdx,gdrIdx,ls);

   if ( nPoints == 0 )
   {
      *nM = GetNegMomentDistFactor(spanIdx,gdrIdx,ls);
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

         if ( bContinuousOnLeft || bContinuousOnRight || bIntegralOnRight )
         {
            //Integral to the left of this point... use DF at prev pier
            *nM = GetNegMomentDistFactorAtPier(prev_pier,gdrIdx,ls,pgsTypes::Ahead);
         }
         else
         {
            // hinged to the left of this point... use DF for the span
            *nM = GetNegMomentDistFactor(spanIdx,gdrIdx,ls);
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
            *nM = GetNegMomentDistFactorAtPier(next_pier,gdrIdx,ls,pgsTypes::Back);
         }
         else
         {
            // Integral to the left of this point... use DF for the span
            *nM = GetNegMomentDistFactor(spanIdx,gdrIdx,ls);
         }
      }
   }
   else
   {
      if ( dist_from_start < dfPoints[0] )
         *nM = GetNegMomentDistFactorAtPier(prev_pier,gdrIdx,ls,pgsTypes::Ahead);
      else if ( dfPoints[0] <= dist_from_start && dist_from_start <= dfPoints[1] )
         *nM = GetNegMomentDistFactor(spanIdx,gdrIdx,ls);
      else
         *nM = GetNegMomentDistFactorAtPier(next_pier,gdrIdx,ls,pgsTypes::Back);
   }
}

void CEngAgentImp::GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState ls,Float64 fcgdr,Float64* pM,Float64* nM,Float64* V)
{
   GET_IFACE(IBridge,pBridge);
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IPointOfInterest,pPOI);
   SpanIndexType spanIdx = pPOI->GetSpan(poi);

   GirderIndexType gdrIdx = segmentKey.girderIndex;

   PierIndexType prev_pier = PierIndexType(spanIdx);
   PierIndexType next_pier = prev_pier + 1;

   SpanIndexType nSpans = pBridge->GetSpanCount();

   Float64 end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 dist_from_start = poi.GetDistFromStart() - end_size;

   Float64 dfPoints[2];
   Uint32 nPoints;
   GetNegMomentDistFactorPoints(spanIdx,gdrIdx,&dfPoints[0],&nPoints);

   *V  = GetShearDistFactor(spanIdx,gdrIdx,ls,fcgdr);
   *pM = GetMomentDistFactor(spanIdx,gdrIdx,ls,fcgdr);

   if ( nPoints == 0 )
   {
      *nM = GetNegMomentDistFactor(spanIdx,gdrIdx,ls,fcgdr);
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
            *nM = GetNegMomentDistFactorAtPier(prev_pier,gdrIdx,ls,pgsTypes::Ahead,fcgdr);
         }
         else
         {
            // hinged to the left of this point... use DF for the span
            *nM = GetNegMomentDistFactor(spanIdx,gdrIdx,ls,fcgdr);
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
            *nM = GetNegMomentDistFactor(spanIdx,gdrIdx,ls,fcgdr);
         }
         else
         {
            // hinged to the left of this point... use DF at the next pier
            *nM = GetNegMomentDistFactorAtPier(next_pier,gdrIdx,ls,pgsTypes::Back,fcgdr);
         }
      }
   }
   else
   {
      if ( dist_from_start < dfPoints[0] )
         *nM = GetNegMomentDistFactorAtPier(prev_pier,gdrIdx,ls,pgsTypes::Ahead,fcgdr);
      else if ( dfPoints[0] <= dist_from_start && dist_from_start <= dfPoints[1] )
         *nM = GetNegMomentDistFactor(spanIdx,gdrIdx,ls,fcgdr);
      else
         *nM = GetNegMomentDistFactorAtPier(next_pier,gdrIdx,ls,pgsTypes::Back,fcgdr);
   }
}

void CEngAgentImp::GetNegMomentDistFactorPoints(SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64* dfPoints,Uint32* nPoints)
{
   GET_IFACE(IContraflexurePoints,pCP);
   pCP->GetContraflexurePoints(spanIdx,gdrIdx,dfPoints,nPoints);
}

void CEngAgentImp::ReportDistributionFactors(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
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
      scalar.SetFormat( sysNumericFormatTool::Fixed );
      scalar.SetWidth(6);
      scalar.SetPrecision(3);
      scalar.SetTolerance(1.0e-6);

      rptParagraph* pPara;
      pPara = new rptParagraph;
      (*pChapter) << pPara;
      (*pPara) << _T("Method of Computation: Directly Input") << rptNewLine;

      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
      pgsTypes::GirderLocation gl = pGroup->IsInteriorGirder(girderKey.girderIndex) ? pgsTypes::Interior : pgsTypes::Exterior;

      ColumnIndexType nCols = 5;
      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         nCols += 4;

      rptRcTable* table = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Live Load Distribution Factors"));

      (*pPara) << table;


      // Set up table headings
      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         table->SetNumberOfHeaderRows(1);
         (*table)(0,0) << _T("");
         (*table)(0,1) << _T("+M");
         (*table)(0,2) << _T("-M");
         (*table)(0,3) << _T("V");
         (*table)(0,4) << _T("R");
      }
      else
      {
         table->SetNumberOfHeaderRows(2);

         table->SetRowSpan(0,0,2);
         table->SetRowSpan(1,0,-1);
         (*table)(0,0) << _T("");

         table->SetColumnSpan(0,1,3);
         (*table)(0,1) << _T("Strength/Service");

         table->SetColumnSpan(0,2,3);
         (*table)(0,2) << _T("Fatigue");

         table->SetColumnSpan(0,3,-1);
         table->SetColumnSpan(0,4,-1);
         table->SetColumnSpan(0,5,-1);
         table->SetColumnSpan(0,6,-1);

         (*table)(1,1) << _T("+M");
         (*table)(1,2) << _T("-M");
         (*table)(1,3) << _T("V");
         (*table)(1,4) << _T("R");
         (*table)(1,5) << _T("+M");
         (*table)(1,6) << _T("-M");
         (*table)(1,7) << _T("V");
         (*table)(1,8) << _T("R");
      }

      const CPierData2* pPier = pBridgeDesc->GetPier(0);
      const CSpanData2* pSpan = NULL;

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
            (*table)(row,2) << scalar.SetValue( pPier->GetLLDFNegMoment(girderKey.girderIndex, pgsTypes::StrengthI) );
         else
            (*table)(row,2) << _T("");

         (*table)(row,3) << _T("");
         (*table)(row,4) << scalar.SetValue( pPier->GetLLDFReaction(girderKey.girderIndex, pgsTypes::StrengthI) );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*table)(row,5) << _T("");

            if ( bContinuous || bIntegralOnLeft || bIntegralOnRight)
               (*table)(row,6) << scalar.SetValue( pPier->GetLLDFNegMoment(girderKey.girderIndex, pgsTypes::FatigueI) );
            else
               (*table)(row,6) << _T("");

            (*table)(row,7) << _T("");
            (*table)(row,8) << scalar.SetValue( pPier->GetLLDFReaction(girderKey.girderIndex, pgsTypes::FatigueI) );
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
               (*table)(row,2) << scalar.SetValue( pSpan->GetLLDFNegMoment(girderKey.girderIndex,pgsTypes::StrengthI) );
            else
               (*table)(row,2) << _T("");

            (*table)(row,3) << scalar.SetValue( pSpan->GetLLDFShear(girderKey.girderIndex,pgsTypes::StrengthI) );
            (*table)(row,4) << _T("");

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*table)(row,5) << scalar.SetValue( pSpan->GetLLDFPosMoment(girderKey.girderIndex,pgsTypes::FatigueI) );
               
               if ( bContinuousStart || bContinuousEnd || bIntegralStart || bIntegralEnd )
                  (*table)(row,6) << scalar.SetValue( pSpan->GetLLDFNegMoment(girderKey.girderIndex,pgsTypes::FatigueI) );
               else
                  (*table)(row,6) << _T("");

               (*table)(row,7) << scalar.SetValue( pSpan->GetLLDFShear(girderKey.girderIndex,pgsTypes::FatigueI) );
               (*table)(row,8) << _T("");
            }

            row++;

            pPier = pSpan->GetNextPier();
         }
      } while ( pSpan );
   }
}

bool CEngAgentImp::Run1250Tests(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

   CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
   ValidateLiveLoadDistributionFactors(girderKey);

   return m_pDistFactorEngineer->Run1250Tests(spanIdx,gdrIdx,ls,pid,bridgeId,resultsFile,poiFile);
}

bool CEngAgentImp::GetDFResultsEx(SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,
                               Float64* gpM, Float64* gpM1, Float64* gpM2,
                               Float64* gnM, Float64* gnM1, Float64* gnM2,
                               Float64* gV,  Float64* gV1,  Float64* gV2,
                               Float64* gR,  Float64* gR1,  Float64* gR2 ) 
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

   CGirderKey girderKey(pGroup->GetIndex(),gdrIdx);
   ValidateLiveLoadDistributionFactors(girderKey);

   return m_pDistFactorEngineer->GetDFResultsEx(spanIdx,gdrIdx, ls,
                               gpM, gpM1, gpM2, gnM, gnM1, gnM2,
                               gV,  gV1, gV2, gR, gR1, gR2 ); 
}

Float64 CEngAgentImp::GetDeflectionDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

   GirderIndexType nGirders = pGroup->GetGirderCount();
   Uint32 nLanes = GetNumberOfDesignLanes(spanIdx);
   Float64 mpf = lrfdUtility::GetMultiplePresenceFactor(nLanes);
   Float64 gD = mpf*nLanes/nGirders;

   return gD;
}

Uint32 CEngAgentImp::GetNumberOfDesignLanes(SpanIndexType spanIdx)
{
   Float64 dist_to_section, curb_to_curb;
   return GetNumberOfDesignLanesEx(spanIdx,&dist_to_section,&curb_to_curb);
}

Uint32 CEngAgentImp::GetNumberOfDesignLanesEx(SpanIndexType spanIdx,Float64* pDistToSection,Float64* pCurbToCurb)
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

   if ( width2 < width1 )
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
Float64 CEngAgentImp::GetMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   MOMENTCAPACITYDETAILS mcd;
   GetMomentCapacityDetails( intervalIdx,poi,bPositiveMoment,&mcd );
   return mcd.Phi * mcd.Mn; // = Mr
}

std::vector<Float64> CEngAgentImp::GetMomentCapacity(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment)
{
   std::vector<Float64> Mn;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      Mn.push_back( GetMomentCapacity(intervalIdx,poi,bPositiveMoment));
   }

   return Mn;
}

void CEngAgentImp::GetMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd)
{
#if defined _DEBUG
   // Mu is only considered once live load is applied to the structure
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   EventIndexType llEventIdx = pIBridgeDesc->GetLiveLoadEventIndex();
   ATLASSERT( llEventIdx <= intervalIdx );
#endif

   if ( poi.GetID() == INVALID_ID )
   {
      // compute but don't cache since poiID is the key
      *pmcd = ComputeMomentCapacity(intervalIdx,poi,bPositiveMoment);
      return;
   }

   const MOMENTCAPACITYDETAILS* pMCD = GetCachedMomentCapacity(intervalIdx,poi,bPositiveMoment);
   if ( pMCD == NULL )
   {
      pMCD = ValidateMomentCapacity(intervalIdx,poi,bPositiveMoment);
   }
   ATLASSERT(pMCD != NULL);

   *pmcd = *pMCD;
}

void CEngAgentImp::GetMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd)
{
    if (poi.GetID() == INVALID_ID)
    {
        // Never store temporary pois
       *pmcd = ComputeMomentCapacity(intervalIdx,poi,config,bPositiveMoment);
    }
    else
    {
       // Get the current configuration and compare it to the provided one
       // If same, call GetMomentCapacityDetails w/o config.
       GET_IFACE(IBridge,pBridge);
       GDRCONFIG curr_config = pBridge->GetSegmentConfiguration(poi.GetSegmentKey());

       if ( poi.GetID()!=INVALID_INDEX && curr_config.IsFlexuralDataEqual(config) )
       {
          GetMomentCapacityDetails(intervalIdx,poi,bPositiveMoment,pmcd);
       }
       else
       {
          // the capacity details for the requested girder configuration is not the same as for the
          // current input... see if it is cached
          const MOMENTCAPACITYDETAILS* pMCD = GetCachedMomentCapacity(intervalIdx,poi,config,bPositiveMoment);
          if ( pMCD == NULL )
          {
             // the capacity has not yet been computed for this config, moment type, stage, and poi
             pMCD = ValidateMomentCapacity(intervalIdx,poi,config,bPositiveMoment); // compute it
          }

          ATLASSERT( pMCD != NULL );

          *pmcd = *pMCD;
       }
    }
}

std::vector<Float64> CEngAgentImp::GetCrackingMoment(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment)
{
   std::vector<Float64> Mcr;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      Mcr.push_back( GetCrackingMoment(intervalIdx,poi,bPositiveMoment));
   }

   return Mcr;
}

Float64 CEngAgentImp::GetCrackingMoment(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   CRACKINGMOMENTDETAILS cmd;
   GetCrackingMomentDetails( intervalIdx,poi,bPositiveMoment,&cmd );

   Float64 Mcr = cmd.Mcr;
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   // in LRFD 2nd Edition 2003 Mcr = Sc(fr + fcpe) - Mdnc(Sc/Snc - 1) <= Scfr
   // however there is a typographical error... Mcr should be
   // Mcr = Sc(fr + fcpe) - Mdnc(Sc/Snc - 1) >= Scfr
   // This correction was made in LRFD 3rd Edition 2005.
   // 
   // We are going to use the correct equation from 2nd Edition 2003 forward
   bool bAfter2002 = ( lrfdVersionMgr::SecondEditionWith2003Interims <= pSpecEntry->GetSpecificationType() ? true : false );
   if ( bAfter2002 )
   {
      Mcr = _cpp_min(cmd.Mcr,cmd.McrLimit);
   }

   return Mcr;
}

void CEngAgentImp::GetCrackingMomentDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd)
{
#if defined _DEBUG
   // Mu is only considered once live load is applied to the structure
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   EventIndexType llEventIdx = pIBridgeDesc->GetLiveLoadEventIndex();
   ATLASSERT( llEventIdx <= intervalIdx );
#endif

   if ( poi.GetID() == INVALID_ID )
   {
      m_MomentCapEngineer.ComputeCrackingMoment(intervalIdx,poi,bPositiveMoment,pcmd);
      return;
   }

   *pcmd = *ValidateCrackingMoments(intervalIdx,poi,bPositiveMoment);
}

void CEngAgentImp::GetCrackingMomentDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd)
{
   // Get the current configuration and compare it to the provided one
   // If same, call GetMomentCapacityDetails w/o config.
   GET_IFACE(IBridge,pBridge);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   
   GDRCONFIG curr_config = pBridge->GetSegmentConfiguration(segmentKey);
   if ( poi.GetID()!=INVALID_INDEX && curr_config.IsFlexuralDataEqual(config) )
      GetCrackingMomentDetails(intervalIdx,poi,bPositiveMoment,pcmd);
   else
      m_MomentCapEngineer.ComputeCrackingMoment(intervalIdx,poi,config,bPositiveMoment,pcmd);
}

std::vector<Float64> CEngAgentImp::GetMinMomentCapacity(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment)
{
   std::vector<Float64> Mmin;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      Mmin.push_back( GetMinMomentCapacity(intervalIdx,poi,bPositiveMoment));
   }

   return Mmin;
}

Float64 CEngAgentImp::GetMinMomentCapacity(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   MINMOMENTCAPDETAILS mmcd;
   GetMinMomentCapacityDetails( intervalIdx, poi, bPositiveMoment, &mmcd );
   return mmcd.MrMin;
}

void CEngAgentImp::GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd)
{

#if defined _DEBUG
   // Mu is only considered once live load is applied to the structure
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   EventIndexType llEventIdx = pIBridgeDesc->GetLiveLoadEventIndex();
   ATLASSERT( llEventIdx <= intervalIdx );
#endif

   *pmmcd = *ValidateMinMomentCapacity(intervalIdx,poi,bPositiveMoment);
}

void CEngAgentImp::GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd)
{
   // Get the current configuration and compare it to the provided one
   // If same, call GetMomentCapacityDetails w/o config.
   GET_IFACE(IBridge,pBridge);
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   
   GDRCONFIG curr_config = pBridge->GetSegmentConfiguration(segmentKey);
   if ( poi.GetID()!=INVALID_INDEX && curr_config.IsFlexuralDataEqual(config) )
      GetMinMomentCapacityDetails(intervalIdx,poi,bPositiveMoment,pmmcd);
   else
      m_MomentCapEngineer.ComputeMinMomentCapacity(intervalIdx,poi,config,bPositiveMoment,pmmcd);
}

std::vector<MOMENTCAPACITYDETAILS> CEngAgentImp::GetMomentCapacityDetails(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment)
{
   std::vector<MOMENTCAPACITYDETAILS> details;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      MOMENTCAPACITYDETAILS mcd;
      GetMomentCapacityDetails(intervalIdx,poi,bPositiveMoment,&mcd);
      details.push_back( mcd );
   }

   return details;
}

std::vector<MINMOMENTCAPDETAILS> CEngAgentImp::GetMinMomentCapacityDetails(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment)
{
   std::vector<MINMOMENTCAPDETAILS> details;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      MINMOMENTCAPDETAILS mcd;
      GetMinMomentCapacityDetails(intervalIdx,poi,bPositiveMoment,&mcd);
      details.push_back( mcd );
   }

   return details;
}

std::vector<CRACKINGMOMENTDETAILS> CEngAgentImp::GetCrackingMomentDetails(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment)
{
   std::vector<CRACKINGMOMENTDETAILS> details;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      CRACKINGMOMENTDETAILS cmd;
      GetCrackingMomentDetails(intervalIdx,poi,bPositiveMoment,&cmd);
      details.push_back( cmd );
   }

   return details;
}

/////////////////////////////////////////////////////////////////////////////
// IShearCapacity
Float64 CEngAgentImp::GetShearCapacity(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi)
{
   SHEARCAPACITYDETAILS scd;
   GetShearCapacityDetails( ls,intervalIdx,poi,&scd );
   return scd.pVn; 
}

std::vector<Float64> CEngAgentImp::GetShearCapacity(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi)
{
   std::vector<Float64> Vn;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      Vn.push_back(GetShearCapacity(ls,intervalIdx,poi));
   }

   return Vn;
}

void CEngAgentImp::GetShearCapacityDetails(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,SHEARCAPACITYDETAILS* pscd)
{
   GET_IFACE(IBridge,pBridge);
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   const CGirderKey& girderKey(segmentKey);
   
   GDRCONFIG curr_config = pBridge->GetSegmentConfiguration(segmentKey);
   GetRawShearCapacityDetails(ls, intervalIdx, poi, pscd);

   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();

   GET_IFACE(IPointOfInterest, pPOI);

   if ( shear_capacity_method == scmBTEquations || shear_capacity_method == scmWSDOT2007 )
   {
      ZoneIndexType csZoneIdx = GetCriticalSectionZoneIndex(ls,poi);
      if ( csZoneIdx != INVALID_INDEX )
      {
         std::vector<pgsPointOfInterest> vCSPoi(pPOI->GetCriticalSections(ls,girderKey));
         ATLASSERT(0 < vCSPoi.size());

         const pgsPointOfInterest& csPoi(vCSPoi[csZoneIdx]);
         ATLASSERT(csPoi.HasAttribute(POI_CRITSECTSHEAR1 | POI_CRITSECTSHEAR2));

         SHEARCAPACITYDETAILS cs_scd;
         GetRawShearCapacityDetails(ls,intervalIdx,csPoi,&cs_scd);
         m_ShearCapEngineer.TweakShearCapacityOutboardOfCriticalSection(csPoi,pscd,&cs_scd);
      }
   }
}

void CEngAgentImp::GetRawShearCapacityDetails(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,SHEARCAPACITYDETAILS* pscd)
{
#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   ATLASSERT( liveLoadIntervalIdx <= intervalIdx ); // shear is only evaluted when live load is present
#endif

   *pscd = *ValidateShearCapacity(ls,intervalIdx,poi);
}

Float64 CEngAgentImp::GetShearCapacity(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,
                                       const pgsPointOfInterest& poi, const GDRCONFIG& config )
{
   SHEARCAPACITYDETAILS scd;
   GetShearCapacityDetails( ls,intervalIdx,poi, config, &scd );
   return scd.pVn; 
}

void CEngAgentImp::GetShearCapacityDetails(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,
                                           const pgsPointOfInterest& poi, const GDRCONFIG& config,
                                           SHEARCAPACITYDETAILS* pscd)
{
   pgsPointOfInterest poi2( poi ); // POI where analysis is done...

   GetRawShearCapacityDetails(ls,intervalIdx,poi2,config,pscd);
}

void CEngAgentImp::GetRawShearCapacityDetails(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,
                                           const pgsPointOfInterest& poi, const GDRCONFIG& config,
                                           SHEARCAPACITYDETAILS* pscd)
{
   // Capacity is only computed in this stage and load case
#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   ATLASSERT( intervalIdx == liveLoadIntervalIdx ); // shear is only computed in the stage when live load is applied
#pragma Reminder("REVIEW") // may need to evalate shear in other stages for spliced girders
#endif

   m_ShearCapEngineer.ComputeShearCapacity(ls,intervalIdx,poi,config,pscd);
}

Float64 CEngAgentImp::GetFpc(const pgsPointOfInterest& poi)
{
   FPCDETAILS fd;
   GetFpcDetails( poi,&fd );
   return fd.fpc; 
}

void CEngAgentImp::GetFpcDetails(const pgsPointOfInterest& poi, FPCDETAILS* pmcd)
{
   *pmcd = *ValidateFpc( poi );
}

Float64 CEngAgentImp::GetFpc(const pgsPointOfInterest& poi, const GDRCONFIG& config)
{
   FPCDETAILS fd;
   GetFpcDetails( poi,config, &fd );
   return fd.fpc; 
}

void CEngAgentImp::GetFpcDetails(const pgsPointOfInterest& poi, const GDRCONFIG& config, FPCDETAILS* pfpc)
{
   m_ShearCapEngineer.ComputeFpc(poi,&config,pfpc);
}

ZoneIndexType CEngAgentImp::GetCriticalSectionZoneIndex(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi)
{
   const std::vector<CRITSECTDETAILS>& vCSDetails(ValidateShearCritSection(limitState,poi.GetSegmentKey()));
   Float64 x = poi.GetDistFromStart();
 
   std::vector<CRITSECTDETAILS>::const_iterator iter(vCSDetails.begin());
   std::vector<CRITSECTDETAILS>::const_iterator end(vCSDetails.end());
   for ( ; iter != end; iter++ )
   {
      const CRITSECTDETAILS& csDetails(*iter);
      if ( InRange(csDetails.Start,x,csDetails.End) )
         return (ZoneIndexType)(iter - vCSDetails.begin());
   }

   return INVALID_INDEX;
}

void CEngAgentImp::GetCriticalSectionZoneBoundary(pgsTypes::LimitState ls,const CSegmentKey& segmentKey,ZoneIndexType csZoneIdx,Float64* pStart,Float64* pEnd)
{
   const std::vector<CRITSECTDETAILS>& vCSDetails(ValidateShearCritSection(ls,segmentKey));
   const CRITSECTDETAILS& details = vCSDetails[csZoneIdx];
   *pStart = details.Start;
   *pEnd   = details.End;
}

std::vector<Float64> CEngAgentImp::GetCriticalSections(pgsTypes::LimitState limitState,const CGirderKey& girderKey)
{
   const std::vector<CRITSECTDETAILS>& csDetails(GetCriticalSectionDetails(limitState,girderKey));
   return GetCriticalSectionFromDetails(csDetails);
}

std::vector<Float64> CEngAgentImp::GetCriticalSectionFromDetails(const std::vector<CRITSECTDETAILS>& csDetails)
{
   std::vector<Float64> csLoc;

   GET_IFACE(IPointOfInterest,pPoi);

   std::vector<CRITSECTDETAILS>::const_iterator iter(csDetails.begin());
   std::vector<CRITSECTDETAILS>::const_iterator end(csDetails.end());
   for ( ; iter != end; iter++ )
   {
      const CRITSECTDETAILS& details(*iter);
      if ( details.bAtFaceOfSupport )
      {
         Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(details.poiFaceOfSupport);
         csLoc.push_back(Xg);
      }
      else
      {
         Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(details.pCriticalSection->Poi);
         csLoc.push_back(Xg);
      }
   }

   return csLoc;
}

std::vector<Float64> CEngAgentImp::GetCriticalSections(pgsTypes::LimitState limitState,const CGirderKey& girderKey,const GDRCONFIG& config)
{
   std::vector<CRITSECTDETAILS> csDetails(GetCriticalSectionDetails(limitState,girderKey,config));
   return GetCriticalSectionFromDetails(csDetails);
}

const std::vector<CRITSECTDETAILS>& CEngAgentImp::GetCriticalSectionDetails(pgsTypes::LimitState limitState,const CGirderKey& girderKey)
{
   return ValidateShearCritSection( limitState,girderKey );
}

std::vector<CRITSECTDETAILS> CEngAgentImp::GetCriticalSectionDetails(pgsTypes::LimitState limitState,const CGirderKey& girderKey,const GDRCONFIG& config)
{
   return CalculateShearCritSection(limitState,girderKey,config);
}

std::vector<SHEARCAPACITYDETAILS> CEngAgentImp::GetShearCapacityDetails(pgsTypes::LimitState ls, IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi)
{
   std::vector<SHEARCAPACITYDETAILS> details;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      SHEARCAPACITYDETAILS scd;
      GetShearCapacityDetails(ls,intervalIdx,poi,&scd);
      details.push_back(scd);
   }

   return details;
}

/////////////////////////////////////////////////////////////////////////////
// IGirderHaunch
Float64 CEngAgentImp::GetRequiredSlabOffset(const CSegmentKey& segmentKey)
{
   HAUNCHDETAILS details;
   GetHaunchDetails(segmentKey,&details);

   Float64 slab_offset = details.RequiredSlabOffset;

   // Round to nearest 1/4" (5 mm) per WSDOT BDM

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   if ( IS_SI_UNITS(pDisplayUnits) )
      slab_offset = RoundOff(slab_offset,::ConvertToSysUnits(5.0,unitMeasure::Millimeter) );
   else
      slab_offset = RoundOff(slab_offset,::ConvertToSysUnits(0.25, unitMeasure::Inch) );

   return slab_offset;
}

void CEngAgentImp::GetHaunchDetails(const CSegmentKey& segmentKey,HAUNCHDETAILS* pDetails)
{
   std::map<CSegmentKey,HAUNCHDETAILS>::iterator found;
   found = m_HaunchDetails.find(segmentKey);

   if ( found == m_HaunchDetails.end() )
   {
      // not found
      m_Designer.GetHaunchDetails(segmentKey,pDetails);

      std::pair<std::map<CSegmentKey,HAUNCHDETAILS>::iterator,bool> result;
      result = m_HaunchDetails.insert( std::make_pair(segmentKey,*pDetails) );
      ATLASSERT(result.second == true);
      return;
   }

   *pDetails = (*found).second;
}

/////////////////////////////////////////////////////////////////////////////
// IFabricationOptimization
void CEngAgentImp::GetFabricationOptimizationDetails(const CSegmentKey& segmentKey,FABRICATIONOPTIMIZATIONDETAILS* pDetails)
{
   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);

   // there is nothing to do if there aren't any strands
   if ( config.PrestressConfig.GetNStrands(pgsTypes::Straight) == 0 && config.PrestressConfig.GetNStrands(pgsTypes::Harped) == 0 )
      return;

   pDetails->Nt = config.PrestressConfig.GetNStrands(pgsTypes::Temporary);
   pDetails->Pjack = config.PrestressConfig.Pjack[pgsTypes::Temporary];

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   pDetails->TempStrandUsage = pStrands->TempStrandUsage;

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

      pgsLiftingAnalysisArtifact artifact1;
      GET_IFACE(IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
      lifting_checker.DesignLifting(segmentKey,config,pGirderLiftingPointsOfInterest,&artifact1,LOGGER);
      pDetails->L[PS_TTS] = artifact1.GetLeftOverhang();
   
      Float64 fci;
      Float64 fci_tens, fci_comp, fci_tens_wrebar;
      artifact1.GetRequiredConcreteStrength(&fci_comp,&fci_tens,&fci_tens_wrebar);
      bool minRebarRequired = fci_tens<0;
      fci = Max3(fci_tens, fci_comp, fci_tens_wrebar);
      pDetails->Fci[PS_TTS] = fci;

      // without TTS
      config.PrestressConfig.TempStrandUsage = pgsTypes::ttsPretensioned;
      StrandIndexType Nt = config.PrestressConfig.GetNStrands(pgsTypes::Temporary);
      Float64 Pjt = config.PrestressConfig.Pjack[pgsTypes::Temporary];

      config.PrestressConfig.ClearStrandFill(pgsTypes::Temporary);
      config.PrestressConfig.Pjack[pgsTypes::Temporary] = 0;

      pgsLiftingAnalysisArtifact artifact2;
      lifting_checker.DesignLifting(segmentKey,config,pGirderLiftingPointsOfInterest,&artifact2,LOGGER);
      pDetails->L[NO_TTS] = artifact2.GetLeftOverhang();
   
      artifact2.GetRequiredConcreteStrength(&fci_comp,&fci_tens,&fci_tens_wrebar);
      minRebarRequired = fci_tens<0;
      fci = Max3(fci_tens, fci_comp, fci_tens_wrebar);
      pDetails->Fci[NO_TTS] = fci;


      // post-tensioned TTS

      // lifting at location for NO_TTS (optional TTS)
      config.PrestressConfig.TempStrandUsage = pgsTypes::ttsPTBeforeLifting;
      config.PrestressConfig.Pjack[pgsTypes::Temporary] = Pjt;
      ConfigStrandFillVector rfillvec = pStrandGeom->ComputeStrandFill(segmentKey, pgsTypes::Temporary, Nt);
      config.PrestressConfig.SetStrandFill(pgsTypes::Temporary, rfillvec);

      HANDLINGCONFIG lift_config;
      lift_config.GdrConfig = config;
      lift_config.LeftOverhang = pDetails->L[NO_TTS];
      lift_config.RightOverhang = pDetails->L[NO_TTS];

      pgsLiftingAnalysisArtifact artifact3;
      lifting_checker.AnalyzeLifting(segmentKey,lift_config,pGirderLiftingPointsOfInterest,&artifact3);
      pDetails->L[PT_TTS_OPTIONAL] = artifact3.GetLeftOverhang();

      artifact3.GetRequiredConcreteStrength(&fci_comp,&fci_tens,&fci_tens_wrebar);
      minRebarRequired = fci_tens<0;
      fci = Max3(fci_tens, fci_comp, fci_tens_wrebar);
      pDetails->Fci[PT_TTS_OPTIONAL] = fci;

      // lifting at location for PS_TTS (required TTS)
      config.PrestressConfig.TempStrandUsage = pgsTypes::ttsPTBeforeLifting;
      config.PrestressConfig.Pjack[pgsTypes::Temporary] = Pjt;
      config.PrestressConfig.SetStrandFill(pgsTypes::Temporary, rfillvec);

      lift_config.GdrConfig = config;
      lift_config.LeftOverhang = pDetails->L[PS_TTS];
      lift_config.RightOverhang = pDetails->L[PS_TTS];

      pgsLiftingAnalysisArtifact artifact4;
      lifting_checker.AnalyzeLifting(segmentKey,lift_config,pGirderLiftingPointsOfInterest,&artifact4);
      pDetails->L[PT_TTS_REQUIRED] = artifact4.GetLeftOverhang();
   
      artifact4.GetRequiredConcreteStrength(&fci_comp,&fci_tens,&fci_tens_wrebar);
      minRebarRequired = fci_tens<0;
      fci = Max3(fci_tens, fci_comp, fci_tens_wrebar);
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

   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(segmentKey) );
   GDRCONFIG config_WithoutTTS;
   config_WithoutTTS = config;
   config_WithoutTTS.PrestressConfig.Pjack[pgsTypes::Temporary] = 0;
   config_WithoutTTS.PrestressConfig.ClearStrandFill(pgsTypes::Temporary);

   Float64 min_stress_WithoutTTS = DBL_MAX;
   Float64 max_stress_WithoutTTS = -DBL_MAX;
   Float64 min_stress_WithTTS = DBL_MAX;
   Float64 max_stress_WithTTS = -DBL_MAX;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPOI.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPOI.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest poi = *iter;

      Float64 fTopLimitStateMin,fTopLimitStateMax;
      pLS->GetStress(pgsTypes::ServiceI,releaseIntervalIdx,poi,pgsTypes::TopGirder,false,bat,&fTopLimitStateMin,&fTopLimitStateMax);

      Float64 fBotLimitStateMin,fBotLimitStateMax;
      pLS->GetStress(pgsTypes::ServiceI,releaseIntervalIdx,poi,pgsTypes::BottomGirder,false,bat,&fBotLimitStateMin,&fBotLimitStateMax);

      Float64 fTopPre_WithoutTTS = pPS->GetDesignStress(releaseIntervalIdx,poi,pgsTypes::TopGirder,config_WithoutTTS);
      Float64 fBotPre_WithoutTTS = pPS->GetDesignStress(releaseIntervalIdx,poi,pgsTypes::BottomGirder,config_WithoutTTS);

      Float64 fTopMin_WithoutTTS = fTopLimitStateMin + fTopPre_WithoutTTS;
      Float64 fTopMax_WithoutTTS = fTopLimitStateMax + fTopPre_WithoutTTS;

      Float64 fBotMin_WithoutTTS = fBotLimitStateMin + fBotPre_WithoutTTS;
      Float64 fBotMax_WithoutTTS = fBotLimitStateMax + fBotPre_WithoutTTS;

      min_stress_WithoutTTS = Min3(fBotMin_WithoutTTS,fTopMin_WithoutTTS,min_stress_WithoutTTS);
      max_stress_WithoutTTS = Max3(fBotMax_WithoutTTS,fTopMax_WithoutTTS,max_stress_WithoutTTS);
   }

   GET_IFACE(IAllowableConcreteStress,pAllowStress);
   pgsPointOfInterest dummyPOI(segmentKey,0.0);
   Float64 c = -pAllowStress->GetAllowableCompressiveStressCoefficient(dummyPOI,releaseIntervalIdx,pgsTypes::ServiceI);
   Float64 t, fmax;
   bool bfMax;
   pAllowStress->GetAllowableTensionStressCoefficient(dummyPOI,releaseIntervalIdx,pgsTypes::ServiceI,false/*without rebar*/,&t,&bfMax,&fmax);

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
            pAllowStress->GetAllowableTensionStressCoefficient(dummyPOI,releaseIntervalIdx,pgsTypes::ServiceI,true/*with rebar*/,&talt,&bCheckMaxAlt,&fMaxAlt);
            fc_reqd_tension = pow(max_stress_WithoutTTS/talt,2);
         }
      }
   }

   if ( fc_reqd_tension < 0 )
      pDetails->Fci_FormStripping_WithoutTTS = -1;
   else
      pDetails->Fci_FormStripping_WithoutTTS = max(fc_reqd_tension,fc_reqd_compression);

   /////////////////////////////////////////////////////////////
   // Shipping with equal cantilevers
   /////////////////////////////////////////////////////////////
   GET_IFACE(IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);

   // Use factory to create appropriate hauling checker
   pgsGirderHandlingChecker checker_factory(m_pBroker,m_StatusGroupID);
   std::auto_ptr<pgsGirderHaulingChecker> hauling_checker( checker_factory.CreateGirderHaulingChecker() );

   config = pBridge->GetSegmentConfiguration(segmentKey);
   config.PrestressConfig.TempStrandUsage = pgsTypes::ttsPretensioned;

   bool bResult;
   std::auto_ptr<pgsHaulingAnalysisArtifact> hauling_artifact_base ( hauling_checker->DesignHauling(segmentKey,config,true,true,pGirderHaulingPointsOfInterest,&bResult,LOGGER));

   // Constructibility is wsdot-based. Cast artifact
   pgsWsdotHaulingAnalysisArtifact* hauling_artifact = dynamic_cast<pgsWsdotHaulingAnalysisArtifact*>(hauling_artifact_base.get());
   if (hauling_artifact==NULL)
   {
      ATLASSERT(0); // Should check that hauling analysis is WSDOT before we get here
      return;
   }

   if ( !bResult )
   {
      pDetails->bTempStrandsRequiredForShipping = true;
      return;
   }

   GET_IFACE(IMaterials,pMaterial);
   Float64 fcMax = pMaterial->GetSegmentFc(segmentKey,haulSegmentIntervalIdx);

   Float64 fcReqd = -1;

   ATLASSERT( IsEqual(hauling_artifact->GetLeadingOverhang(),hauling_artifact->GetTrailingOverhang()) );

   GET_IFACE(IGirderHaulingSpecCriteria,pCriteria);
   Float64 min_location = max(pCriteria->GetMinimumHaulingSupportLocation(segmentKey,pgsTypes::metStart),
                              pCriteria->GetMinimumHaulingSupportLocation(segmentKey,pgsTypes::metEnd));

   Float64 location_accuracy = pCriteria->GetHaulingSupportLocationAccuracy();

   bool bDone = false;
   bool bUsingBigInc = true;
   Float64 bigInc = 4*location_accuracy;
   Float64 smallInc = location_accuracy;
   Float64 inc = bigInc;
   Float64 L = hauling_artifact->GetLeadingOverhang();
   while ( !bDone )
   {
      L += inc;

      HANDLINGCONFIG hauling_config;
      hauling_config.GdrConfig = config;
      hauling_config.LeftOverhang = L;
      hauling_config.RightOverhang = L;

      std::auto_ptr<pgsHaulingAnalysisArtifact> hauling_artifact2( hauling_checker->AnalyzeHauling(segmentKey,hauling_config,pGirderHaulingPointsOfInterest) );
   
      Float64 fc;
      Float64 fc_tens, fc_comp, fc_tens_wrebar;
      hauling_artifact2->GetRequiredConcreteStrength(&fc_comp,&fc_tens,&fc_tens_wrebar);
      bool minRebarRequired = fc_tens<0;
      fc = Max3(fc_tens, fc_comp, fc_tens_wrebar);

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

      fcReqd = max(fc,fcReqd);
   }

   pDetails->Lmin = hauling_artifact->GetLeadingOverhang();
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
      hauling_config.GdrConfig = config;
      hauling_config.LeftOverhang = trailing_overhang;
      hauling_config.RightOverhang = leading_overhang;

      std::auto_ptr<pgsHaulingAnalysisArtifact> hauling_artifact2_base( hauling_checker->AnalyzeHauling(segmentKey,hauling_config,pGirderHaulingPointsOfInterest) );

      pgsWsdotHaulingAnalysisArtifact* hauling_artifact2 = dynamic_cast<pgsWsdotHaulingAnalysisArtifact*>(hauling_artifact2_base.get());
      if (hauling_artifact2==NULL)
      {
         ATLASSERT(0); // Should check that hauling analysis is WSDOT before we get here
         return;
      }

      Float64 fc;
      Float64 fc_tens, fc_comp, fc_tens_wrebar;
      hauling_artifact2->GetRequiredConcreteStrength(&fc_comp,&fc_tens,&fc_tens_wrebar);
      bool minRebarRequired = fc_tens<0;
      fc = Max3(fc_tens, fc_comp, fc_tens_wrebar);

      // check factors of safety
      Float64 FSr  = hauling_artifact2->GetFsRollover();
      Float64 FScr = hauling_artifact2->GetMinFsForCracking();
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

      fcReqd = max(fc,fcReqd);
   }

   pDetails->LUmin = leading_overhang;
   pDetails->LUmax = trailing_overhang;
   pDetails->LUsum = overhang_sum;

   pDetails->Fc = fcReqd;
}

/////////////////////////////////////////////////////////////////////////////
// IArtifact
const pgsGirderArtifact* CEngAgentImp::GetGirderArtifact(const CGirderKey& girderKey)
{
   ValidateArtifacts(girderKey);
   return FindArtifact(girderKey);
}

const pgsSegmentArtifact* CEngAgentImp::GetSegmentArtifact(const CSegmentKey& segmentKey)
{
   const pgsGirderArtifact* pArtifact = GetGirderArtifact(segmentKey);
   return pArtifact->GetSegmentArtifact(segmentKey.segmentIndex);
}

const pgsClosurePourArtifact* CEngAgentImp::GetClosurePourArtifact(const CSegmentKey& closurePourKey)
{
   const pgsGirderArtifact* pArtifact = GetGirderArtifact(closurePourKey);
   return pArtifact->GetClosurePourArtifact(closurePourKey.segmentIndex);
}

const pgsRatingArtifact* CEngAgentImp::GetRatingArtifact(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIndex)
{
   ValidateRatingArtifacts(girderKey,ratingType,vehicleIndex);
   return FindRatingArtifact(girderKey,ratingType,vehicleIndex);
}

const pgsDesignArtifact* CEngAgentImp::CreateDesignArtifact(const CGirderKey& girderKey,arDesignOptions options)
{
   std::map<CGirderKey,pgsDesignArtifact>::size_type cRemove = m_DesignArtifacts.erase(girderKey);
   ATLASSERT( cRemove == 0 || cRemove == 1 );

   std::pair<std::map<CGirderKey,pgsDesignArtifact>::iterator,bool> retval;

#if defined _DEBUG
   GET_IFACE(IDocumentType,pDocType);
   ATLASSERT(pDocType->IsPGSuperDocument()); // design is only for PGSuper
#endif

   CSegmentKey segmentKey(girderKey,0); // there is only one segment per girder for PGSuper (use segment index 0)
   pgsDesignArtifact artifact(segmentKey);
   try 
   {
      artifact = m_Designer.Design(girderKey,options);
   }
   catch (pgsDesignArtifact::Outcome outcome)
   {
      if ( outcome == pgsDesignArtifact::DesignCancelled )
         return NULL;
      else 
         artifact.SetOutcome(outcome);
   }

   retval = m_DesignArtifacts.insert(std::make_pair(girderKey,artifact));
   return &((*retval.first).second);
}

const pgsDesignArtifact* CEngAgentImp::GetDesignArtifact(const CGirderKey& girderKey)
{
   std::map<CGirderKey,pgsDesignArtifact>::iterator found;
   found = m_DesignArtifacts.find(girderKey);
   if ( found == m_DesignArtifacts.end() )
      return 0;

   return &((*found).second);
}

void CEngAgentImp::CreateLiftingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 supportLoc,pgsLiftingAnalysisArtifact* pArtifact)
{
   bool bCreate = false;

   typedef std::map<CSegmentKey, std::map<Float64,pgsLiftingAnalysisArtifact,Float64_less> >::iterator iter_type;
   iter_type found_gdr;
   found_gdr = m_LiftingArtifacts.find(segmentKey);
   if ( found_gdr != m_LiftingArtifacts.end() )
   {
      std::map<Float64,pgsLiftingAnalysisArtifact,Float64_less>::iterator found;
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

      std::map<Float64,pgsLiftingAnalysisArtifact,Float64_less> artifacts;
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
      GET_IFACE(IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
      checker.AnalyzeLifting(segmentKey,config,pGirderLiftingPointsOfInterest,pArtifact);

      (*found_gdr).second.insert( std::make_pair(supportLoc,*pArtifact) );
   }
}

const pgsHaulingAnalysisArtifact* CEngAgentImp::CreateHaulingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 leftSupportLoc,Float64 rightSupportLoc)
{
   const pgsHaulingAnalysisArtifact* pArtifact(NULL);

   bool bCreate = false;

   typedef std::map<CSegmentKey, std::map<Float64,boost::shared_ptr<pgsHaulingAnalysisArtifact>,Float64_less> >::iterator iter_type;
   iter_type found_gdr;
   found_gdr = m_HaulingArtifacts.find(segmentKey);
   if ( found_gdr != m_HaulingArtifacts.end() )
   {
      std::map<Float64,boost::shared_ptr<pgsHaulingAnalysisArtifact>,Float64_less>::iterator found;
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
      std::map<Float64,boost::shared_ptr<pgsHaulingAnalysisArtifact>,Float64_less> artifacts;
      std::pair<iter_type,bool> iter = m_HaulingArtifacts.insert( std::make_pair(segmentKey, artifacts) );
      found_gdr = iter.first;
      bCreate = true;
   }

   if ( bCreate )
   {
      HANDLINGCONFIG config;
      GET_IFACE(IPointOfInterest,pPOI);
      std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(segmentKey,POI_MIDSPAN) );
      pgsPointOfInterest poi( vPOI[0] );

      GET_IFACE(IBridge,pBridge);
      config.GdrConfig = pBridge->GetSegmentConfiguration(segmentKey);
      config.LeftOverhang  = leftSupportLoc;
      config.RightOverhang = rightSupportLoc;

      Float64 slabOffset = pBridge->GetSlabOffset(poi);

      GET_IFACE(IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);

      // Use factory to create appropriate hauling checker
      pgsGirderHandlingChecker checker_factory(m_pBroker,m_StatusGroupID);
      std::auto_ptr<pgsGirderHaulingChecker> hauling_checker( checker_factory.CreateGirderHaulingChecker() );

      boost::shared_ptr<pgsHaulingAnalysisArtifact> my_art (hauling_checker->AnalyzeHauling(segmentKey,config,pGirderHaulingPointsOfInterest));

      // Get const, uncounted pointer
      pArtifact = my_art.get();

      (*found_gdr).second.insert( std::make_pair(leftSupportLoc,my_art) );
   }

   return pArtifact;
}

/////////////////////////////////////////////////////////////////////////////
// ICrackedSection
void CEngAgentImp::GetCrackedSectionDetails(const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKEDSECTIONDETAILS* pCSD)
{
   *pCSD = *ValidateCrackedSectionDetails(poi,bPositiveMoment);
}

Float64 CEngAgentImp::GetIcr(const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   CRACKEDSECTIONDETAILS csd;
   GetCrackedSectionDetails( poi,bPositiveMoment,&csd );
   return csd.Icr;
}

std::vector<CRACKEDSECTIONDETAILS> CEngAgentImp::GetCrackedSectionDetails(const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment)
{
   std::vector<CRACKEDSECTIONDETAILS> details;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      CRACKEDSECTIONDETAILS csd;
      GetCrackedSectionDetails(poi,bPositiveMoment,&csd);
      details.push_back( csd );
   }

   return details;
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

   // invalidate shear capacities associated with rating limit states
   m_ShearCapacity[LimitStateToShearIndex(pgsTypes::StrengthI_Inventory)].clear();
   m_ShearCapacity[LimitStateToShearIndex(pgsTypes::StrengthI_Operating)].clear();
   m_ShearCapacity[LimitStateToShearIndex(pgsTypes::StrengthI_LegalRoutine)].clear();
   m_ShearCapacity[LimitStateToShearIndex(pgsTypes::StrengthI_LegalSpecial)].clear();
   m_ShearCapacity[LimitStateToShearIndex(pgsTypes::StrengthII_PermitRoutine)].clear();
   m_ShearCapacity[LimitStateToShearIndex(pgsTypes::StrengthII_PermitSpecial)].clear();

   m_CritSectionDetails[LimitStateToShearIndex(pgsTypes::StrengthI_Inventory)].clear();
   m_CritSectionDetails[LimitStateToShearIndex(pgsTypes::StrengthI_Operating)].clear();
   m_CritSectionDetails[LimitStateToShearIndex(pgsTypes::StrengthI_LegalRoutine)].clear();
   m_CritSectionDetails[LimitStateToShearIndex(pgsTypes::StrengthI_LegalSpecial)].clear();
   m_CritSectionDetails[LimitStateToShearIndex(pgsTypes::StrengthII_PermitRoutine)].clear();
   m_CritSectionDetails[LimitStateToShearIndex(pgsTypes::StrengthII_PermitSpecial)].clear();

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

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

// EngAgentImp.cpp : Implementation of CEngAgentImp
#include "stdafx.h"
#include "EngAgent.h"
#include "EngAgent_i.h"
#include "EngAgentImp.h"
#include "..\PGSuperException.h"
#include "PGSuperUnits.h"

#include "GirderHandlingChecker.h"
#include "GirderLiftingChecker.h"

#include <IFace\BeamFactory.h>
#include <IFace\StatusCenter.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\StatusItem.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\LoadFactors.h>

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

//-----------------------------------------------------------------------------
Float64 GetHorizPsComponent(IBroker* pBroker, 
                            const pgsPointOfInterest& poi,
                            const GDRCONFIG& config)
{
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   Float64 ss = pStrandGeometry->GetAvgStrandSlope( poi, config.PrestressConfig);
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
void CEngAgentImp::ValidateLosses(const pgsPointOfInterest& poi)
{
   std::map<PoiKey,LOSSDETAILS>::iterator found;
   PoiKey key(poi.GetID(),poi);
   ATLASSERT( m_PsLosses.find(key) == m_PsLosses.end() ); // it shouldn't exist if we are validating

   LOSSDETAILS details;
   BuildLosses(poi,&details);

   m_PsLosses.insert( std::make_pair(key,details) );
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

   m_PsLosses.clear();
   m_DesignLosses.Invalidate();
   m_PsForce.clear(); // if losses are gone, so are forces
}

//-----------------------------------------------------------------------------
void CEngAgentImp::ValidateLiveLoadDistributionFactors(SpanIndexType span,GirderIndexType gdr)
{
   if (!m_bAreDistFactorEngineersValidated)
   {
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

      // Create dist factor engineer
      if ( m_pDistFactorEngineer == NULL )
      {
         const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

         const GirderLibraryEntry* pGdr = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdr);

         CComPtr<IBeamFactory> pFactory;
         pGdr->GetBeamFactory(&pFactory);
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
            if (action==roaIgnore)
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
void CEngAgentImp::ValidateArtifacts(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   std::_tostringstream os;
   os << "Analyzing Span " << LABEL_SPAN(span) << " Girder " << LABEL_GIRDER(gdr) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   std::map<SpanGirderHashType,pgsGirderArtifact>::iterator found;
   found = m_CheckArtifacts.find(HashSpanGirder(span,gdr));
   if ( found != m_CheckArtifacts.end() )
      return; // We already have an artifact for this girder


   pgsGirderArtifact gdr_artifact = m_Designer.Check(span,gdr);

   m_CheckArtifacts.insert( std::make_pair(HashSpanGirder(span,gdr),gdr_artifact) );
}

//-----------------------------------------------------------------------------
void CEngAgentImp::ValidateRatingArtifacts(GirderIndexType gdrLineIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIndex)
{
   GET_IFACE(IRatingSpecification,pRatingSpec);
   if ( !pRatingSpec->IsRatingEnabled(ratingType) )
      return; // this type isn't enabled, so leave

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   std::_tostringstream os;
   os << "Load Rating Girder Line " << LABEL_GIRDER(gdrLineIdx) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   std::map<RatingArtifactKey,pgsRatingArtifact>::iterator found;
   RatingArtifactKey key(gdrLineIdx,vehicleIndex);
   found = m_RatingArtifacts[ratingType].find(key);
   if ( found != m_RatingArtifacts[ratingType].end() )
      return; // We already have an artifact for this girder


   pgsRatingArtifact artifact = m_LoadRater.Rate(gdrLineIdx,ratingType,vehicleIndex);

   m_RatingArtifacts[ratingType].insert( std::make_pair(key,artifact) );
}

//-----------------------------------------------------------------------------
LOSSDETAILS* CEngAgentImp::FindLosses(const pgsPointOfInterest& poi)
{
    std::map<PoiKey,LOSSDETAILS>::iterator found;
    PoiKey key(poi.GetID(),poi);
    found = m_PsLosses.find( key );
    if ( found == m_PsLosses.end() )
    {
       ValidateLosses(poi);
       found = m_PsLosses.find( key );
       ATLASSERT(found != m_PsLosses.end());
    }

    return &(*found).second;
}

//-----------------------------------------------------------------------------
pgsGirderArtifact* CEngAgentImp::FindArtifact(SpanIndexType span,GirderIndexType gdr)
{
    std::map<SpanGirderHashType,pgsGirderArtifact>::iterator found;
    found = m_CheckArtifacts.find( HashSpanGirder(span,gdr) );
    if ( found == m_CheckArtifacts.end() )
        return 0;

    return &(*found).second;
}

pgsRatingArtifact* CEngAgentImp::FindRatingArtifact(GirderIndexType gdrLineIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIndex)
{
    std::map<RatingArtifactKey,pgsRatingArtifact>::iterator found;
    RatingArtifactKey key(gdrLineIdx,vehicleIndex);
    found = m_RatingArtifacts[ratingType].find( key );
    if ( found == m_RatingArtifacts[ratingType].end() )
        return 0;

    return &(*found).second;
}

//-----------------------------------------------------------------------------
const MINMOMENTCAPDETAILS* CEngAgentImp::ValidateMinMomentCapacity(pgsTypes::Stage stage,
                                                                   const pgsPointOfInterest& poi,
                                                                   bool bPositiveMoment)
{
   std::map<PoiKey,MINMOMENTCAPDETAILS>::iterator found;
   std::map<PoiKey,MINMOMENTCAPDETAILS>* pMap;

   if ( poi.GetID() != INVALID_ID )
   {
      pMap = ( stage == pgsTypes::BridgeSite1 ) ? &m_NonCompositeMinMomentCapacity[bPositiveMoment]
                                                : &m_CompositeMinMomentCapacity[bPositiveMoment];

      PoiKey key(poi.GetID(),poi);
      found = pMap->find( key );
      if ( found != pMap->end() )
         return &((*found).second); // capacities have already been computed
   }

   MINMOMENTCAPDETAILS mmcd;
   m_MomentCapEngineer.ComputeMinMomentCapacity(stage,poi,bPositiveMoment,&mmcd);

   PoiKey key(poi.GetID(),poi);
   std::pair<std::map<PoiKey,MINMOMENTCAPDETAILS>::iterator,bool> retval;
   retval = pMap->insert( std::make_pair(key,mmcd) );
   return &((*(retval.first)).second);
}

//-----------------------------------------------------------------------------
const CRACKINGMOMENTDETAILS* CEngAgentImp::ValidateCrackingMoments(pgsTypes::Stage stage,
                                                                   const pgsPointOfInterest& poi,
                                                                   bool bPositiveMoment)
{
   std::map<PoiKey,CRACKINGMOMENTDETAILS>::iterator found;
   std::map<PoiKey,CRACKINGMOMENTDETAILS>* pMap;

   if ( poi.GetID() != INVALID_ID )
   {
      pMap = ( stage == pgsTypes::BridgeSite1 ) ? &m_NonCompositeCrackingMoment[bPositiveMoment] 
                                                : &m_CompositeCrackingMoment[bPositiveMoment];

      PoiKey key(poi.GetID(),poi);
      found = pMap->find( key );
      if ( found != pMap->end() )
         return &( (*found).second ); // capacities have already been computed
   }

   CRACKINGMOMENTDETAILS cmd;
   m_MomentCapEngineer.ComputeCrackingMoment(stage,poi,bPositiveMoment,&cmd);

   PoiKey key(poi.GetID(),poi);
   std::pair<std::map<PoiKey,CRACKINGMOMENTDETAILS>::iterator,bool> retval;
   retval = pMap->insert( std::make_pair(key,cmd) );
   return &((*(retval.first)).second);
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* CEngAgentImp::ValidateMomentCapacity(pgsTypes::Stage stage,
                                                                  const pgsPointOfInterest& poi,
                                                                  bool bPositiveMoment)
{
   MOMENTCAPACITYDETAILS mcd = ComputeMomentCapacity(stage,poi,bPositiveMoment);
   return StoreMomentCapacityDetails(stage,poi,bPositiveMoment,mcd,stage == pgsTypes::BridgeSite1 ? m_NonCompositeMomentCapacity[bPositiveMoment] : m_CompositeMomentCapacity[bPositiveMoment]);
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* CEngAgentImp::ValidateMomentCapacity(pgsTypes::Stage stage,
                                                                  const pgsPointOfInterest& poi,
                                                                  const GDRCONFIG& config,
                                                                  bool bPositiveMoment)
{
   MOMENTCAPACITYDETAILS mcd = ComputeMomentCapacity(stage,poi,config,bPositiveMoment);
   return StoreMomentCapacityDetails(stage,poi,bPositiveMoment,mcd,stage == pgsTypes::BridgeSite1 ? m_TempNonCompositeMomentCapacity[bPositiveMoment] : m_TempCompositeMomentCapacity[bPositiveMoment]);
}

//-----------------------------------------------------------------------------
pgsPointOfInterest CEngAgentImp::GetEquivalentPointOfInterest(pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();
   Float64 dist_from_start = poi.GetDistFromStart();

   pgsPointOfInterest search_poi = poi;

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,pGirder);

   // check for symmetry
   if ( pGirder->IsSymmetric(stage,span,gdr) )
   {
      Float64 girder_length = pBridge->GetGirderLength(span,gdr);

      if ( girder_length/2 < dist_from_start )
      {
         // we are past mid-point of a symmetric girder
         // get the poi that is a mirror about the centerline of the girder

         GET_IFACE(IPointOfInterest,pPOI);
         Float64 dist_from_end = girder_length - dist_from_start;
         search_poi = pPOI->GetPointOfInterest(stage,span,gdr,dist_from_end);

         if ( search_poi.GetID() == INVALID_ID ) // a symmetric POI was not actually found
            search_poi = poi;
      }
   }

   return search_poi;
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* CEngAgentImp::GetCachedMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   return GetCachedMomentCapacity(stage,poi,bPositiveMoment,stage == pgsTypes::BridgeSite1 ? m_NonCompositeMomentCapacity[bPositiveMoment] : m_CompositeMomentCapacity[bPositiveMoment]);
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* CEngAgentImp::GetCachedMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment)
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

   return GetCachedMomentCapacity(stage,poi,bPositiveMoment,stage == pgsTypes::BridgeSite1 ? m_TempNonCompositeMomentCapacity[bPositiveMoment] : m_TempCompositeMomentCapacity[bPositiveMoment]);
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* CEngAgentImp::GetCachedMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,const MomentCapacityDetailsContainer& container)
{
   // if the beam has some symmetry, we can use the results for another poi...
   // get the equivalent, mirrored POI

   // don't do this for negative moment... the symmetry check just isn't working right


   pgsPointOfInterest search_poi = (bPositiveMoment ? GetEquivalentPointOfInterest(stage,poi) : poi);

   MomentCapacityDetailsContainer::const_iterator found;

   // if this is a real POI, then see if we've already computed results
   if ( search_poi.GetID() != INVALID_ID )
   {
      PoiKey key(search_poi.GetID(),search_poi);
      found = container.find( key );
      if ( found != container.end() )
         return &( (*found).second ); // capacities have already been computed
   }

   return NULL;
}

//-----------------------------------------------------------------------------
MOMENTCAPACITYDETAILS CEngAgentImp::ComputeMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   MOMENTCAPACITYDETAILS mcd;
   m_MomentCapEngineer.ComputeMomentCapacity(stage,poi,bPositiveMoment,&mcd);

   return mcd;
}

//-----------------------------------------------------------------------------
MOMENTCAPACITYDETAILS CEngAgentImp::ComputeMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   MOMENTCAPACITYDETAILS mcd;
   m_MomentCapEngineer.ComputeMomentCapacity(stage,poi,config,bPositiveMoment,&mcd);

   return mcd;
}

//-----------------------------------------------------------------------------
const MOMENTCAPACITYDETAILS* CEngAgentImp::StoreMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,const MOMENTCAPACITYDETAILS& mcd,MomentCapacityDetailsContainer& container)
{
   // if the beam has some symmetry, we can use the results for another poi...
   // get the equivalent, mirrored POI

   pgsPointOfInterest search_poi = (bPositiveMoment ? GetEquivalentPointOfInterest(stage,poi) : poi);

   PoiKey key(search_poi.GetID(),search_poi);
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
   CollectionIndexType size = sizeof(m_ShearCapacity)/sizeof(std::map<PoiKey,SHEARCAPACITYDETAILS>);
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
                                                                pgsTypes::Stage stage, 
                                                                const pgsPointOfInterest& poi)
{
   std::map<PoiKey,SHEARCAPACITYDETAILS>::iterator found;

   CollectionIndexType idx = LimitStateToShearIndex(ls);

   if ( poi.GetID() != INVALID_ID )
   {
      PoiKey key(poi.GetID(),poi);
      found = m_ShearCapacity[idx].find( key );

      if ( found != m_ShearCapacity[idx].end() )
         return &( (*found).second ); // capacities have already been computed
   }

   SHEARCAPACITYDETAILS scd;
   m_ShearCapEngineer.ComputeShearCapacity(ls,stage,poi,&scd);

   PoiKey key(poi.GetID(),poi);
   std::pair<std::map<PoiKey,SHEARCAPACITYDETAILS>::iterator,bool> retval;
   retval = m_ShearCapacity[idx].insert( std::make_pair(key,scd) );
   return &((*(retval.first)).second);
}

//-----------------------------------------------------------------------------
const FPCDETAILS* CEngAgentImp::ValidateFpc(const pgsPointOfInterest& poi)
{
   std::map<PoiKey,FPCDETAILS>::iterator found;

   if ( poi.GetID() != INVALID_ID )
   {
      PoiKey key(poi.GetID(),poi);
      found = m_Fpc.find( key );
      if ( found != m_Fpc.end() )
         return &( (*found).second ); // already been computed
   }

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetGirderConfiguration(poi.GetSpan(),poi.GetGirder());
   Float64 slabOffset = pBridge->GetSlabOffset(poi);

   FPCDETAILS mcd;
   m_ShearCapEngineer.ComputeFpc(poi,config,&mcd);

   PoiKey key(poi.GetID(),poi);
   std::pair<std::map<PoiKey,FPCDETAILS>::iterator,bool> retval;
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
   CollectionIndexType size = sizeof(m_CritSectionDetails)/sizeof(std::map<SpanGirderHashType,CRITSECTDETAILS>);
   for (CollectionIndexType idx = 0; idx < size; idx++ )
   {
      m_CritSectionDetails[idx].clear();
   }
}

//-----------------------------------------------------------------------------
const CRITSECTDETAILS* CEngAgentImp::ValidateShearCritSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr)
{
   USES_CONVERSION;
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   // LRFD 2004 and later, critical section is only a function of dv
   if ( lrfdVersionMgr::ThirdEdition2004 <= pSpecEntry->GetSpecificationType() )
      limitState = pgsTypes::StrengthI;

   std::map<SpanGirderHashType,CRITSECTDETAILS>::iterator found;
   found = m_CritSectionDetails[LimitStateToShearIndex(limitState)].find(HashSpanGirder(span,gdr));
   if ( found != m_CritSectionDetails[LimitStateToShearIndex(limitState)].end() )
      return &(found->second); // We already have the value for this girder

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IStageMap,pStageMap);
   std::_tostringstream os;
   os << _T("Computing ") << OLE2T(pStageMap->GetLimitStateName(limitState)) << _T(" critical section for shear for Span ")
      << LABEL_SPAN(span) << _T(" Girder ")<< LABEL_GIRDER(gdr) << std::ends;

   pProgress->UpdateMessage( os.str().c_str() );

   // calculations
   CRITSECTDETAILS scsd;
   CalculateShearCritSection(limitState,span, gdr, &scsd);

   std::pair<std::map<SpanGirderHashType,CRITSECTDETAILS>::iterator,bool> retval;
   retval =    m_CritSectionDetails[LimitStateToShearIndex(limitState)].insert( std::make_pair(HashSpanGirder(span,gdr),scsd) );
   return &((retval.first)->second);
}

//-----------------------------------------------------------------------------
void CEngAgentImp::CalculateShearCritSection(pgsTypes::LimitState limitState,
                                             SpanIndexType span, 
                                             GirderIndexType gdr, 
                                             CRITSECTDETAILS* pdtls)
{
   GDRCONFIG config;
   Float64 slabOffset = -9999;
   CalculateShearCritSection(limitState,span,gdr,false,config,pdtls);
}

//-----------------------------------------------------------------------------
void CEngAgentImp::CalculateShearCritSection(pgsTypes::LimitState limitState,
                                             SpanIndexType span, 
                                             GirderIndexType gdr, 
                                             const GDRCONFIG& config,
                                             CRITSECTDETAILS* pdtls)
{
   CalculateShearCritSection(limitState,span,gdr,true,config,pdtls);
}

//-----------------------------------------------------------------------------
void CEngAgentImp::CalculateShearCritSection(pgsTypes::LimitState limitState,
                                             SpanIndexType span, 
                                             GirderIndexType gdr, 
                                             bool bUseConfig,
                                             const GDRCONFIG& config,
                                             CRITSECTDETAILS* pdtls)
{
   pdtls->bAtLeftFaceOfSupport = false;
   pdtls->bAtRightFaceOfSupport = false;

   // need to check reactions... if uplift then CS is at face of support
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   BridgeAnalysisType bat;
   if ( analysisType == pgsTypes::Envelope )
      bat = MinSimpleContinuousEnvelope;
   else if ( analysisType == pgsTypes::Simple )
      bat = SimpleSpan;
   else
      bat = ContinuousSpan;

   GET_IFACE(IPointOfInterest, pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_FACEOFSUPPORT);
   ATLASSERT(vPoi.size() == 2);
   pgsPointOfInterest leftFaceOfSupport(vPoi.front());
   pgsPointOfInterest rightFaceOfSupport(vPoi.back());

   GET_IFACE(ILimitStateForces,pLSForces);
   Float64 leftRmin,leftRmax,rightRmin,rightRmax;
   pLSForces->GetReaction(limitState,pgsTypes::BridgeSite3,span,  gdr,bat,true,&leftRmin, &leftRmax);
   pLSForces->GetReaction(limitState,pgsTypes::BridgeSite3,span+1,gdr,bat,true,&rightRmin,&rightRmax);

   bool bSkipLeft = false;
   bool bSkipRight = false;
   if ( leftRmin <= 0 )
   {
      // this is an uplift reaction... use the face of support for the critical section
      bSkipLeft = true;
      pdtls->bAtLeftFaceOfSupport = true;
      pdtls->poiLeftFaceOfSupport = leftFaceOfSupport;
   }

   if ( rightRmin <= 0 )
   {
      bSkipRight = true;
      pdtls->bAtRightFaceOfSupport = true;
      pdtls->poiRightFaceOfSupport = rightFaceOfSupport;
   }

   if ( bSkipLeft && bSkipRight )
      return;

   GET_IFACE(IBridge,pBridge);
   Float64 left_end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   Float64 right_end_size = pBridge->GetGirderEndConnectionLength(span,gdr);
   Float64 span_length = pBridge->GetSpanLength(span,gdr);
   Float64 gdr_length  = pBridge->GetGirderLength(span,gdr);

   Float64 left_support_width  = pBridge->GetGirderStartSupportWidth(span,gdr);
   Float64 right_support_width = pBridge->GetGirderEndSupportWidth(span,gdr);

   GET_IFACE(IGirder,pGirder);
   Float64 Hl = pGirder->GetHeight(pgsPointOfInterest(span,gdr,0.00));
   Float64 Hr = pGirder->GetHeight(pgsPointOfInterest(span,gdr,gdr_length));

   // Get points of interest
   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> pois = pIPOI->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_ALLACTIONS,POIFIND_OR);
   std::vector<pgsPointOfInterest>::iterator iter;

   // only use POI's within 2.5H from the face of the supports
   Float64 start = 2.5*Hl + left_end_size + left_support_width/2;
   Float64 end = gdr_length-2.5*Hr - right_end_size - right_support_width/2;
   vPoi.clear();
   for ( iter = pois.begin(); iter != pois.end(); iter++ )
   {
      pgsPointOfInterest& poi = *iter;

      Float64 loc = poi.GetDistFromStart();
      if ( InRange(left_end_size,loc,start) || InRange(end,loc,gdr_length-right_end_size) )
         vPoi.push_back(poi);
   }

   mathPwLinearFunction2dUsingPoints theta;
   mathPwLinearFunction2dUsingPoints dv;
   mathPwLinearFunction2dUsingPoints dv_cos_theta;
   mathPwLinearFunction2dUsingPoints left_intercept;  // 45deg line from left bearing
   mathPwLinearFunction2dUsingPoints right_intercept; // 45deg line from right bearing

   // create a graph for dv and 0.5d*dv*cot(theta)
   // create intercept lines as well since we are looping on poi.
   LOG(endl<<_T("Critical Section Intercept graph"));
   LOG(_T("Location , Dv, Theta, 0.5*Dv*cotan(theta), Yl, Yr"));
   GET_IFACE(IShearCapacity,pShearCapacity);
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      CRITSECTIONDETAILSATPOI csdp;
      const pgsPointOfInterest& poi = *iter;

      Float64 x  = poi.GetDistFromStart();

      SHEARCAPACITYDETAILS scd;
      if ( bUseConfig )
         pShearCapacity->GetRawShearCapacityDetails(limitState,pgsTypes::BridgeSite3,poi,config,&scd);
      else
         pShearCapacity->GetRawShearCapacityDetails(limitState,pgsTypes::BridgeSite3,poi,&scd);

      // dv only
      Float64 dvl    = scd.dv;
      dv.AddPoint( gpPoint2d(x, dvl) );
      csdp.Poi              = poi;
      csdp.Dv               = dvl;

      // 0.5*dv*cot(theta)
      // theta is valid only if shear stress is in range
      if (scd.ShearInRange)
      {
         Float64 thetal = scd.Theta;
         Float64 dvtl   = 0.5*dvl/tan(thetal);
         dv_cos_theta.AddPoint( gpPoint2d(x, dvtl) );
         theta.AddPoint( gpPoint2d(x, thetal) );

         csdp.InRange          = true;
         csdp.Theta            = thetal;
         csdp.CotanThetaDv05   = dvtl;
      }
      else
      {
         csdp.InRange          = false;
         csdp.Theta            = 0.0;
         csdp.CotanThetaDv05   = 0.0;
      }

      // save details
      pdtls->PoiData.push_back(csdp);

      // intercept functions make a 45 degree angle upward from supports
      Float64 Yl = x - left_end_size - left_support_width/2;
      Float64 Yr = span_length + left_end_size - x - right_support_width/2;
      left_intercept.AddPoint( gpPoint2d(x,Yl) );
      right_intercept.AddPoint( gpPoint2d(x,Yr) );

      LOG(poi.GetDistFromStart()<<_T(", ")<<csdp.Dv<<_T(", ")<<csdp.Theta<<_T(", ")<<csdp.CotanThetaDv05<<_T(", ")<<Yl<<_T(", ")<<Yr);
   }

   LOG(_T("End of intercept values")<<endl);

   // determine intersections
   gpPoint2d p;
   Float64 x1;
   math1dRange range = dv.GetRange();  // range is same for all

   // create a dummy poi so we can save our location
   pgsPointOfInterest dumpoi;
   dumpoi.SetSpan(span);
   dumpoi.SetGirder(gdr);

   try
   {
      if ( !bSkipLeft )
      {
         // dv on left
         Int16 retval = dv.Intersect(left_intercept,range, &p );
         PRECONDITION( retval == 1 );
         x1 = p.X();
         dumpoi.SetDistFromStart(x1);
         pdtls->LeftCsDv.Poi            = dumpoi;
         pdtls->LeftCsDv.Dv             = dv.Evaluate(x1);
         pdtls->LeftCsDv.InRange        = theta.GetRange().IsInRange(x1);
         if (pdtls->LeftCsDv.InRange)
         {
            pdtls->LeftCsDv.Theta          = theta.Evaluate(x1);
            pdtls->LeftCsDv.CotanThetaDv05 = dv_cos_theta.Evaluate(x1);
         }
         else
         {
            pdtls->LeftCsDv.Theta          = 0.0;
            pdtls->LeftCsDv.CotanThetaDv05 = 0.0;
         }

         LOG(_T("Dv on Left Intersection = (")<<p.X()-left_end_size<<_T(", ")<<p.Y()<<_T(")"));

         // dv cot(theta) on left
         retval = dv_cos_theta.Intersect(left_intercept,range, &p );
         if( retval == 1 )
         {
            x1 = p.X();
            dumpoi.SetDistFromStart(x1);
            pdtls->LeftCsDvt.InRange        = true;
            pdtls->LeftCsDvt.Poi            = dumpoi;
            pdtls->LeftCsDvt.Dv             = dv.Evaluate(x1);
            pdtls->LeftCsDvt.Theta          = theta.Evaluate(x1);
            pdtls->LeftCsDvt.CotanThetaDv05 = dv_cos_theta.Evaluate(x1);

            LOG(_T(".5*Dv*cot(theta) on Left Intersection = (")<<p.X()-left_end_size<<_T(", ")<<p.Y()<<_T(")"));
         }
         else
         {
            pdtls->LeftCsDvt.InRange        = false;
            LOG(_T(".5*Dv*cot(theta) on Left Intersection = No Intersection"));
         }
      }

      // dv on Right
      if ( !bSkipRight )
      {
         Int16 retval = dv.Intersect(right_intercept,range, &p );
         PRECONDITION( retval == 1 );
         x1 = p.X();
         dumpoi.SetDistFromStart(x1);
         pdtls->RightCsDv.Poi            = dumpoi;
         pdtls->RightCsDv.Dv             = dv.Evaluate(x1);
         pdtls->RightCsDv.InRange        = theta.GetRange().IsInRange(x1);
         if (pdtls->RightCsDv.InRange)
         {
            pdtls->RightCsDv.Theta          = theta.Evaluate(x1);
            pdtls->RightCsDv.CotanThetaDv05 = dv_cos_theta.Evaluate(x1);
         }
         else
         {
            pdtls->RightCsDv.Theta          = 0.0;
            pdtls->RightCsDv.CotanThetaDv05 = 0.0;
         }
         LOG(_T("Dv on Right Intersection = (")<<span_length + right_end_size - p.X()<<_T(", ")<<p.Y()<<_T(")"));

         // dv cot(theta) on Right
         retval = dv_cos_theta.Intersect(right_intercept,range, &p );
         if ( retval == 1 )
         {
            x1 = p.X();
            dumpoi.SetDistFromStart(x1);
            pdtls->RightCsDvt.InRange        = true;
            pdtls->RightCsDvt.Poi            = dumpoi;
            pdtls->RightCsDvt.Dv             = dv.Evaluate(x1);
            pdtls->RightCsDvt.Theta          = theta.Evaluate(x1);
            pdtls->RightCsDvt.CotanThetaDv05 = dv_cos_theta.Evaluate(x1);
            LOG(_T(".5*Dv*cot(theta) on Right Intersection = (")<<span_length + right_end_size - p.X()<<_T(", ")<<p.Y()<<_T(")"));
         }
         else
         {
            pdtls->RightCsDvt.InRange        = false;
            LOG(_T(".5*Dv*cot(theta) on Right Intersection = No Intersection"));
         }
      }
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

const CRACKEDSECTIONDETAILS* CEngAgentImp::ValidateCrackedSectionDetails(const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   std::map<PoiKey,CRACKEDSECTIONDETAILS>::iterator found;

   int idx = (bPositiveMoment ? 0 : 1);
   if ( poi.GetID() != INVALID_ID )
   {
      PoiKey key(poi.GetID(),poi);
      found = m_CrackedSectionDetails[idx].find( key );

      if ( found != m_CrackedSectionDetails[idx].end() )
         return &( (*found).second ); // cracked section has already been computed
   }

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   std::_tostringstream os;
   os << _T("Analyzing cracked section for Span ")
      << LABEL_SPAN(poi.GetSpan()) << _T(" Girder ")
      << LABEL_GIRDER(poi.GetGirder()) << _T(" at ") << (LPCTSTR)FormatDimension(poi.GetDistFromStart(),pDisplayUnits->GetSpanLengthUnit())
      << _T(" from start of girder") << std::ends;

   pProgress->UpdateMessage( os.str().c_str() );

   CRACKEDSECTIONDETAILS csd;
   m_MomentCapEngineer.AnalyzeCrackedSection(poi,bPositiveMoment,&csd);

   PoiKey key(poi.GetID(),poi);
   std::pair<std::map<PoiKey,CRACKEDSECTIONDETAILS>::iterator,bool> retval;
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
   pBrokerInit->RegInterface( IID_IPrestressForce,              this );
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
   
   return S_OK;
}

STDMETHODIMP CEngAgentImp::Init2()
{
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
   LOSSDETAILS* pDetails = FindLosses(poi);
   ATLASSERT(pDetails != 0);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = pDetails->pLosses->TemporaryStrand_ElasticShorteningLosses();
   else
      val = pDetails->pLosses->PermanentStrand_ElasticShorteningLosses();

   return val;
}

Float64 CEngAgentImp::GetBeforeXferLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   LOSSDETAILS* pLossDetails = FindLosses(poi);
   ATLASSERT(pLossDetails != 0);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = pLossDetails->pLosses->TemporaryStrand_BeforeTransfer();
   else
      val = pLossDetails->pLosses->PermanentStrand_BeforeTransfer();

   return val;
}

Float64 CEngAgentImp::GetAfterXferLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   LOSSDETAILS* pLossDetails = FindLosses(poi);
   ATLASSERT(pLossDetails != 0);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = pLossDetails->pLosses->TemporaryStrand_AfterTransfer();
   else
      val = pLossDetails->pLosses->PermanentStrand_AfterTransfer();

   return val;
}

Float64 CEngAgentImp::GetLiftingLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   LOSSDETAILS* pLossDetails = FindLosses(poi);
   ATLASSERT(pLossDetails != 0);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = pLossDetails->pLosses->TemporaryStrand_AtLifting();
   else
      val = pLossDetails->pLosses->PermanentStrand_AtLifting();

   return val;
}

Float64 CEngAgentImp::GetShippingLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   LOSSDETAILS* pLossDetails = FindLosses(poi);
   ATLASSERT(pLossDetails != 0);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = pLossDetails->pLosses->TemporaryStrand_AtShipping();
   else
      val = pLossDetails->pLosses->PermanentStrand_AtShipping();

   return val;
}

Float64 CEngAgentImp::GetAfterTemporaryStrandInstallationLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   LOSSDETAILS* pLossDetails = FindLosses(poi);
   ATLASSERT(pLossDetails != 0);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = pLossDetails->pLosses->TemporaryStrand_AfterTemporaryStrandInstallation();
   else
      val = pLossDetails->pLosses->PermanentStrand_AfterTemporaryStrandInstallation();

   return val;
}

Float64 CEngAgentImp::GetBeforeTemporaryStrandRemovalLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   LOSSDETAILS* pLossDetails = FindLosses(poi);
   ATLASSERT(pLossDetails != 0);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = pLossDetails->pLosses->TemporaryStrand_BeforeTemporaryStrandRemoval();
   else
      val = pLossDetails->pLosses->PermanentStrand_BeforeTemporaryStrandRemoval();

   return val;
}

Float64 CEngAgentImp::GetAfterTemporaryStrandRemovalLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   LOSSDETAILS* pLossDetails = FindLosses(poi);
   ATLASSERT(pLossDetails != 0);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = pLossDetails->pLosses->TemporaryStrand_AfterTemporaryStrandRemoval();
   else
      val = pLossDetails->pLosses->PermanentStrand_AfterTemporaryStrandRemoval();

   return val;
}

Float64 CEngAgentImp::GetDeckPlacementLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   LOSSDETAILS* pLossDetails = FindLosses(poi);
   ATLASSERT(pLossDetails != 0);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = pLossDetails->pLosses->TemporaryStrand_AfterDeckPlacement();
   else
      val = pLossDetails->pLosses->PermanentStrand_AfterDeckPlacement();

   return val;
}

Float64 CEngAgentImp::GetSIDLLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   LOSSDETAILS* pLossDetails = FindLosses(poi);
   ATLASSERT(pLossDetails != 0);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = pLossDetails->pLosses->TemporaryStrand_AfterSIDL();
   else
      val = pLossDetails->pLosses->PermanentStrand_AfterSIDL();

   return val;
}

Float64 CEngAgentImp::GetFinal(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   LOSSDETAILS* pLossDetails = FindLosses(poi);
   ATLASSERT(pLossDetails != 0);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = pLossDetails->pLosses->TemporaryStrand_Final();
   else
      val = pLossDetails->pLosses->PermanentStrand_Final();

   return val;
}

Float64 CEngAgentImp::GetFinalWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType)
{
   LOSSDETAILS* pLossDetails = FindLosses(poi);
   ATLASSERT(pLossDetails != 0);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
   {
      val = pLossDetails->pLosses->TemporaryStrand_Final();
   }
   else
   {
      GET_IFACE(ILoadFactors,pLoadFactors);
      const CLoadFactors* pLF = pLoadFactors->GetLoadFactors();
      Float64 gLL = pLF->LLIMmax[pgsTypes::ServiceIII];
      val = pLossDetails->pLosses->PermanentStrand_FinalWithLiveLoad(gLL);
   }

   return val;
}

LOSSDETAILS CEngAgentImp::GetLossDetails(const pgsPointOfInterest& poi)
{
   LOSSDETAILS* pLosses = FindLosses(poi);
   ATLASSERT(pLosses != 0);

   return *pLosses;
}

void CEngAgentImp::ReportLosses(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   m_PsForceEngineer.ReportLosses(span,gdr,pChapter,pDisplayUnits);
}

void CEngAgentImp::ReportFinalLosses(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   m_PsForceEngineer.ReportFinalLosses(span,gdr,pChapter,pDisplayUnits);
}

Float64 CEngAgentImp::GetElasticShortening(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   LOSSDETAILS details = GetLossDetails(poi,config);

   Float64 val;
   if ( strandType == pgsTypes::Temporary )
      val = details.pLosses->TemporaryStrand_ElasticShorteningLosses();
   else
      val = details.pLosses->PermanentStrand_ElasticShorteningLosses();

   return val;
}

Float64 CEngAgentImp::GetBeforeXferLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   LOSSDETAILS details = GetLossDetails(poi,config);

   Float64 loss;
   if ( strandType == pgsTypes::Temporary )
      loss = details.pLosses->TemporaryStrand_BeforeTransfer();
   else
      loss = details.pLosses->PermanentStrand_BeforeTransfer();
   
   return loss;
}

Float64 CEngAgentImp::GetAfterXferLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   LOSSDETAILS details = GetLossDetails(poi,config);

   Float64 loss;
   if ( strandType == pgsTypes::Temporary )
      loss = details.pLosses->TemporaryStrand_AfterTransfer();
   else
      loss = details.pLosses->PermanentStrand_AfterTransfer();
   
   return loss;
}

Float64 CEngAgentImp::GetLiftingLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   LOSSDETAILS details = GetLossDetails(poi,config);

   Float64 loss;
   if ( strandType == pgsTypes::Temporary )
      loss = details.pLosses->TemporaryStrand_AtLifting();
   else
      loss = details.pLosses->PermanentStrand_AtLifting();
   
   return loss;
}

Float64 CEngAgentImp::GetShippingLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   LOSSDETAILS details = GetLossDetails(poi,config);

   Float64 loss;
   if ( strandType == pgsTypes::Temporary )
      loss = details.pLosses->TemporaryStrand_AtShipping();
   else
      loss = details.pLosses->PermanentStrand_AtShipping();
   
   return loss;
}

Float64 CEngAgentImp::GetAfterTemporaryStrandInstallationLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   LOSSDETAILS details = GetLossDetails(poi,config);

   Float64 loss;
   if ( strandType == pgsTypes::Temporary )
      loss = details.pLosses->TemporaryStrand_AfterTemporaryStrandInstallation();
   else
      loss = details.pLosses->PermanentStrand_AfterTemporaryStrandInstallation();
   
   return loss;
}

Float64 CEngAgentImp::GetBeforeTemporaryStrandRemovalLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   LOSSDETAILS details = GetLossDetails(poi,config);

   Float64 loss;
   if ( strandType == pgsTypes::Temporary )
      loss = details.pLosses->TemporaryStrand_BeforeTemporaryStrandRemoval();
   else
      loss = details.pLosses->PermanentStrand_BeforeTemporaryStrandRemoval();
   
   return loss;
}

Float64 CEngAgentImp::GetAfterTemporaryStrandRemovalLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   LOSSDETAILS details = GetLossDetails(poi,config);

   Float64 loss;
   if ( strandType == pgsTypes::Temporary )
      loss = details.pLosses->TemporaryStrand_AfterTemporaryStrandRemoval();
   else
      loss = details.pLosses->PermanentStrand_AfterTemporaryStrandRemoval();
   
   return loss;
}

Float64 CEngAgentImp::GetDeckPlacementLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   LOSSDETAILS details = GetLossDetails(poi,config);

   Float64 loss;
   if ( strandType == pgsTypes::Temporary )
      loss = details.pLosses->TemporaryStrand_AfterDeckPlacement();
   else
      loss = details.pLosses->PermanentStrand_AfterDeckPlacement();
   
   return loss;
}

Float64 CEngAgentImp::GetSIDLLosses(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   LOSSDETAILS details = GetLossDetails(poi,config);

   Float64 loss;
   if ( strandType == pgsTypes::Temporary )
      loss = details.pLosses->TemporaryStrand_AfterSIDL();
   else
      loss = details.pLosses->PermanentStrand_AfterSIDL();
   
   return loss;
}

Float64 CEngAgentImp::GetFinal(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   LOSSDETAILS details = GetLossDetails(poi,config);

   Float64 loss;
   if ( strandType == pgsTypes::Temporary )
      loss = details.pLosses->TemporaryStrand_Final();
   else
      loss = details.pLosses->PermanentStrand_Final();
   
   return loss;
}

Float64 CEngAgentImp::GetFinalWithLiveLoad(const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,const GDRCONFIG& config)
{
   LOSSDETAILS details = GetLossDetails(poi,config);

   Float64 loss;
   if ( strandType == pgsTypes::Temporary )
   {
      loss = details.pLosses->TemporaryStrand_Final();
   }
   else
   {
      GET_IFACE(ILoadFactors,pLoadFactors);
      const CLoadFactors* pLF = pLoadFactors->GetLoadFactors();
      Float64 gLL = pLF->LLIMmax[pgsTypes::ServiceIII];
      loss = details.pLosses->PermanentStrand_FinalWithLiveLoad(gLL);
   }
   
   return loss;
}

LOSSDETAILS CEngAgentImp::GetLossDetails(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   // first see if we have cached our losses
   LOSSDETAILS details;

   if (m_DesignLosses.GetFromCache(poi,config,&details))
   {
      return details;
   }
   else
   {
      // not cached, compute, cache and return
      BuildLosses(poi,config,&details);

      m_DesignLosses.SaveToCache(poi,config,details);
   }

   return details;
}

void CEngAgentImp::ClearDesignLosses()
{
   m_DesignLosses.Invalidate();
}


/////////////////////////////////////////////////////////////////////////////
// IPrestressForce

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetPjackMax(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType,StrandIndexType nStrands)
{
   return m_PsForceEngineer.GetPjackMax(span,gdr,strandType,nStrands);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetPjackMax(SpanIndexType span,GirderIndexType gdr,const matPsStrand& strand,StrandIndexType nStrands)
{
   return m_PsForceEngineer.GetPjackMax(span,gdr,strand,nStrands);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetXferLength(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType)
{
   return m_PsForceEngineer.GetXferLength(span,gdr,strandType);
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
Float64 CEngAgentImp::GetHoldDownForce(SpanIndexType span,GirderIndexType gdr)
{
   return m_PsForceEngineer.GetHoldDownForce(span,gdr);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetHoldDownForce(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config)
{
   return m_PsForceEngineer.GetHoldDownForce(span,gdr,config);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetPrestressForce(const pgsPointOfInterest& poi,
                                        pgsTypes::StrandType strandType,
                                        pgsTypes::LossStage lossStage)
{
   PrestressPoiKey key(PrestressSubKey(lossStage,strandType),poi);
   std::map<PrestressPoiKey,Float64>::iterator found = m_PsForce.find(key);
   if ( found != m_PsForce.end() )
   {
      return (*found).second;
   }
   else
   {
      Float64 F = m_PsForceEngineer.GetPrestressForce(poi,strandType,lossStage);
      m_PsForce.insert(std::make_pair(key,F));
      return F;
   }
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetPrestressForce(const pgsPointOfInterest& poi,
                                        const GDRCONFIG& config,
                                        pgsTypes::StrandType strandType,
                                        pgsTypes::LossStage lossStage)
{
   return m_PsForceEngineer.GetPrestressForce(poi,
                                              strandType,
                                              lossStage,
                                              config);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetPrestressForcePerStrand(const pgsPointOfInterest& poi,
                                                 pgsTypes::StrandType strandType,
                                                 pgsTypes::LossStage lossStage)
{
   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetGirderConfiguration(poi.GetSpan(),poi.GetGirder());
   Float64 slabOffset = pBridge->GetSlabOffset(poi);
   return GetPrestressForcePerStrand(poi,config,strandType,lossStage);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetPrestressForcePerStrand(const pgsPointOfInterest& poi,
                                                 const GDRCONFIG& config,
                                                 pgsTypes::StrandType strandType,
                                                 pgsTypes::LossStage lossStage)
{
   Float64 Ps = GetPrestressForce(poi,config,strandType,lossStage);
   StrandIndexType nStrands = config.PrestressConfig.GetNStrands(strandType);

   GET_IFACE(IBridge,pBridge);
   Float64 gdr_length = pBridge->GetGirderLength(poi.GetSpan(),poi.GetGirder());

   DebondConfigConstIterator iter    = config.PrestressConfig.Debond[strandType].begin();
   DebondConfigConstIterator iterend = config.PrestressConfig.Debond[strandType].end();
   while(iter != iterend)
   {
      const DEBONDCONFIG& debond_info = *iter;
      if ( InRange(0.0,poi.GetDistFromStart(),debond_info.LeftDebondLength) ||
           InRange(gdr_length - debond_info.RightDebondLength, poi.GetDistFromStart(), gdr_length) )
      {
         nStrands--;
      }

      iter++;
   }

   return Ps/nStrands;
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,
                                                pgsTypes::LossStage lossStage)
{
   Float64 cos = GetHorizPsComponent(m_pBroker,poi);
   return cos * GetPrestressForce(poi,pgsTypes::Harped,lossStage);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetHorizHarpedStrandForce(const pgsPointOfInterest& poi,
                                                const GDRCONFIG& config,
                                                pgsTypes::LossStage lossStage)
{
   Float64 cos = GetHorizPsComponent(m_pBroker,poi,config);
   return cos * GetPrestressForce(poi,config,pgsTypes::Harped,lossStage);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetVertHarpedStrandForce(const pgsPointOfInterest& poi,
                                               pgsTypes::LossStage lossStage)
{
   Float64 sin = GetVertPsComponent(m_pBroker,poi);
   return sin * GetPrestressForce(poi,pgsTypes::Harped,lossStage);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetVertHarpedStrandForce(const pgsPointOfInterest& poi,
                                               const GDRCONFIG& config,
                                               pgsTypes::LossStage lossStage)
{
   Float64 sin = GetVertPsComponent(m_pBroker,poi);
   return sin * GetPrestressForce(poi,config,pgsTypes::Harped,lossStage);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetStrandForce(const pgsPointOfInterest& poi,
                                     pgsTypes::StrandType strandType,
                                     pgsTypes::LossStage lossStage)
{
   return m_PsForceEngineer.GetPrestressForce(poi,strandType,lossStage);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetStrandForce(const pgsPointOfInterest& poi,
                                     pgsTypes::StrandType strandType,
                                     const GDRCONFIG& config,
                                     pgsTypes::LossStage lossStage)
{
   return m_PsForceEngineer.GetPrestressForce(poi,strandType,lossStage,config);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetStrandStress(const pgsPointOfInterest& poi,
                                      pgsTypes::StrandType strandType,
                                      pgsTypes::LossStage lossStage)
{
   return m_PsForceEngineer.GetStrandStress(poi,strandType,lossStage);
}

//-----------------------------------------------------------------------------
Float64 CEngAgentImp::GetStrandStress(const pgsPointOfInterest& poi,
                                      pgsTypes::StrandType strandType,
                                      const GDRCONFIG& config,
                                      pgsTypes::LossStage lossStage)
{
   return m_PsForceEngineer.GetStrandStress(poi,
                                            strandType,
                                            lossStage,
                                            config);
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
   // Returns if OK, throws exception if not
   //

   GET_IFACE(IRoadwayData,pRoadway);
   GET_IFACE(IRoadway,pAlignment);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE(ILiveLoads,pLiveLoads);


   SpanIndexType nSpans = pBridge->GetSpanCount();

   AlignmentData2 alignment_data = pRoadway->GetAlignmentData2();
   CollectionIndexType nCurves = alignment_data.HorzCurves.size();
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
   PierIndexType start_pier = PierIndexType(poi.GetSpan()); // start pier has same id as span
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

   
   GirderIndexType nBeams = pBridge->GetGirderCount(poi.GetSpan());
   Float64 delta_limit;
   bool bIsOK = true;
   if ( nBeams == 0 || nBeams == 1 )
   {
      LPCTSTR msg = _T("The bridge must have at least two beams per span");
      pgsBridgeDescriptionStatusItem* pStatusItem = new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,1,msg);
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

   SpanIndexType span = poi.GetSpan();
   GirderIndexType nGirders = pBridge->GetGirderCount(span);

   GET_IFACE(ISectProp2,pSectProp);

   pgsPointOfInterest current_poi(span,0,poi.GetDistFromStart());
   
   // look at non-composite moment of inertia
   // we want girders that are basically the same
   pgsTypes::Stage stage = pgsTypes::BridgeSite1;
  
   Float64 Imin = pSectProp->GetIx(stage,poi);
   Float64 Imax = Imin;

   for ( GirderIndexType gdrIdx = 1; gdrIdx < nGirders; gdrIdx++ )
   {
      current_poi.SetGirder(gdrIdx);
      
      Float64 I = pSectProp->GetIx(stage,current_poi);
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

   SpanIndexType span = poi.GetSpan();
   GirderIndexType nGirders = pBridge->GetGirderCount(span);

   // direction is measured 0 to 2pi, we want it measured between 0 and +/- pi.
   // if angle is greater than pi, convert it to 2pi - angle
   CComPtr<IDirection> objDirection;
   pBridge->GetGirderBearing(span,0,&objDirection);
   Float64 dir_of_prev_girder;
   objDirection->get_Value(&dir_of_prev_girder);

   if ( M_PI < dir_of_prev_girder )
      dir_of_prev_girder = TWO_PI - dir_of_prev_girder;

   Float64 maxAngularDifference = -DBL_MAX;
   for ( GirderIndexType gdrIdx = 1; gdrIdx < nGirders; gdrIdx++ )
   {
      objDirection.Release();
      pBridge->GetGirderBearing(span,gdrIdx,&objDirection);
      Float64 dir_of_this_girder;
      objDirection->get_Value(&dir_of_this_girder);

      if ( M_PI < dir_of_this_girder )
         dir_of_this_girder = TWO_PI - dir_of_this_girder;

      Float64 angular_diff = fabs(dir_of_this_girder - dir_of_prev_girder);
      maxAngularDifference = _cpp_max(angular_diff,maxAngularDifference);
   }

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
}

Float64 CEngAgentImp::GetMomentDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   ValidateLiveLoadDistributionFactors(span,gdr);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
      return pSpan->GetLLDFPosMoment(gdr,ls);
   }
   else
   {
      return m_pDistFactorEngineer->GetMomentDF(span,gdr,ls);
   }
}

Float64 CEngAgentImp::GetMomentDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   ValidateLiveLoadDistributionFactors(span,gdr);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
      return pSpan->GetLLDFPosMoment(gdr,ls);
   }
   else
   {
      return m_pDistFactorEngineer->GetMomentDF(span,gdr,ls,fcgdr);
   }
}
   
Float64 CEngAgentImp::GetNegMomentDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   ValidateLiveLoadDistributionFactors(span,gdr);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
      return pSpan->GetLLDFNegMoment(gdr,ls);
   }
   else
   {
      return m_pDistFactorEngineer->GetMomentDF(span,gdr,ls);
   }
}
   
Float64 CEngAgentImp::GetNegMomentDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   ValidateLiveLoadDistributionFactors(span,gdr);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
      return pSpan->GetLLDFNegMoment(gdr,ls);
   }
   else
   {
      return m_pDistFactorEngineer->GetMomentDF(span,gdr,ls,fcgdr);
   }
}
   
Float64 CEngAgentImp::GetNegMomentDistFactorAtPier(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPier = pBridgeDesc->GetPier(pier);
   const CSpanData* pSpan = (pierFace == pgsTypes::Back) ? pPier->GetPrevSpan() : pPier->GetNextSpan();

   ValidateLiveLoadDistributionFactors(pSpan->GetSpanIndex(),gdr);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pPier->GetLLDFNegMoment(gdr, ls);
   }
   else
   {
      return m_pDistFactorEngineer->GetNegMomentDF(pier,gdr,ls,pierFace);
   }
}
   
Float64 CEngAgentImp::GetNegMomentDistFactorAtPier(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPier = pBridgeDesc->GetPier(pier);
   const CSpanData* pSpan = (pierFace == pgsTypes::Back) ? pPier->GetPrevSpan() : pPier->GetNextSpan();

   ValidateLiveLoadDistributionFactors(pSpan->GetSpanIndex(),gdr);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pPier->GetLLDFNegMoment(gdr, ls);
   }
   else
   {
      return m_pDistFactorEngineer->GetNegMomentDF(pier,gdr,ls,pierFace,fcgdr);
   }
}

Float64 CEngAgentImp::GetShearDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   ValidateLiveLoadDistributionFactors(span,gdr);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      pgsTypes::GirderLocation gl = pSpan->IsExteriorGirder(gdr) ? pgsTypes::Exterior : pgsTypes::Interior;
      return pSpan->GetLLDFShear(gdr,ls);
   }
   else
   {
      return m_pDistFactorEngineer->GetShearDF(span,gdr,ls);
   }
}

Float64 CEngAgentImp::GetShearDistFactor(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

   ValidateLiveLoadDistributionFactors(span,gdr);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      pgsTypes::GirderLocation gl = pSpan->IsExteriorGirder(gdr) ? pgsTypes::Exterior : pgsTypes::Interior;
      return pSpan->GetLLDFShear(gdr,ls);
   }
   else
   {
      return m_pDistFactorEngineer->GetShearDF(span,gdr,ls,fcgdr);
   }
}

Float64 CEngAgentImp::GetReactionDistFactor(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPier = pBridgeDesc->GetPier(pier);
   const CSpanData* pSpan = (pier == pBridgeDesc->GetPierCount()-1 ? pPier->GetPrevSpan() : pPier->GetNextSpan());

   ValidateLiveLoadDistributionFactors(pier,gdr);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pPier->GetLLDFReaction(gdr, ls);
   }
   else
   {
      return m_pDistFactorEngineer->GetReactionDF(pier,gdr,ls);
   }
}

Float64 CEngAgentImp::GetReactionDistFactor(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData* pPier = pBridgeDesc->GetPier(pier);
   const CSpanData* pSpan = (pier == pBridgeDesc->GetPierCount()-1 ? pPier->GetPrevSpan() : pPier->GetNextSpan());

   ValidateLiveLoadDistributionFactors(pSpan->GetSpanIndex(),gdr);

   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      return pPier->GetLLDFReaction(gdr, ls);
   }
   else
   {
      return m_pDistFactorEngineer->GetReactionDF(pier,gdr,ls,fcgdr);
   }
}

void CEngAgentImp::GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState ls,Float64* pM,Float64* nM,Float64* V)
{
   GET_IFACE(IBridge,pBridge);
   SpanIndexType span   = poi.GetSpan();
   GirderIndexType girder = poi.GetGirder();
   PierIndexType prev_pier = PierIndexType(span);
   PierIndexType next_pier = prev_pier + 1;

   SpanIndexType nSpans = pBridge->GetSpanCount();

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   Float64 dist_from_start = poi.GetDistFromStart() - end_size;

   Float64 dfPoints[2];
   Uint32 nPoints;
   GetNegMomentDistFactorPoints(span,girder,&dfPoints[0],&nPoints);

   *V  = GetShearDistFactor(span,girder,ls);
   *pM = GetMomentDistFactor(span,girder,ls);

   if ( nPoints == 0 )
   {
      *nM = GetNegMomentDistFactor(span,girder,ls);
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
            *nM = GetNegMomentDistFactorAtPier(prev_pier,girder,ls,pgsTypes::Ahead);
         }
         else
         {
            // hinged to the left of this point... use DF for the span
            *nM = GetNegMomentDistFactor(span,girder,ls);
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
            *nM = GetNegMomentDistFactorAtPier(next_pier,girder,ls,pgsTypes::Back);
         }
         else
         {
            // Integral to the left of this point... use DF for the span
            *nM = GetNegMomentDistFactor(span,girder,ls);
         }
      }
   }
   else
   {
      if ( dist_from_start < dfPoints[0] )
         *nM = GetNegMomentDistFactorAtPier(prev_pier,girder,ls,pgsTypes::Ahead);
      else if ( dfPoints[0] <= dist_from_start && dist_from_start <= dfPoints[1] )
         *nM = GetNegMomentDistFactor(span,girder,ls);
      else
         *nM = GetNegMomentDistFactorAtPier(next_pier,girder,ls,pgsTypes::Back);
   }
}

void CEngAgentImp::GetDistributionFactors(const pgsPointOfInterest& poi,pgsTypes::LimitState ls,Float64 fcgdr,Float64* pM,Float64* nM,Float64* V)
{
   GET_IFACE(IBridge,pBridge);
   SpanIndexType span   = poi.GetSpan();
   GirderIndexType girder = poi.GetGirder();
   PierIndexType prev_pier = PierIndexType(span);
   PierIndexType next_pier = prev_pier + 1;

   SpanIndexType nSpans = pBridge->GetSpanCount();

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);
   Float64 dist_from_start = poi.GetDistFromStart() - end_size;

   Float64 dfPoints[2];
   Uint32 nPoints;
   GetNegMomentDistFactorPoints(span,girder,&dfPoints[0],&nPoints);

   *V  = GetShearDistFactor(span,girder,ls,fcgdr);
   *pM = GetMomentDistFactor(span,girder,ls,fcgdr);

   if ( nPoints == 0 )
   {
      *nM = GetNegMomentDistFactor(span,girder,ls,fcgdr);
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
            *nM = GetNegMomentDistFactorAtPier(prev_pier,girder,ls,pgsTypes::Ahead,fcgdr);
         }
         else
         {
            // hinged to the left of this point... use DF for the span
            *nM = GetNegMomentDistFactor(span,girder,ls,fcgdr);
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
            *nM = GetNegMomentDistFactor(span,girder,ls,fcgdr);
         }
         else
         {
            // hinged to the left of this point... use DF at the next pier
            *nM = GetNegMomentDistFactorAtPier(next_pier,girder,ls,pgsTypes::Back,fcgdr);
         }
      }
   }
   else
   {
      if ( dist_from_start < dfPoints[0] )
         *nM = GetNegMomentDistFactorAtPier(prev_pier,girder,ls,pgsTypes::Ahead,fcgdr);
      else if ( dfPoints[0] <= dist_from_start && dist_from_start <= dfPoints[1] )
         *nM = GetNegMomentDistFactor(span,girder,ls,fcgdr);
      else
         *nM = GetNegMomentDistFactorAtPier(next_pier,girder,ls,pgsTypes::Back,fcgdr);
   }
}

void CEngAgentImp::GetNegMomentDistFactorPoints(SpanIndexType span,GirderIndexType gdr,Float64* dfPoints,Uint32* nPoints)
{
   GET_IFACE(IContraflexurePoints,pCP);
   pCP->GetContraflexurePoints(span,gdr,dfPoints,nPoints);
}

void CEngAgentImp::ReportDistributionFactors(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   ValidateLiveLoadDistributionFactors(span,gdr);

   if ( pBridgeDesc->GetDistributionFactorMethod() != pgsTypes::DirectlyInput )
   {
      m_pDistFactorEngineer->BuildReport(span,gdr,pChapter,pDisplayUnits);
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

      const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
      pgsTypes::GirderLocation gl = pSpan->IsInteriorGirder(gdr) ? pgsTypes::Interior : pgsTypes::Exterior;

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
         table->SetRowSpan(1,0,SKIP_CELL);
         (*table)(0,0) << _T("");

         table->SetColumnSpan(0,1,4);
         (*table)(0,1) << _T("Strength/Service");

         table->SetColumnSpan(0,2,4);
         (*table)(0,2) << _T("Fatigue");

         table->SetColumnSpan(0,3,SKIP_CELL);
         table->SetColumnSpan(0,4,SKIP_CELL);
         table->SetColumnSpan(0,5,SKIP_CELL);
         table->SetColumnSpan(0,6,SKIP_CELL);
         table->SetColumnSpan(0,7,SKIP_CELL);
         table->SetColumnSpan(0,8,SKIP_CELL);

         (*table)(1,1) << _T("+M");
         (*table)(1,2) << _T("-M");
         (*table)(1,3) << _T("V");
         (*table)(1,4) << _T("R");
         (*table)(1,5) << _T("+M");
         (*table)(1,6) << _T("-M");
         (*table)(1,7) << _T("V");
         (*table)(1,8) << _T("R");
      }

      const CPierData* pPier = pBridgeDesc->GetPier(0);

      RowIndexType row = table->GetNumberOfHeaderRows();
      do
      {
         PierIndexType pierIdx = pPier->GetPierIndex();

         (*table)(row,0) << _T("Pier ") << LABEL_PIER(pierIdx);
         (*table)(row,1) << _T("");

         bool bContinuous = pPier->IsContinuous();

         bool bIntegralOnLeft, bIntegralOnRight;
         pPier->IsIntegral(&bIntegralOnLeft,&bIntegralOnRight);

         if ( bContinuous || bIntegralOnLeft || bIntegralOnRight)
            (*table)(row,2) << scalar.SetValue( pPier->GetLLDFNegMoment(gdr, pgsTypes::StrengthI) );
         else
            (*table)(row,2) << _T("");

         (*table)(row,3) << _T("");
         (*table)(row,4) << scalar.SetValue( pPier->GetLLDFReaction(gdr, pgsTypes::StrengthI) );

         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*table)(row,5) << _T("");

            if ( bContinuous || bIntegralOnLeft || bIntegralOnRight)
               (*table)(row,6) << scalar.SetValue( pPier->GetLLDFNegMoment(gdr, pgsTypes::FatigueI) );
            else
               (*table)(row,6) << _T("");

            (*table)(row,7) << _T("");
            (*table)(row,8) << scalar.SetValue( pPier->GetLLDFReaction(gdr, pgsTypes::FatigueI) );
         }

         row++;

         pSpan = pPier->GetNextSpan();
         if ( pSpan )
         {
            SpanIndexType spanIdx = pSpan->GetSpanIndex();

            bool bContinuousStart = pSpan->GetPrevPier()->IsContinuous();
            bool bContinuousEnd   = pSpan->GetNextPier()->IsContinuous();
            
            pSpan->GetPrevPier()->IsIntegral(&bIntegralOnLeft,&bIntegralOnRight);
            bool bIntegralStart = bIntegralOnRight;
            
            pSpan->GetNextPier()->IsIntegral(&bIntegralOnLeft,&bIntegralOnRight);
            bool bIntegralEnd = bIntegralOnLeft;

            (*table)(row,0) << _T("Span ") << LABEL_SPAN(spanIdx);
            (*table)(row,1) << scalar.SetValue( pSpan->GetLLDFPosMoment(gdr,pgsTypes::StrengthI) );
            
            if ( bContinuousStart || bContinuousEnd || bIntegralStart || bIntegralEnd )
               (*table)(row,2) << scalar.SetValue( pSpan->GetLLDFNegMoment(gdr,pgsTypes::StrengthI) );
            else
               (*table)(row,2) << _T("");

            (*table)(row,3) << scalar.SetValue( pSpan->GetLLDFShear(gdr,pgsTypes::StrengthI) );
            (*table)(row,4) << _T("");

            if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
            {
               (*table)(row,5) << scalar.SetValue( pSpan->GetLLDFPosMoment(gdr,pgsTypes::FatigueI) );
               
               if ( bContinuousStart || bContinuousEnd || bIntegralStart || bIntegralEnd )
                  (*table)(row,6) << scalar.SetValue( pSpan->GetLLDFNegMoment(gdr,pgsTypes::FatigueI) );
               else
                  (*table)(row,6) << _T("");

               (*table)(row,7) << scalar.SetValue( pSpan->GetLLDFShear(gdr,pgsTypes::FatigueI) );
               (*table)(row,8) << _T("");
            }

            row++;

            pPier = pSpan->GetNextPier();
         }
      } while ( pSpan );
   }
}

bool CEngAgentImp::Run1250Tests(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile)
{
   ValidateLiveLoadDistributionFactors(span,gdr);
   return m_pDistFactorEngineer->Run1250Tests(span,gdr,ls,pid,bridgeId,resultsFile,poiFile);
}

bool CEngAgentImp::GetDFResultsEx(SpanIndexType span, GirderIndexType gdr,pgsTypes::LimitState ls,
                               Float64* gpM, Float64* gpM1, Float64* gpM2,
                               Float64* gnM, Float64* gnM1, Float64* gnM2,
                               Float64* gV,  Float64* gV1,  Float64* gV2,
                               Float64* gR,  Float64* gR1,  Float64* gR2 ) 
{
   ValidateLiveLoadDistributionFactors(span,gdr);
   return m_pDistFactorEngineer->GetDFResultsEx(span, gdr, ls,
                               gpM, gpM1, gpM2, gnM, gnM1, gnM2,
                               gV,  gV1, gV2, gR, gR1, gR2 ); 
}

Float64 CEngAgentImp::GetDeflectionDistFactor(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE(IBridge,pBridge);
   GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
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
Float64 CEngAgentImp::GetMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   MOMENTCAPACITYDETAILS mcd;
   GetMomentCapacityDetails( stage,poi,bPositiveMoment,&mcd );
   return mcd.Phi * mcd.Mn; // = Mr
}

std::vector<Float64> CEngAgentImp::GetMomentCapacity(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment)
{
   std::vector<Float64> Mn;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      Mn.push_back( GetMomentCapacity(stage,poi,bPositiveMoment));
   }

   return Mn;
}

void CEngAgentImp::GetMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd)
{
   ATLASSERT(poi.GetID()!=INVALID_INDEX); // shouldn't be asking for temporary pois for no design case

   // Capacity is only computed in these stages
   ATLASSERT( stage == pgsTypes::BridgeSite1 || stage == pgsTypes::BridgeSite3 );

   const MOMENTCAPACITYDETAILS* pMCD = GetCachedMomentCapacity(stage,poi,bPositiveMoment);
   if ( pMCD == NULL )
   {
      pMCD = ValidateMomentCapacity(stage,poi,bPositiveMoment);
   }
   ATLASSERT(pMCD != NULL);

   *pmcd = *pMCD;
}

void CEngAgentImp::GetMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MOMENTCAPACITYDETAILS* pmcd)
{
    if (poi.GetID()==INVALID_INDEX)
    {
        // Never store temporary pois
       *pmcd = ComputeMomentCapacity(stage,poi,config,bPositiveMoment);
    }
    else
    {
       // Get the current configuration and compare it to the provided one
       // If same, call GetMomentCapacityDetails w/o config.
       GET_IFACE(IBridge,pBridge);
       GDRCONFIG curr_config = pBridge->GetGirderConfiguration(poi.GetSpan(),poi.GetGirder());

       if ( poi.GetID()!=INVALID_INDEX && curr_config.IsFlexuralDataEqual(config) )
       {
          GetMomentCapacityDetails(stage,poi,bPositiveMoment,pmcd);
       }
       else
       {
          // the capacity details for the requested girder configuration is not the same as for the
          // current input... see if it is cached
          const MOMENTCAPACITYDETAILS* pMCD = GetCachedMomentCapacity(stage,poi,config,bPositiveMoment);
          if ( pMCD == NULL )
          {
             // the capacity has not yet been computed for this config, moment type, stage, and poi
             pMCD = ValidateMomentCapacity(stage,poi,config,bPositiveMoment); // compute it
          }

          ATLASSERT( pMCD != NULL );

          *pmcd = *pMCD;
       }
    }
}

std::vector<Float64> CEngAgentImp::GetCrackingMoment(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment)
{
   std::vector<Float64> Mcr;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      Mcr.push_back( GetCrackingMoment(stage,poi,bPositiveMoment));
   }

   return Mcr;
}

Float64 CEngAgentImp::GetCrackingMoment(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   CRACKINGMOMENTDETAILS cmd;
   GetCrackingMomentDetails( stage,poi,bPositiveMoment,&cmd );

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
   bool bAfter2002 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2003Interims ? true : false );
   if ( bAfter2002 )
   {
      Mcr = _cpp_min(cmd.Mcr,cmd.McrLimit);
   }

   return Mcr;
}

void CEngAgentImp::GetCrackingMomentDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd)
{
   // Capacity is only computed in these stages
   ATLASSERT( stage == pgsTypes::BridgeSite1 || stage == pgsTypes::BridgeSite3 );
   ATLASSERT( poi.GetID() != INVALID_ID );

   *pcmd = *ValidateCrackingMoments(stage,poi,bPositiveMoment);
}

void CEngAgentImp::GetCrackingMomentDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,CRACKINGMOMENTDETAILS* pcmd)
{
   // Get the current configuration and compare it to the provided one
   // If same, call GetMomentCapacityDetails w/o config.
   GET_IFACE(IBridge,pBridge);
   GDRCONFIG curr_config = pBridge->GetGirderConfiguration(poi.GetSpan(),poi.GetGirder());
   if ( poi.GetID()!=INVALID_INDEX && curr_config.IsFlexuralDataEqual(config) )
      GetCrackingMomentDetails(stage,poi,bPositiveMoment,pcmd);
   else
      m_MomentCapEngineer.ComputeCrackingMoment(stage,poi,config,bPositiveMoment,pcmd);
}

std::vector<Float64> CEngAgentImp::GetMinMomentCapacity(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment)
{
   std::vector<Float64> Mmin;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      Mmin.push_back( GetMinMomentCapacity(stage,poi,bPositiveMoment));
   }

   return Mmin;
}

Float64 CEngAgentImp::GetMinMomentCapacity(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment)
{
   MINMOMENTCAPDETAILS mmcd;
   GetMinMomentCapacityDetails( stage, poi, bPositiveMoment, &mmcd );
   return mmcd.MrMin;
}

void CEngAgentImp::GetMinMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd)
{
   // Capacity is only computed in these stages
   ATLASSERT( stage == pgsTypes::BridgeSite1 || stage == pgsTypes::BridgeSite3 );

   *pmmcd = *ValidateMinMomentCapacity(stage,poi,bPositiveMoment);
}

void CEngAgentImp::GetMinMomentCapacityDetails(pgsTypes::Stage stage,const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bPositiveMoment,MINMOMENTCAPDETAILS* pmmcd)
{
   // Get the current configuration and compare it to the provided one
   // If same, call GetMomentCapacityDetails w/o config.
   GET_IFACE(IBridge,pBridge);
   GDRCONFIG curr_config = pBridge->GetGirderConfiguration(poi.GetSpan(),poi.GetGirder());
   if ( poi.GetID()!=INVALID_INDEX && curr_config.IsFlexuralDataEqual(config) )
      GetMinMomentCapacityDetails(stage,poi,bPositiveMoment,pmmcd);
   else
      m_MomentCapEngineer.ComputeMinMomentCapacity(stage,poi,config,bPositiveMoment,pmmcd);
}

std::vector<MOMENTCAPACITYDETAILS> CEngAgentImp::GetMomentCapacityDetails(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment)
{
   std::vector<MOMENTCAPACITYDETAILS> details;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      MOMENTCAPACITYDETAILS mcd;
      GetMomentCapacityDetails(stage,poi,bPositiveMoment,&mcd);
      details.push_back( mcd );
   }

   return details;
}

std::vector<MINMOMENTCAPDETAILS> CEngAgentImp::GetMinMomentCapacityDetails(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment)
{
   std::vector<MINMOMENTCAPDETAILS> details;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      MINMOMENTCAPDETAILS mcd;
      GetMinMomentCapacityDetails(stage,poi,bPositiveMoment,&mcd);
      details.push_back( mcd );
   }

   return details;
}

std::vector<CRACKINGMOMENTDETAILS> CEngAgentImp::GetCrackingMomentDetails(pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi,bool bPositiveMoment)
{
   std::vector<CRACKINGMOMENTDETAILS> details;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      CRACKINGMOMENTDETAILS cmd;
      GetCrackingMomentDetails(stage,poi,bPositiveMoment,&cmd);
      details.push_back( cmd );
   }

   return details;
}

/////////////////////////////////////////////////////////////////////////////
// IShearCapacity
Float64 CEngAgentImp::GetShearCapacity(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi)
{
   SHEARCAPACITYDETAILS scd;
   GetShearCapacityDetails( ls,stage,poi,&scd );
   return scd.pVn; 
}

std::vector<Float64> CEngAgentImp::GetShearCapacity(pgsTypes::LimitState ls, pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi)
{
   std::vector<Float64> Vn;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      Vn.push_back(GetShearCapacity(ls,stage,poi));
   }

   return Vn;
}

void CEngAgentImp::GetShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi,SHEARCAPACITYDETAILS* pscd)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   GetRawShearCapacityDetails(ls, stage, poi, pscd);

   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   ShearCapacityMethod shear_capacity_method = pSpecEntry->GetShearCapacityMethod();


   if ( shear_capacity_method == scmBTEquations || shear_capacity_method == scmWSDOT2007 )
   {
//      ATLASSERT( lrfdVersionMgr::FourthEditionWith2008Interims <= lrfdVersionMgr::GetVersion() );

      GET_IFACE(IPointOfInterest, pIPoi);
      pgsPointOfInterest LeftCS;
      pgsPointOfInterest RightCS;
      pIPoi->GetCriticalSection(ls,span,gdr,&LeftCS,&RightCS);

      if (poi.GetDistFromStart() < LeftCS.GetDistFromStart())
      {
         // before left critical section... use strain at left cs
         SHEARCAPACITYDETAILS cs_scd;
         GetRawShearCapacityDetails(ls,stage,LeftCS,&cs_scd);
         m_ShearCapEngineer.TweakShearCapacityOutboardOfCriticalSection(LeftCS,pscd,&cs_scd);
      }
      else if (poi.GetDistFromStart() > RightCS.GetDistFromStart())
      {
         SHEARCAPACITYDETAILS cs_scd;
         GetRawShearCapacityDetails(ls,stage,RightCS,&cs_scd);
         m_ShearCapEngineer.TweakShearCapacityOutboardOfCriticalSection(RightCS,pscd,&cs_scd);
      }
   }

}

void CEngAgentImp::GetRawShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,const pgsPointOfInterest& poi,SHEARCAPACITYDETAILS* pscd)
{
   // Capacity is only computed in this stage and load case
   ATLASSERT( stage == pgsTypes::BridgeSite3 );

   *pscd = *ValidateShearCapacity(ls,stage,poi);
}

Float64 CEngAgentImp::GetShearCapacity(pgsTypes::LimitState ls, pgsTypes::Stage stage,
                                       const pgsPointOfInterest& poi, const GDRCONFIG& config )
{
   SHEARCAPACITYDETAILS scd;
   GetShearCapacityDetails( ls,stage,poi, config, &scd );
   return scd.pVn; 
}

void CEngAgentImp::GetShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,
                                           const pgsPointOfInterest& poi, const GDRCONFIG& config,
                                           SHEARCAPACITYDETAILS* pscd)
{
   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();
   pgsPointOfInterest poi2 = poi; // POI where analysis is done...

   GetRawShearCapacityDetails(ls,stage,poi2,config,pscd);
}

void CEngAgentImp::GetRawShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,
                                           const pgsPointOfInterest& poi, const GDRCONFIG& config,
                                           SHEARCAPACITYDETAILS* pscd)
{
   // Capacity is only computed in this stage and load case
   ATLASSERT( stage == pgsTypes::BridgeSite3 );

   m_ShearCapEngineer.ComputeShearCapacity(ls,stage,poi,config,pscd);
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
   m_ShearCapEngineer.ComputeFpc(poi,config,pfpc);
}

void CEngAgentImp::GetCriticalSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,Float64* pLeft,Float64* pRight)
{
   CRITSECTDETAILS det;
   GetCriticalSectionDetails(limitState,span,gdr,&det);
   GetCriticalSectionFromDetails(det,pLeft,pRight);
}

void CEngAgentImp::GetCriticalSectionFromDetails(const CRITSECTDETAILS& details,Float64* pLeft,Float64* pRight)
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bThirdEdition = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::ThirdEdition2004 ? true : false );

   if ( details.bAtLeftFaceOfSupport )
   {
      *pLeft = details.poiLeftFaceOfSupport.GetDistFromStart();
   }
   else
   {
      if ( bThirdEdition )
      {
         // Critical section is at dv from face of support
         *pLeft  = details.LeftCsDv.Poi.GetDistFromStart();
      }
      else
      {
         // take max of intersections to get crit section
         // left
         if (details.LeftCsDv.Dv > details.LeftCsDvt.CotanThetaDv05)
            *pLeft  = details.LeftCsDv.Poi.GetDistFromStart();
         else
            *pLeft  = details.LeftCsDvt.Poi.GetDistFromStart();
      }
   }

   if ( details.bAtRightFaceOfSupport )
   {
      *pRight = details.poiRightFaceOfSupport.GetDistFromStart();
   }
   else
   {
      if ( bThirdEdition )
      {
         // Critical section is at dv from face of support
         *pRight = details.RightCsDv.Poi.GetDistFromStart();
      }
      else
      {
         // take max of intersections to get crit section
         if (details.RightCsDv.Dv > details.RightCsDvt.CotanThetaDv05)
            *pRight  = details.RightCsDv.Poi.GetDistFromStart();
         else
            *pRight  = details.RightCsDvt.Poi.GetDistFromStart();
      }
   }
}

void CEngAgentImp::GetCriticalSection(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,Float64* pLeft,Float64* pRight)
{
   CRITSECTDETAILS det;
   GetCriticalSectionDetails(limitState,span,gdr,config,&det);
   GetCriticalSectionFromDetails(det,pLeft,pRight);
}

void CEngAgentImp::GetCriticalSectionDetails(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,CRITSECTDETAILS* pDet)
{
   *pDet = *ValidateShearCritSection( limitState, span, gdr);
}

void CEngAgentImp::GetCriticalSectionDetails(pgsTypes::LimitState limitState,SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,CRITSECTDETAILS* pDetails)
{
   CalculateShearCritSection(limitState,span,gdr,config,pDetails);
}

std::vector<SHEARCAPACITYDETAILS> CEngAgentImp::GetShearCapacityDetails(pgsTypes::LimitState ls, pgsTypes::Stage stage,const std::vector<pgsPointOfInterest>& vPoi)
{
   std::vector<SHEARCAPACITYDETAILS> details;
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      SHEARCAPACITYDETAILS scd;
      GetShearCapacityDetails(ls,stage,poi,&scd);
      details.push_back(scd);
   }

   return details;
}

/////////////////////////////////////////////////////////////////////////////
// IGirderHaunch
Float64 CEngAgentImp::GetRequiredSlabOffset(SpanIndexType span,GirderIndexType gdr)
{
   HAUNCHDETAILS details;
   GetHaunchDetails(span,gdr,&details);

   Float64 slab_offset = details.RequiredSlabOffset;

   // Round to nearest 1/4" (5 mm) per WSDOT BDM

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   if ( IS_SI_UNITS(pDisplayUnits) )
      slab_offset = RoundOff(slab_offset,::ConvertToSysUnits(5.0,unitMeasure::Millimeter) );
   else
      slab_offset = RoundOff(slab_offset,::ConvertToSysUnits(0.25, unitMeasure::Inch) );

   return slab_offset;
}

void CEngAgentImp::GetHaunchDetails(SpanIndexType span,GirderIndexType gdr,HAUNCHDETAILS* pDetails)
{
   SpanGirderHashType key = HashSpanGirder(span,gdr);
   std::map<SpanGirderHashType,HAUNCHDETAILS>::iterator found;
   found = m_HaunchDetails.find(key);

   if ( found == m_HaunchDetails.end() )
   {
      // not found
      m_Designer.GetHaunchDetails(span,gdr,pDetails);

      std::pair<std::map<SpanGirderHashType,HAUNCHDETAILS>::iterator,bool> result;
      result = m_HaunchDetails.insert( std::make_pair(key,*pDetails) );
      ATLASSERT(result.second == true);
      return;
   }

   *pDetails = (*found).second;
}

/////////////////////////////////////////////////////////////////////////////
// IFabricationOptimization
void CEngAgentImp::GetFabricationOptimizationDetails(SpanIndexType span,GirderIndexType gdr,FABRICATIONOPTIMIZATIONDETAILS* pDetails)
{
   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetGirderConfiguration(span,gdr);

   // there is nothing to do if there aren't any strands
   if ( config.PrestressConfig.GetNStrands(pgsTypes::Straight) == 0 && config.PrestressConfig.GetNStrands(pgsTypes::Harped) == 0 )
      return;

   pDetails->Nt = config.PrestressConfig.GetNStrands(pgsTypes::Temporary);
   pDetails->Pjack = config.PrestressConfig.Pjack[pgsTypes::Temporary];

   GET_IFACE(IGirderData,pGirderData);
   const CGirderData* pgirderData = pGirderData->GetGirderData(span,gdr);
   pDetails->TempStrandUsage = pgirderData->PrestressData.TempStrandUsage;

   pgsGirderLiftingChecker lifting_checker(m_pBroker,m_StatusGroupID);

   GET_IFACE(IStrandGeometry,pStrandGeom);
   if ( 0 <  pStrandGeom->GetMaxStrands(span,gdr,pgsTypes::Temporary) && 0 < pDetails->Nt )
   {
      // there are temp strands

      /////////////////////////////////////////////////////////////
      // lifting analysis
      /////////////////////////////////////////////////////////////

      // pretensioned TTS
      config.PrestressConfig.TempStrandUsage = pgsTypes::ttsPretensioned;

      pgsLiftingAnalysisArtifact artifact1;
      GET_IFACE(IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
      lifting_checker.DesignLifting(span,gdr,config,pGirderLiftingPointsOfInterest,&artifact1,LOGGER);
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
      lifting_checker.DesignLifting(span,gdr,config,pGirderLiftingPointsOfInterest,&artifact2,LOGGER);
      pDetails->L[NO_TTS] = artifact2.GetLeftOverhang();
   
      artifact2.GetRequiredConcreteStrength(&fci_comp,&fci_tens,&fci_tens_wrebar);
      minRebarRequired = fci_tens<0;
      fci = Max3(fci_tens, fci_comp, fci_tens_wrebar);
      pDetails->Fci[NO_TTS] = fci;


      // post-tensioned TTS

      // lifting at location for NO_TTS (optional TTS)
      config.PrestressConfig.TempStrandUsage = pgsTypes::ttsPTBeforeLifting;
      config.PrestressConfig.Pjack[pgsTypes::Temporary] = Pjt;

      ConfigStrandFillVector rfillvec = pStrandGeom->ComputeStrandFill(span, gdr, pgsTypes::Temporary, Nt);
      config.PrestressConfig.SetStrandFill(pgsTypes::Temporary, rfillvec);

      HANDLINGCONFIG lift_config;
      lift_config.GdrConfig = config;
      lift_config.LeftOverhang = pDetails->L[NO_TTS];
      lift_config.RightOverhang = pDetails->L[NO_TTS];

      pgsLiftingAnalysisArtifact artifact3;
      lifting_checker.AnalyzeLifting(span,gdr,lift_config,pGirderLiftingPointsOfInterest,&artifact3);
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
      lifting_checker.AnalyzeLifting(span,gdr,lift_config,pGirderLiftingPointsOfInterest,&artifact4);
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
   GET_IFACE(IPrestressStresses,pPS);
   GET_IFACE(IPointOfInterest,pPOI);

   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(span,gdr,pgsTypes::CastingYard,POI_FLEXURESTRESS | POI_TABULAR);
   GDRCONFIG config_WithoutTTS;
   config_WithoutTTS = config;
   config_WithoutTTS.PrestressConfig.Pjack[pgsTypes::Temporary] = 0;
   config_WithoutTTS.PrestressConfig.ClearStrandFill(pgsTypes::Temporary);

   Float64 min_stress_WithoutTTS = DBL_MAX;
   Float64 max_stress_WithoutTTS = -DBL_MAX;
   Float64 min_stress_WithTTS = DBL_MAX;
   Float64 max_stress_WithTTS = -DBL_MAX;
   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPOI.begin(); iter != vPOI.end(); iter++ )
   {
      pgsPointOfInterest poi = *iter;

      Float64 fTopLimitStateMin,fTopLimitStateMax;
      pLS->GetStress(pgsTypes::ServiceI,pgsTypes::CastingYard,poi,pgsTypes::TopGirder,false,SimpleSpan,&fTopLimitStateMin,&fTopLimitStateMax);

      Float64 fBotLimitStateMin,fBotLimitStateMax;
      pLS->GetStress(pgsTypes::ServiceI,pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,false,SimpleSpan,&fBotLimitStateMin,&fBotLimitStateMax);

      Float64 fTopPre_WithoutTTS = pPS->GetDesignStress(pgsTypes::CastingYard,poi,pgsTypes::TopGirder,config_WithoutTTS);
      Float64 fBotPre_WithoutTTS = pPS->GetDesignStress(pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,config_WithoutTTS);

      Float64 fTopMin_WithoutTTS = fTopLimitStateMin + fTopPre_WithoutTTS;
      Float64 fTopMax_WithoutTTS = fTopLimitStateMax + fTopPre_WithoutTTS;

      Float64 fBotMin_WithoutTTS = fBotLimitStateMin + fBotPre_WithoutTTS;
      Float64 fBotMax_WithoutTTS = fBotLimitStateMax + fBotPre_WithoutTTS;

      min_stress_WithoutTTS = Min3(fBotMin_WithoutTTS,fTopMin_WithoutTTS,min_stress_WithoutTTS);
      max_stress_WithoutTTS = Max3(fBotMax_WithoutTTS,fTopMax_WithoutTTS,max_stress_WithoutTTS);
   }

   GET_IFACE(IAllowableConcreteStress,pAllowStress);

   Float64 c = -pAllowStress->GetAllowableCompressiveStressCoefficient(pgsTypes::CastingYard,pgsTypes::ServiceI);
   Float64 t, fmax;
   bool bfMax;
   pAllowStress->GetAllowableTensionStressCoefficient(pgsTypes::CastingYard,pgsTypes::ServiceI,&t,&bfMax,&fmax);

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
            Float64 talt = pAllowStress->GetCastingYardAllowableTensionStressCoefficientWithRebar();
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

   config = pBridge->GetGirderConfiguration(span,gdr);
   config.PrestressConfig.TempStrandUsage = pgsTypes::ttsPretensioned;

   bool bResult;
   std::auto_ptr<pgsHaulingAnalysisArtifact> hauling_artifact_base ( hauling_checker->DesignHauling(span,gdr,config,true,true,pGirderHaulingPointsOfInterest,&bResult,LOGGER));

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

   GET_IFACE(IBridgeMaterial,pMaterial);
   Float64 fcMax = pMaterial->GetFcGdr(span,gdr);

   Float64 fcReqd = -1;

   ATLASSERT( IsEqual(hauling_artifact->GetLeadingOverhang(),hauling_artifact->GetTrailingOverhang()) );

   GET_IFACE(IGirderHaulingSpecCriteria,pCriteria);
   Float64 min_location = max(pCriteria->GetMinimumHaulingSupportLocation(span,gdr,pgsTypes::metStart),pCriteria->GetMinimumHaulingSupportLocation(span,gdr,pgsTypes::metEnd));
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

      std::auto_ptr<pgsHaulingAnalysisArtifact> hauling_artifact2( hauling_checker->AnalyzeHauling(span,gdr,hauling_config,pGirderHaulingPointsOfInterest) );
   
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
   Float64 FSrMin = pCriteria->GetHaulingRolloverFs();

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

      std::auto_ptr<pgsHaulingAnalysisArtifact> hauling_artifact2_base( hauling_checker->AnalyzeHauling(span,gdr,hauling_config,pGirderHaulingPointsOfInterest) );

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
const pgsGirderArtifact* CEngAgentImp::GetArtifact(SpanIndexType span,GirderIndexType gdr)
{
   ValidateArtifacts(span,gdr);

   return FindArtifact(span,gdr);
}

const pgsRatingArtifact* CEngAgentImp::GetRatingArtifact(GirderIndexType gdrLineIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIndex)
{
   ValidateRatingArtifacts(gdrLineIdx,ratingType,vehicleIndex);
   return FindRatingArtifact(gdrLineIdx,ratingType,vehicleIndex);
}

const pgsDesignArtifact* CEngAgentImp::CreateDesignArtifact(SpanIndexType span,GirderIndexType gdr,arDesignOptions options)
{
   SpanGirderHashType key = HashSpanGirder(span,gdr);
   std::map<SpanGirderHashType,pgsDesignArtifact>::size_type cRemove = m_DesignArtifacts.erase(key);
   ATLASSERT( cRemove == 0 || cRemove == 1 );

   std::pair<std::map<SpanGirderHashType,pgsDesignArtifact>::iterator,bool> retval;

   pgsDesignArtifact artifact(span,gdr);
   try 
   {
      artifact = m_Designer.Design(span,gdr,options);
   }
   catch (pgsDesignArtifact::Outcome outcome)
   {
      if ( outcome == pgsDesignArtifact::DesignCancelled )
         return NULL;
      else 
         artifact.SetOutcome(outcome);
   }

   retval = m_DesignArtifacts.insert(std::make_pair(key,artifact));
   return &((*retval.first).second);
}

const pgsDesignArtifact* CEngAgentImp::GetDesignArtifact(SpanIndexType span,GirderIndexType gdr)
{
   SpanGirderHashType key = HashSpanGirder(span,gdr);
   std::map<SpanGirderHashType,pgsDesignArtifact>::iterator found;
   found = m_DesignArtifacts.find(key);
   if ( found == m_DesignArtifacts.end() )
      return 0;

   return &((*found).second);
}

void CEngAgentImp::CreateLiftingAnalysisArtifact(SpanIndexType span,GirderIndexType gdr,Float64 supportLoc,pgsLiftingAnalysisArtifact* pArtifact)
{
   bool bCreate = false;

   SpanGirderHashType key = HashSpanGirder(span,gdr);
   typedef std::map<SpanGirderHashType, std::map<Float64,pgsLiftingAnalysisArtifact,Float64_less> >::iterator iter_type;
   iter_type found_gdr;
   found_gdr = m_LiftingArtifacts.find(key);
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
      std::pair<iter_type,bool> iter = m_LiftingArtifacts.insert( std::make_pair(key, artifacts) );
      found_gdr = iter.first;
      bCreate = true;
   }


   if ( bCreate )
   {
      HANDLINGCONFIG config;
      GET_IFACE(IPointOfInterest,pPOI);
      std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_MIDSPAN);
      pgsPointOfInterest poi = vPOI[0];

      GET_IFACE(IBridge,pBridge);
      config.GdrConfig = pBridge->GetGirderConfiguration(poi.GetSpan(),poi.GetGirder());
      config.LeftOverhang = supportLoc;
      config.RightOverhang = supportLoc;
      Float64 slabOffset = pBridge->GetSlabOffset(poi);

      pgsGirderLiftingChecker checker(m_pBroker,m_StatusGroupID);
      GET_IFACE(IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
      checker.AnalyzeLifting(span,gdr,config,pGirderLiftingPointsOfInterest,pArtifact);

      (*found_gdr).second.insert( std::make_pair(supportLoc,*pArtifact) );
   }
}

const pgsHaulingAnalysisArtifact* CEngAgentImp::CreateHaulingAnalysisArtifact(SpanIndexType span,GirderIndexType gdr,Float64 leftSupportLoc,Float64 rightSupportLoc)
{
   const pgsHaulingAnalysisArtifact* pArtifact(NULL);

   bool bCreate = false;

   SpanGirderHashType key = HashSpanGirder(span,gdr);
   typedef std::map<SpanGirderHashType, std::map<Float64,boost::shared_ptr<pgsHaulingAnalysisArtifact>,Float64_less> >::iterator iter_type;
   iter_type found_gdr;
   found_gdr = m_HaulingArtifacts.find(key);
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
      std::pair<iter_type,bool> iter = m_HaulingArtifacts.insert( std::make_pair(key, artifacts) );
      found_gdr = iter.first;
      bCreate = true;
   }

   if ( bCreate )
   {
      HANDLINGCONFIG config;
      GET_IFACE(IPointOfInterest,pPOI);
      std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_MIDSPAN);
      pgsPointOfInterest poi = vPOI[0];

      GET_IFACE(IBridge,pBridge);
      config.GdrConfig = pBridge->GetGirderConfiguration(poi.GetSpan(),poi.GetGirder());
      config.LeftOverhang  = leftSupportLoc;
      config.RightOverhang = rightSupportLoc;

      Float64 slabOffset = pBridge->GetSlabOffset(poi);

      GET_IFACE(IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);

      // Use factory to create appropriate hauling checker
      pgsGirderHandlingChecker checker_factory(m_pBroker,m_StatusGroupID);
      std::auto_ptr<pgsGirderHaulingChecker> hauling_checker( checker_factory.CreateGirderHaulingChecker() );

      boost::shared_ptr<pgsHaulingAnalysisArtifact> my_art (hauling_checker->AnalyzeHauling(span,gdr,config,pGirderHaulingPointsOfInterest));

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
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
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

HRESULT CEngAgentImp::OnGirderChanged(SpanIndexType span,GirderIndexType gdr,Uint32 lHint)
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

void CEngAgentImp::BuildLosses(const pgsPointOfInterest& poi,LOSSDETAILS* pLosses)
{
   PRECONDITION( pLosses != 0 );
   m_PsForceEngineer.ComputeLosses(poi,pLosses);
}

void CEngAgentImp::BuildLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,LOSSDETAILS* pLosses)
{
   PRECONDITION( pLosses != 0 );
   m_PsForceEngineer.ComputeLosses(poi,config,pLosses);
}

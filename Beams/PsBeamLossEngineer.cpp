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

// PsBeamLossEngineer.cpp : Implementation of CPsBeamLossEngineer
#include "stdafx.h"
#include "PsBeamLossEngineer.h"
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

#if defined _DEBUG
#include <IFace\Project.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDesignLosses::CDesignLosses()
{
   Invalidate();
}

void CDesignLosses::Invalidate()
{
   m_Losses.clear();
}

const LOSSDETAILS* CDesignLosses::GetFromCache(const pgsPointOfInterest& poi, const GDRCONFIG& config)
{
   Losses* pLosses = nullptr;

   auto found(m_Losses.find(poi));
   if ( found != m_Losses.end() )
   {
      pLosses = &(found->second);
   }

   if ( pLosses == nullptr )
   {
      return nullptr; // not found... we don't have it cached
   }

   // have results for this POI cached, but was it for the same configuration?
   if ( config.IsFlexuralDataEqual(pLosses->m_Config) )
   {
      // yep... return the results
      return &(pLosses->m_Details);
   }
   else
   {
      // the one that was found doesn't match for this POI, so remove it because
      // we have new values for this POI
      m_Losses.erase(found);
      return nullptr;
   }

   ATLASSERT(false); // should never get here
}

void CDesignLosses::SaveToCache(const pgsPointOfInterest& poi, const GDRCONFIG& config, const LOSSDETAILS& losses)
{
   Losses l;
   l.m_Config = config;
   l.m_Details = losses;

   auto result = m_Losses.insert( std::make_pair(poi,l) );
   ATLASSERT( result.second == true );
}

/////////////////////////////////////////////////////////////////////////////
// CUBeamPsLossEngineer
HRESULT CPsBeamLossEngineer::FinalConstruct()
{
   return S_OK;
}

void CPsBeamLossEngineer::SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   m_Engineer.Init(m_pBroker,m_StatusGroupID);
}

const LOSSDETAILS* CPsBeamLossEngineer::GetLosses(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx)
{
   ATLASSERT(poi.GetID() != INVALID_ID);
   std::map<PoiIDType,LOSSDETAILS>::const_iterator found;
   PoiIDType key =poi.GetID();
   found = m_PsLosses.find( key );
   if ( found == m_PsLosses.end() )
   {
      // losses not found... compute them
      LOSSDETAILS details = m_Engineer.ComputeLosses((CPsLossEngineer::BeamType)m_BeamType,poi);
      auto itf = m_PsLosses.insert(std::make_pair(key,details));
      ATLASSERT(itf.second);
      return &(itf.first->second);
   }
   else
   {
      return &(*found).second;
   }
}

const LOSSDETAILS* CPsBeamLossEngineer::GetLosses(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx)
{
   ATLASSERT(intervalIdx == INVALID_INDEX); // this kind of loss calculation is for all intervals... always

#if defined _DEBUG
   GET_IFACE(ILossParameters,pLossParameters);
   ATLASSERT( pLossParameters->GetLossMethod() != pgsTypes::TIME_STEP );
   // this shouldn't ever happen because the beam factory should create the appropreate loss engineer
   // but just in case we create a new beam type and forget to do this in the factor, assert here
#endif

   const LOSSDETAILS* pLossDetails = m_DesignLosses.GetFromCache(poi,config);
   if ( pLossDetails == nullptr )
   {
      LOSSDETAILS details = m_Engineer.ComputeLossesForDesign((CPsLossEngineer::BeamType)m_BeamType,poi,config);
      m_DesignLosses.SaveToCache(poi,config,details);
      pLossDetails = m_DesignLosses.GetFromCache(poi,config);
      ATLASSERT(pLossDetails != nullptr);
   }

   return pLossDetails;
}

void CPsBeamLossEngineer::ClearDesignLosses()
{
   m_DesignLosses.Invalidate();
}

void CPsBeamLossEngineer::BuildReport(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   m_Engineer.BuildReport((CPsLossEngineer::BeamType)m_BeamType,girderKey,pChapter,pDisplayUnits);
}

void CPsBeamLossEngineer::ReportFinalLosses(const CGirderKey& girderKey,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits)
{
   m_Engineer.ReportFinalLosses((CPsLossEngineer::BeamType)m_BeamType,girderKey,pChapter,pDisplayUnits);
}

const ANCHORSETDETAILS* CPsBeamLossEngineer::GetGirderTendonAnchorSetDetails(const CGirderKey& girderKey, DuctIndexType ductIdx)
{
   // This returns basically a dummy object... non-spliced girders don't have PT so
   // there is no anchor set... this implementation keeps the compiler happy
   ATLASSERT(false); // why did this method get called? it shouldn't happen
   return nullptr;
}

const ANCHORSETDETAILS* CPsBeamLossEngineer::GetSegmentTendonAnchorSetDetails(const CSegmentKey& segmentKey, DuctIndexType ductIdx)
{
   // This returns basically a dummy object... non-spliced girders don't have PT so
   // there is no anchor set... this implementation keeps the compiler happy
   ATLASSERT(false); // why did this method get called? it shouldn't happen
   return nullptr;
}

Float64 CPsBeamLossEngineer::GetGirderTendonElongation(const CGirderKey& girderKey,DuctIndexType ductIdx,pgsTypes::MemberEndType endType)
{
   // This returns basically a dummy object... non-spliced girders don't have PT so
   // there is no anchor set... this implementation keeps the compiler happy
   ATLASSERT(false); // why did this method get called? it shouldn't happen
   return 0;
}

Float64 CPsBeamLossEngineer::GetSegmentTendonElongation(const CSegmentKey& segmentKey, DuctIndexType ductIdx, pgsTypes::MemberEndType endType)
{
   // This returns basically a dummy object... non-spliced girders don't have PT so
   // there is no anchor set... this implementation keeps the compiler happy
   ATLASSERT(false); // why did this method get called? it shouldn't happen
   return 0;
}

void CPsBeamLossEngineer::GetGirderTendonAverageFrictionAndAnchorSetLoss(const CGirderKey& girderKey, DuctIndexType ductIdx, Float64* pfpF, Float64* pfpA)
{
   // This returns basically a dummy object... non-spliced girders don't have PT so
   // there is no anchor set... this implementation keeps the compiler happy
   ATLASSERT(false); // why did this method get called? it shouldn't happen
   *pfpF = 0;
   *pfpA = 0;
}

void CPsBeamLossEngineer::GetSegmentTendonAverageFrictionAndAnchorSetLoss(const CSegmentKey& segmentKey, DuctIndexType ductIdx, Float64* pfpF, Float64* pfpA)
{
   // This returns basically a dummy object... non-spliced girders don't have PT so
   // there is no anchor set... this implementation keeps the compiler happy
   ATLASSERT(false); // why did this method get called? it shouldn't happen
   *pfpF = 0;
   *pfpA = 0;
}

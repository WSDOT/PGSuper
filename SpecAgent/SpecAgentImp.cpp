///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// SpecAgentImp.cpp : Implementation of CSpecAgentImp
#include "stdafx.h"
#include "SpecAgent.h"
#include "SpecAgentImp.h"
#include "StatusItems.h"

#include <PgsExt\BridgeDescription2.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <Lrfd\PsStrand.h>
#include <Lrfd\Rebar.h>

#include <IFace\StatusCenter.h>
#include <IFace\PrestressForce.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>

#include <Units\SysUnits.h>

#include <PgsExt\GirderLabel.h>
#include <MfcTools\Exceptions.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecAgentImp


/////////////////////////////////////////////////////////////////////////////
// IAgent
//
STDMETHODIMP CSpecAgentImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CSpecAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface( IID_IStressCheck,                   this );
   pBrokerInit->RegInterface( IID_IAllowableStrandStress,         this );
   pBrokerInit->RegInterface( IID_IAllowableTendonStress,         this );
   pBrokerInit->RegInterface( IID_IAllowableConcreteStress,       this );
   pBrokerInit->RegInterface( IID_ITransverseReinforcementSpec,   this );
   pBrokerInit->RegInterface( IID_IPrecastIGirderDetailsSpec,     this );
   pBrokerInit->RegInterface( IID_ISegmentLiftingSpecCriteria,    this );
   pBrokerInit->RegInterface( IID_ISegmentHaulingSpecCriteria,    this );
   pBrokerInit->RegInterface( IID_IKdotGirderHaulingSpecCriteria, this );
   pBrokerInit->RegInterface( IID_IDebondLimits,                  this );
   pBrokerInit->RegInterface( IID_IResistanceFactors,             this );
   pBrokerInit->RegInterface( IID_IInterfaceShearRequirements,    this );
   pBrokerInit->RegInterface( IID_IDuctLimits,                    this );

   return S_OK;
}

STDMETHODIMP CSpecAgentImp::Init()
{
   CREATE_LOGFILE("SpecAgent");
   EAF_AGENT_INIT;
   m_scidHaulTruckError = pStatusCenter->RegisterCallback(new pgsHaulTruckStatusCallback(m_pBroker, eafTypes::statusError));
   return AGENT_S_SECONDPASSINIT;
}

STDMETHODIMP CSpecAgentImp::Init2()
{
   // Attach to connection points
   CComQIPtr<IBrokerInitEx2, &IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   // Connection point for the bridge description
   hr = pBrokerInit->FindConnectionPoint(IID_IBridgeDescriptionEventSink, &pCP);
   ATLASSERT(SUCCEEDED(hr));
   hr = pCP->Advise(GetUnknown(), &m_dwBridgeDescCookie);
   ATLASSERT(SUCCEEDED(hr));
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   return S_OK;
}

STDMETHODIMP CSpecAgentImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_SpecAgent;
   return S_OK;
}

STDMETHODIMP CSpecAgentImp::Reset()
{
   return S_OK;
}

STDMETHODIMP CSpecAgentImp::ShutDown()
{
   //
   // Detach to connection points
   //
   CComQIPtr<IBrokerInitEx2, &IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   hr = pBrokerInit->FindConnectionPoint(IID_IBridgeDescriptionEventSink, &pCP);
   ATLASSERT(SUCCEEDED(hr));
   hr = pCP->Unadvise(m_dwBridgeDescCookie);
   ATLASSERT(SUCCEEDED(hr));
   pCP.Release(); // Recycle the connection point

   EAF_AGENT_CLEAR_INTERFACE_CACHE;
   CLOSE_LOGFILE;
   return S_OK;
}

////////////////////////////////////////////////////////////////////////
// IBridgeDescriptionEventSink
//
HRESULT CSpecAgentImp::OnBridgeChanged(CBridgeChangedHint* pHint)
{
   //   LOG(_T("OnBridgeChanged Event Received"));
   Invalidate();
   return S_OK;
}

HRESULT CSpecAgentImp::OnGirderFamilyChanged()
{
   //   LOG(_T("OnGirderFamilyChanged Event Received"));
   Invalidate();
   return S_OK;
}

HRESULT CSpecAgentImp::OnGirderChanged(const CGirderKey& girderKey, Uint32 lHint)
{
   Invalidate();
   return S_OK;
}

HRESULT CSpecAgentImp::OnLiveLoadChanged()
{
   // No changes necessary to bridge model
   LOG(_T("OnLiveLoadChanged Event Received"));
   return S_OK;
}

HRESULT CSpecAgentImp::OnLiveLoadNameChanged(LPCTSTR strOldName, LPCTSTR strNewName)
{
   // No changes necessary to bridge model
   LOG(_T("OnLiveLoadNameChanged Event Received"));
   return S_OK;
}

HRESULT CSpecAgentImp::OnConstructionLoadChanged()
{
   LOG(_T("OnConstructionLoadChanged Event Received"));
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IStressCheck
//
std::vector<StressCheckTask> CSpecAgentImp::GetStressCheckTasks(const CGirderKey& girderKey, bool bDesign) const
{
   std::vector<StressCheckTask> vStressCheckTasks;

   GET_IFACE(IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      std::vector<StressCheckTask> vTasks = GetStressCheckTasks(segmentKey,bDesign);
      vStressCheckTasks.insert(std::end(vStressCheckTasks), std::begin(vTasks), std::end(vTasks));
   }

   std::sort(std::begin(vStressCheckTasks), std::end(vStressCheckTasks));
   vStressCheckTasks.erase(std::unique(std::begin(vStressCheckTasks), std::end(vStressCheckTasks)), vStressCheckTasks.end());
   return vStressCheckTasks;
}

std::vector<StressCheckTask> CSpecAgentImp::GetStressCheckTasks(const CSegmentKey& segmentKey, bool bDesign) const
{
   std::vector<StressCheckTask> vStressCheckTasks;

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType noncompositeIntervalIdx = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   GET_IFACE_NOCHECK(IGirder, pGirder);
   GET_IFACE_NOCHECK(IBridge, pBridge);

   vStressCheckTasks.emplace_back(releaseIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
   vStressCheckTasks.emplace_back(releaseIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);

   if (CheckTemporaryStresses())
   {
      GET_IFACE(IStrandGeometry, pStrandGeom);
      StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Temporary);
      if (tsRemovalIntervalIdx != INVALID_INDEX && 0 < Nt || bDesign)
      {
         // always include temporary strand removal task for design... design may start without TTS but then add them
         // if that happens, we need a task
         vStressCheckTasks.emplace_back(tsRemovalIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
         vStressCheckTasks.emplace_back(tsRemovalIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);
      }

      // this is the last interval when the girder is acting by it self... max load case for bare girder
      vStressCheckTasks.emplace_back(noncompositeIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
      vStressCheckTasks.emplace_back(noncompositeIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);

      if (pGirder->HasStructuralLongitudinalJoints() && pBridge->GetDeckType() != pgsTypes::sdtNone)
      {
         // interval when the deck is cast onto the girders, with composite longitudinal joints, but before the deck adds to the composite section
         IntervalIndexType castDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();

         vStressCheckTasks.emplace_back(castDeckIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
         vStressCheckTasks.emplace_back(castDeckIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);
      }
   }

   // final without live load (effective prestress + permanent loads)
   vStressCheckTasks.emplace_back(lastIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression,false /*explicitly no live load*/);

   if (CheckFinalDeadLoadTensionStress())
   {
      // final without live load tension is an option check
      vStressCheckTasks.emplace_back(lastIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension, false /*explicitly no live load*/);
   }

   // final with live load
   vStressCheckTasks.emplace_back(lastIntervalIdx, pgsTypes::ServiceIII, pgsTypes::Tension);
   vStressCheckTasks.emplace_back(lastIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
   vStressCheckTasks.emplace_back(lastIntervalIdx, lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI, pgsTypes::Compression);

   // for spliced girders, spec changes must occur every time there is a change in boundary condition, change in external loading, and application of prestress force
   GET_IFACE(IDocumentType, pDocType);
   if (pDocType->IsPGSpliceDocument())
   {
      // only need to check stress during storage if support conditions are different than release support conditions
      // because this would consititute a change in loading
      Float64 LeftReleasePoint, RightReleasePoint;
      pGirder->GetSegmentReleaseSupportLocations(segmentKey, &LeftReleasePoint, &RightReleasePoint);

      Float64 LeftStoragePoint, RightStoragePoint;
      pGirder->GetSegmentStorageSupportLocations(segmentKey, &LeftStoragePoint, &RightStoragePoint);

      GET_IFACE(IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 2);
      Float64 LeftBrgPoint = vPoi.front().get().GetDistFromStart();
      Float64 RightBrgPoint = vPoi.back().get().GetDistFromStart();
      Float64 Ls = pBridge->GetSegmentLength(segmentKey);
      RightBrgPoint = Ls - RightBrgPoint;

      if (!IsEqual(LeftReleasePoint, LeftStoragePoint) || !IsEqual(RightReleasePoint, RightStoragePoint))
      {
         IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
         vStressCheckTasks.emplace_back(storageIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
         vStressCheckTasks.emplace_back(storageIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);
      }

      if (!IsEqual(LeftBrgPoint, LeftStoragePoint) || !IsEqual(RightBrgPoint, RightStoragePoint))
      {
         IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
         vStressCheckTasks.emplace_back(erectionIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
         vStressCheckTasks.emplace_back(erectionIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);
      }

      // Segment erection intervals only need to be checked if there are piers or temporary supports
      // that cause negative moments in the segment. This occurs when the segment is continuous over
      // these support types. Erecting the segment causes a change in loading condition compared to the
      // simple span loading condition before erection.
      bool bIsContinuousOverSupport = false;
      GET_IFACE(IBridgeDescription, pIBridgeDesc);
      const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
      std::vector<const CPierData2*> vPiers = pSegment->GetPiers();
      for (const auto* pPier : vPiers)
      {
         if (pPier->IsInteriorPier())
         {
            pgsTypes::PierSegmentConnectionType cType = pPier->GetSegmentConnectionType();
            if (cType == pgsTypes::psctContinuousSegment || cType == pgsTypes::psctIntegralSegment)
            {
               bIsContinuousOverSupport = true;
               break;
            }
         }
      }

      if (!bIsContinuousOverSupport)
      {
         std::vector<const CTemporarySupportData*> vTempSupports = pSegment->GetTemporarySupports();
         for (const auto* pTS : vTempSupports)
         {
            if (pTS->GetConnectionType() == pgsTypes::tsctContinuousSegment)
            {
               bIsContinuousOverSupport = true;
               break;
            }
         }
      }

      if (bIsContinuousOverSupport)
      {
         IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
         vStressCheckTasks.emplace_back(erectionIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
         vStressCheckTasks.emplace_back(erectionIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);
      }

      // Spec check whenever a segment tendon is stressed
      GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
      DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
      if (0 < nSegmentDucts)
      {
         IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);
         vStressCheckTasks.emplace_back(stressTendonIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
         vStressCheckTasks.emplace_back(stressTendonIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);
      }

      // Spec check whenever a girder tendon is stressed
      GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);
      DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(segmentKey);
      for (DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++)
      {
         IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressGirderTendonInterval(segmentKey,ductIdx);
         vStressCheckTasks.emplace_back(stressTendonIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
         vStressCheckTasks.emplace_back(stressTendonIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);
      }

      // check each time a deck region is cast
      IndexType nCastingRegions = pBridge->GetDeckCastingRegionCount();
      for (IndexType regionIdx = 0; regionIdx < nCastingRegions; regionIdx++)
      {
         IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(regionIdx);
         vStressCheckTasks.emplace_back(castDeckIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
         vStressCheckTasks.emplace_back(castDeckIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);
      }

      // Spec check whenever a user defined load is applied
      SpanIndexType startSpanIdx, endSpanIdx;
      pBridge->GetGirderGroupSpans(segmentKey.groupIndex, &startSpanIdx, &endSpanIdx);
      for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
      {
         CSpanKey spanKey(spanIdx, segmentKey.girderIndex);
         std::vector<IntervalIndexType> vUserLoadIntervals(pIntervals->GetUserDefinedLoadIntervals(spanKey));
         for (auto intervalIdx : vUserLoadIntervals)
         {
            vStressCheckTasks.emplace_back(intervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
            vStressCheckTasks.emplace_back(intervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);
         }
      }

      // Spec check when the railing system is installed
      IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
      vStressCheckTasks.emplace_back(railingSystemIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
      vStressCheckTasks.emplace_back(railingSystemIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);

      // Spec check when the overlay is installed
      IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
      if (overlayIntervalIdx != INVALID_INDEX)
      {
         vStressCheckTasks.emplace_back(overlayIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
         vStressCheckTasks.emplace_back(overlayIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);
      }
   }

   std::sort(std::begin(vStressCheckTasks), std::end(vStressCheckTasks));
   vStressCheckTasks.erase(std::unique(std::begin(vStressCheckTasks), std::end(vStressCheckTasks)), vStressCheckTasks.end());
   return vStressCheckTasks;
}

std::vector<IntervalIndexType> CSpecAgentImp::GetStressCheckIntervals(const CGirderKey& girderKey, bool bDesign) const
{
   std::vector<IntervalIndexType> vIntervals;
   GET_IFACE(IBridge, pBridge);
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? pBridge->GetGirderGroupCount() - 1 : firstGroupIdx);
   for (GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++)
   {
      CGirderKey thisGirderKey(grpIdx, girderKey.girderIndex);
      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         CSegmentKey segmentKey(thisGirderKey, segIdx);
         std::vector<StressCheckTask> vStressChecks = GetStressCheckTasks(segmentKey, bDesign);
         for (const auto& task : vStressChecks)
         {
            vIntervals.push_back(task.intervalIdx);
         }
      }
   }
   std::sort(std::begin(vIntervals), std::end(vIntervals));
   vIntervals.erase(std::unique(std::begin(vIntervals), std::end(vIntervals)), std::end(vIntervals));
   return vIntervals;
}

/////////////////////////////////////////////////////////////////////////////
// IAllowableStrandStress
//
bool CSpecAgentImp::CheckStressAtJacking() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckStrandStress(CSS_AT_JACKING);
}

bool CSpecAgentImp::CheckStressBeforeXfer() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckStrandStress(CSS_BEFORE_TRANSFER);
}

bool CSpecAgentImp::CheckStressAfterXfer() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckStrandStress(CSS_AFTER_TRANSFER);
}

bool CSpecAgentImp::CheckStressAfterLosses() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckStrandStress(CSS_AFTER_ALL_LOSSES);
}

Float64 CSpecAgentImp::GetAllowableAtJacking(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const
{
   if ( !CheckStressAtJacking() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,strandType);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetStrandStressCoefficient(CSS_AT_JACKING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetAllowableBeforeXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const
{
   if ( !CheckStressBeforeXfer() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,strandType);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetStrandStressCoefficient(CSS_BEFORE_TRANSFER,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetAllowableAfterXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const
{
   if ( !CheckStressAfterXfer() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,strandType);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetStrandStressCoefficient(CSS_AFTER_TRANSFER,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetAllowableAfterLosses(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const
{
   if ( !CheckStressAfterLosses() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetStrandMaterial(segmentKey,strandType);

   Float64 fpy = lrfdPsStrand::GetYieldStrength(pStrand->GetGrade(),pStrand->GetType());

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetStrandStressCoefficient(CSS_AFTER_ALL_LOSSES,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff*fpy;
}

/////////////////////////////////////////////////////////
// IAllowableTendonStress
//
bool CSpecAgentImp::CheckTendonStressAtJacking() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckTendonStressAtJacking();
}

bool CSpecAgentImp::CheckTendonStressPriorToSeating() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckTendonStressPriorToSeating();
}

Float64 CSpecAgentImp::GetSegmentTendonAllowableAtJacking(const CSegmentKey& segmentKey) const
{
   if (!CheckTendonStressAtJacking())
   {
      return 0.0;
   }

   GET_IFACE(IMaterials, pMaterial);
   const matPsStrand* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetSegmentTendonAllowableCoefficientAtJacking(segmentKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetSegmentTendonAllowablePriorToSeating(const CSegmentKey& segmentKey) const
{
   if (!CheckTendonStressPriorToSeating())
   {
      return 0.0;
   }

   GET_IFACE(IMaterials, pMaterial);
   const matPsStrand* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   Float64 fpy = lrfdPsStrand::GetYieldStrength(pStrand->GetGrade(), pStrand->GetType());

   Float64 coeff = GetSegmentTendonAllowableCoefficientPriorToSeating(segmentKey);

   return coeff*fpy;
}

Float64 CSpecAgentImp::GetSegmentTendonAllowableAfterAnchorSetAtAnchorage(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const matPsStrand* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetSegmentTendonAllowableCoefficientAfterAnchorSetAtAnchorage(segmentKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetSegmentTendonAllowableAfterAnchorSet(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const matPsStrand* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetSegmentTendonAllowableCoefficientAfterAnchorSet(segmentKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetSegmentTendonAllowableAfterLosses(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const matPsStrand* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   Float64 fpy = lrfdPsStrand::GetYieldStrength(pStrand->GetGrade(), pStrand->GetType());

   Float64 coeff = GetSegmentTendonAllowableCoefficientAfterLosses(segmentKey);

   return coeff*fpy;
}

Float64 CSpecAgentImp::GetSegmentTendonAllowableCoefficientAtJacking(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const matPsStrand* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_AT_JACKING, pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);
   return coeff;
}

Float64 CSpecAgentImp::GetSegmentTendonAllowableCoefficientPriorToSeating(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const matPsStrand* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_PRIOR_TO_SEATING, pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);
   return coeff;
}

Float64 CSpecAgentImp::GetSegmentTendonAllowableCoefficientAfterAnchorSetAtAnchorage(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const matPsStrand* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_ANCHORAGES_AFTER_SEATING, pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);
   return coeff;
}

Float64 CSpecAgentImp::GetSegmentTendonAllowableCoefficientAfterAnchorSet(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const matPsStrand* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_ELSEWHERE_AFTER_SEATING, pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);
   return coeff;
}

Float64 CSpecAgentImp::GetSegmentTendonAllowableCoefficientAfterLosses(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const matPsStrand* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_AFTER_ALL_LOSSES, pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff;
}

Float64 CSpecAgentImp::GetGirderTendonAllowableAtJacking(const CGirderKey& girderKey) const
{
   if ( !CheckTendonStressAtJacking() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetGirderTendonAllowableCoefficientAtJacking(girderKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetGirderTendonAllowablePriorToSeating(const CGirderKey& girderKey) const
{
   if ( !CheckTendonStressPriorToSeating() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   Float64 fpy = lrfdPsStrand::GetYieldStrength(pStrand->GetGrade(),pStrand->GetType());

   Float64 coeff = GetGirderTendonAllowableCoefficientPriorToSeating(girderKey);

   return coeff*fpy;
}

Float64 CSpecAgentImp::GetGirderTendonAllowableAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetGirderTendonAllowableCoefficientAfterAnchorSetAtAnchorage(girderKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetGirderTendonAllowableAfterAnchorSet(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   Float64 fpu = lrfdPsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetGirderTendonAllowableCoefficientAfterAnchorSet(girderKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetGirderTendonAllowableAfterLosses(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   Float64 fpy = lrfdPsStrand::GetYieldStrength(pStrand->GetGrade(),pStrand->GetType());

   Float64 coeff = GetGirderTendonAllowableCoefficientAfterLosses(girderKey);

   return coeff*fpy;
}

Float64 CSpecAgentImp::GetGirderTendonAllowableCoefficientAtJacking(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_AT_JACKING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);
   return coeff;
}

Float64 CSpecAgentImp::GetGirderTendonAllowableCoefficientPriorToSeating(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_PRIOR_TO_SEATING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);
   return coeff;
}

Float64 CSpecAgentImp::GetGirderTendonAllowableCoefficientAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_ANCHORAGES_AFTER_SEATING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);
   return coeff;
}

Float64 CSpecAgentImp::GetGirderTendonAllowableCoefficientAfterAnchorSet(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_ELSEWHERE_AFTER_SEATING,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);
   return coeff;
}

Float64 CSpecAgentImp::GetGirderTendonAllowableCoefficientAfterLosses(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const matPsStrand* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 coeff = pSpec->GetTendonStressCoefficient(CSS_AFTER_ALL_LOSSES,pStrand->GetType() == matPsStrand::LowRelaxation ? LOW_RELAX : STRESS_REL);

   return coeff;
}

/////////////////////////////////////////////////////////////////////////////
// IAllowableConcreteStress
//
Float64 CSpecAgentImp::GetAllowableCompressionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         return GetClosureJointAllowableCompressionStress(poi,task);
      }
      else
      {
         return GetSegmentAllowableCompressionStress(poi,task);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      return GetDeckAllowableCompressionStress(poi,task);
   }
}

Float64 CSpecAgentImp::GetAllowableTensionStress(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         return GetClosureJointAllowableTensionStress(poi,task,bWithBondedReinforcement,bInPrecompressedTensileZone);
      }
      else
      {
         return GetSegmentAllowableTensionStress(poi,task,bWithBondedReinforcement);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      return GetDeckAllowableTensionStress(poi,task,bWithBondedReinforcement);
   }
}

Float64 CSpecAgentImp::GetAllowableTensionStress(pgsTypes::LoadRatingType ratingType,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation) const
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType ratingIntervalIdx = pIntervals->GetLoadRatingInterval();

   Float64 fc;
   Float64 lambda;
   GET_IFACE(IMaterials,pMaterials);
   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         fc = pMaterials->GetClosureJointDesignFc(closureKey,ratingIntervalIdx);
         lambda = pMaterials->GetClosureJointLambda(closureKey);
      }
      else
      {
         fc = pMaterials->GetSegmentDesignFc(segmentKey,ratingIntervalIdx);
         lambda = pMaterials->GetSegmentLambda(segmentKey);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      fc = pMaterials->GetDeckDesignFc(ratingIntervalIdx);
      lambda = pMaterials->GetDeckLambda();
   }

   GET_IFACE(IRatingSpecification,pRatingSpec);
   Float64 x = pRatingSpec->GetAllowableTensionCoefficient(ratingType);

   Float64 fallow = x*lambda*sqrt(fc);
   return fallow;
}

Float64 CSpecAgentImp::GetAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         return GetClosureJointAllowableCompressionStressCoefficient(poi,task);
      }
      else
      {
         return GetSegmentAllowableCompressionStressCoefficient(poi,task);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      return GetDeckAllowableCompressionStressCoefficient(poi,task);
   }
}

void CSpecAgentImp::GetAllowableTensionStressCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         GetClosureJointAllowableTensionStressCoefficient(poi,task,bWithBondedReinforcement,bInPrecompressedTensileZone,pCoeff,pbMax,pMaxValue);
      }
      else
      {
         GetSegmentAllowableTensionStressCoefficient(poi,task,bWithBondedReinforcement,pCoeff,pbMax,pMaxValue);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      GetDeckAllowableTensionStressCoefficient(poi,task,bWithBondedReinforcement,pCoeff,pbMax,pMaxValue);
   }
}

Float64 CSpecAgentImp::GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,task));

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetSegmentDesignFc(segmentKey,task.intervalIdx);

   Float64 fAllow = GetSegmentAllowableCompressionStress(poi,task,fc);
   return fAllow;
}

Float64 CSpecAgentImp::GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   GET_IFACE(IPointOfInterest,pPoi);
   CClosureKey closureKey;
   VERIFY(pPoi->IsInClosureJoint(poi,&closureKey));

   ATLASSERT(IsStressCheckApplicable(closureKey,task));

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetClosureJointDesignFc(closureKey,task.intervalIdx);

   Float64 fAllow = GetClosureJointAllowableCompressionStress(poi,task,fc);
   return fAllow;
}

Float64 CSpecAgentImp::GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,task));

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetDeckDesignFc(task.intervalIdx);

   Float64 fAllow = GetDeckAllowableCompressionStress(poi,task,fc);
   return fAllow;
}

Float64 CSpecAgentImp::GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,task));

   if ( IsLoadRatingServiceIIILimitState(task.limitState) )
   {
#if defined _DEBUG
      // allowable stresses during load rating only make sense if live load is applied
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      ATLASSERT(liveLoadIntervalIdx <= task.intervalIdx );
#endif
      pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(task.limitState);
      return GetAllowableTensionStress(ratingType,poi,pgsTypes::BottomGirder);
   }

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetSegmentDesignFc(segmentKey,task.intervalIdx);

   Float64 fAllow = GetSegmentAllowableTensionStress(poi,task,fc,bWithBondedReinforcement);
   return fAllow;
}

Float64 CSpecAgentImp::GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   if ( IsLoadRatingServiceIIILimitState(task.limitState) )
   {
#if defined _DEBUG
      // allowable stresses during load rating only make sense if live load is applied
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      ATLASSERT(liveLoadIntervalIdx <= task.intervalIdx );
#endif
      pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(task.limitState);
      return GetAllowableTensionStress(ratingType,poi,pgsTypes::BottomGirder);
   }

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IPointOfInterest, pPoi);
   CClosureKey closureKey;
   VERIFY(pPoi->IsInClosureJoint(poi, &closureKey));

   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetClosureJointDesignFc(closureKey,task.intervalIdx);

   Float64 fAllow = GetClosureJointAllowableTensionStress(poi,task,fc,bWithBondedReinforcement,bInPrecompressedTensileZone);
   return fAllow;
}

Float64 CSpecAgentImp::GetDeckAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,task));

   if ( IsLoadRatingServiceIIILimitState(task.limitState) )
   {
#if defined _DEBUG
      // allowable stresses during load rating only make sense if live load is applied
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      ATLASSERT(liveLoadIntervalIdx <= task.intervalIdx );
#endif
      pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(task.limitState);
      return GetAllowableTensionStress(ratingType,poi,pgsTypes::TopDeck);
   }

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetDeckDesignFc(task.intervalIdx);

   Float64 fAllow = GetDeckAllowableTensionStress(poi,task,fc,bWithBondedReinforcement);
   return fAllow;
}

std::vector<Float64> CSpecAgentImp::GetGirderAllowableCompressionStress(const PoiList& vPoi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);
   ATLASSERT(IsStressCheckApplicable(vPoi.front().get().GetSegmentKey(),task));

   GET_IFACE(IPointOfInterest,pPoi);

   std::vector<Float64> vStress;
   vStress.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         vStress.push_back( GetClosureJointAllowableCompressionStress(poi,task));
      }
      else
      {
         vStress.push_back( GetSegmentAllowableCompressionStress(poi,task));
      }
   }

   return vStress;
}

std::vector<Float64> CSpecAgentImp::GetDeckAllowableCompressionStress(const PoiList& vPoi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   ATLASSERT(IsStressCheckApplicable(vPoi.front().get().GetSegmentKey(),task));

   std::vector<Float64> vStress;
   vStress.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      vStress.push_back( GetDeckAllowableCompressionStress(poi,task));
   }

   return vStress;
}

std::vector<Float64> CSpecAgentImp::GetGirderAllowableTensionStress(const PoiList& vPoi, const StressCheckTask& task,bool bWithBondededReinforcement,bool bInPrecompressedTensileZone) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   ATLASSERT(IsStressCheckApplicable(vPoi.front().get().GetSegmentKey(),task));

   GET_IFACE(IPointOfInterest,pPoi);

   std::vector<Float64> vStress;
   vStress.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         vStress.push_back( GetClosureJointAllowableTensionStress(poi,task,bWithBondededReinforcement,bInPrecompressedTensileZone));
      }
      else
      {
         vStress.push_back( GetSegmentAllowableTensionStress(poi,task,bWithBondededReinforcement));
      }
   }

   return vStress;
}

std::vector<Float64> CSpecAgentImp::GetDeckAllowableTensionStress(const PoiList& vPoi, const StressCheckTask& task,bool bWithBondededReinforcement) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   ATLASSERT(IsStressCheckApplicable(vPoi.front().get().GetSegmentKey(),task));

   std::vector<Float64> vStress;
   vStress.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      vStress.push_back( GetDeckAllowableTensionStress(poi,task,bWithBondededReinforcement));
   }

   return vStress;
}

Float64 CSpecAgentImp::GetSegmentAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const
{
   Float64 x = GetSegmentAllowableCompressionStressCoefficient(poi,task);

   // Add a minus sign because compression is negative
   return -x*fc;
}

Float64 CSpecAgentImp::GetClosureJointAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const
{
   Float64 x = GetClosureJointAllowableCompressionStressCoefficient(poi,task);

   GET_IFACE(IPointOfInterest,pPoi);
   CClosureKey closureKey;
   VERIFY( pPoi->IsInClosureJoint(poi,&closureKey) );

   // Add a minus sign because compression is negative
   return -x*fc;
}

Float64 CSpecAgentImp::GetDeckAllowableCompressionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const
{
   Float64 x = GetDeckAllowableCompressionStressCoefficient(poi,task);

   // Add a minus sign because compression is negative
   return -x*fc;
}

Float64 CSpecAgentImp::GetSegmentAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   Float64 x;
   bool bCheckMax;
   Float64 fmax; // In system units
   GetSegmentAllowableTensionStressCoefficient(poi,task,bWithBondedReinforcement,&x,&bCheckMax,&fmax);

   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(poi.GetSegmentKey());

   Float64 f = x * lambda * sqrt( fc );
   if ( bCheckMax )
   {
      f = Min(f,fmax);
   }

   return f;
}

Float64 CSpecAgentImp::GetClosureJointAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const
{
   Float64 x;
   bool bCheckMax;
   Float64 fmax; // In system units
   GetClosureJointAllowableTensionStressCoefficient(poi,task,bWithBondedReinforcement,bInPrecompressedTensileZone,&x,&bCheckMax,&fmax);

   GET_IFACE(IPointOfInterest,pPoi);
   CClosureKey closureKey;
   VERIFY( pPoi->IsInClosureJoint(poi,&closureKey) );

   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetClosureJointLambda(closureKey);

   Float64 f = x * lambda * sqrt( fc );
   if ( bCheckMax )
   {
      f = Min(f,fmax);
   }

   return f;
}

Float64 CSpecAgentImp::GetDeckAllowableTensionStress(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement) const
{
   Float64 x;
   bool bCheckMax;
   Float64 fmax; // In system units
   GetDeckAllowableTensionStressCoefficient(poi,task,bWithBondedReinforcement,&x,&bCheckMax,&fmax);

   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetDeckLambda();

   Float64 f = x * lambda * sqrt( fc );
   if ( bCheckMax )
   {
      f = Min(f,fmax);
   }

   return f;
}

Float64 CSpecAgentImp::GetLiftingWithMildRebarAllowableStressFactor() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = pSpec->GetLiftingTensionStressFactorWithRebar();
   return x;
}

Float64 CSpecAgentImp::GetLiftingWithMildRebarAllowableStress(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fci = pMaterial->GetSegmentDesignFc(segmentKey,liftSegmentIntervalIdx);
   Float64 lambda = pMaterial->GetSegmentLambda(segmentKey);

   Float64 x = GetLiftingWithMildRebarAllowableStressFactor();

   return x*lambda*sqrt(fci);
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStressFactorNormalCrown() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = pSpec->GetHaulingTensionStressFactorWithRebarNormalCrown();
   return x;
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStressNormalCrown(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fc = pMaterial->GetSegmentDesignFc(segmentKey,haulSegmentIntervalIdx);
   Float64 lambda = pMaterial->GetSegmentLambda(segmentKey);

   Float64 x = GetHaulingWithMildRebarAllowableStressFactorNormalCrown();

   return x*lambda*sqrt(fc);
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStressFactorMaxSuper(const CSegmentKey& segmentKey) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = pSpec->GetHaulingTensionStressFactorWithRebarMaxSuper();
   return x;
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStressMaxSuper(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fc = pMaterial->GetSegmentDesignFc(segmentKey,haulSegmentIntervalIdx);
   Float64 lambda = pMaterial->GetSegmentLambda(segmentKey);

   Float64 x = GetHaulingWithMildRebarAllowableStressFactorMaxSuper(segmentKey);

   return x*lambda*sqrt(fc);
}

Float64 CSpecAgentImp::GetHaulingModulusOfRupture(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetSegmentDesignFc(segmentKey,intervalIdx);
   pgsTypes::ConcreteType type = pMaterials->GetSegmentConcreteType(segmentKey);

   return GetHaulingModulusOfRupture(segmentKey,fc,type);
}

Float64 CSpecAgentImp::GetHaulingModulusOfRupture(const CSegmentKey& segmentKey,Float64 fc,pgsTypes::ConcreteType concType) const
{
   Float64 x = GetHaulingModulusOfRuptureFactor(concType);

   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

   return x*lambda*sqrt(fc);
}

Float64 CSpecAgentImp::GetHaulingModulusOfRuptureFactor(pgsTypes::ConcreteType concType) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingModulusOfRuptureFactor(concType);
}

Float64 CSpecAgentImp::GetSegmentAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi,const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,task));

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liftIntervalIdx          = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType haulIntervalIdx          = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectSegmentIdx          = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tempStrandRemovalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   // first the special cases
   if ( task.intervalIdx == liftIntervalIdx )
   {
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      ATLASSERT(false); // this assert is to get your attention... there are two compression limits for lifting, you are only getting one of them here. Is that what you want?
      x = pSpec->GetLiftingCompressionGlobalStressFactor();
   }
   else if ( storageIntervalIdx <= task.intervalIdx && task.intervalIdx < haulIntervalIdx )
   {
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      x = pSpec->GetAtReleaseCompressionStressFactor();
   }
   else if ( task.intervalIdx == haulIntervalIdx )
   {
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      ATLASSERT(false); // this assert is to get your attention... there are two compression limits for hauling, you are only getting one of them here. Is that what you want?
      x = pSpec->GetHaulingCompressionGlobalStressFactor();
   }
   else if ( task.intervalIdx == tempStrandRemovalIdx )
   {
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      x = pSpec->GetTempStrandRemovalCompressionStressFactor();
   }
   else
   {
      // now for the normal cases
      bool bIsStressingInterval = pIntervals->IsStressingInterval(segmentKey,task.intervalIdx);

      if ( bIsStressingInterval )
      {
         // stressing interval
         ATLASSERT( task.limitState == pgsTypes::ServiceI );
         x = pSpec->GetAtReleaseCompressionStressFactor();
      }
      else
      {
         // non-stressing interval
         if ( task.intervalIdx < liveLoadIntervalIdx )
         {
            if ( task.intervalIdx < railingSystemIntervalIdx )
            {
               // before the deck is composite (this is for temporary loading conditions)
               // this is basically the wet slab on girder case
               ATLASSERT( task.limitState == pgsTypes::ServiceI );
               x = pSpec->GetErectionCompressionStressFactor();
            }
            else
            {
               ATLASSERT(task.limitState == pgsTypes::ServiceI);
               x = pSpec->GetFinalWithoutLiveLoadCompressionStressFactor();
            }
         }
         else
         {
            // live load is on the structure so this is the "at service limit states" "after all losses"
            // case
            ATLASSERT( (task.limitState == pgsTypes::ServiceI) || (task.limitState == pgsTypes::ServiceIA) || (task.limitState == pgsTypes::FatigueI));
            if (task.bIncludeLiveLoad)
            {
               x = (task.limitState == pgsTypes::ServiceI ? pSpec->GetFinalWithLiveLoadCompressionStressFactor() : pSpec->GetFatigueCompressionStressFactor());
            }
            else
            {
               ATLASSERT(task.limitState == pgsTypes::ServiceI);
               x = pSpec->GetFinalWithoutLiveLoadCompressionStressFactor();
            }
         }
      }
   }

   ATLASSERT(x != -99999);
   return x;
}

Float64 CSpecAgentImp::GetClosureJointAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   bool bIsTendonStressingInterval = pIntervals->IsGirderTendonStressingInterval(segmentKey,task.intervalIdx);

   if ( bIsTendonStressingInterval )
   {
      // stressing interval
      x = pSpec->GetAtStressingCompressingStressFactor();
   }
   else
   {
      // non-stressing interval
      if ( task.intervalIdx < liveLoadIntervalIdx )
      {
         // Effective Prestress + Permanent Loads
         x = pSpec->GetAtServiceCompressingStressFactor();
      }
      else
      {
         // Effective Prestress + Permanent Loads + Transient Loads
         if (task.bIncludeLiveLoad)
         {
            if (task.limitState == pgsTypes::ServiceIA || task.limitState == pgsTypes::FatigueI)
            {
               x = pSpec->GetClosureFatigueCompressionStressFactor();
            }
            else
            {
               x = pSpec->GetAtServiceWithLiveLoadCompressingStressFactor();
            }
         }
         else
         {
            ATLASSERT(task.limitState == pgsTypes::ServiceI);
            x = pSpec->GetFinalWithoutLiveLoadCompressionStressFactor();
         }
      }
   }

   return x;
}

Float64 CSpecAgentImp::GetDeckAllowableCompressionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,task));

   GET_IFACE(IPointOfInterest, pPoi);
   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
   ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   bool bIsSegmentTendonStressingInterval = pIntervals->IsSegmentTendonStressingInterval(segmentKey, task.intervalIdx);
   bool bIsGirderTendonStressingInterval = pIntervals->IsGirderTendonStressingInterval(segmentKey, task.intervalIdx);

   ATLASSERT(compositeDeckIntervalIdx <= task.intervalIdx); // why are you asking for allowable deck stresses before the deck can take load?

   if ( bIsSegmentTendonStressingInterval || bIsGirderTendonStressingInterval )
   {
      // stressing interval
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      x = pSpec->GetAtReleaseCompressionStressFactor();
   }
   else
   {
      // non-stressing interval
      if ( task.intervalIdx < liveLoadIntervalIdx )
      {
         // before the deck is composite (this is for temporary loading conditions)
         // this is basically the wet slab on girder case
         if ( task.intervalIdx < compositeDeckIntervalIdx )
         {
            ATLASSERT( task.limitState == pgsTypes::ServiceI );
            x = pSpec->GetErectionCompressionStressFactor();
         }
         else
         {
            // the deck is now composite so this is the Effective Prestress + Permanent Loads case
            // (basically the case when the railing system has been installed, but no live load)
            ATLASSERT( task.limitState == pgsTypes::ServiceI );
            x = pSpec->GetFinalWithoutLiveLoadCompressionStressFactor();
         }
      }
      else
      {
         // live load is on the structure so this is the "at service limit states" "after all losses"
         // case
         ATLASSERT( (task.limitState == pgsTypes::ServiceI) || (task.limitState == pgsTypes::ServiceIA) || (task.limitState == pgsTypes::FatigueI));
         x = (task.limitState == pgsTypes::ServiceI ? pSpec->GetFinalWithLiveLoadCompressionStressFactor() : pSpec->GetFatigueCompressionStressFactor());
      }
   }

   return x;
}

void CSpecAgentImp::GetSegmentAllowableTensionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;
   bool bCheckMax = false;
   Float64 fmax = -99999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,task));


   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liftingIntervalIdx       = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType haulingIntervalIdx       = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectSegmentIdx          = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tempStrandRemovalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   bool bIsStressingInterval = pIntervals->IsStressingInterval(segmentKey,task.intervalIdx);

   // first deal with the special cases
   if ( task.intervalIdx == liftingIntervalIdx )
   {
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      if ( bWithBondedReinforcement )
      {
         x = pSpec->GetLiftingTensionStressFactorWithRebar();
      }
      else
      {
         x = pSpec->GetLiftingTensionStressFactor();
         pSpec->GetLiftingMaximumTensionStress(&bCheckMax,&fmax);
      }
   }
   else if ( storageIntervalIdx <= task.intervalIdx && task.intervalIdx < haulingIntervalIdx )
   {
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      if ( bWithBondedReinforcement )
      {
         x = pSpec->GetAtReleaseTensionStressFactorWithRebar();
      }
      else
      {
         x = pSpec->GetAtReleaseTensionStressFactor();
         pSpec->GetAtReleaseMaximumTensionStress(&bCheckMax,&fmax);
      }
   }
   else if ( task.intervalIdx == haulingIntervalIdx )
   {
      ATLASSERT(false); // can't use this method for hauling because we don't know
      // if the caller wants the allowable tension factor for normal crown or max super

      //ATLASSERT( ls == pgsTypes::ServiceI );
      //if ( bWithBondedReinforcement )
      //{
      //   x = pSpec->GetHaulingTensionStressFactorWithRebarNormalCrown();
      //}
      //else
      //{
      //   x = pSpec->GetHaulingTensionStressFactorNormalCrown();
      //   pSpec->GetHaulingMaximumTensionStressNormalCrown(&bCheckMax,&fmax);
      //}
   } 
   else if ( task.intervalIdx == tempStrandRemovalIdx )
   {
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      ATLASSERT( CheckTemporaryStresses() ); // if this fires, why are you asking for this if they aren't being used?
      if ( bWithBondedReinforcement )
      {
         x = pSpec->GetTempStrandRemovalTensionStressFactorWithRebar();
      }
      else
      {
         x = pSpec->GetTempStrandRemovalTensionStressFactor();
         pSpec->GetTempStrandRemovalMaximumTensionStress(&bCheckMax,&fmax);
      }
   }
   else
   {
      // now for the "normal" cases...
      if ( bIsStressingInterval )
      {
         // if this is a stressing interval, use allowables from Table 5.9.2.3.2a-1 (pre2017: 5.9.4.2.1-1)
         ATLASSERT( task.limitState == pgsTypes::ServiceI );
         if ( bWithBondedReinforcement )
         {
            x = pSpec->GetAtReleaseTensionStressFactorWithRebar();
         }
         else
         {
            x = pSpec->GetAtReleaseTensionStressFactor();
            pSpec->GetAtReleaseMaximumTensionStress(&bCheckMax,&fmax);
         }
      }
      else
      {
         // if this is a non-stressing interval, use allowables from Table 5.9.2.3.2b-1 (pre2017: 5.9.4.2.2-1)
         if ( task.intervalIdx < railingSystemIntervalIdx )
         {

            ATLASSERT( task.limitState == pgsTypes::ServiceI );
            x = pSpec->GetErectionTensionStressFactor();
            pSpec->GetErectionMaximumTensionStress(&bCheckMax,&fmax);
         }
         else 
         {
            if ( task.limitState == pgsTypes::ServiceI && CheckFinalDeadLoadTensionStress() )
            {
               x = pSpec->GetFinalTensionPermanentLoadsStressFactor();
               pSpec->GetFinalTensionPermanentLoadStressFactor(&bCheckMax,&fmax);
            }
            else
            {
#if defined _DEBUG
               GET_IFACE(IDocumentType, pDocType);
               ATLASSERT((pDocType->IsPGSpliceDocument() && task.limitState == pgsTypes::ServiceI) || task.limitState == pgsTypes::ServiceIII);
#endif
               GET_IFACE(IEnvironment,pEnv);
               int exposureCondition = pEnv->GetExposureCondition() == expNormal ? EXPOSURE_NORMAL : EXPOSURE_SEVERE;
               x = pSpec->GetFinalTensionStressFactor(exposureCondition);
               pSpec->GetFinalTensionStressFactor(exposureCondition,&bCheckMax,&fmax);
            }
         }
      } // end if bIsStressingInterval
   }// end if,else-if 


   *pCoeff    = x;
   *pbMax     = bCheckMax;
   *pMaxValue = fmax;
}

void CSpecAgentImp::GetClosureJointAllowableTensionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);
   
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;
   bool bCheckMax = false;
   Float64 fmax = -99999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,task));

   GET_IFACE(IIntervals,pIntervals);
   bool bIsTendonStressingInterval = pIntervals->IsGirderTendonStressingInterval(segmentKey,task.intervalIdx);

   // closure joints have allowables for both "in the precompressed tensile zone" and "in areas other than
   // the precompressed tensile zone" (See Table 5.9.2.3.1b-1 and 5.9.2.3.2b-1 (pre2017: 5.9.4.1.2-1 and -5.9.4.2.2-1)) for both during stressing
   // and non-stressing intervals

   if ( bIsTendonStressingInterval )
   {
      // stressing interval, use Table 5.9.2.3.1b-1
      if ( bInPrecompressedTensileZone )
      {
         if ( bWithBondedReinforcement )
         {
            x = pSpec->GetAtStressingPrecompressedTensileZoneTensionStressFactorWithRebar();
         }
         else
         {
            x = pSpec->GetAtStressingPrecompressedTensileZoneTensionStressFactor();
         }
      }
      else
      {
         if ( bWithBondedReinforcement )
         {
            x = pSpec->GetAtStressingOtherLocationTensionStressFactorWithRebar();
         }
         else
         {
            x = pSpec->GetAtStressingOtherLocationTensionStressFactor();
         }
      }
   }
   else
   {
      // non-stressing interval, use Table 5.9.2.3.2b-1 (pre2017: 5.9.4.2.2-1)
      if ( bInPrecompressedTensileZone )
      {
         if ( bWithBondedReinforcement )
         {
            x = pSpec->GetAtServicePrecompressedTensileZoneTensionStressFactorWithRebar();
         }
         else
         {
            x = pSpec->GetAtServicePrecompressedTensileZoneTensionStressFactor();
         }
      }
      else
      {
         if ( bWithBondedReinforcement )
         {
            x = pSpec->GetAtServiceOtherLocationTensionStressFactorWithRebar();
         }
         else
         {
            x = pSpec->GetAtServiceOtherLocationTensionStressFactor();
         }
      }
   }

   *pCoeff    = x;
   *pbMax     = bCheckMax;
   *pMaxValue = fmax;
}

void CSpecAgentImp::GetDeckAllowableTensionStressCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement,Float64* pCoeff,bool* pbMax,Float64* pMaxValue) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 x = -999999;
   bool bCheckMax = false;
   Float64 fmax = -99999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsStressCheckApplicable(segmentKey,task));

   GET_IFACE(IIntervals,pIntervals);

#if defined _DEBUG
   GET_IFACE(IPointOfInterest, pPoi);
   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
   ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);
   ATLASSERT(pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx) <= task.intervalIdx);
#endif

   bool bIsTendonStressingInterval = pIntervals->IsGirderTendonStressingInterval(segmentKey,task.intervalIdx);

   if ( bIsTendonStressingInterval )
   {
      // if this is a stressing interval, use allowables from Table 5.9.2.3.2a (pre2017: 5.9.4.2.1-1)
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      if ( bWithBondedReinforcement )
      {
         x = pSpec->GetAtReleaseTensionStressFactorWithRebar();
      }
      else
      {
         x = pSpec->GetAtReleaseTensionStressFactor();
         pSpec->GetAtReleaseMaximumTensionStress(&bCheckMax,&fmax);
      }
   }
   else
   {
      // if this is a non-stressing interval, use allowables from Table 5.9.2.3.2b-1 (pre2017: 5.9.4.2.2-1)
      GET_IFACE(IEnvironment,pEnv);
      int exposureCondition = pEnv->GetExposureCondition() == expNormal ? EXPOSURE_NORMAL : EXPOSURE_SEVERE;
      x = pSpec->GetFinalTensionStressFactor(exposureCondition);
      pSpec->GetFinalTensionStressFactor(exposureCondition,&bCheckMax,&fmax);
   }

   *pCoeff    = x;
   *pbMax     = bCheckMax;
   *pMaxValue = fmax;
}

bool CSpecAgentImp::IsStressCheckApplicable(const CGirderKey& girderKey, const StressCheckTask& task) const
{
   ATLASSERT(::IsServiceLimitState(task.limitState) || ::IsFatigueLimitState(task.limitState) ); // must be a service limit state
   if ( (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims && task.limitState == pgsTypes::FatigueI) || 
        (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion()&& task.limitState == pgsTypes::ServiceIA)
        )
   {
      // if before LRFD 2009 and Fatigue I 
      // - OR -
      // LRFD 2009 and later and Service IA
      //
      // ... don't evaluate this case
      return false;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   IntervalIndexType noncompositeIntervalIdx  = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();

   if ( task.stressType == pgsTypes::Tension )
   {
      switch(task.limitState)
      {
      case pgsTypes::ServiceI:
         if ( (erectSegmentIntervalIdx <= task.intervalIdx && task.intervalIdx <= noncompositeIntervalIdx && !CheckTemporaryStresses())
              ||
              (liveLoadIntervalIdx <= task.intervalIdx && !CheckFinalDeadLoadTensionStress())
            )
         {
            return false;
         }
         else
         {
            return true;
         }

      case pgsTypes::ServiceI_PermitRoutine:
      case pgsTypes::ServiceI_PermitSpecial:
         return (liveLoadIntervalIdx <= task.intervalIdx ? true : false);

      case pgsTypes::ServiceIA:
      case pgsTypes::FatigueI:
         return false; // these are compression only limit states

      case pgsTypes::ServiceIII:
      case pgsTypes::ServiceIII_Inventory:
      case pgsTypes::ServiceIII_Operating:
      case pgsTypes::ServiceIII_LegalRoutine:
      case pgsTypes::ServiceIII_LegalSpecial:
      case pgsTypes::ServiceIII_LegalEmergency:
         return (liveLoadIntervalIdx <= task.intervalIdx ? true : false);

      default:
         ATLASSERT(false); // either a new service limit state or a non-service limit state was passed in
      }
   }
   else
   {
      ATLASSERT(task.stressType == pgsTypes::Compression);

      switch(task.limitState)
      {
      case pgsTypes::ServiceI:
         if ( erectSegmentIntervalIdx <= task.intervalIdx && task.intervalIdx <= noncompositeIntervalIdx && !CheckTemporaryStresses() )
         {
            return false;
         }
         else
         {
            return true;
         }

      case pgsTypes::ServiceI_PermitRoutine:
      case pgsTypes::ServiceI_PermitSpecial:
         return (liveLoadIntervalIdx <= task.intervalIdx ? true : false);

      case pgsTypes::ServiceIA:
      case pgsTypes::FatigueI:
         if ( liveLoadIntervalIdx <= task.intervalIdx )
         {
            return true; // these are compression only limit states
         }
         else
         {
            return false; // only check if there is live load
         }

      case pgsTypes::ServiceIII:
      case pgsTypes::ServiceIII_Inventory:
      case pgsTypes::ServiceIII_Operating:
      case pgsTypes::ServiceIII_LegalRoutine:
      case pgsTypes::ServiceIII_LegalSpecial:
      case pgsTypes::ServiceIII_LegalEmergency:
         return false;

      default:
         ATLASSERT(false); // either a new service limit state or a non-service limit state was passed in
      }
   }

   ATLASSERT(false); // I think the code above should have covered all possible cases... why did we get here?

   // not other stress check are applicable
   return false;
}

bool CSpecAgentImp::HasAllowableTensionWithRebarOption(IntervalIndexType intervalIdx,bool bInPTZ,bool bSegment,const CSegmentKey& segmentKey) const
{
   if ( !bSegment )
   {
      // At closure joints, there is always a "with rebar" option in both the PTZ and other areas
      return true;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType noncompositeIntervalIdx = pIntervals->GetLastNoncompositeInterval();
   if ( intervalIdx < erectionIntervalIdx ||
        intervalIdx == tsRemovalIntervalIdx ||
        intervalIdx == noncompositeIntervalIdx)
   {
      // LRFD Table 5.9.2.3.1b-1 (pre2017: 5.9.4.1.2-1), third bullet... there is always a "with rebar" option
      // in both the PTZ and other areas.

      // by looking at intervals before segment erection, we cover release, lifting, storage, and hauling
      // all of which have a "with rebar" option.

      // Temporary conditions for deck casting and temporary strand removal are also "with rebar" cases
      return true;
   }

   // there is no "with rebar" in the other intervals
   return false;
}

bool CSpecAgentImp::CheckTemporaryStresses() const
{
   // I hate using the IDocumentType interface, but I don't
   // think there is a better way to figure out if we have a PGSuper or PGSplice file
   // The temporary stress checks are always required for spliced girders
   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSpliceDocument() )
   {
      // always checking for spliced girders (See LRFD 5.12.3.4.3 (pre2017: 5.14.1.3.3))
      return true;
   }
   else
   {
      const SpecLibraryEntry* pSpec = GetSpec();
      return pSpec->CheckTemporaryStresses();
   }
}

bool CSpecAgentImp::CheckFinalDeadLoadTensionStress() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->CheckFinalTensionPermanentLoadStresses();
}

/////////////////////////////////////////////////////////////////////////////
// ITransverseReinforcementSpec
//
Float64 CSpecAgentImp::GetMaxSplittingStress(Float64 fyRebar) const
{
   return lrfdRebar::GetMaxBurstingStress(fyRebar);
}

Float64 CSpecAgentImp::GetSplittingZoneLengthFactor() const
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   Float64 bzlf = pSpecEntry->GetSplittingZoneLengthFactor();
   return bzlf;
}

Float64 CSpecAgentImp::GetSplittingZoneLength( Float64 girderHeight ) const
{
   return girderHeight / GetSplittingZoneLengthFactor();
}

matRebar::Size CSpecAgentImp::GetMinConfinmentBarSize() const
{
   return lrfdRebar::GetMinConfinmentBarSize();
}

Float64 CSpecAgentImp::GetMaxConfinmentBarSpacing() const
{
   return lrfdRebar::GetMaxConfinmentBarSpacing();
}

Float64 CSpecAgentImp::GetMinConfinmentAvS() const
{
   return lrfdRebar::GetMinConfinmentAvS();
}

void CSpecAgentImp::GetMaxStirrupSpacing(Float64 dv,Float64* pSmax1, Float64* pSmax2) const
{
   Float64 k1,k2,s1,s2;
   const SpecLibraryEntry* pSpec = GetSpec();
   pSpec->GetMaxStirrupSpacing(&k1,&s1,&k2,&s2);
   *pSmax1 = min(k1*dv,s1); // LRFD equation 5.7.2.6-1 (pre2017: 5.8.2.7-1)
   *pSmax2 = min(k2*dv,s2); // LRFD equation 5.7.2.6-2 (pre2017: 5.8.2.7-2) 
}

Float64 CSpecAgentImp::GetMinStirrupSpacing(Float64 maxAggregateSize, Float64 barDiameter) const
{
   ATLASSERT(maxAggregateSize>0.0);
   ATLASSERT(barDiameter>0.0);

   Float64 min_spc = Max(1.33*maxAggregateSize, barDiameter);

   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 abs_min_spc;
   if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
   {
      abs_min_spc = ::ConvertToSysUnits(25., unitMeasure::Millimeter);
   }
   else
   {
      abs_min_spc = ::ConvertToSysUnits(1., unitMeasure::Inch);
   }

   // lrfd requirements are for clear distance, we want cl-to-cl spacing
   min_spc += barDiameter;
   abs_min_spc += barDiameter;

   return Max(min_spc, abs_min_spc);
}


Float64 CSpecAgentImp::GetMinTopFlangeThickness() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 dim;
   if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
   {
      dim = ::ConvertToSysUnits(50., unitMeasure::Millimeter);
   }
   else
   {
      dim = ::ConvertToSysUnits(2., unitMeasure::Inch);
   }

   return dim;
}

Float64 CSpecAgentImp::GetMinWebThickness() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   bool bPostTension = false;
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
   GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups && !bPostTension; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CGirderKey girderKey(grpIdx, gdrIdx);
         DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);
         DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);
         if ( 0 < nMaxSegmentDucts+nGirderDucts )
         {
            bPostTension = true;
            break;
         }
      }
   }

   Float64 dim;
   if ( bPostTension )
   {
      if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
      {
         dim = ::ConvertToSysUnits(165., unitMeasure::Millimeter);
      }
      else
      {
         dim = ::ConvertToSysUnits(6.5, unitMeasure::Inch);
      }
   }
   else
   {
      if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
      {
         dim = ::ConvertToSysUnits(125., unitMeasure::Millimeter);
      }
      else
      {
         dim = ::ConvertToSysUnits(5., unitMeasure::Inch);
      }
   }

   return dim;
}

Float64 CSpecAgentImp::GetMinBottomFlangeThickness() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 dim;
   if (pSpec->GetSpecificationUnits()==lrfdVersionMgr::SI)
   {
      dim = ::ConvertToSysUnits(125., unitMeasure::Millimeter);
   }
   else
   {
      dim = ::ConvertToSysUnits(5., unitMeasure::Inch);
   }

   return dim;
}

/////////////////////////////////////////////////////////////////////
//  ISegmentLiftingSpecCriteria
bool CSpecAgentImp::IsLiftingAnalysisEnabled() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->IsLiftingAnalysisEnabled();
}

void  CSpecAgentImp::GetLiftingImpact(Float64* pDownward, Float64* pUpward) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *pDownward = pSpec->GetLiftingDownwardImpactFactor();
   *pUpward   = pSpec->GetLiftingUpwardImpactFactor();
}

Float64 CSpecAgentImp::GetLiftingCrackingFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetCrackingFOSLifting();
}

Float64 CSpecAgentImp::GetLiftingFailureFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingFailureFOS();
}

void CSpecAgentImp::GetLiftingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *factor = GetLiftingAllowableTensionFactor();
   pSpec->GetLiftingMaximumTensionStress(pbMax,fmax);
}

Float64 CSpecAgentImp::GetLiftingAllowableTensileConcreteStress(const CSegmentKey& segmentKey) const
{
   Float64 factor = GetLiftingAllowableTensionFactor();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fci = pMaterial->GetSegmentDesignFc(segmentKey,liftSegmentIntervalIdx);
   Float64 lambda = pMaterial->GetSegmentLambda(segmentKey);

   Float64 f = factor * lambda * sqrt( fci );

   bool is_max;
   Float64 maxval;
   const SpecLibraryEntry* pSpec = GetSpec();

   pSpec->GetLiftingMaximumTensionStress(&is_max,&maxval);
   if (is_max)
   {
      f = Min(f, maxval);
   }

   return f;
}

Float64 CSpecAgentImp::GetLiftingAllowableTensionFactor() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 factor = pSpec->GetLiftingTensionStressFactor();
   return factor;
}

Float64 CSpecAgentImp::GetLiftingAllowableGlobalCompressiveConcreteStress(const CSegmentKey& segmentKey) const
{
   Float64 factor = GetLiftingAllowableGlobalCompressionFactor();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fci = pMaterial->GetSegmentDesignFc(segmentKey,liftSegmentIntervalIdx);

   Float64 allowable = factor * fci;

   return allowable;
}

Float64 CSpecAgentImp::GetLiftingAllowablePeakCompressiveConcreteStress(const CSegmentKey& segmentKey) const
{
   Float64 factor = GetLiftingAllowablePeakCompressionFactor();

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials, pMaterial);
   Float64 fci = pMaterial->GetSegmentDesignFc(segmentKey, liftSegmentIntervalIdx);

   Float64 allowable = factor * fci;

   return allowable;
}

Float64 CSpecAgentImp::GetLiftingAllowableGlobalCompressionFactor() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 factor = pSpec->GetLiftingCompressionGlobalStressFactor();
   return -factor;
}

Float64 CSpecAgentImp::GetLiftingAllowablePeakCompressionFactor() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   Float64 factor = pSpec->GetLiftingCompressionPeakStressFactor();
   return -factor;
}

Float64 CSpecAgentImp::GetLiftingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fci, bool withMinRebar) const
{
   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

   if (withMinRebar)
   {
      Float64 x = GetLiftingWithMildRebarAllowableStressFactor();

      Float64 f = x * lambda * sqrt( fci );
      return f;
   }
   else
   {
      Float64 x; 
      bool bCheckMax;
      Float64 fmax;

      GetLiftingAllowableTensileConcreteStressParameters(&x,&bCheckMax,&fmax);

      Float64 f = x * lambda * sqrt( fci );

      if ( bCheckMax )
      {
         f = Min(f,fmax);
      }

      return f;
   }
}

Float64 CSpecAgentImp::GetLiftingAllowableGlobalCompressiveConcreteStressEx(const CSegmentKey &segmentKey, Float64 fci) const
{
   Float64 x = GetLiftingAllowableGlobalCompressionFactor();

   return x*fci;
}

Float64 CSpecAgentImp::GetLiftingAllowablePeakCompressiveConcreteStressEx(const CSegmentKey &segmentKey, Float64 fci) const
{
   Float64 x = GetLiftingAllowablePeakCompressionFactor();

   return x*fci;
}

Float64 CSpecAgentImp::GetHeightOfPickPointAboveGirderTop() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   return pSpec->GetPickPointHeight();
}

Float64 CSpecAgentImp::GetLiftingLoopPlacementTolerance() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   return pSpec->GetLiftingLoopTolerance();
}

Float64 CSpecAgentImp::GetLiftingCableMinInclination() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   return pSpec->GetMinCableInclination();
}

Float64 CSpecAgentImp::GetLiftingSweepTolerance() const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   return pSpec->GetLiftingMaximumGirderSweepTolerance();
}

Float64 CSpecAgentImp::GetLiftingModulusOfRupture(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterials);
   Float64 fci = pMaterials->GetSegmentDesignFc(segmentKey,intervalIdx);
   pgsTypes::ConcreteType type = pMaterials->GetSegmentConcreteType(segmentKey);

   return GetLiftingModulusOfRupture(segmentKey,fci,type);
}

Float64 CSpecAgentImp::GetLiftingModulusOfRupture(const CSegmentKey& segmentKey,Float64 fci,pgsTypes::ConcreteType concType) const
{
   Float64 x = GetLiftingModulusOfRuptureFactor(concType);

   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

   return x*lambda*sqrt(fci);
}

Float64 CSpecAgentImp::GetLiftingModulusOfRuptureFactor(pgsTypes::ConcreteType concType) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingModulusOfRuptureFactor(concType);
}

Float64 CSpecAgentImp::GetMinimumLiftingPointLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 min_lift_point = pSpec->GetMininumLiftingPointLocation();

   // if less than zero, then use H from the end of the girder
   if ( min_lift_point < 0 )
   {
      GET_IFACE(IPointOfInterest,pIPoi);

      PoiAttributeType searchAttribute = POI_RELEASED_SEGMENT;
      if ( end == pgsTypes::metStart )
      {
         searchAttribute |= POI_0L;
      }
      else
      {
         searchAttribute |= POI_10L;
      }

      PoiList vPoi;
      pIPoi->GetPointsOfInterest(segmentKey, searchAttribute, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      const pgsPointOfInterest& poi(vPoi.front());

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      GET_IFACE(ISectionProperties,pSectProp);
      min_lift_point = pSectProp->GetHg( releaseIntervalIdx, poi );
   }

   return min_lift_point;
}

Float64 CSpecAgentImp::GetLiftingPointLocationAccuracy() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingPointLocationAccuracy();
}

stbLiftingCriteria CSpecAgentImp::GetLiftingStabilityCriteria(const CSegmentKey& segmentKey) const
{
   stbLiftingCriteria criteria;

   GET_IFACE(IMaterials,pMaterial);
   criteria.Lambda = pMaterial->GetSegmentLambda(segmentKey);
   criteria.CompressionCoefficient_GlobalStress = -GetLiftingAllowableGlobalCompressionFactor();
   criteria.CompressionCoefficient_PeakStress = -GetLiftingAllowablePeakCompressionFactor();
   GetLiftingAllowableTensileConcreteStressParameters(&criteria.TensionCoefficient,&criteria.bMaxTension,&criteria.MaxTension);
   criteria.TensionCoefficientWithRebar = GetLiftingWithMildRebarAllowableStressFactor();
   criteria.AllowableCompression_GlobalStress = GetLiftingAllowableGlobalCompressiveConcreteStress(segmentKey);
   criteria.AllowableCompression_PeakStress = GetLiftingAllowablePeakCompressiveConcreteStress(segmentKey);
   criteria.AllowableTension = GetLiftingAllowableTensileConcreteStress(segmentKey);
   criteria.AllowableTensionWithRebar = GetLiftingWithMildRebarAllowableStress(segmentKey);
   criteria.MinFScr = GetLiftingCrackingFs();
   criteria.MinFSf = GetLiftingFailureFs();

   return criteria;
}

stbLiftingCriteria CSpecAgentImp::GetLiftingStabilityCriteria(const CSegmentKey& segmentKey,const HANDLINGCONFIG& liftConfig) const
{
   stbLiftingCriteria criteria;

   GET_IFACE(IMaterials,pMaterial);
   criteria.Lambda = pMaterial->GetSegmentLambda(segmentKey);
   criteria.CompressionCoefficient_GlobalStress = -GetLiftingAllowableGlobalCompressionFactor();
   criteria.CompressionCoefficient_PeakStress = -GetLiftingAllowablePeakCompressionFactor();
   GetLiftingAllowableTensileConcreteStressParameters(&criteria.TensionCoefficient,&criteria.bMaxTension,&criteria.MaxTension);
   criteria.TensionCoefficientWithRebar = GetLiftingWithMildRebarAllowableStressFactor();
   criteria.AllowableCompression_GlobalStress = GetLiftingAllowableGlobalCompressiveConcreteStressEx(segmentKey, liftConfig.GdrConfig.fci);
   criteria.AllowableCompression_PeakStress = GetLiftingAllowablePeakCompressiveConcreteStressEx(segmentKey, liftConfig.GdrConfig.fci);
   criteria.AllowableTension = GetLiftingAllowableTensileConcreteStressEx(segmentKey,liftConfig.GdrConfig.fci,false);
   criteria.AllowableTensionWithRebar = GetLiftingAllowableTensileConcreteStressEx(segmentKey,liftConfig.GdrConfig.fci,true);
   criteria.MinFScr = GetLiftingCrackingFs();
   criteria.MinFSf = GetLiftingFailureFs();

   return criteria;
}

Float64 CSpecAgentImp::GetLiftingCamberMultiplier() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingCamberMultiplier();
}

pgsTypes::WindType CSpecAgentImp::GetLiftingWindType() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingWindType();
}

Float64 CSpecAgentImp::GetLiftingWindLoad() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetLiftingWindLoad();
}

//////////////////////////////////////////////////////////////////////
// ISegmentHaulingSpecCriteria
bool CSpecAgentImp::IsHaulingAnalysisEnabled() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->IsHaulingAnalysisEnabled();
}

pgsTypes::HaulingAnalysisMethod CSpecAgentImp::GetHaulingAnalysisMethod() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingAnalysisMethod();
}

void CSpecAgentImp::GetHaulingImpact(Float64* pDownward, Float64* pUpward) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *pDownward = pSpec->GetHaulingDownwardImpactFactor();
   *pUpward   = pSpec->GetHaulingUpwardImpactFactor();
}

Float64 CSpecAgentImp::GetHaulingCrackingFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingCrackingFOS();
}

Float64 CSpecAgentImp::GetHaulingRolloverFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingFailureFOS();
}

void CSpecAgentImp::GetHaulingAllowableTensileConcreteStressParametersNormalCrown(Float64* factor,bool* pbMax,Float64* fmax) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *factor = GetHaulingAllowableTensionFactorNormalCrown();
   pSpec->GetHaulingMaximumTensionStressNormalCrown(pbMax,fmax);
}

Float64 CSpecAgentImp::GetHaulingAllowableTensileConcreteStressNormalCrown(const CSegmentKey& segmentKey) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = GetHaulingAllowableTensionFactorNormalCrown();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fc = pMaterial->GetSegmentDesignFc(segmentKey,haulSegmentIntervalIdx);
   Float64 lambda = pMaterial->GetSegmentLambda(segmentKey);

   Float64 allow = factor * lambda * sqrt( fc );

   bool is_max;
   Float64 maxval;
   pSpec->GetHaulingMaximumTensionStressNormalCrown(&is_max,&maxval);
   if (is_max)
   {
      allow = Min(allow, maxval);
   }

   return allow;
}

void CSpecAgentImp::GetHaulingAllowableTensileConcreteStressParametersMaxSuper(Float64* factor,bool* pbMax,Float64* fmax) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *factor = GetHaulingAllowableTensionFactorMaxSuper();
   pSpec->GetHaulingMaximumTensionStressMaxSuper(pbMax,fmax);
}

Float64 CSpecAgentImp::GetHaulingAllowableTensileConcreteStressMaxSuper(const CSegmentKey& segmentKey) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = GetHaulingAllowableTensionFactorMaxSuper();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fc = pMaterial->GetSegmentDesignFc(segmentKey,haulSegmentIntervalIdx);
   Float64 lambda = pMaterial->GetSegmentLambda(segmentKey);

   Float64 allow = factor * lambda * sqrt( fc );

   bool is_max;
   Float64 maxval;
   pSpec->GetHaulingMaximumTensionStressMaxSuper(&is_max,&maxval);
   if (is_max)
   {
      allow = Min(allow, maxval);
   }

   return allow;
}

Float64 CSpecAgentImp::GetHaulingAllowableGlobalCompressiveConcreteStress(const CSegmentKey& segmentKey) const
{
   Float64 factor = GetHaulingAllowableGlobalCompressionFactor();

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials, pMaterial);
   Float64 fc = pMaterial->GetSegmentDesignFc(segmentKey, haulSegmentIntervalIdx);

   Float64 allowable = factor * fc;
   return allowable;
}

Float64 CSpecAgentImp::GetHaulingAllowablePeakCompressiveConcreteStress(const CSegmentKey& segmentKey) const
{
   Float64 factor = GetHaulingAllowablePeakCompressionFactor();

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials, pMaterial);
   Float64 fc = pMaterial->GetSegmentDesignFc(segmentKey, haulSegmentIntervalIdx);

   Float64 allowable = factor * fc;
   return allowable;
}

Float64 CSpecAgentImp::GetHaulingAllowableTensionFactorNormalCrown() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = pSpec->GetHaulingTensionStressFactorNormalCrown();
   return factor;
}

Float64 CSpecAgentImp::GetHaulingAllowableTensionFactorMaxSuper() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = pSpec->GetHaulingTensionStressFactorMaxSuper();
   return factor;
}

Float64 CSpecAgentImp::GetHaulingAllowableGlobalCompressionFactor() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = pSpec->GetHaulingCompressionGlobalStressFactor();
   return -factor;
}

Float64 CSpecAgentImp::GetHaulingAllowablePeakCompressionFactor() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 factor = pSpec->GetHaulingCompressionPeakStressFactor();
   return -factor;
}

Float64 CSpecAgentImp::GetHaulingAllowableTensileConcreteStressExNormalCrown(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar) const
{
   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

   if (includeRebar)
   {
      Float64 x = GetHaulingWithMildRebarAllowableStressFactorNormalCrown();

      Float64 f = x * lambda * sqrt( fc );
      return f;
   }
   else
   {
      Float64 x; 
      bool bCheckMax;
      Float64 fmax;

      GetHaulingAllowableTensileConcreteStressParametersNormalCrown(&x,&bCheckMax,&fmax);

      Float64 f = x * lambda * sqrt( fc );

      if ( bCheckMax )
      {
         f = Min(f,fmax);
      }

      return f;
   }
}

Float64 CSpecAgentImp::GetHaulingAllowableTensileConcreteStressExMaxSuper(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar) const
{
   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

   if (includeRebar)
   {
      Float64 x = GetHaulingWithMildRebarAllowableStressFactorMaxSuper(segmentKey);

      Float64 f = x * lambda * sqrt( fc );
      return f;
   }
   else
   {
      Float64 x; 
      bool bCheckMax;
      Float64 fmax;

      GetHaulingAllowableTensileConcreteStressParametersMaxSuper(&x,&bCheckMax,&fmax);

      Float64 f = x * lambda * sqrt( fc );

      if ( bCheckMax )
      {
         f = Min(f,fmax);
      }

      return f;
   }
}

Float64 CSpecAgentImp::GetHaulingAllowableGlobalCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fc) const
{
   Float64 x = GetHaulingAllowableGlobalCompressionFactor();

   return x*fc;
}

Float64 CSpecAgentImp::GetHaulingAllowablePeakCompressiveConcreteStressEx(const CSegmentKey& segmentKey, Float64 fc) const
{
   Float64 x = GetHaulingAllowablePeakCompressionFactor();

   return x*fc;
}

pgsTypes::HaulingImpact CSpecAgentImp::GetHaulingImpactUsage() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingImpactUsage();
}

Float64 CSpecAgentImp::GetNormalCrownSlope() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetRoadwayCrownSlope();
}

Float64 CSpecAgentImp::GetMaxSuperelevation() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetRoadwaySuperelevation();
}

Float64 CSpecAgentImp::GetHaulingSweepTolerance() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingMaximumGirderSweepTolerance();
}

Float64 CSpecAgentImp::GetHaulingSweepGrowth() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingSweepGrowth();
}

Float64 CSpecAgentImp::GetHaulingSupportPlacementTolerance() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingSupportPlacementTolerance();
}

Float64 CSpecAgentImp::GetHaulingCamberMultiplier() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingCamberMultiplier();
}

Float64 CSpecAgentImp::GetRollStiffness(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetRollStiffness();
}

Float64 CSpecAgentImp::GetHeightOfGirderBottomAboveRoadway(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetBottomOfGirderHeight();
}

Float64 CSpecAgentImp::GetHeightOfTruckRollCenterAboveRoadway(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetRollCenterHeight();
}

Float64 CSpecAgentImp::GetAxleWidth(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetAxleWidth();
}

Float64 CSpecAgentImp::GetAllowableDistanceBetweenSupports(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetMaxDistanceBetweenBunkPoints();
}

Float64 CSpecAgentImp::GetAllowableLeadingOverhang(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetMaximumLeadingOverhang();
}

Float64 CSpecAgentImp::GetMaxGirderWgt(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   return pSegment->HandlingData.pHaulTruckLibraryEntry->GetMaxGirderWeight();
}

Float64 CSpecAgentImp::GetMinimumHaulingSupportLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 min_pick_point = pSpec->GetMininumTruckSupportLocation();

   // if less than zero, then use H from the end of the girder
   if ( min_pick_point < 0 )
   {
      GET_IFACE(IPointOfInterest,pIPoi);

      PoiAttributeType searchAttribute = POI_RELEASED_SEGMENT;
      if ( end == pgsTypes::metStart )
      {
         searchAttribute |= POI_0L;
      }
      else
      {
         searchAttribute |= POI_10L;
      }

      PoiList vPoi;
      pIPoi->GetPointsOfInterest(segmentKey, searchAttribute, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      const pgsPointOfInterest& poi(vPoi.front());

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      GET_IFACE(ISectionProperties,pSectProp);
      min_pick_point = pSectProp->GetHg( releaseIntervalIdx, poi );
   }

   return min_pick_point;
}

Float64 CSpecAgentImp::GetHaulingSupportLocationAccuracy() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetTruckSupportLocationAccuracy();
}

pgsTypes::WindType CSpecAgentImp::GetHaulingWindType() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingWindType();
}

Float64 CSpecAgentImp::GetHaulingWindLoad() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingWindLoad();
}

pgsTypes::CFType CSpecAgentImp::GetCentrifugalForceType() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetCentrifugalForceType();
}

Float64 CSpecAgentImp::GetHaulingSpeed() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetHaulingSpeed();
}

Float64 CSpecAgentImp::GetTurningRadius() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetTurningRadius();
}

stbHaulingCriteria CSpecAgentImp::GetHaulingStabilityCriteria(const CSegmentKey& segmentKey) const
{
   stbHaulingCriteria criteria;

   GET_IFACE(IMaterials,pMaterial);
   pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
   criteria.Lambda = pMaterial->GetSegmentLambda(segmentKey);

   criteria.CompressionCoefficient_GlobalStress = -GetHaulingAllowableGlobalCompressionFactor();
   criteria.CompressionCoefficient_PeakStress = -GetHaulingAllowablePeakCompressionFactor();
   criteria.AllowableCompression_GlobalStress = GetHaulingAllowableGlobalCompressiveConcreteStress(segmentKey);
   criteria.AllowableCompression_PeakStress = GetHaulingAllowablePeakCompressiveConcreteStress(segmentKey);

   GetHaulingAllowableTensileConcreteStressParametersNormalCrown(&criteria.TensionCoefficient[stbTypes::CrownSlope],&criteria.bMaxTension[stbTypes::CrownSlope],&criteria.MaxTension[stbTypes::CrownSlope]);
   criteria.TensionCoefficientWithRebar[stbTypes::CrownSlope]    = GetHaulingWithMildRebarAllowableStressFactorNormalCrown();
   criteria.AllowableTension[stbTypes::CrownSlope]               = GetHaulingAllowableTensileConcreteStressNormalCrown(segmentKey);
   criteria.AllowableTensionWithRebar[stbTypes::CrownSlope]      = GetHaulingWithMildRebarAllowableStressNormalCrown(segmentKey);

   GetHaulingAllowableTensileConcreteStressParametersMaxSuper(&criteria.TensionCoefficient[stbTypes::MaxSuper],&criteria.bMaxTension[stbTypes::MaxSuper],&criteria.MaxTension[stbTypes::MaxSuper]);
   criteria.TensionCoefficientWithRebar[stbTypes::MaxSuper]    = GetHaulingWithMildRebarAllowableStressFactorMaxSuper(segmentKey);
   criteria.AllowableTension[stbTypes::MaxSuper]               = GetHaulingAllowableTensileConcreteStressMaxSuper(segmentKey);
   criteria.AllowableTensionWithRebar[stbTypes::MaxSuper]      = GetHaulingWithMildRebarAllowableStressMaxSuper(segmentKey);

   criteria.MinFScr = GetHaulingCrackingFs();
   criteria.MinFSf  = GetHaulingRolloverFs();

   criteria.MaxClearSpan       = GetAllowableDistanceBetweenSupports(segmentKey);
   criteria.MaxLeadingOverhang = GetAllowableLeadingOverhang(segmentKey);
   criteria.MaxGirderWeight    = GetMaxGirderWgt(segmentKey);


   return criteria;
}

stbHaulingCriteria CSpecAgentImp::GetHaulingStabilityCriteria(const CSegmentKey& segmentKey,const HANDLINGCONFIG& haulConfig) const
{
   stbHaulingCriteria criteria;

   GET_IFACE(IMaterials,pMaterial);
   pgsTypes::ConcreteType concType = pMaterial->GetSegmentConcreteType(segmentKey);
   criteria.Lambda = pMaterial->GetSegmentLambda(segmentKey);

   criteria.CompressionCoefficient_GlobalStress = -GetHaulingAllowableGlobalCompressionFactor();
   criteria.CompressionCoefficient_PeakStress = -GetHaulingAllowablePeakCompressionFactor();
   criteria.AllowableCompression_GlobalStress = GetHaulingAllowableGlobalCompressiveConcreteStressEx(segmentKey, haulConfig.GdrConfig.fc);
   criteria.AllowableCompression_PeakStress = GetHaulingAllowablePeakCompressiveConcreteStressEx(segmentKey, haulConfig.GdrConfig.fc);

   GetHaulingAllowableTensileConcreteStressParametersNormalCrown(&criteria.TensionCoefficient[stbTypes::CrownSlope],&criteria.bMaxTension[stbTypes::CrownSlope],&criteria.MaxTension[stbTypes::CrownSlope]);
   criteria.TensionCoefficientWithRebar[stbTypes::CrownSlope] = GetHaulingWithMildRebarAllowableStressFactorNormalCrown();
   if (haulConfig.bIgnoreGirderConfig)
   {
      criteria.AllowableTension[stbTypes::CrownSlope]               = GetHaulingAllowableTensileConcreteStressNormalCrown(segmentKey);
      criteria.AllowableTensionWithRebar[stbTypes::CrownSlope]      = GetHaulingWithMildRebarAllowableStressNormalCrown(segmentKey);
   }
   else
   {
      criteria.AllowableTension[stbTypes::CrownSlope] = GetHaulingAllowableTensileConcreteStressExNormalCrown(segmentKey,haulConfig.GdrConfig.fc,false);
      criteria.AllowableTensionWithRebar[stbTypes::CrownSlope] = GetHaulingAllowableTensileConcreteStressExNormalCrown(segmentKey,haulConfig.GdrConfig.fc,true);
   }

   GetHaulingAllowableTensileConcreteStressParametersMaxSuper(&criteria.TensionCoefficient[stbTypes::MaxSuper],&criteria.bMaxTension[stbTypes::MaxSuper],&criteria.MaxTension[stbTypes::MaxSuper]);
   criteria.TensionCoefficientWithRebar[stbTypes::MaxSuper]    = GetHaulingWithMildRebarAllowableStressFactorMaxSuper(segmentKey);

   if (haulConfig.bIgnoreGirderConfig)
   {
      criteria.AllowableTension[stbTypes::MaxSuper]               = GetHaulingAllowableTensileConcreteStressMaxSuper(segmentKey);
      criteria.AllowableTensionWithRebar[stbTypes::MaxSuper]      = GetHaulingWithMildRebarAllowableStressMaxSuper(segmentKey);
   }
   else
   {
      criteria.AllowableTension[stbTypes::MaxSuper]          = GetHaulingAllowableTensileConcreteStressExMaxSuper(segmentKey,haulConfig.GdrConfig.fc,false);
      criteria.AllowableTensionWithRebar[stbTypes::MaxSuper] = GetHaulingAllowableTensileConcreteStressExMaxSuper(segmentKey,haulConfig.GdrConfig.fc,true);
   }

   criteria.MinFScr = GetHaulingCrackingFs();
   criteria.MinFSf  = GetHaulingRolloverFs();

   if (haulConfig.pHaulTruckEntry == nullptr )
   {
      criteria.MaxClearSpan       = GetAllowableDistanceBetweenSupports(segmentKey);
      criteria.MaxLeadingOverhang = GetAllowableLeadingOverhang(segmentKey);
      criteria.MaxGirderWeight    = GetMaxGirderWgt(segmentKey);
   }
   else
   {
      criteria.MaxClearSpan       = haulConfig.pHaulTruckEntry->GetMaxDistanceBetweenBunkPoints();
      criteria.MaxLeadingOverhang = haulConfig.pHaulTruckEntry->GetMaximumLeadingOverhang();
      criteria.MaxGirderWeight    = haulConfig.pHaulTruckEntry->GetMaxGirderWeight();
   }

   return criteria;
}

/////////////////////////////////////////////////////////////////////
//  IKdotGirderLiftingSpecCriteria
// Spec criteria for KDOT analyses
Float64 CSpecAgentImp::GetKdotHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey) const
{
   return GetHaulingAllowableTensileConcreteStressNormalCrown(segmentKey);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey) const
{
   return GetHaulingAllowableGlobalCompressiveConcreteStress(segmentKey);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableTensionFactor() const
{
   return GetHaulingAllowableTensionFactorNormalCrown();
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableCompressionFactor() const
{
   return GetHaulingAllowableGlobalCompressionFactor();
}

Float64 CSpecAgentImp::GetKdotHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey) const

{
   return GetHaulingWithMildRebarAllowableStressNormalCrown(segmentKey);
}

Float64 CSpecAgentImp::GetKdotHaulingWithMildRebarAllowableStressFactor() const
{
   return GetHaulingWithMildRebarAllowableStressFactorNormalCrown();
}

void CSpecAgentImp::GetKdotHaulingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) const
{
   GetHaulingAllowableTensileConcreteStressParametersNormalCrown(factor, pbMax, fmax);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar) const
{
   return GetHaulingAllowableTensileConcreteStressExNormalCrown(segmentKey, fc, includeRebar);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableCompressiveConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc) const
{
   return GetHaulingAllowableGlobalCompressiveConcreteStressEx(segmentKey,fc);
}

void CSpecAgentImp::GetMinimumHaulingSupportLocation(Float64* pHardDistance, bool* pUseFactoredLength, Float64* pLengthFactor) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *pHardDistance = pSpec->GetMininumTruckSupportLocation();
   *pUseFactoredLength = pSpec->GetUseMinTruckSupportLocationFactor();
   *pLengthFactor = pSpec->GetMinTruckSupportLocationFactor();
}

Float64 CSpecAgentImp::GetHaulingDesignLocationAccuracy() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetTruckSupportLocationAccuracy();
}

void CSpecAgentImp::GetHaulingGFactors(Float64* pOverhangFactor, Float64* pInteriorFactor) const
{
   const SpecLibraryEntry* pSpec = GetSpec();

   *pOverhangFactor = pSpec->GetOverhangGFactor();
   *pInteriorFactor = pSpec->GetInteriorGFactor();
}

/////////////////////////////////////////////////////////////////////
// IDebondLimits
Float64 CSpecAgentImp::GetMaxDebondedStrands(const CSegmentKey& segmentKey) const
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);

   return pGirderEntry->GetMaxTotalFractionDebondedStrands();
}

Float64 CSpecAgentImp::GetMaxDebondedStrandsPerRow(const CSegmentKey& segmentKey) const
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   return pGirderEntry->GetMaxFractionDebondedStrandsPerRow();
}

Float64 CSpecAgentImp::GetMaxDebondedStrandsPerSection(const CSegmentKey& segmentKey) const
{
   StrandIndexType nMax;
   Float64 fMax;

   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   pGirderEntry->GetMaxDebondedStrandsPerSection(&nMax,&fMax);

   return fMax;
}

StrandIndexType CSpecAgentImp::GetMaxNumDebondedStrandsPerSection(const CSegmentKey& segmentKey) const
{
   StrandIndexType nMax;
   Float64 fMax;

   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   pGirderEntry->GetMaxDebondedStrandsPerSection(&nMax,&fMax);

   return nMax;
}

void CSpecAgentImp::GetMaxDebondLength(const CSegmentKey& segmentKey, Float64* pLen, pgsTypes::DebondLengthControl* pControl) const
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);

   bool bSpanFraction, buseHard;
   Float64 spanFraction, hardDistance;
   pGirderEntry->GetMaxDebondedLength(&bSpanFraction, &spanFraction, &buseHard, &hardDistance);

   GET_IFACE(IBridge,pBridge);

   GET_IFACE(ISegmentData, pSegmentData);
   bool bUHPC = pSegmentData->GetSegmentMaterial(segmentKey)->Concrete.Type == pgsTypes::UHPC ? true : false;


   Float64 gdrlength = pBridge->GetSegmentLength(segmentKey);

   GET_IFACE(IPointOfInterest,pPOI);
   PoiList vPOI;
   pPOI->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPOI);
   ATLASSERT(vPOI.size() == 1);
   const pgsPointOfInterest& poi( vPOI.front() );

   // always use half girder length - development length
   GET_IFACE(IPretensionForce, pPrestressForce ); 
   Float64 dev_len = pPrestressForce->GetDevLength(poi,true,bUHPC); // set debonding to true to get max length

   Float64 min_len = gdrlength/2.0 - dev_len;
   *pControl = pgsTypes::mdbDefault;

   if (bSpanFraction)
   {
      Float64 sflen = gdrlength * spanFraction;
      if (sflen < min_len)
      {
         min_len = sflen;
         *pControl = pgsTypes::mbdFractional;
      }
   }

   if (buseHard)
   {
      if (hardDistance < min_len )
      {
         min_len = hardDistance;
         *pControl = pgsTypes::mdbHardLength;
      }
   }

   *pLen = min_len>0.0 ? min_len : 0.0; // don't return less than zero
}

Float64 CSpecAgentImp::GetMinDebondSectionDistance(const CSegmentKey& segmentKey) const
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   return pGirderEntry->GetMinDebondSectionLength();
}

/////////////////////////////////////////////////////////////////////
// IResistanceFactors
void CSpecAgentImp::GetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64* phiTensionPS,Float64* phiTensionRC,Float64* phiTensionSpliced,Float64* phiCompression) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   pSpec->GetFlexureResistanceFactors(type,phiTensionPS,phiTensionRC,phiTensionSpliced,phiCompression);
}

void CSpecAgentImp::GetFlexuralStrainLimits(matPsStrand::Grade grade,matPsStrand::Type type,Float64* pecl,Float64* petl) const
{
   // The values for Grade 60 are the same as for all types of strand
   GetFlexuralStrainLimits(matRebar::Grade60,pecl,petl);
}

void CSpecAgentImp::GetFlexuralStrainLimits(matRebar::Grade rebarGrade,Float64* pecl,Float64* petl) const
{
   *pecl = lrfdRebar::GetCompressionControlledStrainLimit(rebarGrade);
   *petl = lrfdRebar::GetTensionControlledStrainLimit(rebarGrade);

#if defined _DEBUG
   Float64 ecl, etl;
   switch (rebarGrade )
   {
   case matRebar::Grade40:
      ecl = 0.0014;
      etl = 0.005;
      break;

   case matRebar::Grade60:
      ecl = 0.002;
      etl = 0.005;
      break;

   case matRebar::Grade75:
      ecl = 0.0028;
      etl = 0.0050;
      break;

   case matRebar::Grade80:
      ecl = 0.0030;
      etl = 0.0056;
      break;

   case matRebar::Grade100:
      ecl = 0.0040;
      etl = 0.0080;
      break;

   default:
      ATLASSERT(false); // new rebar grade?
   }
   ATLASSERT(IsEqual(*pecl,ecl,0.0001));
   ATLASSERT(IsEqual(*petl,etl,0.0001));
#endif
}

Float64 CSpecAgentImp::GetShearResistanceFactor(bool isDebonded, pgsTypes::ConcreteType type) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetShearResistanceFactor(isDebonded, type);
}

Float64 CSpecAgentImp::GetShearResistanceFactor(const pgsPointOfInterest& poi, pgsTypes::ConcreteType type) const
{
   // Test to see if poi is in a debonded region
   bool is_debond = false;

   const SpecLibraryEntry* pSpec = GetSpec();

   // different phi factor for debonding only applies to 8th edition and later
   if (pSpec->GetSpecificationType() >= lrfdVersionMgr::EighthEdition2017)
   {
      const CSegmentKey& segkey(poi.GetSegmentKey());
      GET_IFACE(IStrandGeometry, pStrandGeom);
      if (pStrandGeom->HasDebonding(segkey))
      {
         Float64 poi_loc = poi.GetDistFromStart();

         // first check left end
         SectionIndexType numsecs = pStrandGeom->GetNumDebondSections(segkey, pgsTypes::metStart, pgsTypes::Straight);
         if (numsecs > 0)
         {
            Float64 secloc = pStrandGeom->GetDebondSection(segkey, pgsTypes::metStart, numsecs - 1, pgsTypes::Straight);
            if (poi_loc <= secloc)
            {
               is_debond = true;
            }
         }

         if (!is_debond)
         {
            // Now right end
            SectionIndexType numsecs = pStrandGeom->GetNumDebondSections(segkey, pgsTypes::metEnd, pgsTypes::Straight);
            if (numsecs > 0)
            {
               Float64 secloc = pStrandGeom->GetDebondSection(segkey, pgsTypes::metEnd, 0, pgsTypes::Straight);
               if (poi_loc >= secloc)
               {
                  is_debond = true;
               }
            }
         }
      }
   }

   return pSpec->GetShearResistanceFactor(is_debond, type);
}

Float64 CSpecAgentImp::GetClosureJointFlexureResistanceFactor(pgsTypes::ConcreteType type) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetClosureJointFlexureResistanceFactor(type);
}

Float64 CSpecAgentImp::GetClosureJointShearResistanceFactor(pgsTypes::ConcreteType type) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetClosureJointShearResistanceFactor(type);
}

///////////////////////////////////////////////////
// IInterfaceShearRequirements 
ShearFlowMethod CSpecAgentImp::GetShearFlowMethod() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   return pSpec->GetShearFlowMethod();
}

Float64 CSpecAgentImp::GetMaxShearConnectorSpacing(const pgsPointOfInterest& poi) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   Float64 sMax = pSpec->GetMaxInterfaceShearConnectorSpacing();
   if ( lrfdVersionMgr::SeventhEdition2014 <= lrfdVersionMgr::GetVersion() )
   {
      GET_IFACE(ISectionProperties,pSectProp);
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;
      Float64 Hg = pSectProp->GetHg(intervalIdx,poi);
      sMax = min(Hg,sMax);
   }
   return sMax;
}

////////////////////
// IDuctLimits
Float64 CSpecAgentImp::GetRadiusOfCurvatureLimit(pgsTypes::DuctType ductType) const
{
   // LRFD 5.4.6.1
   // NOTE: This requirement changed from the 30 ft for plastic and 20 ft for metal in the 7th Edition and earlier to
   // "The minimum radius of curvature of tendon ducts shall take into account the tendon size, duct type and shape,
   // and the location relative to the stress anchorage; subject to the manufacturer's recommendations"...
   // This is not an enforceable requirement... we will retain the 30 ft and 20 ft limitations but could
   // expend the Project Criteria to make this user defined input
   return ::ConvertToSysUnits(ductType == pgsTypes::dtPlastic ? 30.0 : 20.0, unitMeasure::Feet);
}

Float64 CSpecAgentImp::GetSegmentTendonRadiusOfCurvatureLimit(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   return GetRadiusOfCurvatureLimit(pSegment->Tendons.DuctType);
}

Float64 CSpecAgentImp::GetSegmentTendonAreaLimit(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   return GetTendonAreaLimit(pSegment->Tendons.InstallationType);
}

Float64 CSpecAgentImp::GetSegmentTendonDuctSizeLimit(const CSegmentKey& segmentKey) const
{
   // LRFD 5.4.6.2
   const SpecLibraryEntry* pSpecEntry = GetSpec();
   return pSpecEntry->GetDuctDiameterRatio();
}

Float64 CSpecAgentImp::GetGirderTendonRadiusOfCurvatureLimit(const CGirderKey& girderKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPTData* pPTData = pIBridgeDesc->GetPostTensioning(girderKey);
   return GetRadiusOfCurvatureLimit(pPTData->DuctType);
}

Float64 CSpecAgentImp::GetGirderTendonAreaLimit(const CGirderKey& girderKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPTData* pPTData = pIBridgeDesc->GetPostTensioning(girderKey);
   return GetTendonAreaLimit(pPTData->InstallationType);
}

Float64 CSpecAgentImp::GetGirderTendonDuctSizeLimit(const CGirderKey& girderKey) const
{
   // LRFD 5.4.6.2
   const SpecLibraryEntry* pSpecEntry = GetSpec();
   return pSpecEntry->GetDuctDiameterRatio();
}

Float64 CSpecAgentImp::GetTendonAreaLimit(pgsTypes::StrandInstallationType installationType) const
{
   // LRFD 5.4.6.2
   const SpecLibraryEntry* pSpecEntry = GetSpec();
   Float64 pushRatio, pullRatio;
   pSpecEntry->GetDuctAreaRatio(&pushRatio, &pullRatio);
   return (installationType == pgsTypes::sitPush ? pushRatio : pullRatio);
}

////////////////////
// Private methods

const SpecLibraryEntry* CSpecAgentImp::GetSpec() const
{
   GET_IFACE( ISpecification, pSpec );
   GET_IFACE( ILibrary,       pLib );

   std::_tstring specName = pSpec->GetSpecification();
   return pLib->GetSpecEntry( specName.c_str() );
}

const GirderLibraryEntry* CSpecAgentImp::GetGirderEntry(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   return pGirderEntry;
}

bool CSpecAgentImp::IsLoadRatingServiceIIILimitState(pgsTypes::LimitState ls) const
{
   return ( ls == pgsTypes::ServiceIII_Inventory ||
            ls == pgsTypes::ServiceIII_Operating ||
            ls == pgsTypes::ServiceIII_LegalRoutine ||
            ls == pgsTypes::ServiceIII_LegalSpecial ||
            ls == pgsTypes::ServiceIII_LegalEmergency) ? true : false;
}

void CSpecAgentImp::ValidateHaulTruck(const CPrecastSegmentData* pSegment) const
{
   if (pSegment->HandlingData.pHaulTruckLibraryEntry == nullptr)
   {
      const CSegmentKey& segmentKey = pSegment->GetSegmentKey();

      CString strMsg;
      GET_IFACE(IDocumentType, pDocType);
      if (pDocType->IsPGSpliceDocument())
      {
         strMsg.Format(_T("The haul truck is not defined for Group %d Girder %s Segment %d"), LABEL_GROUP(segmentKey.groupIndex), LABEL_GIRDER(segmentKey.girderIndex), LABEL_SEGMENT(segmentKey.segmentIndex));
      }
      else
      {
         strMsg.Format(_T("The haul truck is not defined for Span %d Girder %s"), LABEL_SPAN(segmentKey.groupIndex), LABEL_GIRDER(segmentKey.girderIndex));
      }

      pgsSegmentRelatedStatusItem* pStatusItem = new pgsHaulTruckStatusItem(m_StatusGroupID, m_scidHaulTruckError, strMsg, segmentKey);

      GET_IFACE(IEAFStatusCenter, pStatusCenter);
      pStatusCenter->Add(pStatusItem);

      strMsg += "\r\nSee the Status Center for Details";

      THROW_UNWIND(strMsg, -1);
   }
}

void CSpecAgentImp::Invalidate()
{
   // remove our items from the status center
   GET_IFACE(IEAFStatusCenter, pStatusCenter);
   pStatusCenter->RemoveByStatusGroupID(m_StatusGroupID);
}
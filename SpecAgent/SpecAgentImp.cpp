///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <algorithm>

#include <PsgLib\BridgeDescription2.h>
#include <PgsExt\SegmentArtifact.h>
#include <PgsExt\GirderArtifact.h>
#include <PsgLib/HaulingCriteria.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <LRFD\PsStrand.h>
#include <LRFD\Rebar.h>

#include <EAF/EAFStatusCenter.h>
#include <IFace\PrestressForce.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>
#include <IFace\Intervals.h>
#include <IFace/PointOfInterest.h>

#include <Units\Convert.h>

#include <MfcTools\Exceptions.h>

#include <EAF\EAFDisplayUnits.h>

#include <PsgLib\GirderLabel.h>
#include <psgLib/StrandStressCriteria.h>
#include <psgLib/TendonStressCriteria.h>
#include <psgLib/LiftingCriteria.h>
#include <psgLib/PrestressedElementCriteria.h>
#include <psgLib/ClosureJointCriteria.h>
#include <psgLib/PrincipalTensionStressCriteria.h>
#include <psgLib/ShearCapacityCriteria.h>
#include <psgLib/SpecificationCriteria.h>
#include <psgLib/EndZoneCriteria.h>
#include <psgLib/MomentCapacityCriteria.h>
#include <psgLib/InterfaceShearCriteria.h>
#include <psgLib/DuctSizeCriteria.h>


bool CSpecAgentImp::RegisterInterfaces()
{
   EAF_AGENT_REGISTER_INTERFACES;

   REGISTER_INTERFACE(IStressCheck);
   REGISTER_INTERFACE(IStrandStressLimit);
   REGISTER_INTERFACE(ITendonStressLimit);
   REGISTER_INTERFACE(IConcreteStressLimits);
   REGISTER_INTERFACE(ITransverseReinforcementSpec);
   REGISTER_INTERFACE(ISplittingChecks);
   REGISTER_INTERFACE(IPrecastIGirderDetailsSpec);
   REGISTER_INTERFACE(ISegmentLiftingSpecCriteria);
   REGISTER_INTERFACE(ISegmentHaulingSpecCriteria);
   REGISTER_INTERFACE(IKdotGirderHaulingSpecCriteria);
   REGISTER_INTERFACE(IDebondLimits);
   REGISTER_INTERFACE(IResistanceFactors);
   REGISTER_INTERFACE(IInterfaceShearRequirements);
   REGISTER_INTERFACE(IDuctLimits);

   return true;
}

bool CSpecAgentImp::Init()
{
   EAF_AGENT_INIT;
   CREATE_LOGFILE("SpecAgent");

   GET_IFACE(IEAFStatusCenter, pStatusCenter);
   m_scidHaulTruckError = pStatusCenter->RegisterCallback(std::make_shared<pgsHaulTruckStatusCallback>(WBFL::EAF::StatusSeverityType::Error));

   // Attach to connection points
   m_dwBridgeDescCookie = REGISTER_EVENT_SINK(IBridgeDescriptionEventSink);

   return true;
}

CLSID CSpecAgentImp::GetCLSID() const
{
   return CLSID_SpecAgent;
}

bool CSpecAgentImp::Reset()
{
   EAF_AGENT_RESET;
   return true;
}

bool CSpecAgentImp::ShutDown()
{
   EAF_AGENT_SHUTDOWN;

   //
   // Detach to connection points
   //
   UNREGISTER_EVENT_SINK(IBridgeDescriptionEventSink, m_dwBridgeDescCookie);

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
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
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

   GET_IFACE(IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(segmentKey) != pgsTypes::PCI_UHPC)
   {
      // fatigue checks are not applicable to PCI_UHPC, put are applicable to all other
      vStressCheckTasks.emplace_back(lastIntervalIdx, WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI, pgsTypes::Compression);

      // this is a tension stress check for fatigue in UHPC. See GS 1.5.3
      if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
      {
         vStressCheckTasks.emplace_back(lastIntervalIdx, pgsTypes::FatigueI, pgsTypes::Tension);
      }
   }

   // for spliced girders, spec changes must occur every time there is a change in boundary condition, change in external loading, and application of prestress force
   GET_IFACE(IDocumentType, pDocType);
   if (pDocType->IsPGSpliceDocument())
   {
      // only need to check stress during storage if support conditions are different than release support conditions
      // because this would constitute a change in loading
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

      // Segment erection intervals are considered a change in loading condition due to potentially moved support locations.
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      vStressCheckTasks.emplace_back(erectionIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
      vStressCheckTasks.emplace_back(erectionIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);

      // Erection of other segments, after this segment is erected, is also a change in loading condition.
      // This isn't always true, but it is easier to assume it is true. A case when the assumption is false
      // would be a 5 span bridge and the first and last segments are erected... the dead load of the second
      // segment isn't carried by the first segment. However, in the case of a drop-in segment being erected
      // onto a hammerhead segment, the hammerhead carries the weight of the drop-in so the hammerhead must
      // be spec checked for stresses when the drop-in is erected.
      IntervalIndexType firstSegmentErectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(segmentKey);
      IntervalIndexType lastSegmentErectionIntervalIdx = pIntervals->GetLastSegmentErectionInterval(segmentKey);
      for (IntervalIndexType otherSegmentErectionIntervalIdx = Max(firstSegmentErectionIntervalIdx, erectionIntervalIdx+1); otherSegmentErectionIntervalIdx <= lastSegmentErectionIntervalIdx; otherSegmentErectionIntervalIdx++)
      {
         if (pIntervals->IsSegmentErectionInterval(otherSegmentErectionIntervalIdx) && erectionIntervalIdx < otherSegmentErectionIntervalIdx)
         {
            vStressCheckTasks.emplace_back(otherSegmentErectionIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression);
            vStressCheckTasks.emplace_back(otherSegmentErectionIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension);
         }
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
            vStressCheckTasks.emplace_back(intervalIdx, liveLoadIntervalIdx <= intervalIdx ? pgsTypes::ServiceIII : pgsTypes::ServiceI, pgsTypes::Tension);
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
         vStressCheckTasks.emplace_back(overlayIntervalIdx, liveLoadIntervalIdx <= overlayIntervalIdx ? pgsTypes::ServiceIII : pgsTypes::ServiceI, pgsTypes::Tension);
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
      IndexType nGirders = pBridge->GetGirderCount(grpIdx);
      CGirderKey thisGirderKey(grpIdx, Min(girderKey.girderIndex,nGirders-1));
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
bool CSpecAgentImp::CheckStrandStressAtJacking() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& strand_stress_criteria = pSpec->GetStrandStressCriteria();
   return strand_stress_criteria.CheckStrandStress(StrandStressCriteria::CheckStage::AtJacking);
}

bool CSpecAgentImp::CheckStrandStressBeforeXfer() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& strand_stress_criteria = pSpec->GetStrandStressCriteria();
   return strand_stress_criteria.CheckStrandStress(StrandStressCriteria::CheckStage::BeforeTransfer);
}

bool CSpecAgentImp::CheckStrandStressAfterXfer() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& strand_stress_criteria = pSpec->GetStrandStressCriteria();
   return strand_stress_criteria.CheckStrandStress(StrandStressCriteria::CheckStage::AfterTransfer);
}

bool CSpecAgentImp::CheckStrandStressAfterLosses() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& strand_stress_criteria = pSpec->GetStrandStressCriteria();
   return strand_stress_criteria.CheckStrandStress(StrandStressCriteria::CheckStage::AfterAllLosses);
}

Float64 CSpecAgentImp::GetStrandStressLimitAtJacking(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const
{
   if ( !CheckStrandStressAtJacking() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetStrandMaterial(segmentKey,strandType);

   Float64 fpu = WBFL::LRFD::PsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& strand_stress_criteria = pSpec->GetStrandStressCriteria();
   Float64 coeff = strand_stress_criteria.GetStrandStressCoefficient(StrandStressCriteria::CheckStage::AtJacking,pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? StrandStressCriteria::StrandType::LowRelaxation : StrandStressCriteria::StrandType::StressRelieved);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetStrandStressLimitBeforeXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const
{
   if ( !CheckStrandStressBeforeXfer() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetStrandMaterial(segmentKey,strandType);

   Float64 fpu = WBFL::LRFD::PsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& strand_stress_criteria = pSpec->GetStrandStressCriteria();
   Float64 coeff = strand_stress_criteria.GetStrandStressCoefficient(StrandStressCriteria::CheckStage::BeforeTransfer,pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? StrandStressCriteria::StrandType::LowRelaxation : StrandStressCriteria::StrandType::StressRelieved);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetStrandStressLimitAfterXfer(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const
{
   if ( !CheckStrandStressAfterXfer() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetStrandMaterial(segmentKey,strandType);

   Float64 fpu = WBFL::LRFD::PsStrand::GetUltimateStrength(pStrand->GetGrade());

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& strand_stress_criteria = pSpec->GetStrandStressCriteria();
   Float64 coeff = strand_stress_criteria.GetStrandStressCoefficient(StrandStressCriteria::CheckStage::AfterTransfer,pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? StrandStressCriteria::StrandType::LowRelaxation : StrandStressCriteria::StrandType::StressRelieved);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetStrandStressLimitAfterLosses(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType) const
{
   if ( !CheckStrandStressAfterLosses() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetStrandMaterial(segmentKey,strandType);

   Float64 fpy = WBFL::LRFD::PsStrand::GetYieldStrength(pStrand->GetGrade(),pStrand->GetType());

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& strand_stress_criteria = pSpec->GetStrandStressCriteria();
   Float64 coeff = strand_stress_criteria.GetStrandStressCoefficient(StrandStressCriteria::CheckStage::AfterAllLosses,pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? StrandStressCriteria::StrandType::LowRelaxation : StrandStressCriteria::StrandType::StressRelieved);

   return coeff*fpy;
}

/////////////////////////////////////////////////////////
// IAllowableTendonStress
//
bool CSpecAgentImp::CheckTendonStressAtJacking() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& tendon_stress_criteria = pSpec->GetTendonStressCriteria();
   return tendon_stress_criteria.bCheckAtJacking;
}

bool CSpecAgentImp::CheckTendonStressPriorToSeating() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& tendon_stress_criteria = pSpec->GetTendonStressCriteria();
   return tendon_stress_criteria.bCheckPriorToSeating;
}

Float64 CSpecAgentImp::GetSegmentTendonStressLimitAtJacking(const CSegmentKey& segmentKey) const
{
   if (!CheckTendonStressAtJacking())
   {
      return 0.0;
   }

   GET_IFACE(IMaterials, pMaterial);
   const auto* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   Float64 fpu = WBFL::LRFD::PsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetSegmentTendonStressLimitCoefficientAtJacking(segmentKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetSegmentTendonStressLimitPriorToSeating(const CSegmentKey& segmentKey) const
{
   if (!CheckTendonStressPriorToSeating())
   {
      return 0.0;
   }

   GET_IFACE(IMaterials, pMaterial);
   const auto* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   Float64 fpy = WBFL::LRFD::PsStrand::GetYieldStrength(pStrand->GetGrade(), pStrand->GetType());

   Float64 coeff = GetSegmentTendonStressLimitCoefficientPriorToSeating(segmentKey);

   return coeff*fpy;
}

Float64 CSpecAgentImp::GetSegmentTendonStressLimitAfterAnchorSetAtAnchorage(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const auto* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   Float64 fpu = WBFL::LRFD::PsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetSegmentTendonStressLimitCoefficientAfterAnchorSetAtAnchorage(segmentKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetSegmentTendonStressLimitAfterAnchorSet(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const auto* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   Float64 fpu = WBFL::LRFD::PsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetSegmentTendonStressLimitCoefficientAfterAnchorSet(segmentKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetSegmentTendonStressLimitAfterLosses(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const auto* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   Float64 fpy = WBFL::LRFD::PsStrand::GetYieldStrength(pStrand->GetGrade(), pStrand->GetType());

   Float64 coeff = GetSegmentTendonStressLimitCoefficientAfterLosses(segmentKey);

   return coeff*fpy;
}

Float64 CSpecAgentImp::GetSegmentTendonStressLimitCoefficientAtJacking(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const auto* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& tendon_stress_criteria = pSpec->GetTendonStressCriteria();
   auto coeff = tendon_stress_criteria.GetTendonStressCoefficient(TendonStressCriteria::CheckStage::AtJacking, pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? TendonStressCriteria::StrandType::LowRelaxation : TendonStressCriteria::StrandType::StressRelieved);
   return coeff;
}

Float64 CSpecAgentImp::GetSegmentTendonStressLimitCoefficientPriorToSeating(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const auto* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& tendon_stress_criteria = pSpec->GetTendonStressCriteria();
   auto coeff = tendon_stress_criteria.GetTendonStressCoefficient(TendonStressCriteria::CheckStage::PriorToSeating, pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? TendonStressCriteria::StrandType::LowRelaxation : TendonStressCriteria::StrandType::StressRelieved);
   return coeff;
}

Float64 CSpecAgentImp::GetSegmentTendonStressLimitCoefficientAfterAnchorSetAtAnchorage(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const auto* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& tendon_stress_criteria = pSpec->GetTendonStressCriteria();
   auto coeff = tendon_stress_criteria.GetTendonStressCoefficient(TendonStressCriteria::CheckStage::AtAnchoragesAfterSeating, pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? TendonStressCriteria::StrandType::LowRelaxation : TendonStressCriteria::StrandType::StressRelieved);
   return coeff;
}

Float64 CSpecAgentImp::GetSegmentTendonStressLimitCoefficientAfterAnchorSet(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const auto* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& tendon_stress_criteria = pSpec->GetTendonStressCriteria();
   auto coeff = tendon_stress_criteria.GetTendonStressCoefficient(TendonStressCriteria::CheckStage::ElsewhereAfterSeating, pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? TendonStressCriteria::StrandType::LowRelaxation : TendonStressCriteria::StrandType::StressRelieved);
   return coeff;
}

Float64 CSpecAgentImp::GetSegmentTendonStressLimitCoefficientAfterLosses(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterial);
   const auto* pStrand = pMaterial->GetSegmentTendonMaterial(segmentKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& tendon_stress_criteria = pSpec->GetTendonStressCriteria();
   auto coeff = tendon_stress_criteria.GetTendonStressCoefficient(TendonStressCriteria::CheckStage::AfterAllLosses, pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? TendonStressCriteria::StrandType::LowRelaxation : TendonStressCriteria::StrandType::StressRelieved);
   return coeff;
}

Float64 CSpecAgentImp::GetGirderTendonStressLimitAtJacking(const CGirderKey& girderKey) const
{
   if ( !CheckTendonStressAtJacking() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   Float64 fpu = WBFL::LRFD::PsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetGirderTendonStressLimitCoefficientAtJacking(girderKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetGirderTendonStressLimitPriorToSeating(const CGirderKey& girderKey) const
{
   if ( !CheckTendonStressPriorToSeating() )
   {
      return 0.0;
   }

   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   Float64 fpy = WBFL::LRFD::PsStrand::GetYieldStrength(pStrand->GetGrade(),pStrand->GetType());

   Float64 coeff = GetGirderTendonStressLimitCoefficientPriorToSeating(girderKey);

   return coeff*fpy;
}

Float64 CSpecAgentImp::GetGirderTendonStressLimitAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   Float64 fpu = WBFL::LRFD::PsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetGirderTendonStressLimitCoefficientAfterAnchorSetAtAnchorage(girderKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetGirderTendonStressLimitAfterAnchorSet(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   Float64 fpu = WBFL::LRFD::PsStrand::GetUltimateStrength(pStrand->GetGrade());

   Float64 coeff = GetGirderTendonStressLimitCoefficientAfterAnchorSet(girderKey);

   return coeff*fpu;
}

Float64 CSpecAgentImp::GetGirderTendonStressLimitAfterLosses(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   Float64 fpy = WBFL::LRFD::PsStrand::GetYieldStrength(pStrand->GetGrade(),pStrand->GetType());

   Float64 coeff = GetGirderTendonStressLimitCoefficientAfterLosses(girderKey);

   return coeff*fpy;
}

Float64 CSpecAgentImp::GetGirderTendonStressLimitCoefficientAtJacking(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& tendon_stress_criteria = pSpec->GetTendonStressCriteria();
   auto coeff = tendon_stress_criteria.GetTendonStressCoefficient(TendonStressCriteria::CheckStage::AtJacking, pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? TendonStressCriteria::StrandType::LowRelaxation : TendonStressCriteria::StrandType::StressRelieved);
   return coeff;
}

Float64 CSpecAgentImp::GetGirderTendonStressLimitCoefficientPriorToSeating(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& tendon_stress_criteria = pSpec->GetTendonStressCriteria();
   auto coeff = tendon_stress_criteria.GetTendonStressCoefficient(TendonStressCriteria::CheckStage::PriorToSeating, pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? TendonStressCriteria::StrandType::LowRelaxation : TendonStressCriteria::StrandType::StressRelieved);
   return coeff;
}

Float64 CSpecAgentImp::GetGirderTendonStressLimitCoefficientAfterAnchorSetAtAnchorage(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& tendon_stress_criteria = pSpec->GetTendonStressCriteria();
   auto coeff = tendon_stress_criteria.GetTendonStressCoefficient(TendonStressCriteria::CheckStage::AtAnchoragesAfterSeating, pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? TendonStressCriteria::StrandType::LowRelaxation : TendonStressCriteria::StrandType::StressRelieved);
   return coeff;
}

Float64 CSpecAgentImp::GetGirderTendonStressLimitCoefficientAfterAnchorSet(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& tendon_stress_criteria = pSpec->GetTendonStressCriteria();
   auto coeff = tendon_stress_criteria.GetTendonStressCoefficient(TendonStressCriteria::CheckStage::ElsewhereAfterSeating, pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? TendonStressCriteria::StrandType::LowRelaxation : TendonStressCriteria::StrandType::StressRelieved);
   return coeff;
}

Float64 CSpecAgentImp::GetGirderTendonStressLimitCoefficientAfterLosses(const CGirderKey& girderKey) const
{
   GET_IFACE(IMaterials,pMaterial);
   const auto* pStrand = pMaterial->GetGirderTendonMaterial(girderKey);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& tendon_stress_criteria = pSpec->GetTendonStressCriteria();
   auto coeff = tendon_stress_criteria.GetTendonStressCoefficient(TendonStressCriteria::CheckStage::AfterAllLosses, pStrand->GetType() == WBFL::Materials::PsStrand::Type::LowRelaxation ? TendonStressCriteria::StrandType::LowRelaxation : TendonStressCriteria::StrandType::StressRelieved);
   return coeff;
}

/////////////////////////////////////////////////////////////////////////////
// IAllowableConcreteStress
//
Float64 CSpecAgentImp::GetConcreteCompressionStressLimit(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         return GetClosureJointConcreteCompressionStressLimit(poi,task);
      }
      else
      {
         return GetSegmentConcreteCompressionStressLimit(poi,task);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      return GetDeckConcreteCompressionStressLimit(poi,task);
   }
}

Float64 CSpecAgentImp::GetConcreteTensionStressLimit(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         return GetClosureJointConcreteTensionStressLimit(poi,task,bWithBondedReinforcement,bInPrecompressedTensileZone);
      }
      else
      {
         return GetSegmentConcreteTensionStressLimit(poi,task,bWithBondedReinforcement);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      return GetDeckConcreteTensionStressLimit(poi,task,bWithBondedReinforcement);
   }
}

void CSpecAgentImp::ReportSegmentConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   ATLASSERT(!poi.HasAttribute(POI_CLOSURE));

   INIT_UV_PROTOTYPE(rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true);

   GET_IFACE(IIntervals, pIntervals);   // use f'ci if this is at release, otherwise use f'c
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   bool bFci = (task.intervalIdx == releaseIntervalIdx ? true : false);

   Float64 c = GetSegmentConcreteCompressionStressLimitCoefficient(poi, task);
   Float64 fLimit = GetSegmentConcreteCompressionStressLimit(poi, task);

   *pPara << _T("Compression stress limit = -") << c;

   if (bFci)
   {
      (*pPara) << RPT_FCI;
   }
   else
   {
      (*pPara) << RPT_FC;
   }

   *pPara << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;
}

std::_tstring CSpecAgentImp::GetConcreteStressLimitParameterName(pgsTypes::StressType stressType, pgsTypes::ConcreteType concreteType) const
{
   std::_tstring strParamName;
   if (stressType == pgsTypes::Compression)
   {
      strParamName = _T("concrete strength");
   }
   else
   {
      switch (concreteType)
      {
      case pgsTypes::Normal:
      case pgsTypes::AllLightweight:
      case pgsTypes::SandLightweight:
      case pgsTypes::PCI_UHPC:
         strParamName = _T("concrete strength");
         break;
      case pgsTypes::UHPC:
         strParamName = _T("cracking strength");
         break;
      }
   }

   return strParamName;
}

void CSpecAgentImp::ReportSegmentConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, const pgsSegmentArtifact* pSegmentArtifact, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   ATLASSERT(!poi.HasAttribute(POI_CLOSURE));

   INIT_UV_PROTOTYPE(rptPressureSectionValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE(IIntervals, pIntervals);
   // use f'ci for stressing intervals, otherwise f'c
   auto bIsStressingInterval = pIntervals->IsStressingInterval(segmentKey, task.intervalIdx);
   TensionStressLimit::ConcreteSymbol concrete_symbol = (bIsStressingInterval ? TensionStressLimit::ConcreteSymbol::fci : TensionStressLimit::ConcreteSymbol::fc);

   GET_IFACE(IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
   {
      Float64 f_fc = pMaterials->GetSegmentConcreteFirstCrackingStrength(segmentKey);
      Float64 fLimit = GetSegmentConcreteTensionStressLimit(poi, task, false/*without rebar*/);
      ATLASSERT(IsEqual(GetPCIUHPCTensionStressLimitCoefficient(), 2.0 / 3.0));

      if (concrete_symbol == TensionStressLimit::ConcreteSymbol::fci)
      {
         *pPara << _T("Tension stress limit in areas other than the precompressed tensile zone = (2/3)(") << RPT_STRESS(_T("fc")) << _T(")") << symbol(ROOT) << _T("(") << RPT_FCI << _T("/") << RPT_FC << _T(")");
      }
      else
      {
         *pPara << _T("Tension stress limit in the precompressed tensile zone = (2/3)(") << RPT_STRESS(_T("fc")) << _T(") = (2/3)(") << stress_u.SetValue(f_fc) << _T(")");
      }
      *pPara << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;
   }
   else if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      Float64 ft_cri = pMaterials->GetSegmentConcreteInitialEffectiveCrackingStrength(segmentKey);
      Float64 ft_cr = pMaterials->GetSegmentConcreteDesignEffectiveCrackingStrength(segmentKey);
      if (task.limitState == pgsTypes::FatigueI)
      {
         Float64 k = GetUHPCFatigueTensionStressLimitModifier();
         Float64 gamma_u = GetUHPCTensionStressLimitCoefficient(segmentKey);
         *pPara << _T("Tension stress limit = ") << k << Sub2(symbol(gamma), _T("u")) << Sub2(_T("f"), _T("t,cr")) << _T(" = (") << k << _T(")(") << scalar.SetValue(gamma_u) << _T(")(") << stress_u.SetValue(ft_cr) << _T(")");
      }
      else
      {
         Float64 gamma_u = GetUHPCTensionStressLimitCoefficient(segmentKey);
         if (concrete_symbol == TensionStressLimit::ConcreteSymbol::fci)
         {
            *pPara << _T("Tension stress limit in areas other than the precompressed tensile zone = ") << Sub2(symbol(gamma), _T("u")) << Sub2(_T("f"), _T("t,cri")) << _T(" = ") << scalar.SetValue(gamma_u) << _T("(") << stress_u.SetValue(ft_cri) << _T(")");
         }
         else
         {
            *pPara << _T("Tension stress limit in the precompressed tensile zone = ") << Sub2(symbol(gamma), _T("u")) << Sub2(_T("f"), _T("t,cr")) << _T(" = ") << scalar.SetValue(gamma_u) << __T("(") << stress_u.SetValue(ft_cr) << _T(")");
         }
      }

      Float64 fLimit = GetSegmentConcreteTensionStressLimit(poi, task, false/*without rebar*/);
      *pPara << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;
   }
   else
   {
      bool bIsStressingInterval = pIntervals->IsStressingInterval(segmentKey, task.intervalIdx);

      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType haulIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

      auto tension_stress_limit = GetSegmentConcreteTensionStressLimitParameters(poi, task, false/*without rebar*/);

      // The storage interval represents a change in loading conditions because supports move relative to release.
      // Per LRFD 5.12.3.4.3 for spliced girder segments the concrete stress limits for after losses in
      // LRFD 5.9.2.3.2. However this doesn't may any sense. At release the BeforeLosses case applies and
      // a short time later the AfterLosses case applies, and the stress limit change. This doesn't make sense.
      // For this reason we use the "Before Losses" case for storage

      if (bIsStressingInterval || storageIntervalIdx <= task.intervalIdx && task.intervalIdx < haulIntervalIdx)
      {
         if (WBFL::LRFD::BDSManager::Edition::TenthEdition2024 <= WBFL::LRFD::BDSManager::GetEdition())
         {
            (*pPara) << _T("Tension stress limit in areas without bonded reinforcement = ");
         }
         else
         {
            (*pPara) << _T("Tension stress limit in precompressed tensile zone without bounded reinforcement = N/A") << rptNewLine;
            (*pPara) << _T("Tension stress limit in areas other than the precompressed tensile zone and without bonded reinforcement = ");
         }
         tension_stress_limit.Report(pPara, pDisplayUnits, concrete_symbol);
         Float64 fLimit = GetSegmentConcreteTensionStressLimit(poi, task, false/*without rebar*/);
         (*pPara) << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;

         tension_stress_limit = GetSegmentConcreteTensionStressLimitParameters(poi, task, true/*with rebar*/);
         (*pPara) << _T("Tension stress limit in areas with sufficient bonded reinforcement = ");
         tension_stress_limit.Report(pPara, pDisplayUnits, concrete_symbol);

         fLimit = GetSegmentConcreteTensionStressLimit(poi, task, true/*with rebar*/);
         (*pPara) << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;
      }
      else
      {
         GET_IFACE(IEnvironment, pEnvironment);
         if(pEnvironment->GetExposureCondition() == pgsTypes::ExposureCondition::Normal)
            (*pPara) << _T("Tension stress limit for components with bonded prestressing tendons that are subjected to not worse than moderate corrosion conditions = ");
         else
            (*pPara) << _T("Tension stress limit for components with bonded prestressing tendons that are subjected to severe corrosive conditions = ");
         tension_stress_limit.Report(pPara, pDisplayUnits, concrete_symbol);

         Float64 fLimit = GetSegmentConcreteTensionStressLimit(poi, task, false/*without rebar*/);
         *pPara << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;
      }
   }
}

void CSpecAgentImp::ReportClosureJointConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   const CClosureKey& clousureKey(poi.GetSegmentKey());
   ATLASSERT(poi.HasAttribute(POI_CLOSURE));

   INIT_UV_PROTOTYPE(rptPressureSectionValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);

   // use f'ci for all intervals up to and including
   // when the closure joint becomes composite (initial loading of closure joint)
   // otherwise use f'c
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType compositeClosureIntervalIdx = pIntervals->GetCompositeClosureJointInterval(clousureKey);
   bool bFci = (task.intervalIdx < compositeClosureIntervalIdx ? true : false);

   Float64 c = GetClosureJointConcreteCompressionStressLimitCoefficient(poi, task);
   Float64 fLimit = GetClosureJointConcreteCompressionStressLimit(poi, task);
   *pPara << _T("Compression stress limit = -") << c;

   if (bFci)
   {
      *pPara << RPT_FCI;
   }
   else
   {
      *pPara << RPT_FC;
   }

   *pPara << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;
}

void CSpecAgentImp::ReportClosureJointConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task, const pgsSegmentArtifact* pSegmentArtifact, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   const CClosureKey& closureKey(poi.GetSegmentKey());
   ATLASSERT(poi.HasAttribute(POI_CLOSURE));

   INIT_UV_PROTOTYPE(rptPressureSectionValue, stress, pDisplayUnits->GetStressUnit(), false);
   INIT_UV_PROTOTYPE(rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);

   GET_IFACE(IIntervals, pIntervals);

   // use f'ci for stressing intervals, otherwise f'c
   auto bIsStressingInterval = pIntervals->IsStressingInterval(closureKey, task.intervalIdx);
   TensionStressLimit::ConcreteSymbol concrete_symbol = (bIsStressingInterval ? TensionStressLimit::ConcreteSymbol::fci : TensionStressLimit::ConcreteSymbol::fc);

#if defined _DEBUG
   GET_IFACE(IMaterials, pMaterials);
   ATLASSERT(!IsUHPC(pMaterials->GetClosureJointConcreteType(closureKey)));
#endif

   // Precompressed tensile zone
   auto tension_stress_limit = GetClosureJointConcreteTensionStressLimitParameters(poi, task, false/*without rebar*/, true/*in PTZ*/);
   Float64 fLimit = GetClosureJointConcreteTensionStressLimit(poi, task, false/*without rebar*/, true/*in PTZ*/);

   (*pPara) << _T("Tension stress limit in the precompressed tensile zone = ");
   tension_stress_limit.Report(pPara, pDisplayUnits, concrete_symbol);
   *pPara << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;

   tension_stress_limit = GetClosureJointConcreteTensionStressLimitParameters(poi, task, true/*with rebar*/, true/*in PTZ*/);
   fLimit = GetClosureJointConcreteTensionStressLimit(poi, task, true/*with rebar*/, true/*in PTZ*/);

   (*pPara) << _T("Tension stress limit in joints with minimum bonded auxiliary reinforcement in the precompressed tensile zone = ");
   tension_stress_limit.Report(pPara, pDisplayUnits, concrete_symbol);
   *pPara << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;


   // Other than Precompressed tensile zone
   tension_stress_limit = GetClosureJointConcreteTensionStressLimitParameters(poi, task, false/*without rebar*/, false/*not in PTZ*/);
   fLimit = GetClosureJointConcreteTensionStressLimit(poi, task, false/*without rebar*/, false/*not in PTZ*/);

   (*pPara) << _T("Tension stress limit in areas other than the precompressed tensile zone = ");
   tension_stress_limit.Report(pPara, pDisplayUnits, concrete_symbol);
   *pPara << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;

   tension_stress_limit = GetClosureJointConcreteTensionStressLimitParameters(poi, task, true/*with rebar*/, false/*not in PTZ*/);
   fLimit = GetClosureJointConcreteTensionStressLimit(poi, task, true/*with rebar*/, false/*not in PTZ*/);

   (*pPara) << _T("Tension stress limit in joints with minimum bonded auxiliary reinforcement in areas other than the precompressed tensile zone = ");
   tension_stress_limit.Report(pPara, pDisplayUnits, concrete_symbol);
   (*pPara) << _T(" = ") << stress_u.SetValue(fLimit) << rptNewLine;
}

Float64 CSpecAgentImp::GetConcreteTensionStressLimit(pgsTypes::LoadRatingType ratingType,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation) const
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType ratingIntervalIdx = pIntervals->GetLoadRatingInterval();


   Float64 fc;
   Float64 lambda;
   pgsTypes::ConcreteType concreteType = pgsTypes::Normal;
   GET_IFACE(IMaterials,pMaterials);
   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         fc = pMaterials->GetClosureJointDesignFc(closureKey,ratingIntervalIdx);
         lambda = pMaterials->GetClosureJointLambda(closureKey);
         ATLASSERT(!IsUHPC(pMaterials->GetClosureJointConcreteType(closureKey))); // not support UHPC spliced girders with UHPC joints yet
      }
      else
      {
         fc = pMaterials->GetSegmentDesignFc(segmentKey,ratingIntervalIdx);
         lambda = pMaterials->GetSegmentLambda(segmentKey);
         concreteType = pMaterials->GetSegmentConcreteType(segmentKey);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      fc = pMaterials->GetDeckDesignFc(ratingIntervalIdx);
      lambda = pMaterials->GetDeckLambda();
      ATLASSERT(!IsUHPC(pMaterials->GetDeckConcreteType())); // not supporting UHPC deck yet
   }

   Float64 fLimit;
   if (concreteType == pgsTypes::PCI_UHPC)
   {
      Float64 f_fc = pMaterials->GetSegmentConcreteFirstCrackingStrength(segmentKey);
      Float64 k = GetPCIUHPCTensionStressLimitCoefficient();
      fLimit = k * f_fc;
   }
   else if(concreteType == pgsTypes::UHPC)
   {
      Float64 gamma_u = GetUHPCTensionStressLimitCoefficient(segmentKey);
      Float64 ft_cr = pMaterials->GetSegmentConcreteDesignEffectiveCrackingStrength(segmentKey);
      fLimit = gamma_u * ft_cr;
   }
   else
   {
      GET_IFACE(IRatingSpecification, pRatingSpec);
      bool bCheckMax;
      Float64 fmax;
      Float64 x = pRatingSpec->GetAllowableTensionCoefficient(ratingType, &bCheckMax, &fmax);

      fLimit = x * lambda * sqrt(fc);

      if (bCheckMax)
      {
         fLimit = Min(fLimit, fmax);
      }
   }

   return fLimit;
}

Float64 CSpecAgentImp::GetConcreteCompressionStressLimitCoefficient(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         return GetClosureJointConcreteCompressionStressLimitCoefficient(poi,task);
      }
      else
      {
         return GetSegmentConcreteCompressionStressLimitCoefficient(poi,task);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      return GetDeckConcreteCompressionStressLimitCoefficient(poi,task);
   }
}

TensionStressLimit CSpecAgentImp::GetConcreteTensionStressLimitParameters(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   TensionStressLimit tension_stress_limit;

   if ( IsGirderStressLocation(stressLocation) )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         tension_stress_limit = GetClosureJointConcreteTensionStressLimitParameters(poi,task,bWithBondedReinforcement,bInPrecompressedTensileZone);
      }
      else
      {
         tension_stress_limit = GetSegmentConcreteTensionStressLimitParameters(poi,task,bWithBondedReinforcement);
      }
   }
   else
   {
      ATLASSERT(IsDeckStressLocation(stressLocation));
      tension_stress_limit = GetDeckConcreteTensionStressLimitParameters(poi,task,bWithBondedReinforcement);
   }

   return tension_stress_limit;
}

Float64 CSpecAgentImp::GetSegmentConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsConcreteStressLimitApplicable(segmentKey,task));

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetSegmentDesignFc(segmentKey,task.intervalIdx);

   Float64 fLimit = GetSegmentConcreteCompressionStressLimit(poi,task,fc);
   return fLimit;
}

Float64 CSpecAgentImp::GetClosureJointConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   GET_IFACE(IPointOfInterest,pPoi);
   CClosureKey closureKey;
   VERIFY(pPoi->IsInClosureJoint(poi,&closureKey));

   ATLASSERT(IsConcreteStressLimitApplicable(closureKey,task));

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetClosureJointDesignFc(closureKey,task.intervalIdx);

   Float64 fLimit = GetClosureJointConcreteCompressionStressLimit(poi,task,fc);
   return fLimit;
}

Float64 CSpecAgentImp::GetDeckConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsConcreteStressLimitApplicable(segmentKey,task));

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetDeckDesignFc(task.intervalIdx);

   Float64 fLimit = GetDeckConcreteCompressionStressLimit(poi,task,fc);
   return fLimit;
}

Float64 CSpecAgentImp::GetSegmentConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsConcreteStressLimitApplicable(segmentKey,task));

   if ( IsLoadRatingServiceIIILimitState(task.limitState) )
   {
#if defined _DEBUG
      // allowable stresses during load rating only make sense if live load is applied
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      ATLASSERT(liveLoadIntervalIdx <= task.intervalIdx );
#endif
      pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(task.limitState);
      return GetConcreteTensionStressLimit(ratingType,poi,pgsTypes::BottomGirder);
   }

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetSegmentDesignFc(segmentKey,task.intervalIdx);

   Float64 fLimit = GetSegmentConcreteTensionStressLimit(poi,task,fc,bWithBondedReinforcement);
   return fLimit;
}

Float64 CSpecAgentImp::GetClosureJointConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const
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
      return GetConcreteTensionStressLimit(ratingType,poi,pgsTypes::BottomGirder);
   }

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IPointOfInterest, pPoi);
   CClosureKey closureKey;
   VERIFY(pPoi->IsInClosureJoint(poi, &closureKey));

   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetClosureJointDesignFc(closureKey,task.intervalIdx);

   Float64 fLimit = GetClosureJointConcreteTensionStressLimit(poi,task,fc,bWithBondedReinforcement,bInPrecompressedTensileZone);
   return fLimit;
}

Float64 CSpecAgentImp::GetDeckConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsConcreteStressLimitApplicable(segmentKey,task));

   if ( IsLoadRatingServiceIIILimitState(task.limitState) )
   {
#if defined _DEBUG
      // allowable stresses during load rating only make sense if live load is applied
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      ATLASSERT(liveLoadIntervalIdx <= task.intervalIdx );
#endif
      pgsTypes::LoadRatingType ratingType = ::RatingTypeFromLimitState(task.limitState);
      return GetConcreteTensionStressLimit(ratingType,poi,pgsTypes::TopDeck);
   }

   // This is a design/check case, so use the regular specifications
   GET_IFACE(IMaterials,pMaterials);
   Float64 fc = pMaterials->GetDeckDesignFc(task.intervalIdx);

   Float64 fLimit = GetDeckConcreteTensionStressLimit(poi,task,fc,bWithBondedReinforcement);
   return fLimit;
}

std::vector<Float64> CSpecAgentImp::GetGirderConcreteCompressionStressLimit(const PoiList& vPoi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);
   ATLASSERT(IsConcreteStressLimitApplicable(vPoi.front().get().GetSegmentKey(),task));

   GET_IFACE(IPointOfInterest,pPoi);

   std::vector<Float64> vStress;
   vStress.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         vStress.push_back( GetClosureJointConcreteCompressionStressLimit(poi,task));
      }
      else
      {
         vStress.push_back( GetSegmentConcreteCompressionStressLimit(poi,task));
      }
   }

   return vStress;
}

std::vector<Float64> CSpecAgentImp::GetDeckConcreteCompressionStressLimit(const PoiList& vPoi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   ATLASSERT(IsConcreteStressLimitApplicable(vPoi.front().get().GetSegmentKey(),task));

   std::vector<Float64> vStress;
   vStress.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      vStress.push_back( GetDeckConcreteCompressionStressLimit(poi,task));
   }

   return vStress;
}

std::vector<Float64> CSpecAgentImp::GetGirderConcreteTensionStressLimit(const PoiList& vPoi, const StressCheckTask& task,bool bWithBondededReinforcement,bool bInPrecompressedTensileZone) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   ATLASSERT(IsConcreteStressLimitApplicable(vPoi.front().get().GetSegmentKey(),task));

   GET_IFACE(IPointOfInterest,pPoi);

   std::vector<Float64> vStress;
   vStress.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         vStress.push_back( GetClosureJointConcreteTensionStressLimit(poi,task,bWithBondededReinforcement,bInPrecompressedTensileZone));
      }
      else
      {
         vStress.push_back( GetSegmentConcreteTensionStressLimit(poi,task,bWithBondededReinforcement));
      }
   }

   return vStress;
}

std::vector<Float64> CSpecAgentImp::GetDeckConcreteTensionStressLimit(const PoiList& vPoi, const StressCheckTask& task,bool bWithBondededReinforcement) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   ATLASSERT(IsConcreteStressLimitApplicable(vPoi.front().get().GetSegmentKey(),task));

   std::vector<Float64> vStress;
   vStress.reserve(vPoi.size());
   for (const pgsPointOfInterest& poi : vPoi)
   {
      vStress.push_back( GetDeckConcreteTensionStressLimit(poi,task,bWithBondededReinforcement));
   }

   return vStress;
}

Float64 CSpecAgentImp::GetSegmentConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const
{
   Float64 x = GetSegmentConcreteCompressionStressLimitCoefficient(poi,task);

   // Add a minus sign because compression is negative
   return -x*fc;
}

Float64 CSpecAgentImp::GetClosureJointConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const
{
   Float64 x = GetClosureJointConcreteCompressionStressLimitCoefficient(poi,task);

   GET_IFACE(IPointOfInterest,pPoi);
   CClosureKey closureKey;
   VERIFY( pPoi->IsInClosureJoint(poi,&closureKey) );

   // Add a minus sign because compression is negative
   return -x*fc;
}

Float64 CSpecAgentImp::GetDeckConcreteCompressionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc) const
{
   Float64 x = GetDeckConcreteCompressionStressLimitCoefficient(poi,task);

   // Add a minus sign because compression is negative
   return -x*fc;
}

Float64 CSpecAgentImp::GetSegmentConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   GET_IFACE(IMaterials, pMaterials);
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   Float64 f = 0;
   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
   {
      Float64 f_fc = pMaterials->GetSegmentConcreteFirstCrackingStrength(segmentKey);
      Float64 k = GetPCIUHPCTensionStressLimitCoefficient();

      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
      if (haulingIntervalIdx <= task.intervalIdx)
      {
         f = k * f_fc;
      }
      else
      {
         Float64 fc28 = pMaterials->GetSegmentFc28(segmentKey);
         f = (k * f_fc)*sqrt(fc/fc28);
      }
   }
   else if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      Float64 ft_cri = pMaterials->GetSegmentConcreteInitialEffectiveCrackingStrength(segmentKey);
      Float64 ft_cr = pMaterials->GetSegmentConcreteDesignEffectiveCrackingStrength(segmentKey);
      Float64 gamma_u = GetUHPCTensionStressLimitCoefficient(segmentKey);
      Float64 k = (task.limitState == pgsTypes::FatigueI ? GetUHPCFatigueTensionStressLimitModifier() : 1.0);

      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
      if (haulingIntervalIdx <= task.intervalIdx)
      {
         f = k * gamma_u * ft_cr;
      }
      else
      {
         ATLASSERT(IsEqual(k, 1.0)); // K should always be 1.0 for initial strength since k is for a fatigue check
         f = k * gamma_u * ft_cri;
      }
   }
   else
   {
      auto tension_stress_limit = GetSegmentConcreteTensionStressLimitParameters(poi, task, bWithBondedReinforcement);
      Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);
      f = tension_stress_limit.GetStressLimit(lambda, fc);
   }

   return f;
}

Float64 CSpecAgentImp::GetClosureJointConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const
{
   auto tension_stress_limit = GetClosureJointConcreteTensionStressLimitParameters(poi,task,bWithBondedReinforcement,bInPrecompressedTensileZone);

   GET_IFACE(IPointOfInterest,pPoi);
   CClosureKey closureKey;
   VERIFY( pPoi->IsInClosureJoint(poi,&closureKey) );

   Float64 f = 0;
   GET_IFACE(IMaterials, pMaterials);
   ASSERT(!IsUHPC(pMaterials->GetClosureJointConcreteType(closureKey)));

   Float64 lambda = pMaterials->GetClosureJointLambda(closureKey);
   f = tension_stress_limit.GetStressLimit(lambda, fc);
   return f;
}

Float64 CSpecAgentImp::GetDeckConcreteTensionStressLimit(const pgsPointOfInterest& poi, const StressCheckTask& task,Float64 fc,bool bWithBondedReinforcement) const
{
   auto tension_stress_limit = GetDeckConcreteTensionStressLimitParameters(poi,task,bWithBondedReinforcement);

   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetDeckLambda();

   ATLASSERT(!IsUHPC(pMaterials->GetDeckConcreteType())); // deck concrete not allowed to be UHPC

   Float64 f = tension_stress_limit.GetStressLimit(lambda, fc);

   return f;
}

Float64 CSpecAgentImp::GetLiftingWithMildRebarAllowableStressFactor() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.TensionStressLimitWithReinforcement.Coefficient;
}

Float64 CSpecAgentImp::GetLiftingWithMildRebarAllowableStress(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   GET_IFACE(IMaterials, pMaterials);
   ATLASSERT(!IsUHPC(pMaterials->GetSegmentConcreteType(segmentKey)));
   Float64 fci = pMaterials->GetSegmentDesignFc(segmentKey,liftSegmentIntervalIdx);
   Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

   Float64 x = GetLiftingWithMildRebarAllowableStressFactor();

   return x*lambda*sqrt(fci);
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStressFactor(WBFL::Stability::HaulingSlope slope) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   return hauling_criteria.WSDOT.TensionStressLimitWithReinforcement[+slope].Coefficient;
}

Float64 CSpecAgentImp::GetHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey, WBFL::Stability::HaulingSlope slope) const
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);
   Float64 fc = pMaterial->GetSegmentDesignFc(segmentKey,haulSegmentIntervalIdx);
   Float64 lambda = pMaterial->GetSegmentLambda(segmentKey);

   Float64 x = GetHaulingWithMildRebarAllowableStressFactor(slope);

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
   if (IsUHPC(concType))
   {
      // for UHPC, the allowable tensile stress is the modulus of rupture
      return GetHaulingAllowableTensileConcreteStress(segmentKey, WBFL::Stability::HaulingSlope::CrownSlope);
   }
   else
   {
      Float64 x = GetHaulingModulusOfRuptureFactor(concType);

      GET_IFACE(IMaterials, pMaterials);
      Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

      return x * lambda * sqrt(fc);
   }
}

Float64 CSpecAgentImp::GetHaulingModulusOfRuptureFactor(pgsTypes::ConcreteType concType) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   return hauling_criteria.WSDOT.ModulusOfRuptureCoefficient[concType];
}

Float64 CSpecAgentImp::GetSegmentConcreteCompressionStressLimitCoefficient(const pgsPointOfInterest& poi,const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& prestressed_element_criteria = pSpec->GetPrestressedElementCriteria();

   Float64 x = -99999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsConcreteStressLimitApplicable(segmentKey,task));

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liftIntervalIdx          = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType haulIntervalIdx          = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectSegmentIdx          = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tempStrandRemovalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   GET_IFACE_NOCHECK(IDocumentType, pDocType);

   bool bIsStressingInterval = pIntervals->IsStressingInterval(segmentKey, task.intervalIdx);

   // first the special cases
   if ( task.intervalIdx == liftIntervalIdx )
   {
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      ATLASSERT(false); // this assert is to get your attention... there are two compression limits for lifting, you are only getting one of them here. Is that what you want?
      const auto& lifting_criteria = pSpec->GetLiftingCriteria();
      x = lifting_criteria.CompressionStressCoefficient_GlobalStress;
   }
   else if ( storageIntervalIdx <= task.intervalIdx && task.intervalIdx < haulIntervalIdx )
   {
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      // The storage interval represents a change in loading conditions because supports move relative to release.
      // Per LRFD 5.12.3.4.3 for spliced girder segments the concrete stress limits for after losses in
      // LRFD 5.9.2.3.2. However this doesn't may any sense. At release the BeforeLosses case applies and
      // a short time later the AfterLosses case applies, and the stress limit goes from 0.65f'ci to 0.45f'ci
      // For this reason we use the "Before Losses" case for storage
      x = prestressed_element_criteria.CompressionStressCoefficient_BeforeLosses;
   }
   else if ( task.intervalIdx == haulIntervalIdx )
   {
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      ATLASSERT(false); // this assert is to get your attention... there are two compression limits for hauling, you are only getting one of them here. Is that what you want?
      const auto& hauling_criteria = pSpec->GetHaulingCriteria();
      x = hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT ? hauling_criteria.WSDOT.CompressionStressCoefficient_GlobalStress : hauling_criteria.KDOT.CompressionStressLimitCoefficient;
   }
   else if ( task.intervalIdx == tempStrandRemovalIdx && CheckTemporaryStresses() && pDocType->IsPGSuperDocument())
   {
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      ATLASSERT(prestressed_element_criteria.bCheckTemporaryStresses);
      x = prestressed_element_criteria.CompressionStressCoefficient_TemporaryStrandRemoval;
   }
   else
   {
      // now for the normal cases
      if ( bIsStressingInterval )
      {
         // stressing interval
         ATLASSERT( task.limitState == pgsTypes::ServiceI );
         x = prestressed_element_criteria.CompressionStressCoefficient_BeforeLosses;
      }
      else
      {
         // non-stressing interval
         if ( task.intervalIdx < liveLoadIntervalIdx )
         {
            GET_IFACE_NOCHECK(IDocumentType, pDocType);
            if ( task.intervalIdx < railingSystemIntervalIdx && prestressed_element_criteria.bCheckTemporaryStresses && pDocType->IsPGSuperDocument())
            {
               // before the deck is composite (this is for temporary loading conditions)
               // this is basically the wet slab on girder case
               ATLASSERT( task.limitState == pgsTypes::ServiceI );
               x = prestressed_element_criteria.CompressionStressCoefficient_AfterDeckPlacement;
            }
            else
            {
               ATLASSERT(task.limitState == pgsTypes::ServiceI);
               x = prestressed_element_criteria.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses;
            }
         }
         else
         {
            // live load is on the structure so this is the "at service limit states" "after all losses"
            // case
            ATLASSERT( (task.limitState == pgsTypes::ServiceI) || (task.limitState == pgsTypes::ServiceIA) || (task.limitState == pgsTypes::FatigueI));
            if (task.bIncludeLiveLoad)
            {
               x = (task.limitState == pgsTypes::ServiceI ? prestressed_element_criteria.CompressionStressCoefficient_AllLoads_AfterLosses : prestressed_element_criteria.CompressionStressCoefficient_Fatigue);
            }
            else
            {
               ATLASSERT(task.limitState == pgsTypes::ServiceI);
               x = prestressed_element_criteria.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses;
            }
         }
      }
   }

   ATLASSERT(x != -99999);
   return x;
}

Float64 CSpecAgentImp::GetClosureJointConcreteCompressionStressLimitCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& closure_joint_criteria = pSpec->GetClosureJointCriteria();
   Float64 x = -99999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   bool bIsTendonStressingInterval = pIntervals->IsGirderTendonStressingInterval(segmentKey,task.intervalIdx);

   if ( bIsTendonStressingInterval )
   {
      // stressing interval
      x = closure_joint_criteria.CompressionStressCoefficient_BeforeLosses; // 5.12.3.4.2d
   }
   else
   {
      // non-stressing interval
      if ( task.intervalIdx < liveLoadIntervalIdx )
      {
         // Effective Prestress + Permanent Loads
         x = closure_joint_criteria.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses;
      }
      else
      {
         // Effective Prestress + Permanent Loads + Transient Loads
         if (task.bIncludeLiveLoad)
         {
            if (task.limitState == pgsTypes::ServiceIA || task.limitState == pgsTypes::FatigueI)
            {
               x = closure_joint_criteria.CompressionStressCoefficient_Fatigue;
            }
            else
            {
               x = closure_joint_criteria.CompressionStressCoefficient_AllLoads_AfterLosses;
            }
         }
         else
         {
            ATLASSERT(task.limitState == pgsTypes::ServiceI);
            x = closure_joint_criteria.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses;
         }
      }
   }

   return x;
}

Float64 CSpecAgentImp::GetDeckConcreteCompressionStressLimitCoefficient(const pgsPointOfInterest& poi, const StressCheckTask& task) const
{
   ATLASSERT(task.stressType == pgsTypes::Compression);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& prestressed_element_criteria = pSpec->GetPrestressedElementCriteria();

   Float64 x = -99999;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsConcreteStressLimitApplicable(segmentKey,task));

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
      x = prestressed_element_criteria.CompressionStressCoefficient_BeforeLosses;
   }
   else
   {
      // non-stressing interval
      if (liveLoadIntervalIdx <= task.intervalIdx && task.bIncludeLiveLoad == true)
      {
         // live load is on the structure so this is the "at service limit states" "after all losses" case
         ATLASSERT((task.limitState == pgsTypes::ServiceI) || (task.limitState == pgsTypes::ServiceIA) || (task.limitState == pgsTypes::FatigueI));
         x = (task.limitState == pgsTypes::ServiceI ? prestressed_element_criteria.CompressionStressCoefficient_AllLoads_AfterLosses : prestressed_element_criteria.CompressionStressCoefficient_Fatigue);
      }
      else
      {
         // before the deck is composite (this is for temporary loading conditions)
         // this is basically the wet slab on girder case
         if (task.intervalIdx < compositeDeckIntervalIdx)
         {
            ATLASSERT(task.limitState == pgsTypes::ServiceI);
            x = prestressed_element_criteria.CompressionStressCoefficient_AfterDeckPlacement;
         }
         else
         {
            // the deck is now composite so this is the Effective Prestress + Permanent Loads case
            // (basically the case when the railing system has been installed, but no live load)
            ATLASSERT(task.limitState == pgsTypes::ServiceI);
            x = prestressed_element_criteria.CompressionStressCoefficient_PermanentLoadsOnly_AfterLosses;
         }
      }
   }

   return x;
}

TensionStressLimit CSpecAgentImp::GetSegmentConcreteTensionStressLimitParameters(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& prestress_element_criteria = pSpec->GetPrestressedElementCriteria();
   TensionStressLimit tension_stress_limit;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsConcreteStressLimitApplicable(segmentKey,task));


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

   GET_IFACE_NOCHECK(IDocumentType, pDocType);

   // first deal with the special cases
   if ( task.intervalIdx == liftingIntervalIdx )
   {
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      const auto& lifting_criteria = pSpec->GetLiftingCriteria();
      if ( bWithBondedReinforcement )
      {
         tension_stress_limit = lifting_criteria.TensionStressLimitWithReinforcement;
      }
      else
      {
         tension_stress_limit = lifting_criteria.TensionStressLimitWithoutReinforcement;
      }
   }
   else if ( storageIntervalIdx <= task.intervalIdx && task.intervalIdx < haulingIntervalIdx )
   {
      // The storage interval represents a change in loading conditions because supports move relative to release.
      // Per LRFD 5.12.3.4.3 for spliced girder segments the concrete stress limits for after losses in
      // LRFD 5.9.2.3.2. However this doesn't may any sense. At release the BeforeLosses case applies and
      // a short time later the AfterLosses case applies, and the stress limit change. This doesn't make sense.
      // For this reason we use the "Before Losses" case for storage

      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      if ( bWithBondedReinforcement )
      {
         tension_stress_limit = prestress_element_criteria.TensionStressLimit_WithReinforcement_BeforeLosses;
      }
      else
      {
         tension_stress_limit = prestress_element_criteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses;
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
   else if ( task.intervalIdx == tempStrandRemovalIdx && CheckTemporaryStresses() && pDocType->IsPGSuperDocument())
   {
      ATLASSERT( task.limitState == pgsTypes::ServiceI );
      if ( bWithBondedReinforcement )
      {
         tension_stress_limit = prestress_element_criteria.TensionStressLimit_WithReinforcement_TemporaryStrandRemoval;
      }
      else
      {
         tension_stress_limit = prestress_element_criteria.TensionStressLimit_WithoutReinforcement_TemporaryStrandRemoval;
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
            tension_stress_limit = prestress_element_criteria.TensionStressLimit_WithReinforcement_BeforeLosses;
         }
         else
         {
            tension_stress_limit = prestress_element_criteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses;
         }
      }
      else
      {
         // if this is a non-stressing interval, use allowables from Table 5.9.2.3.2b-1 (pre2017: 5.9.4.2.2-1)
         if ( task.intervalIdx < railingSystemIntervalIdx && pDocType->IsPGSuperDocument())
         {
            // this is a PGSuper only stress limit (immediately after deck placement)
            ATLASSERT( task.limitState == pgsTypes::ServiceI );
            tension_stress_limit = prestress_element_criteria.TensionStressLimit_AfterDeckPlacement;
         }
         else 
         {
            // There is an issue with design policy for spliced girder bridges. WSDOT uses zero tension for final by modifying
            // LRFD 5.9.2.3.2 with a zero tension stress coefficient. However, when this is used for intermediate analysis interval
            // stress checks such as at segment erection or application of dead load, zero tension doesn't make sense.
            //
            // This problem was discovered just after the 7.0.0 release. The ideal solution is to add a new project criteria for
            // tensile stresses at erection and other loading conditions. However this would require new data items and an new file format
            // which would require the release to jump to 7.1.0. This is not desirable.
            //
            // An alternative solution is provided here and should be removed for the version 7.1 release later.
            // The solution is to compare the task.interval to the live load interval and, if the task.interval is at or after live load
            // then get the tension stress limit as normal, otherwise, get the tension stress limit based on the following rules:
            // 
            // Use the final tension stress as permitted by AASHTO 5.9.2.3.2, unless it is zero AND the CheckFinalDeadLoadTensionStress option is enabled, 
            // in which case, use the final tension for permanent load tension stress limit.
            if (liveLoadIntervalIdx <= task.intervalIdx)
            {
               if (task.limitState == pgsTypes::ServiceI && CheckFinalDeadLoadTensionStress())
               {
                  tension_stress_limit = prestress_element_criteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses;
               }
               else
               {
#if defined _DEBUG
                  ATLASSERT((pDocType->IsPGSpliceDocument() && task.limitState == pgsTypes::ServiceI) || task.limitState == pgsTypes::ServiceIII);
#endif
                  GET_IFACE(IEnvironment, pEnv);
                  auto exposureCondition = pEnv->GetExposureCondition();
                  tension_stress_limit = (exposureCondition == pgsTypes::ExposureCondition::Normal ? 
                     prestress_element_criteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses : 
                     prestress_element_criteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses);
               }
            }
            else
            {
               ATLASSERT(task.limitState == pgsTypes::ServiceI);
               GET_IFACE(IEnvironment, pEnv);
               auto exposureCondition = pEnv->GetExposureCondition();
               tension_stress_limit = (exposureCondition == pgsTypes::ExposureCondition::Normal ?
                  prestress_element_criteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses :
                  prestress_element_criteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses);
               if (IsZero(tension_stress_limit.Coefficient) && CheckFinalDeadLoadTensionStress() && pDocType->IsPGSpliceDocument())
               {
                  tension_stress_limit = prestress_element_criteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses;
               }
            }
         }
      } // end if bIsStressingInterval
   }// end if,else-if 

   return tension_stress_limit;
}

TensionStressLimit CSpecAgentImp::GetClosureJointConcreteTensionStressLimitParameters(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);
   
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& closure_joint_criteria = pSpec->GetClosureJointCriteria();

   TensionStressLimit tension_stress_limit;

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsConcreteStressLimitApplicable(segmentKey,task));

   GET_IFACE(IIntervals,pIntervals);
   bool bIsTendonStressingInterval = pIntervals->IsGirderTendonStressingInterval(segmentKey,task.intervalIdx);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   // closure joints have allowables for both "in the precompressed tensile zone" and "in areas other than
   // the precompressed tensile zone" (See Table 5.9.2.3.1b-1 and 5.9.2.3.2b-1 (pre2017: 5.9.4.1.2-1 and -5.9.4.2.2-1)) for both during stressing (before losses)
   // and non-stressing intervals (after losses)

   if ( bIsTendonStressingInterval )
   {
      // stressing interval, use Table 5.9.2.3.1b-1
      if ( bInPrecompressedTensileZone )
      {
         if ( bWithBondedReinforcement )
         {
            tension_stress_limit = closure_joint_criteria.TensionStressLimit_InPTZ_WithReinforcement_BeforeLosses;
         }
         else
         {
            tension_stress_limit = closure_joint_criteria.TensionStressLimit_InPTZ_WithoutReinforcement_BeforeLosses;
         }
      }
      else
      {
         if ( bWithBondedReinforcement )
         {
            tension_stress_limit = closure_joint_criteria.TensionStressLimit_OtherAreas_WithReinforcement_BeforeLosses;
         }
         else
         {
            tension_stress_limit = closure_joint_criteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses;
         }
      }
   }
   else
   {
      // non-stressing interval, use Table 5.9.2.3.2b-1 (pre2017: 5.9.4.2.2-1)
      // 
      // For non-live load case, use similar logic to that in GetSegmentConcreteTensionStressLimitParameters
      if (liveLoadIntervalIdx <= task.intervalIdx && task.limitState == pgsTypes::ServiceI && CheckFinalDeadLoadTensionStress())
      {
         tension_stress_limit = closure_joint_criteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses;
      }
      else
      {
         if (bInPrecompressedTensileZone)
         {
            if (bWithBondedReinforcement)
            {
               tension_stress_limit = closure_joint_criteria.TensionStressLimit_InPTZ_WithReinforcement_AfterLosses;
            }
            else
            {
               tension_stress_limit = closure_joint_criteria.TensionStressLimit_InPTZ_WithoutReinforcement_AfterLosses;
            }
         }
         else
         {
            if (bWithBondedReinforcement)
            {
               tension_stress_limit = closure_joint_criteria.TensionStressLimit_OtherAreas_WithReinforcement_AfterLosses;
            }
            else
            {
               tension_stress_limit = closure_joint_criteria.TensionStressLimit_OtherAreas_WithoutReinforcement_AfterLosses;
            }
         }

         // Same-ilar hack as done for segment stresses:
         // Use the final tension stress as permitted by AASHTO 5.9.2.3.2, unless it is zero AND the CheckFinalDeadLoadTensionStress option is enabled, 
         // in which case, use the final tension for permanent load tension stress limit
         if (IsZero(tension_stress_limit.Coefficient) && CheckFinalDeadLoadTensionStress())
         {
            tension_stress_limit = closure_joint_criteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses;
         }
      }
   }

   return tension_stress_limit;
}

TensionStressLimit CSpecAgentImp::GetDeckConcreteTensionStressLimitParameters(const pgsPointOfInterest& poi, const StressCheckTask& task,bool bWithBondedReinforcement) const
{
   ATLASSERT(task.stressType == pgsTypes::Tension);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& prestress_element_criteria = pSpec->GetPrestressedElementCriteria();
   TensionStressLimit tension_stress_limit;
   
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ATLASSERT(IsConcreteStressLimitApplicable(segmentKey,task));

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
         tension_stress_limit = prestress_element_criteria.TensionStressLimit_WithReinforcement_BeforeLosses;
      }
      else
      {
         tension_stress_limit = prestress_element_criteria.TensionStressLimit_OtherAreas_WithoutReinforcement_BeforeLosses;
      }
   }
   else
   {
      // if this is a non-stressing interval, use allowables from Table 5.9.2.3.2b-1 (pre2017: 5.9.4.2.2-1)
      if (task.limitState == pgsTypes::ServiceI && CheckFinalDeadLoadTensionStress())
      {
         tension_stress_limit = prestress_element_criteria.TensionStressLimit_ServiceI_PermanentLoadsOnly_AfterLosses;
      }
      else
      {
#if defined _DEBUG
         GET_IFACE(IDocumentType, pDocType);
         ATLASSERT((pDocType->IsPGSpliceDocument() && task.limitState == pgsTypes::ServiceI) || task.limitState == pgsTypes::ServiceIII);
#endif
         GET_IFACE(IEnvironment, pEnv);
         auto exposureCondition = pEnv->GetExposureCondition();
         tension_stress_limit = (exposureCondition == pgsTypes::ExposureCondition::Normal ?
            prestress_element_criteria.TensionStressLimit_ServiceIII_InPTZ_ModerateCorrosionConditions_AfterLosses :
            prestress_element_criteria.TensionStressLimit_ServiceIII_InPTZ_SevereCorrosionConditions_AfterLosses);
      }
   }

   return tension_stress_limit;
}

bool CSpecAgentImp::IsConcreteStressLimitApplicable(const CGirderKey& girderKey, const StressCheckTask& task) const
{
   GET_IFACE(IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      if (!IsConcreteStressLimitApplicable(segmentKey, task))
         return false;
   }

   return true;
}

bool CSpecAgentImp::IsConcreteStressLimitApplicable(const CSegmentKey& segmentKey, const StressCheckTask& task) const
{
   ATLASSERT(::IsServiceLimitState(task.limitState) || ::IsFatigueLimitState(task.limitState) ); // must be a service limit state

   if ( (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims && task.limitState == pgsTypes::FatigueI) ||
        (WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims <= WBFL::LRFD::BDSManager::GetEdition()&& task.limitState == pgsTypes::ServiceIA)
        )
   {
      // if before LRFD 2009 and Fatigue I 
      // - OR -
      // LRFD 2009 and later and Service IA
      //
      // ... don't evaluate this case
      //
      // The limit state changed from Service IA to Fatigue I in LRFD 2009
      return false;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType noncompositeIntervalIdx  = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();


   if ( task.stressType == pgsTypes::Tension )
   {
      switch(task.limitState)
      {
      case pgsTypes::ServiceI:
      {
         if ((erectSegmentIntervalIdx <= task.intervalIdx && task.intervalIdx <= noncompositeIntervalIdx && !CheckTemporaryStresses())
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
      }

      case pgsTypes::ServiceI_PermitRoutine:
      case pgsTypes::ServiceI_PermitSpecial:
         return (liveLoadIntervalIdx <= task.intervalIdx ? true : false);

      case pgsTypes::ServiceIA:
      case pgsTypes::FatigueI:
      {
         GET_IFACE(IMaterials, pMaterials);
         if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
            return true; // UHPC has a tension stress check for the fatigue limit state
         else
            return false; // these are compression only limit states
      }

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

bool CSpecAgentImp::HasConcreteTensionStressLimitWithRebarOption(IntervalIndexType intervalIdx,bool bInPTZ,bool bSegment,const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterials);
   if ( !bSegment )
   {
      if (IsUHPC(pMaterials->GetClosureJointConcreteType(segmentKey)))
         return false; // no "with rebar" for UHPC

      // At closure joints, there is always a "with rebar" option in both the PTZ and other areas
      return true;
   }

   if (IsUHPC(pMaterials->GetSegmentConcreteType(segmentKey)))
      return false; // no "with rebar" option for UHPC

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

Float64 CSpecAgentImp::GetMaxCoverToUseHigherTensionStressLimit() const
{
   if (WBFL::LRFD::BDSManager::Edition::TenthEdition2024 <= WBFL::LRFD::BDSManager::GetEdition())
   {
      // Cover limit was added in 10th edition
      const SpecLibraryEntry* pSpec = GetSpec();
      return pSpec->GetPrestressedElementCriteria().MaxCoverToUseHigherTensionStressLimit;
   }
   else
   {
      return 0.0;
   }
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
      const auto& prestressed_element_criteria = pSpec->GetPrestressedElementCriteria();
      return prestressed_element_criteria.bCheckTemporaryStresses;
   }
}

bool CSpecAgentImp::CheckFinalDeadLoadTensionStress() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& prestressed_element_criteria = pSpec->GetPrestressedElementCriteria();
   return prestressed_element_criteria.bCheckFinalServiceITension;
}

Float64 CSpecAgentImp::GetSegmentConcreteWebPrincipalTensionStressLimit(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterials);

   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
   {
      Float64 f_fc = pMaterials->GetSegmentConcreteFirstCrackingStrength(segmentKey);
      Float64 k = GetPCIUHPCTensionStressLimitCoefficient();
      return k * f_fc;
   }
   else if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      Float64 ft_cr = pMaterials->GetSegmentConcreteDesignEffectiveCrackingStrength(segmentKey);
      Float64 gamma_u = GetUHPCTensionStressLimitCoefficient(segmentKey);
      return gamma_u * ft_cr;
   }
   else
   {
      Float64 k = GetConcreteWebPrincipalTensionStressLimitCoefficient();

      Float64 lambda, fc;
      GET_IFACE(IMaterials, pMaterials);
      lambda = pMaterials->GetSegmentLambda(segmentKey);
      fc = pMaterials->GetSegmentFc28(segmentKey);

      Float64 f = k * lambda * sqrt(fc);
      return f;
   }
}

void CSpecAgentImp::ReportSegmentConcreteWebPrincipalTensionStressLimit(const CSegmentKey& segmentKey, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   Float64 fLimit = GetSegmentConcreteWebPrincipalTensionStressLimit(segmentKey);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);
   INIT_UV_PROTOTYPE(rptPressureSectionValue, stress_u, pDisplayUnits->GetStressUnit(), true);
   INIT_SCALAR_PROTOTYPE(rptRcScalar, scalar, pDisplayUnits->GetScalarFormat());

   GET_IFACE(IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
   {
      Float64 f_fc = pMaterials->GetSegmentConcreteFirstCrackingStrength(segmentKey);
      ATLASSERT(IsEqual(GetPCIUHPCTensionStressLimitCoefficient(),(2.0/3.0)));

      *pPara << _T("Tension stress limit = (2/3)(") << Sub2(_T("f"), _T("fc")) << _T(") = (2/3)(") << stress.SetValue(f_fc);
      *pPara << _T(") = ") << stress.SetValue(fLimit) << rptNewLine;
   }
   else if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      Float64 ft_cr = pMaterials->GetSegmentConcreteDesignEffectiveCrackingStrength(segmentKey);
      Float64 gamma_u = GetUHPCTensionStressLimitCoefficient(segmentKey);
      *pPara << _T("Tension stress limit = ") << Sub2(symbol(gamma), _T("u")) << Sub2(_T("f"), _T("t,cr")) << _T(" = ") << scalar.SetValue(gamma_u) << _T("(") << stress_u.SetValue(ft_cr) << _T(")");
      *pPara << _T(" = ") << stress.SetValue(fLimit) << rptNewLine;
   }
   else
   {
      INIT_UV_PROTOTYPE(rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);

      Float64 coefficient = GetConcreteWebPrincipalTensionStressLimitCoefficient();
      *pPara << _T("Tension stress limit = ") << tension_coeff.SetValue(coefficient);
      if (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= WBFL::LRFD::BDSManager::GetEdition())
      {
         (*pPara) << symbol(lambda);
      }
      (*pPara) << RPT_SQRT_FC << _T(" = ") << stress.SetValue(fLimit) << rptNewLine;
   }
}

Float64 CSpecAgentImp::GetClosureJointConcreteWebPrincipalTensionStressLimit(const CClosureKey& closureKey) const
{
   Float64 k = GetConcreteWebPrincipalTensionStressLimitCoefficient();

   Float64 lambda, fc;
   GET_IFACE(IMaterials, pMaterials);

   lambda = pMaterials->GetClosureJointLambda(closureKey);
   fc = pMaterials->GetClosureJointFc28(closureKey);

   Float64 f = k*lambda*sqrt(fc);
   return f;
}

void CSpecAgentImp::ReportClosureJointConcreteWebPrincipalTensionStressLimit(const CClosureKey& closureKey, rptParagraph* pPara, std::shared_ptr<IEAFDisplayUnits> pDisplayUnits) const
{
   Float64 fLimit = GetClosureJointConcreteWebPrincipalTensionStressLimit(closureKey);
   INIT_UV_PROTOTYPE(rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), true);

#if defined _DEBUG
   GET_IFACE(IMaterials, pMaterials);
   ATLASSERT(!IsUHPC(pMaterials->GetClosureJointConcreteType(closureKey)));
#endif

   INIT_UV_PROTOTYPE(rptSqrtPressureValue, tension_coeff, pDisplayUnits->GetTensionCoefficientUnit(), false);

   Float64 coefficient = GetConcreteWebPrincipalTensionStressLimitCoefficient();
   *pPara << _T("Tension stress limit = ") << tension_coeff.SetValue(coefficient);
   if (WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims <= WBFL::LRFD::BDSManager::GetEdition())
   {
      (*pPara) << symbol(lambda);
   }
   (*pPara) << RPT_SQRT_FC << _T(" = ") << stress.SetValue(fLimit) << rptNewLine;
}

Float64 CSpecAgentImp::GetConcreteWebPrincipalTensionStressLimit(const pgsPointOfInterest& poi) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   CClosureKey closureKey;
   if (pPoi->IsInClosureJoint(poi, &closureKey))
   {
      return GetClosureJointConcreteWebPrincipalTensionStressLimit(closureKey);
   }
   else
   {
      return GetSegmentConcreteWebPrincipalTensionStressLimit(poi.GetSegmentKey());
   }
}

Float64 CSpecAgentImp::GetConcreteWebPrincipalTensionStressLimitCoefficient() const
{
   const SpecLibraryEntry* pSpecEntry = GetSpec();
   const auto& principal_tension_stress_criteria = pSpecEntry->GetPrincipalTensionStressCriteria();
   return principal_tension_stress_criteria.Coefficient;
}

Float64 CSpecAgentImp::GetPrincipalTensileStressFcThreshold() const
{
   const SpecLibraryEntry* pSpecEntry = GetSpec();
   const auto& principal_tension_stress_criteria = pSpecEntry->GetPrincipalTensionStressCriteria();
   // -1 means all f'cs, so return the minimum f'c of 0.0
   return (principal_tension_stress_criteria.FcThreshold == -1 ? 0.0 : principal_tension_stress_criteria.FcThreshold);
}

Float64 CSpecAgentImp::GetPrincipalTensileStressRequiredConcreteStrength(const pgsPointOfInterest& poi, Float64 stress) const
{
   GET_IFACE(IMaterials, pMaterials);
   GET_IFACE(IPointOfInterest, pPoi);
   pgsTypes::ConcreteType concreteType = pgsTypes::Normal;
   Float64 lambda;
   CClosureKey closureKey;

   if (pPoi->IsInClosureJoint(poi, &closureKey))
   {
      ATLASSERT(!IsUHPC(pMaterials->GetClosureJointConcreteType(closureKey)));
      lambda = pMaterials->GetClosureJointLambda(closureKey);
   }
   else
   {
      const CSegmentKey& segmentKey(poi.GetSegmentKey());
      concreteType = pMaterials->GetSegmentConcreteType(segmentKey);
      lambda = pMaterials->GetSegmentLambda(segmentKey);
   }

   if (concreteType == pgsTypes::PCI_UHPC)
   {
      // PCI does not have a principal tension stress requirement
      return 0.0; // zero means "do nothing" 
   }
   else if (concreteType == pgsTypes::UHPC)
   {
      // GS 1.9.2.3.3, limit is gamma_u*ft,cr
      Float64 gamma_u = GetUHPCTensionStressLimitCoefficient(poi.GetSegmentKey());
      Float64 ftcr_reqd = stress / gamma_u;
      return ftcr_reqd;
   }
   else
   {
      Float64 coefficient = GetConcreteWebPrincipalTensionStressLimitCoefficient();
      Float64 fc_reqd = pow(stress / (coefficient * lambda), 2);
      return fc_reqd;
   }
}

Float64 CSpecAgentImp::GetPCIUHPCTensionStressLimitCoefficient() const
{
   return 2.0 / 3.0;
}

Float64 CSpecAgentImp::GetUHPCTensionStressLimitCoefficient(const CSegmentKey& segmentKey) const
{
   // this is gamma_u that gets multiplied with ft,cr, ft,loc, and et,loc
   GET_IFACE(IMaterials, pMaterial);
   return pMaterial->GetSegmentConcreteFiberOrientationReductionFactor(segmentKey);
}

Float64 CSpecAgentImp::GetUHPCFatigueTensionStressLimitModifier() const
{
   // this is the value that gets multiplied with gamma_u*ft,cr
   return 0.95; // GS 1.5.2.3 (may want to make this a parameter in the project criteria or material definition)
}

Float64 CSpecAgentImp::ComputeRequiredConcreteStrength(const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,Float64 stressDemand,const StressCheckTask& task,bool bWithBondedReinforcement,bool bInPrecompressedTensileZone) const
{
   Float64 fc_reqd = 0;
   if (task.stressType == pgsTypes::Compression)
   {
      // Compression
      Float64 c = GetConcreteCompressionStressLimitCoefficient(poi, stressLocation, task);
      fc_reqd = (IsZero(c) ? NO_AVAILABLE_CONCRETE_STRENGTH : stressDemand / -c);

      if (fc_reqd < 0) // the minimum stress is tensile so compression isn't an issue
      {
         fc_reqd = 0;
      }
   }
   else
   {
      // Tension
      ASSERT(task.stressType == pgsTypes::Tension);

      bool bIsInClosureJoint = false;
      CClosureKey closureKey;

      GET_IFACE(IMaterials, pMaterials);
      Float64 lambda = 1.0;
      if (::IsGirderStressLocation(stressLocation))
      {
         GET_IFACE(IPointOfInterest, pPoi);
         bIsInClosureJoint = pPoi->IsInClosureJoint(poi, &closureKey);
         if (bIsInClosureJoint)
         {
            lambda = pMaterials->GetClosureJointLambda(closureKey);
         }
         else
         {
            lambda = pMaterials->GetSegmentLambda(poi.GetSegmentKey());
         }
      }
      else
      {
         lambda = pMaterials->GetDeckLambda();
      }

      if (0.0 < stressDemand)
      {
         const CSegmentKey& segmentKey(poi.GetSegmentKey());
         if (bIsInClosureJoint || !IsUHPC(pMaterials->GetSegmentConcreteType(segmentKey)))
         {
            // Normal and Lightweight concrete
            // 
            // get allowable tension for the "without rebar" case.
            // since we need top and bottom stresses for the "with rebar" case, we will deal with
            // that outside of this loop (see below)
            auto tension_stress_limit = GetConcreteTensionStressLimitParameters(poi, stressLocation, task, bWithBondedReinforcement, bInPrecompressedTensileZone);

            // if t is zero the allowable will be zero... demand "f" is > 0 so there
            // isn't a concrete strength that will work.... if t is not zero, compute
            // the required concrete strength
            fc_reqd = (IsZero(tension_stress_limit.Coefficient) ? NO_AVAILABLE_CONCRETE_STRENGTH : BinarySign(stressDemand) * pow(stressDemand / (lambda * tension_stress_limit.Coefficient), 2));

            if (tension_stress_limit.bHasMaxValue &&                  // allowable stress is limited -AND-
               (0 < fc_reqd) &&              // there is a concrete strength that might work -AND-
               (pow(tension_stress_limit.MaxValue / (lambda * tension_stress_limit.Coefficient), 2) < fc_reqd))   // that strength will exceed the max limit on allowable
            {
               // then that concrete strength wont really work after all
               // too bad... this isn't going to work
               fc_reqd = NO_AVAILABLE_CONCRETE_STRENGTH;
            }
         }
         else
         {
            // UHPC
            if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
            {
               // the general form of the tension stress limit at release is (2/3)(f_fc)*sqrt(f'ci/f'c)
               // this can be solved for f'ci or f'c as needed

               Float64 fc_28 = pMaterials->GetSegmentFc28(segmentKey);
               const auto& pConcrete = pMaterials->GetSegmentConcrete(poi.GetSegmentKey());
               const auto* pLRFDConcrete = dynamic_cast<const WBFL::LRFD::LRFDConcreteBase*>(pConcrete.get());
               Float64 f_fc = pLRFDConcrete->GetFirstCrackingStrength();

               GET_IFACE(IIntervals, pIntervals);
               if (pIntervals->GetHaulSegmentInterval(segmentKey) <= task.intervalIdx)
               {
                  fc_reqd = 0;
               }
               else
               {
                  Float64 k = GetPCIUHPCTensionStressLimitCoefficient();
                  fc_reqd = pow(1/k * stressDemand / f_fc, 2) * fc_28;
               }
            }
            else
            {
               ASSERT(pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC);
               Float64 gamma_u = GetUHPCTensionStressLimitCoefficient(segmentKey);
               Float64 k = (task.limitState == pgsTypes::FatigueI ? GetUHPCFatigueTensionStressLimitModifier() : 1.0);
               // Tension stress limit is a function of tensile strength not compression strength
               // Compute the required tensile strength
               fc_reqd = stressDemand / (k*gamma_u);
            }
         }
      }
   }

   return fc_reqd;
}

/////////////////////////////////////////////////////////////////////////////
// ITransverseReinforcementSpec
//
WBFL::Materials::Rebar::Size CSpecAgentImp::GetMinConfinementBarSize() const
{
   return WBFL::LRFD::Rebar::GetMinConfinementBarSize();
}

Float64 CSpecAgentImp::GetMaxConfinementBarSpacing() const
{
   return WBFL::LRFD::Rebar::GetMaxConfinementBarSpacing();
}

Float64 CSpecAgentImp::GetMinConfinementAvS() const
{
   return WBFL::LRFD::Rebar::GetMinConfinementAvS();
}

void CSpecAgentImp::GetMaxStirrupSpacing(Float64 dv,Float64* pSmax1, Float64* pSmax2) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& shear_capacity_criteria = pSpec->GetShearCapacityCriteria();

   Float64 k1 = shear_capacity_criteria.StirrupSpacingCoefficient[0];
   Float64 k2 = shear_capacity_criteria.StirrupSpacingCoefficient[1];
   Float64 s1 = shear_capacity_criteria.MaxStirrupSpacing[0];
   Float64 s2 = shear_capacity_criteria.MaxStirrupSpacing[1];

   *pSmax1 = min(k1*dv,s1); // LRFD equation 5.7.2.6-1 (pre2017: 5.8.2.7-1)
   *pSmax2 = min(k2*dv,s2); // LRFD equation 5.7.2.6-2 (pre2017: 5.8.2.7-2) 
}

Float64 CSpecAgentImp::GetMinStirrupSpacing(Float64 maxAggregateSize, Float64 barDiameter) const
{
   PRECONDITION(0.0 < maxAggregateSize);
   PRECONDITION(0.0 < barDiameter);

   Float64 min_spc = Max(1.33*maxAggregateSize, barDiameter);

   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& specification_criteria = pSpec->GetSpecificationCriteria();

   Float64 abs_min_spc;
   if (specification_criteria.Units == WBFL::LRFD::BDSManager::Units::SI)
   {
      abs_min_spc = WBFL::Units::ConvertToSysUnits(25., WBFL::Units::Measure::Millimeter);
   }
   else
   {
      abs_min_spc = WBFL::Units::ConvertToSysUnits(1., WBFL::Units::Measure::Inch);
   }

   // lrfd requirements are for clear distance, we want cl-to-cl spacing
   min_spc += barDiameter;
   abs_min_spc += barDiameter;

   return Max(min_spc, abs_min_spc);
}


Float64 CSpecAgentImp::GetMinTopFlangeThickness() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& specification_criteria = pSpec->GetSpecificationCriteria();

   Float64 dim;
   if (specification_criteria.Units == WBFL::LRFD::BDSManager::Units::SI)
   {
      dim = WBFL::Units::ConvertToSysUnits(50., WBFL::Units::Measure::Millimeter);
   }
   else
   {
      dim = WBFL::Units::ConvertToSysUnits(2., WBFL::Units::Measure::Inch);
   }

   return dim;
}

Float64 CSpecAgentImp::GetMinWebThickness() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& specification_criteria = pSpec->GetSpecificationCriteria();

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
      if (specification_criteria.Units == WBFL::LRFD::BDSManager::Units::SI)
      {
         dim = WBFL::Units::ConvertToSysUnits(165., WBFL::Units::Measure::Millimeter);
      }
      else
      {
         dim = WBFL::Units::ConvertToSysUnits(6.5, WBFL::Units::Measure::Inch);
      }
   }
   else
   {
      if (specification_criteria.Units == WBFL::LRFD::BDSManager::Units::SI)
      {
         dim = WBFL::Units::ConvertToSysUnits(125., WBFL::Units::Measure::Millimeter);
      }
      else
      {
         dim = WBFL::Units::ConvertToSysUnits(5., WBFL::Units::Measure::Inch);
      }
   }

   return dim;
}

Float64 CSpecAgentImp::GetMinBottomFlangeThickness() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& specification_criteria = pSpec->GetSpecificationCriteria();

   Float64 dim;
   if (specification_criteria.Units == WBFL::LRFD::BDSManager::Units::SI)
   {
      dim = WBFL::Units::ConvertToSysUnits(125., WBFL::Units::Measure::Millimeter);
   }
   else
   {
      dim = WBFL::Units::ConvertToSysUnits(5., WBFL::Units::Measure::Inch);
   }

   return dim;
}

//////////////////////////////////////
// ISplittingChecks
Float64 CSpecAgentImp::GetSplittingZoneLength(const CSegmentKey& segmentKey,pgsTypes::MemberEndType endType) const
{
   const pgsSplittingCheckEngineer& engineer = GetSplittingCheckEngineer(segmentKey);
   return engineer.GetSplittingZoneLength(segmentKey,endType);
}

std::shared_ptr<pgsSplittingCheckArtifact> CSpecAgentImp::CheckSplitting(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig) const
{
   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   const auto& end_zone_criteria = pSpecEntry->GetEndZoneCriteria();
   if(end_zone_criteria.bCheckSplitting)
   {
      const pgsSplittingCheckEngineer& engineer = GetSplittingCheckEngineer(segmentKey);
      return engineer.Check(segmentKey, pConfig);
   }
   else
   {
      return std::shared_ptr<pgsSplittingCheckArtifact>();
   }
}

Float64 CSpecAgentImp::GetAsRequired(const pgsSplittingCheckArtifact* pArtifact) const
{
   const pgsSplittingCheckEngineer& engineer = GetSplittingCheckEngineer(pArtifact->GetSegmentKey());
   return engineer.GetAsRequired(pArtifact);
}

const pgsSplittingCheckEngineer& CSpecAgentImp::GetSplittingCheckEngineer(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
   {
      return m_PCIUHPCSplittingCheckEngineer;
   }
   else if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      return m_UHPCSplittingCheckEngineer;
   }
   else
   {
      return m_LRFDSplittingCheckEngineer;
   }
}

void CSpecAgentImp::ReportSplittingChecks(const pgsGirderArtifact* pGirderArtifact, rptChapter* pChapter) const
{
   GET_IFACE(IBridge, pBridge);

   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   std::_tstring strName = pgsSplittingCheckEngineer::GetCheckName();

   rptParagraph* pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
   *pChapter << pPara;
   (*pPara) << strName << _T(" Resistance Check") << rptNewLine;

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   std::_tstring strSegment(1 < nSegments ? _T("Segment") : _T("Girder"));
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      if (1 < nSegments)
      {
         rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }

      const pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const auto pArtifact = pSegmentArtifact->GetStirrupCheckArtifact()->GetSplittingCheckArtifact();

      const pgsSplittingCheckEngineer& engineer = GetSplittingCheckEngineer(segmentKey);
      engineer.ReportSpecCheck(pChapter, pArtifact.get());
   } // next segment
}

void CSpecAgentImp::ReportSplittingCheckDetails(const pgsGirderArtifact* pGirderArtifact, rptChapter* pChapter) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE(IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      if (1 < nSegments)
      {
         rptParagraph* pPara = new rptParagraph(rptStyleManager::GetSubheadingStyle());
         *pChapter << pPara;
         *pPara << _T("Segment ") << LABEL_SEGMENT(segIdx) << rptNewLine;
      }

      const pgsSegmentArtifact* pSegArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      const std::shared_ptr<pgsSplittingCheckArtifact> pArtifact = pSegArtifact->GetStirrupCheckArtifact()->GetSplittingCheckArtifact();
      if (pArtifact)
      {
         const pgsSplittingCheckEngineer& engineer = GetSplittingCheckEngineer(segmentKey);
         engineer.ReportDetails(pChapter, pArtifact.get());
      }
      else
      {
         rptParagraph* pPara = new rptParagraph;
         *pChapter << pPara;
         (*pPara) << _T("Check for ") << pgsSplittingCheckEngineer::GetCheckName() << _T(" resistance is disabled in Project Criteria.") << rptNewLine;
      }
   }
}

std::_tstring CSpecAgentImp::GetSplittingCheckName() const
{
   return pgsSplittingCheckEngineer::GetCheckName();
}

/////////////////////////////////////////////////////////////////////
//  ISegmentLiftingSpecCriteria
bool CSpecAgentImp::IsLiftingAnalysisEnabled() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.bCheck;
}

void  CSpecAgentImp::GetLiftingImpact(Float64* pDownward, Float64* pUpward) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   *pDownward = lifting_criteria.ImpactDown;
   *pUpward = lifting_criteria.ImpactUp;
}

Float64 CSpecAgentImp::GetLiftingCrackingFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.FsCracking;
}

Float64 CSpecAgentImp::GetLiftingFailureFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.FsFailure;
}

void CSpecAgentImp::GetLiftingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   *factor = GetLiftingAllowableTensionFactor();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   *factor = lifting_criteria.TensionStressLimitWithoutReinforcement.Coefficient;
   *pbMax = lifting_criteria.TensionStressLimitWithoutReinforcement.bHasMaxValue;
   *fmax = lifting_criteria.TensionStressLimitWithoutReinforcement.MaxValue;
}

Float64 CSpecAgentImp::GetLiftingAllowableTensileConcreteStress(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
   GET_IFACE(IMaterials, pMaterials);
   Float64 fci = pMaterials->GetSegmentDesignFc(segmentKey, liftSegmentIntervalIdx);
   return GetLiftingAllowableTensileConcreteStressEx(segmentKey, fci, false/*without rebar*/);
}

Float64 CSpecAgentImp::GetLiftingAllowableTensionFactor() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.TensionStressLimitWithoutReinforcement.Coefficient;
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
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return -lifting_criteria.CompressionStressCoefficient_GlobalStress; // negative because compression is < 0
}

Float64 CSpecAgentImp::GetLiftingAllowablePeakCompressionFactor() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return -lifting_criteria.CompressionStressCoefficient_PeakStress; // negative because compression is < 0
}

Float64 CSpecAgentImp::GetLiftingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fci, bool withMinRebar) const
{
   Float64 f = 0;
   GET_IFACE(IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
   {
      Float64 f_fc = pMaterials->GetSegmentConcreteFirstCrackingStrength(segmentKey);
      Float64 k = GetPCIUHPCTensionStressLimitCoefficient();

      // f = (2/3)(f_fc)sqrt(f'ci/f'c)
      // for PCI UHPC recommended minimums f'ci=10ksi, f'c=17.4ksi, f_fc = 1.5ksi
      // f = (2/3)(1.5)*sqrt(10/17.4) = 0.758 ksi
      // this is approximated by (3/4)(2/3)(f_fc) = (1/2)f_fc
      // 
      // this is the allowable analogous to conventional concrete for f'ci. So, the passed in fc is f'ci
      Float64 fc28 = pMaterials->GetSegmentFc28(segmentKey);
      f = k * f_fc * sqrt(fci / fc28);
   }
   else if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      Float64 ft_cri = pMaterials->GetSegmentConcreteInitialEffectiveCrackingStrength(segmentKey);
      Float64 gamma_u = GetUHPCTensionStressLimitCoefficient(segmentKey);
      return gamma_u * ft_cri;
   }
   else
   {
      Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

      if (withMinRebar)
      {
         Float64 x = GetLiftingWithMildRebarAllowableStressFactor();

         f = x * lambda * sqrt(fci);
      }
      else
      {
         Float64 x;
         bool bCheckMax;
         Float64 fmax;

         GetLiftingAllowableTensileConcreteStressParameters(&x, &bCheckMax, &fmax);

         f = x * lambda * sqrt(fci);

         if (bCheckMax)
         {
            f = Min(f, fmax);
         }
      }
   }
   return f;
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
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.PickPointHeight;
}

Float64 CSpecAgentImp::GetLiftingLoopPlacementTolerance() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.LiftingLoopTolerance;
}

Float64 CSpecAgentImp::GetLiftingCableMinInclination() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.MinCableInclination;
}

Float64 CSpecAgentImp::GetLiftingSweepTolerance() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.SweepTolerance;
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
   if (IsUHPC(concType))
   {
      return GetLiftingAllowableTensileConcreteStressEx(segmentKey, fci, true/*with rebar*/);
   }
   else
   {
      Float64 x = GetLiftingModulusOfRuptureFactor(concType);

      GET_IFACE(IMaterials, pMaterials);
      Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

      return x * lambda * sqrt(fci);
   }
}

Float64 CSpecAgentImp::GetLiftingModulusOfRuptureFactor(pgsTypes::ConcreteType concType) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.ModulusOfRuptureCoefficient[concType];
}

Float64 CSpecAgentImp::GetMinimumLiftingPointLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   Float64 min_lift_point = lifting_criteria.MinPickPoint;

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
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.PickPointAccuracy;
}

WBFL::Stability::LiftingCriteria CSpecAgentImp::GetLiftingStabilityCriteria(const CSegmentKey& segmentKey,const HANDLINGCONFIG* pLiftConfig) const
{
   WBFL::Stability::LiftingCriteria criteria;

   criteria.MinFScr = GetLiftingCrackingFs();
   criteria.MinFSf = GetLiftingFailureFs();

   criteria.CompressionCoefficient_GlobalStress = -GetLiftingAllowableGlobalCompressionFactor();
   criteria.CompressionCoefficient_PeakStress = -GetLiftingAllowablePeakCompressionFactor();
   if (pLiftConfig)
   {
      criteria.AllowableCompression_GlobalStress = GetLiftingAllowableGlobalCompressiveConcreteStressEx(segmentKey, pLiftConfig->GdrConfig.fci);
      criteria.AllowableCompression_PeakStress = GetLiftingAllowablePeakCompressiveConcreteStressEx(segmentKey, pLiftConfig->GdrConfig.fci);
   }
   else
   {
      criteria.AllowableCompression_GlobalStress = GetLiftingAllowableGlobalCompressiveConcreteStress(segmentKey);
      criteria.AllowableCompression_PeakStress = GetLiftingAllowablePeakCompressiveConcreteStress(segmentKey);
   }

   GET_IFACE(IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
   {
      auto pTensionStressLimit(std::make_shared<WBFL::Stability::PCIUHPCLiftingTensionStressLimit>(WBFL::Stability::PCIUHPCLiftingTensionStressLimit()));
      pTensionStressLimit->AllowableTension = GetLiftingAllowableTensileConcreteStress(segmentKey);

      pTensionStressLimit->fc28 = pMaterials->GetSegmentFc28(segmentKey);
      pTensionStressLimit->ffc = pMaterials->GetSegmentConcreteFirstCrackingStrength(segmentKey);

      criteria.TensionStressLimit = pTensionStressLimit;
   }
   else if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      auto pTensionStressLimit(std::make_shared<WBFL::Stability::UHPCLiftingTensionStressLimit>(WBFL::Stability::UHPCLiftingTensionStressLimit()));
      pTensionStressLimit->AllowableTension = GetLiftingAllowableTensileConcreteStress(segmentKey);
      pTensionStressLimit->gamma_u = GetUHPCTensionStressLimitCoefficient(segmentKey);
      pTensionStressLimit->ft_cri = pMaterials->GetSegmentConcreteInitialEffectiveCrackingStrength(segmentKey);

      criteria.TensionStressLimit = pTensionStressLimit;
   }
   else
   {
      auto pTensionStressLimit(std::make_shared<WBFL::Stability::CCLiftingTensionStressLimit>(WBFL::Stability::CCLiftingTensionStressLimit()));

      pTensionStressLimit->Lambda = pMaterials->GetSegmentLambda(segmentKey);
      GetLiftingAllowableTensileConcreteStressParameters(&pTensionStressLimit->TensionCoefficient, &pTensionStressLimit->bMaxTension, &pTensionStressLimit->MaxTension);
      pTensionStressLimit->bWithRebarLimit = true;
      pTensionStressLimit->TensionCoefficientWithRebar = GetLiftingWithMildRebarAllowableStressFactor();
      
      if (pLiftConfig)
      {
         pTensionStressLimit->AllowableTension = GetLiftingAllowableTensileConcreteStressEx(segmentKey, pLiftConfig->GdrConfig.fci, false);
         pTensionStressLimit->AllowableTensionWithRebar = GetLiftingAllowableTensileConcreteStressEx(segmentKey, pLiftConfig->GdrConfig.fci, true);
      }
      else
      {
         pTensionStressLimit->AllowableTension = GetLiftingAllowableTensileConcreteStress(segmentKey);
         pTensionStressLimit->AllowableTensionWithRebar = GetLiftingWithMildRebarAllowableStress(segmentKey);
      }

      criteria.TensionStressLimit = pTensionStressLimit;
   }

   return criteria;
}

Float64 CSpecAgentImp::GetLiftingCamberMultiplier() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.CamberMultiplier;
}

WBFL::Stability::WindLoadType CSpecAgentImp::GetLiftingWindType() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.WindLoadType;
}

Float64 CSpecAgentImp::GetLiftingWindLoad() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& lifting_criteria = pSpec->GetLiftingCriteria();
   return lifting_criteria.WindLoad;
}

//////////////////////////////////////////////////////////////////////
// ISegmentHaulingSpecCriteria
bool CSpecAgentImp::IsHaulingAnalysisEnabled() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   return hauling_criteria.bCheck;
}

pgsTypes::HaulingAnalysisMethod CSpecAgentImp::GetHaulingAnalysisMethod() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   return hauling_criteria.AnalysisMethod;
}

void CSpecAgentImp::GetHaulingImpact(Float64* pDownward, Float64* pUpward) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   *pUpward = hauling_criteria.WSDOT.ImpactUp;
   *pDownward = hauling_criteria.WSDOT.ImpactDown;
}

Float64 CSpecAgentImp::GetHaulingCrackingFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   return hauling_criteria.WSDOT.FsCracking;
}

Float64 CSpecAgentImp::GetHaulingRolloverFs() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   return hauling_criteria.WSDOT.FsFailure;
}

void CSpecAgentImp::GetHaulingAllowableTensileConcreteStressParameters(WBFL::Stability::HaulingSlope slope, Float64* factor,bool* pbMax,Float64* fmax) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   *factor = hauling_criteria.WSDOT.TensionStressLimitWithoutReinforcement[+slope].Coefficient;
   *pbMax = hauling_criteria.WSDOT.TensionStressLimitWithoutReinforcement[+slope].bHasMaxValue;
   *fmax = hauling_criteria.WSDOT.TensionStressLimitWithoutReinforcement[+slope].MaxValue;
}

Float64 CSpecAgentImp::GetHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey, WBFL::Stability::HaulingSlope slope) const
{
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   GET_IFACE(IMaterials, pMaterial);
   Float64 fc = pMaterial->GetSegmentDesignFc(segmentKey, haulSegmentIntervalIdx);

   return GetHaulingAllowableTensileConcreteStressEx(segmentKey, slope, fc, false/*don't include rebar*/);
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

Float64 CSpecAgentImp::GetHaulingAllowableTensionFactor(WBFL::Stability::HaulingSlope slope) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   return hauling_criteria.WSDOT.TensionStressLimitWithReinforcement[+slope].Coefficient;
}

Float64 CSpecAgentImp::GetHaulingAllowableGlobalCompressionFactor() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   Float64 factor = (hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT) ? hauling_criteria.WSDOT.CompressionStressCoefficient_GlobalStress : hauling_criteria.KDOT.CompressionStressLimitCoefficient;
   return -factor;
}

Float64 CSpecAgentImp::GetHaulingAllowablePeakCompressionFactor() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   Float64 factor = hauling_criteria.WSDOT.CompressionStressCoefficient_PeakStress;
   return -factor;
}

Float64 CSpecAgentImp::GetHaulingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey, WBFL::Stability::HaulingSlope slope, Float64 fc, bool includeRebar) const
{
   Float64 f = 0;
   GET_IFACE(IMaterials, pMaterials);

   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
   {
      Float64 f_fc = pMaterials->GetSegmentConcreteFirstCrackingStrength(segmentKey);
      Float64 k = GetPCIUHPCTensionStressLimitCoefficient();
      f = k * f_fc;
   }
   else if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      Float64 ft_cr = pMaterials->GetSegmentConcreteDesignEffectiveCrackingStrength(segmentKey);
      Float64 gamma_u = GetUHPCTensionStressLimitCoefficient(segmentKey);
      return gamma_u * ft_cr;
   }
   else
   {
      Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

      if (includeRebar)
      {
         Float64 x = GetHaulingWithMildRebarAllowableStressFactor(slope);

         f = x * lambda * sqrt(fc);
      }
      else
      {
         Float64 x;
         bool bCheckMax;
         Float64 fmax;

         GetHaulingAllowableTensileConcreteStressParameters(slope, &x, &bCheckMax, &fmax);

         f = x * lambda * sqrt(fc);

         if (bCheckMax)
         {
            f = Min(f, fmax);
         }
      }
   }
   return f;
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

WBFL::Stability::HaulingImpact CSpecAgentImp::GetHaulingImpactUsage() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   return hauling_criteria.WSDOT.ImpactUsage;
}

Float64 CSpecAgentImp::GetNormalCrownSlope() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   return hauling_criteria.WSDOT.RoadwayCrownSlope;
}

Float64 CSpecAgentImp::GetMaxSuperelevation() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   return hauling_criteria.WSDOT.RoadwaySuperelevation;
}

Float64 CSpecAgentImp::GetHaulingSweepTolerance() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   return hauling_criteria.WSDOT.SweepTolerance;
}

Float64 CSpecAgentImp::GetHaulingSweepGrowth() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   return hauling_criteria.WSDOT.SweepGrowth;
}

Float64 CSpecAgentImp::GetHaulingSupportPlacementTolerance() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   return hauling_criteria.WSDOT.SupportPlacementTolerance;
}

Float64 CSpecAgentImp::GetHaulingCamberMultiplier() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   return hauling_criteria.WSDOT.CamberMultiplier;
}

Float64 CSpecAgentImp::GetRollStiffness(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   GET_IFACE(ILibrary, pLibrary);
   auto pHaulTruckLibraryEntry = pLibrary->GetHaulTruckEntry(pSegment->HandlingData.HaulTruckName.c_str());
   return pHaulTruckLibraryEntry->GetRollStiffness();
}

Float64 CSpecAgentImp::GetHeightOfGirderBottomAboveRoadway(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   GET_IFACE(ILibrary, pLibrary);
   auto pHaulTruckLibraryEntry = pLibrary->GetHaulTruckEntry(pSegment->HandlingData.HaulTruckName.c_str());
   return pHaulTruckLibraryEntry->GetBottomOfGirderHeight();
}

Float64 CSpecAgentImp::GetHeightOfTruckRollCenterAboveRoadway(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   GET_IFACE(ILibrary, pLibrary);
   auto pHaulTruckLibraryEntry = pLibrary->GetHaulTruckEntry(pSegment->HandlingData.HaulTruckName.c_str());
   return pHaulTruckLibraryEntry->GetRollCenterHeight();
}

Float64 CSpecAgentImp::GetAxleWidth(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   GET_IFACE(ILibrary, pLibrary);
   auto pHaulTruckLibraryEntry = pLibrary->GetHaulTruckEntry(pSegment->HandlingData.HaulTruckName.c_str());
   return pHaulTruckLibraryEntry->GetAxleWidth();
}

Float64 CSpecAgentImp::GetAllowableDistanceBetweenSupports(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   GET_IFACE(ILibrary, pLibrary);
   auto pHaulTruckLibraryEntry = pLibrary->GetHaulTruckEntry(pSegment->HandlingData.HaulTruckName.c_str());
   return pHaulTruckLibraryEntry->GetMaxDistanceBetweenBunkPoints();
}

Float64 CSpecAgentImp::GetAllowableLeadingOverhang(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   GET_IFACE(ILibrary, pLibrary);
   auto pHaulTruckLibraryEntry = pLibrary->GetHaulTruckEntry(pSegment->HandlingData.HaulTruckName.c_str());
   return pHaulTruckLibraryEntry->GetMaximumLeadingOverhang();
}

Float64 CSpecAgentImp::GetMaxGirderWgt(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   ValidateHaulTruck(pSegment);
   GET_IFACE(ILibrary, pLibrary);
   auto pHaulTruckLibraryEntry = pLibrary->GetHaulTruckEntry(pSegment->HandlingData.HaulTruckName.c_str());
   return pHaulTruckLibraryEntry->GetMaxGirderWeight();
}

Float64 CSpecAgentImp::GetMinimumHaulingSupportLocation(const CSegmentKey& segmentKey,pgsTypes::MemberEndType end) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   
   Float64 min_pick_point = hauling_criteria.MinBunkPoint;

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
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   return hauling_criteria.BunkPointAccuracy;
}

WBFL::Stability::WindLoadType CSpecAgentImp::GetHaulingWindType() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   return hauling_criteria.WSDOT.WindLoadType;
}

Float64 CSpecAgentImp::GetHaulingWindLoad() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   return hauling_criteria.WSDOT.WindLoad;
}

WBFL::Stability::CFType CSpecAgentImp::GetCentrifugalForceType() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   return hauling_criteria.WSDOT.CentrifugalForceType;
}

Float64 CSpecAgentImp::GetHaulingSpeed() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   return hauling_criteria.WSDOT.HaulingSpeed;
}

Float64 CSpecAgentImp::GetTurningRadius() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT);
   return hauling_criteria.WSDOT.TurningRadius;
}


WBFL::Stability::HaulingCriteria CSpecAgentImp::GetHaulingStabilityCriteria(const CSegmentKey& segmentKey,const HANDLINGCONFIG* pHaulConfig) const
{
   WBFL::Stability::HaulingCriteria criteria;

   criteria.MinFScr = GetHaulingCrackingFs();
   criteria.MinFSf = GetHaulingRolloverFs();

   if (pHaulConfig && pHaulConfig->pHaulTruckEntry)
   {
      criteria.MaxClearSpan = pHaulConfig->pHaulTruckEntry->GetMaxDistanceBetweenBunkPoints();
      criteria.MaxLeadingOverhang = pHaulConfig->pHaulTruckEntry->GetMaximumLeadingOverhang();
      criteria.MaxGirderWeight = pHaulConfig->pHaulTruckEntry->GetMaxGirderWeight();
   }
   else
   {
      criteria.MaxClearSpan = GetAllowableDistanceBetweenSupports(segmentKey);
      criteria.MaxLeadingOverhang = GetAllowableLeadingOverhang(segmentKey);
      criteria.MaxGirderWeight = GetMaxGirderWgt(segmentKey);
   }

   criteria.CompressionCoefficient_GlobalStress = -GetHaulingAllowableGlobalCompressionFactor();
   criteria.CompressionCoefficient_PeakStress = -GetHaulingAllowablePeakCompressionFactor();

   if (pHaulConfig)
   {
      criteria.AllowableCompression_GlobalStress = GetHaulingAllowableGlobalCompressiveConcreteStressEx(segmentKey, pHaulConfig->GdrConfig.fc);
      criteria.AllowableCompression_PeakStress = GetHaulingAllowablePeakCompressiveConcreteStressEx(segmentKey, pHaulConfig->GdrConfig.fc);
   }
   else
   {
      criteria.AllowableCompression_GlobalStress = GetHaulingAllowableGlobalCompressiveConcreteStress(segmentKey);
      criteria.AllowableCompression_PeakStress = GetHaulingAllowablePeakCompressiveConcreteStress(segmentKey);
   }

   GET_IFACE(IMaterials,pMaterials);
   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
   {
      auto pTensionStressLimit(std::make_shared<WBFL::Stability::PCIUHPCHaulingTensionStressLimit>(WBFL::Stability::PCIUHPCHaulingTensionStressLimit()));

      pTensionStressLimit->AllowableTension[+WBFL::Stability::HaulingSlope::CrownSlope] = GetHaulingAllowableTensileConcreteStress(segmentKey, WBFL::Stability::HaulingSlope::CrownSlope);
      pTensionStressLimit->AllowableTension[+WBFL::Stability::HaulingSlope::Superelevation] = GetHaulingAllowableTensileConcreteStress(segmentKey, WBFL::Stability::HaulingSlope::Superelevation);

      pTensionStressLimit->fc28 = pMaterials->GetSegmentFc28(segmentKey);
      pTensionStressLimit->ffc = pMaterials->GetSegmentConcreteFirstCrackingStrength(segmentKey);

      criteria.TensionStressLimit = pTensionStressLimit;
   }
   else if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      auto pTensionStressLimit(std::make_shared<WBFL::Stability::UHPCHaulingTensionStressLimit>(WBFL::Stability::UHPCHaulingTensionStressLimit()));

      pTensionStressLimit->AllowableTension[+WBFL::Stability::HaulingSlope::CrownSlope] = GetHaulingAllowableTensileConcreteStress(segmentKey, WBFL::Stability::HaulingSlope::CrownSlope);
      pTensionStressLimit->AllowableTension[+WBFL::Stability::HaulingSlope::Superelevation] = GetHaulingAllowableTensileConcreteStress(segmentKey, WBFL::Stability::HaulingSlope::Superelevation);

      pTensionStressLimit->gamma_u = GetUHPCTensionStressLimitCoefficient(segmentKey);
      pTensionStressLimit->ft_cr = pMaterials->GetSegmentConcreteDesignEffectiveCrackingStrength(segmentKey);

      criteria.TensionStressLimit = pTensionStressLimit;
   }
   else
   {
      auto pTensionStressLimit(std::make_shared<WBFL::Stability::CCHaulingTensionStressLimit>(WBFL::Stability::CCHaulingTensionStressLimit()));

      pTensionStressLimit->Lambda = pMaterials->GetSegmentLambda(segmentKey);

      for (int i = 0; i < 2; i++)
      {
         WBFL::Stability::HaulingSlope slope = (WBFL::Stability::HaulingSlope)i;
         GetHaulingAllowableTensileConcreteStressParameters(slope,&pTensionStressLimit->TensionCoefficient[+slope], &pTensionStressLimit->bMaxTension[+slope], &pTensionStressLimit->MaxTension[+slope]);
         pTensionStressLimit->bWithRebarLimit[+slope] = true;
         pTensionStressLimit->TensionCoefficientWithRebar[+slope] = GetHaulingWithMildRebarAllowableStressFactor(slope);
         if (pHaulConfig && !pHaulConfig->bIgnoreGirderConfig)
         {
            pTensionStressLimit->AllowableTension[+slope] = GetHaulingAllowableTensileConcreteStressEx(segmentKey, slope, pHaulConfig->GdrConfig.fc, false);
            pTensionStressLimit->AllowableTensionWithRebar[+slope] = GetHaulingAllowableTensileConcreteStressEx(segmentKey, slope, pHaulConfig->GdrConfig.fc, true);
         }
         else
         {
            pTensionStressLimit->AllowableTension[+slope] = GetHaulingAllowableTensileConcreteStress(segmentKey, slope);
            pTensionStressLimit->AllowableTensionWithRebar[+slope] = GetHaulingWithMildRebarAllowableStress(segmentKey, slope);
         }
      }

      criteria.TensionStressLimit = pTensionStressLimit;
   }

   return criteria;
}

/////////////////////////////////////////////////////////////////////
//  IKdotGirderLiftingSpecCriteria
// Spec criteria for KDOT analyses
Float64 CSpecAgentImp::GetKdotHaulingAllowableTensileConcreteStress(const CSegmentKey& segmentKey) const
{
   return GetHaulingAllowableTensileConcreteStress(segmentKey, WBFL::Stability::HaulingSlope::CrownSlope);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableCompressiveConcreteStress(const CSegmentKey& segmentKey) const
{
   return GetHaulingAllowableGlobalCompressiveConcreteStress(segmentKey);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableTensionFactor() const
{
   return GetHaulingAllowableTensionFactor(WBFL::Stability::HaulingSlope::CrownSlope);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableCompressionFactor() const
{
   return GetHaulingAllowableGlobalCompressionFactor();
}

Float64 CSpecAgentImp::GetKdotHaulingWithMildRebarAllowableStress(const CSegmentKey& segmentKey) const
{
   return GetHaulingWithMildRebarAllowableStress(segmentKey, WBFL::Stability::HaulingSlope::CrownSlope);
}

Float64 CSpecAgentImp::GetKdotHaulingWithMildRebarAllowableStressFactor() const
{
   return GetHaulingWithMildRebarAllowableStressFactor(WBFL::Stability::HaulingSlope::CrownSlope);
}

void CSpecAgentImp::GetKdotHaulingAllowableTensileConcreteStressParameters(Float64* factor,bool* pbMax,Float64* fmax) const
{
   GetHaulingAllowableTensileConcreteStressParameters(WBFL::Stability::HaulingSlope::CrownSlope, factor, pbMax, fmax);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableTensileConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc, bool includeRebar) const
{
   return GetHaulingAllowableTensileConcreteStressEx(segmentKey, WBFL::Stability::HaulingSlope::CrownSlope, fc, includeRebar);
}

Float64 CSpecAgentImp::GetKdotHaulingAllowableCompressiveConcreteStressEx(const CSegmentKey& segmentKey,Float64 fc) const
{
   return GetHaulingAllowableGlobalCompressiveConcreteStressEx(segmentKey,fc);
}

void CSpecAgentImp::GetMinimumHaulingSupportLocation(Float64* pHardDistance, bool* pUseFactoredLength, Float64* pLengthFactor) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();

   *pHardDistance = hauling_criteria.MinBunkPoint;
   *pUseFactoredLength = hauling_criteria.bUseMinBunkPointLimit;
   *pLengthFactor = hauling_criteria.MinBunkPointLimitFactor;
}

Float64 CSpecAgentImp::GetHaulingDesignLocationAccuracy() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   return hauling_criteria.BunkPointAccuracy;
}

void CSpecAgentImp::GetHaulingGFactors(Float64* pOverhangFactor, Float64* pInteriorFactor) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& hauling_criteria = pSpec->GetHaulingCriteria();
   CHECK(hauling_criteria.AnalysisMethod == pgsTypes::HaulingAnalysisMethod::KDOT);

   *pOverhangFactor = hauling_criteria.KDOT.OverhangGFactor;
   *pInteriorFactor = hauling_criteria.KDOT.InteriorGFactor;
}

/////////////////////////////////////////////////////////////////////
// IDebondLimits
bool CSpecAgentImp::CheckMaxDebondedStrands(const CSegmentKey& segmentKey) const
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   return pGirderEntry->CheckMaxTotalFractionDebondedStrands();
}

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

void CSpecAgentImp::GetMaxDebondedStrandsPerSection(const CSegmentKey& segmentKey, StrandIndexType* p10orLess, StrandIndexType* pNS, bool* pbCheckMax, Float64* pMaxFraction) const
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   pGirderEntry->GetMaxDebondedStrandsPerSection(p10orLess,pNS,pbCheckMax,pMaxFraction);
}

void CSpecAgentImp::GetMaxDebondLength(const CSegmentKey& segmentKey, Float64* pLen, pgsTypes::DebondLengthControl* pControl) const
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);

   bool bSpanFraction, buseHard;
   Float64 spanFraction, hardDistance;
   pGirderEntry->GetMaxDebondedLength(&bSpanFraction, &spanFraction, &buseHard, &hardDistance);

   GET_IFACE(IBridge,pBridge);
   Float64 gdrlength = pBridge->GetSegmentLength(segmentKey);

   GET_IFACE(IPointOfInterest,pPOI);
   PoiList vPOI;
   pPOI->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPOI);
   ATLASSERT(vPOI.size() == 1);
   const pgsPointOfInterest& poi( vPOI.front() );

   // always use half girder length - development length
   GET_IFACE(IPretensionForce, pPrestressForce ); 
   Float64 dev_len = pPrestressForce->GetDevelopmentLength(poi,pgsTypes::Straight,true); // set debonding to true to get max length

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

   *pLen = 0.0 < min_len ? min_len : 0.0; // don't return less than zero
}

void CSpecAgentImp::GetMinDistanceBetweenDebondSections(const CSegmentKey& segmentKey, Float64* pndb, bool* pbUseMinDistance, Float64* pMinDistance) const
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   pGirderEntry->GetMinDistanceBetweenDebondSections(pndb, pbUseMinDistance, pMinDistance);
}

Float64 CSpecAgentImp::GetMinDistanceBetweenDebondSections(const CSegmentKey& segmentKey) const
{
   GET_IFACE(IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      // LRFD 5.9.4.3.3, Restriction C, replaced with GS 1.9.4.3.3
      GET_IFACE(IPretensionForce, pPrestress);
      return pPrestress->GetTransferLength(segmentKey, pgsTypes::Straight, pgsTypes::TransferLengthType::Minimum);
   }
   else
   {
      Float64 ndb, minDist;
      bool bMinDist;
      GetMinDistanceBetweenDebondSections(segmentKey, &ndb, &bMinDist, &minDist);

      const auto* pStrand = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::Straight); // only Straight can be debonded
      Float64 db = pStrand->GetNominalDiameter();
      Float64 debond_dist = ndb * db;
      if (bMinDist)
      {
         debond_dist = Max(debond_dist, minDist);
      }
      return debond_dist;
   }
}

bool CSpecAgentImp::CheckDebondingSymmetry(const CSegmentKey& segmentKey) const
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   return pGirderEntry->CheckDebondingSymmetry();
}

bool CSpecAgentImp::CheckAdjacentDebonding(const CSegmentKey& segmentKey) const
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   return pGirderEntry->CheckAdjacentDebonding();
}

bool CSpecAgentImp::CheckDebondingInWebWidthProjections(const CSegmentKey& segmentKey) const
{
   const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
   return pGirderEntry->CheckDebondingInWebWidthProjections();
}

#if defined _DEBUG
#include <initguid.h>
#include <Plugins\BeamFamilyCLSID.h>
#endif
bool CSpecAgentImp::IsExteriorStrandBondingRequiredInRow(const CSegmentKey& segmentKey, pgsTypes::MemberEndType endType, RowIndexType rowIdx) const
{
   if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::NinthEdition2020)
   {
      // exterior strands in each row are required to be bonded
      return true;
   }

   // Beginning with LRFD 9th Edition, 5.9.4.3.3, third bullet point of Item I, only the rows within the full width portion of the bottom flange
   // need to have the exterior strands debonded.
   GET_IFACE(IPointOfInterest, pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT | (endType == pgsTypes::metStart ? POI_0L : POI_10L), &vPoi);
   const pgsPointOfInterest& poi(vPoi.front());

   GET_IFACE(IGirder, pGirder);
   WebIndexType nWebs = pGirder->GetWebCount(segmentKey);
   FlangeIndexType nFlanges = pGirder->GetBottomFlangeCount(segmentKey);

   if(nWebs == 1 && nFlanges == 1)
   {
      // this is a single web flanged girder
#if defined _DEBUG
      const GirderLibraryEntry* pGirderEntry = GetGirderEntry(segmentKey);
      auto factory = pGirderEntry->GetBeamFactory();

      auto clsid = factory->GetFamilyCLSID();
      ATLASSERT(clsid == CLSID_WFBeamFamily || clsid == CLSID_DeckBulbTeeBeamFamily || clsid == CLSID_SplicedIBeamFamily);
      // if this assert fires, is there a new family for single web flanged girders?
#endif
      Float64 Hg = pGirder->GetHeight(poi);
      Float64 bf = pGirder->GetMinBottomFlangeThickness(poi);
      Float64 ybf = -(Hg - bf); // elevation to top of full width section of bottom flange in girder section coordinates (0,0 at top center, negative downwards)

      GET_IFACE(IStrandGeometry, pStrandGeometry);
      auto vStrands = pStrandGeometry->GetStrandsInRow(poi, rowIdx, pgsTypes::Straight);
      ATLASSERT(0 < vStrands.size());
      CComPtr<IPoint2d> pnt;
      pStrandGeometry->GetStrandPosition(poi, vStrands.front(), pgsTypes::Straight, &pnt);
      Float64 y;
      pnt->get_Y(&y);

      return (y < ybf) ? true : false;
      // strand row is below top of full width section of bottom flange so the exterior strands must be bonded
   }
   else
   {
      return true;
   }
}

/////////////////////////////////////////////////////////////////////
// IResistanceFactors
void CSpecAgentImp::GetFlexureResistanceFactors(pgsTypes::ConcreteType type,Float64* phiTensionPS,Float64* phiTensionRC,Float64* phiTensionSpliced,Float64* phiCompression) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& moment_capacity_criteria = pSpec->GetMomentCapacityCriteria();
   moment_capacity_criteria.GetResistanceFactors(type,phiTensionPS,phiTensionRC,phiTensionSpliced,phiCompression);
}

void CSpecAgentImp::GetFlexuralStrainLimits(WBFL::Materials::PsStrand::Grade grade,WBFL::Materials::PsStrand::Type type,Float64* pecl,Float64* petl) const
{
   // The values for Grade 60 are the same as for all types of strand
   GetFlexuralStrainLimits(WBFL::Materials::Rebar::Grade::Grade60,pecl,petl);
}

void CSpecAgentImp::GetFlexuralStrainLimits(WBFL::Materials::Rebar::Grade rebarGrade,Float64* pecl,Float64* petl) const
{
   *pecl = WBFL::LRFD::Rebar::GetCompressionControlledStrainLimit(rebarGrade);
   *petl = WBFL::LRFD::Rebar::GetTensionControlledStrainLimit(rebarGrade);

#if defined _DEBUG
   Float64 ecl, etl;
   switch (rebarGrade )
   {
   case WBFL::Materials::Rebar::Grade40:
      ecl = 0.0014;
      etl = 0.005;
      break;

   case WBFL::Materials::Rebar::Grade::Grade60:
      ecl = 0.002;
      etl = 0.005;
      break;

   case WBFL::Materials::Rebar::Grade75:
      ecl = 0.0028;
      etl = 0.0050;
      break;

   case WBFL::Materials::Rebar::Grade80:
      ecl = 0.0030;
      etl = 0.0056;
      break;

   case WBFL::Materials::Rebar::Grade100:
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
   const auto& shear_capacity_criteria = pSpec->GetShearCapacityCriteria();
   return shear_capacity_criteria.GetResistanceFactor(type, isDebonded, WBFL::LRFD::BDSManager::GetEdition());
}

Float64 CSpecAgentImp::GetShearResistanceFactor(const pgsPointOfInterest& poi, pgsTypes::ConcreteType type) const
{
   // Test to see if poi is in a debonded region
   bool is_debond = false;

   // different phi factor for debonding only applies to 8th edition and later
   if (WBFL::LRFD::BDSManager::Edition::EighthEdition2017 <= WBFL::LRFD::BDSManager::GetEdition())
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

   return GetShearResistanceFactor(is_debond, type);
}

Float64 CSpecAgentImp::GetClosureJointFlexureResistanceFactor(pgsTypes::ConcreteType type) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& moment_capacity_criteria = pSpec->GetMomentCapacityCriteria();
   return moment_capacity_criteria.GetClosureJointResistanceFactor(type);
}

Float64 CSpecAgentImp::GetClosureJointShearResistanceFactor(pgsTypes::ConcreteType type) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& shear_capacity_criteria = pSpec->GetShearCapacityCriteria();
   return shear_capacity_criteria.GetClosureJointResistanceFactor(type);
}

Float64 CSpecAgentImp::GetDuctilityCurvatureRatioLimit() const
{
   return 3.0; // UHPC GS 1.6.2. this is the minimum value... in the future, it could be a user input
}

///////////////////////////////////////////////////
// IInterfaceShearRequirements 
pgsTypes::ShearFlowMethod CSpecAgentImp::GetShearFlowMethod() const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& interface_shear_criteria = pSpec->GetInterfaceShearCriteria();
   return interface_shear_criteria.ShearFlowMethod;
}

Float64 CSpecAgentImp::GetMaxShearConnectorSpacing(const pgsPointOfInterest& poi) const
{
   const SpecLibraryEntry* pSpec = GetSpec();
   const auto& interface_shear_criteria = pSpec->GetInterfaceShearCriteria();
   Float64 sMax = interface_shear_criteria.MaxInterfaceShearConnectorSpacing;
   if ( WBFL::LRFD::BDSManager::Edition::SeventhEdition2014 <= WBFL::LRFD::BDSManager::GetEdition() )
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
   return WBFL::Units::ConvertToSysUnits(ductType == pgsTypes::dtPlastic ? 30.0 : 20.0, WBFL::Units::Measure::Feet);
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
   const auto& duct_size_criteria = pSpecEntry->GetDuctSizeCriteria();
   return duct_size_criteria.DuctDiameterRatio;
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
   const auto& duct_size_criteria = pSpecEntry->GetDuctSizeCriteria();
   return duct_size_criteria.DuctDiameterRatio;
}

Float64 CSpecAgentImp::GetTendonAreaLimit(pgsTypes::StrandInstallationType installationType) const
{
   // LRFD 5.4.6.2
   const SpecLibraryEntry* pSpecEntry = GetSpec();
   const auto& duct_size_criteria = pSpecEntry->GetDuctSizeCriteria();
   return (installationType == pgsTypes::sitPush ? duct_size_criteria.DuctAreaPushRatio : duct_size_criteria.DuctAreaPullRatio);
}

Float64 CSpecAgentImp::GetSegmentDuctDeductionFactor(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx) const
{
   // assumed ducts are grouted and cured in the interval following their installation and stressing
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType groutDuctIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey) + 1;
   return GetDuctDeductFactor(intervalIdx, groutDuctIntervalIdx);
}

Float64 CSpecAgentImp::GetGirderDuctDeductionFactor(const CGirderKey& girderKey, DuctIndexType ductIdx, IntervalIndexType intervalIdx) const
{
   // assumed ducts are grouted and cured in the interval following their installation and stressing
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType groutDuctIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey, ductIdx) + 1;
   return GetDuctDeductFactor(intervalIdx, groutDuctIntervalIdx);
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
   GET_IFACE(ILibrary, pLibrary);
   auto pGirderEntry = pLibrary->GetGirderEntry(pGirder->GetGirderName());
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
   GET_IFACE(ILibrary, pLibrary);
   auto pHaulTruckLibraryEntry = pLibrary->GetHaulTruckEntry(pSegment->HandlingData.HaulTruckName.c_str());
   if (pHaulTruckLibraryEntry == nullptr)
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
         strMsg.Format(_T("The haul truck is not defined for Span %s Girder %s"), LABEL_SPAN(segmentKey.groupIndex), LABEL_GIRDER(segmentKey.girderIndex));
      }

      GET_IFACE(IEAFStatusCenter, pStatusCenter);
      pStatusCenter->Add(std::make_shared<pgsHaulTruckStatusItem>(m_StatusGroupID, m_scidHaulTruckError, strMsg, segmentKey));

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

Float64 CSpecAgentImp::GetDuctDeductFactor(IntervalIndexType intervalIdx, IntervalIndexType groutDuctIntervalIdx) const
{
   Float64 deduct_factor;

   // Get principal web stress parameters
   const SpecLibraryEntry* pSpecEntry = GetSpec();
   const auto& principal_tension_stress_criteria = pSpecEntry->GetPrincipalTensionStressCriteria();

   if (intervalIdx < groutDuctIntervalIdx)
   {
      deduct_factor = principal_tension_stress_criteria.UngroutedMultiplier;
   }
   else
   {
      deduct_factor = principal_tension_stress_criteria.GroutedMultiplier;
   }

   return deduct_factor;
}

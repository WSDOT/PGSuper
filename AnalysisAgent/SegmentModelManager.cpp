///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include "SegmentModelManager.h"
#include <pgsExt\AnalysisResult.h>

#include <EAF\EAFAutoProgress.h>
#include <PgsExt\GirderModelFactory.h>
#include <PgsExt\LoadFactors.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderLabel.h>

#include <IFace\Intervals.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <IFace\GirderHandling.h>

#if defined _DEBUG
#include <IFace\DocumentType.h>
#endif

#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\PrecastSegmentData.h>
#include <PgsExt\ClosureJointData.h>

#include <iterator>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSegmentModelManager::CSegmentModelManager(SHARED_LOGFILE lf,IBroker* pBroker) :
LOGFILE(lf),m_pBroker(pBroker)
{
}

void CSegmentModelManager::Clear()
{
   m_ReleaseModels.clear();
   m_LiftingModels.clear();
   m_StorageModels.clear();
   m_HaulingModels.clear();
   m_LoadCombinationMaps.clear();
}

void CSegmentModelManager::DumpAnalysisModels(GirderIndexType gdrIdx) const
{
   GET_IFACE(IBridge, pBridge);
   std::vector<CGirderKey> girderKeys;
   pBridge->GetGirderline(gdrIdx, &girderKeys);

   for (const auto& girderKey : girderKeys)
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         CSegmentKey segmentKey(girderKey, segIdx);
         BuildReleaseModel(segmentKey);
         BuildLiftingModel(segmentKey);
         BuildStorageModel(segmentKey);
         BuildHaulingModel(segmentKey);
      }
   }


   DumpAnalysisModels(gdrIdx,&m_ReleaseModels,_T("Release"));
   DumpAnalysisModels(gdrIdx,&m_LiftingModels,_T("Lifting"));
   DumpAnalysisModels(gdrIdx,&m_StorageModels,_T("Storage"));
   DumpAnalysisModels(gdrIdx,&m_HaulingModels,_T("Hauling"));
}

void CSegmentModelManager::DumpAnalysisModels(GirderIndexType gdrIdx,const SegmentModels* pModels,LPCTSTR name) const
{
   USES_CONVERSION;
   for(const auto& modelData : *pModels)
   {
      const CSegmentKey& segmentKey(modelData.first);
      const CSegmentModelData& segmentModelData(modelData.second);

      if (segmentKey.girderIndex == gdrIdx)
      {
         CString strFilename;
         strFilename.Format(_T("Group_%d_Girder_%s_Segment_%d_%s_Fem2d.xml"), LABEL_GROUP(segmentKey.groupIndex), LABEL_GIRDER(segmentKey.girderIndex), LABEL_SEGMENT(segmentKey.segmentIndex), name);

         CComPtr<IStructuredSave2> save;
         save.CoCreateInstance(CLSID_StructuredSave2);
         save->Open(T2BSTR(strFilename));

         CComQIPtr<IStructuredStorage2> storage(segmentModelData.Model);
         storage->Save(save);

         save->Close();
      }
   }
}

Float64 CSegmentModelManager::GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> axial = GetAxial(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(axial.size() == 1);

   return axial.front();
}

sysSectionValue CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> shears = GetShear(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(shears.size() == 1);

   return shears.front();
}

Float64 CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> moments = GetMoment(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(moments.size() == 1);

   return moments.front();
}

Float64 CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> deflections = GetDeflection(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(deflections.size() == 1);

   return deflections.front();
}

Float64 CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> rotations = GetRotation(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(rotations.size() == 1);

   return rotations.front();
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop, fBot;
   GetStress(intervalIdx,pfType,vPoi,resultsType,topLocation,botLocation,&fTop,&fBot);

   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);

   *pfTop = fTop.front();
   *pfBot = fBot.front();
}

void CSegmentModelManager::GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,ResultsType resultsType,Float64* pRleft,Float64* pRright) const
{
   if ( pfType != pgsTypes::pftGirder )
   {
      // only girder load creates reaction
      *pRleft = 0;
      *pRright = 0;
      return;
   }

   LoadCaseIDType lcid = GetLoadCaseID(pfType);
   GetReaction(segmentKey,intervalIdx,lcid,resultsType,pRleft,pRright);
}

void CSegmentModelManager::GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,ResultsType resultsType,Float64* pRleft,Float64* pRright) const
{
   *pRleft = 0;
   *pRright = 0;

   CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);

   const auto& loadCombinationMap(GetLoadCombinationMap(segmentKey));
   auto range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.end() )
   {
      if ( range.second != loadCombinationMap.end() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( auto iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         LoadCaseIDType lcid = GetLoadCaseID(pModelData,strLoadingName.c_str());

         Float64 Rl, Rr;
         GetReaction(segmentKey,intervalIdx,lcid,resultsType,&Rl,&Rr);
         (*pRleft)  += Rl;
         (*pRright) += Rr;
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   for(const auto& pfType : pfTypes)
   {
      Float64 Rl, Rr;
      if ( pfType == pgsTypes::pftSecondaryEffects || pfType == pgsTypes::pftPostTensioning )
      {
         Rl = 0;
         Rr = 0;
      }
      else
      {
         LoadCaseIDType lcid = GetLoadCaseID(pfType);
         GetReaction(segmentKey,intervalIdx,lcid,resultsType,&Rl,&Rr);
      }
      (*pRleft)  += Rl;
      (*pRright) += Rr;
   }
}

Float64 CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType) const
{
   if ( pfType != pgsTypes::pftGirder )
   {
      // only girder load creates reaction
      return 0;
   }

   LoadCaseIDType lcid = GetLoadCaseID(pfType);
   return GetReaction(intervalIdx,lcid,pierIdx,girderKey,resultsType);
}

std::vector<Float64> CSegmentModelManager::GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<sysSectionValue> vFx,vFy,vMz;
   std::vector<Float64> vDx,vDy,vRz;

   if ( pfType == pgsTypes::pftPretension )
   {
      GetPrestressSectionResults(intervalIdx, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );
   }
   else
   {
      LoadCaseIDType lcid = GetLoadCaseID(pfType);
      GetSectionResults(intervalIdx, lcid, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );
   }

   std::vector<Float64> axial;
   axial.reserve(vPoi.size());
   std::transform(std::cbegin(vPoi), std::cend(vPoi), std::cbegin(vFx), std::back_inserter(axial), [](const pgsPointOfInterest& poi, const sysSectionValue& fx) {return (IsZero(poi.GetDistFromStart()) ? -fx.Right() : fx.Left());});
   return axial;
}

std::vector<Float64> CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<sysSectionValue> vFx,vFy,vMz;
   std::vector<Float64> vDx,vDy,vRz;

   if ( pfType == pgsTypes::pftPretension )
   {
      GetPrestressSectionResults(intervalIdx, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );
   }
   else
   {
      LoadCaseIDType lcid = GetLoadCaseID(pfType);
      GetSectionResults(intervalIdx, lcid, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );
   }

   std::vector<Float64> moments;
   moments.reserve(vPoi.size());
   std::transform(std::cbegin(vPoi), std::cend(vPoi), std::cbegin(vMz), std::back_inserter(moments), [](const pgsPointOfInterest& poi, const sysSectionValue& mz) {return (IsZero(poi.GetDistFromStart()) ? -mz.Right() : mz.Left());});

   return moments;
}

std::vector<sysSectionValue> CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   // NOTE: shear due to prestressing is a little funny. It is sufficient to use the shear from the
   // equivalent loading.
   std::vector<sysSectionValue> vFx,vFy,vMz;
   std::vector<Float64> vDx,vDy,vRz;

   if ( pfType == pgsTypes::pftPretension )
   {
      GetPrestressSectionResults(intervalIdx, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );
   }
   else
   {
      LoadCaseIDType lcid = GetLoadCaseID(pfType);
      GetSectionResults(intervalIdx, lcid, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );
   }

   return vFy;
}

std::vector<Float64> CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   // NOTE: Deflections for prestress are based on the equivalent loading. This is sufficient.
   // If we need to be more precise, we can use the method of virtual work and integrate the
   // M/EI diagram for the prestress moment based on P*e.

   std::vector<sysSectionValue> vFx,vFy,vMz;
   std::vector<Float64> vDx,vDy,vRz;
   if ( pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning)
   {
      // For elastic analysis we assume that the deflection due to the pretension force does not
      // change with time. The only change in deflection that we account for is due to rigid body
      // displacement of the girder segment. The girder segment is on different support locations
      // at release, during storage, and after erection. However, the deflected shape of the girder
      // due to prestress does not change. We account for the rigid body displacement by deducting
      // the release deflection at the support locations from the deflections.
      GET_IFACE_NOCHECK(ISectionProperties, pSectProps);
      GET_IFACE_NOCHECK(IBridgeDescription, pIBridgeDesc);
      GET_IFACE(IIntervals,pIntervals);
      GET_IFACE(IPointOfInterest,pPoi);

      std::list<PoiList> sPoi;
      pPoi->GroupBySegment(vPoi, &sPoi);
      for ( const auto& vPoiThisSegment : sPoi)
      {
         const pgsPointOfInterest& poi(vPoiThisSegment.front());
         const CSegmentKey& segmentKey(poi.GetSegmentKey());
      
         std::vector<Float64> vDxThisSegment;
         vDxThisSegment.resize(vPoiThisSegment.size(), 0.0);

         std::vector<Float64> vDyThisSegment;
         vDyThisSegment.resize(vPoiThisSegment.size(), 0.0);

         if (pfType == pgsTypes::pftPretension)
         {
            // get the Pretension deflection at release
            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
            IntervalIndexType tsInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
            IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);

            if (resultsType == rtCumulative || intervalIdx == releaseIntervalIdx || intervalIdx == tsRemovalIntervalIdx)
            {
               int firstStrandType = 0;
               int nStrandTypes = 2;
               if (tsInstallationIntervalIdx != INVALID_INDEX && (tsInstallationIntervalIdx <= intervalIdx && intervalIdx < tsRemovalIntervalIdx))
               {
                  nStrandTypes++;
               }

               if (intervalIdx == tsRemovalIntervalIdx && resultsType == rtIncremental)
               {
                  firstStrandType = (int)(pgsTypes::Temporary);
                  nStrandTypes = firstStrandType + 1;
               }

               for (int i = firstStrandType; i < nStrandTypes; i++)
               {
                  pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
                  LoadCaseIDType lcidMx, lcidMy;
                  GetLoadCaseID(strandType, &lcidMx, &lcidMy);

                  std::vector<Float64> vDyThisStrandType;
                  GetSectionResults(releaseIntervalIdx, lcidMx, vPoiThisSegment, resultsType, &vFx, &vFy, &vMz, &vDx, &vDyThisStrandType, &vRz);
                  std::transform(vDyThisStrandType.cbegin(), vDyThisStrandType.cend(), vDyThisSegment.cbegin(), vDyThisSegment.begin(), [](const auto& a, const auto& b) {return a + b; });

                  std::vector<Float64> vDxThisStrandType;
                  GetSectionResults(releaseIntervalIdx, lcidMy, vPoiThisSegment, resultsType, &vFx, &vFy, &vMz, &vDx, &vDxThisStrandType, &vRz);
                  std::transform(vDxThisStrandType.cbegin(), vDxThisStrandType.cend(), vDxThisSegment.cbegin(), vDxThisSegment.begin(), [](const auto& a, const auto& b) {return a + b; });
               }

               if (intervalIdx == tsRemovalIntervalIdx && resultsType == rtIncremental)
               {
                  // removal of strands causes opposite deflection and Ec changes
                  GET_IFACE(IMaterials, pMaterials);
                  Float64 Eci = pMaterials->GetSegmentEc(segmentKey, releaseIntervalIdx);
                  Float64 Ec = pMaterials->GetSegmentEc(segmentKey, intervalIdx);
                  std::transform(vDyThisSegment.cbegin(), vDyThisSegment.cend(), vDyThisSegment.begin(), [Eci, Ec](const auto& D) {return -Eci*D / Ec; });
                  std::transform(vDxThisSegment.cbegin(), vDxThisSegment.cend(), vDxThisSegment.begin(), [Eci, Ec](const auto& D) {return -Eci*D / Ec; });
               }

               // at this time, vDxThisSegment has deflections due to the lateral prestressing moment based on the vertical stiffness of the girder
               // we need to factor out the vertical stiffness and factor in the lateral stiffness.
               // At the same time, we need the vertical deflection due to the lateral prestressing
               PoiList vMidSpanPoi;
               pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vMidSpanPoi);
               const pgsPointOfInterest& msPoi(vMidSpanPoi.front());
               Float64 Ixx = pSectProps->GetIxx(releaseIntervalIdx, msPoi);
               Float64 Iyy = pSectProps->GetIyy(releaseIntervalIdx, msPoi);
               Float64 Ixy = pSectProps->GetIxy(releaseIntervalIdx, msPoi);

               // the model was built with EI = E(Ixx*Iyy - Ixy*Ixy)/Iyy. We want to multiply vDxThisSegment deflections by this value to remove its effect
               // and then divide by E(Ixx*Iyy - Ixy*Ixy)/Ixx to get the actual lateral deflection... if we simplify the expressions, all we need to do
               // is multiply vDxThisSegment by Ixx/Iyy

               // Also, we need to get the vertical deflection due to the lateral prestressing. This is accomplished by getting the lateral deflection
               // due to prestressing moment and multipying it by -Ixy/Ixx... the net results is multiplying by -Ixy/Iyy
#if defined _DEBUG
               Float64 D1 = Ixx / Iyy; // multiply by vDxThisSegment to get actual x-direction deflection
               std::vector<Float64> vdx = vDxThisSegment;
               std::transform(vdx.cbegin(), vdx.cend(), vdx.begin(), [&D1](const auto& dx) {return D1*dx; }); // actual x-direction deflection due to prestress My

               Float64 D2 = -Ixy / Ixx; // then multiply by this to get the vertical deflection associated with the x-direction deflection
               std::vector<Float64> vdy; // vertical deflection associated with lateral prestress My deflection
               std::transform(vdx.cbegin(), vdx.cend(), std::back_inserter(vdy), [&D2](const auto& dx) {return D2*dx; });

               // add vdy with vDyThisSegment to get total y deflection
               std::transform(vdy.cbegin(), vdy.cend(), vDyThisSegment.cbegin(), vdy.begin(), [](const auto& dy1, const auto& dy2) {return dy1 + dy2; });
#endif

               //Float64 D = (Ixx/Iyy)*(-Ixy/Ixx) = -Ixy/Iyy
               Float64 D = -Ixy / Iyy;

               // multiply vDxThisSegment by D to get the indirect deflection in the y-direction
               std::vector<Float64> Dy2;
               Dy2.reserve(vDxThisSegment.size());
               std::transform(vDxThisSegment.cbegin(), vDxThisSegment.cend(), std::back_inserter(Dy2), [&D](const auto& dx) {return D*dx; });

               // multiply vDyThisSegment by D to get the indirect deflection in the x-direction
               std::vector<Float64> Dx2;
               Dx2.reserve(vDyThisSegment.size());
               std::transform(vDyThisSegment.cbegin(), vDyThisSegment.cend(), std::back_inserter(Dx2), [&D](const auto& dx) {return D*dx; });

               // Add the direct and indirect deflections to get the total deflection
               std::transform(vDxThisSegment.cbegin(), vDxThisSegment.cend(), Dx2.cbegin(), vDxThisSegment.begin(), [](const auto& dx1, const auto& dx2) {return dx1 + dx2; });
               std::transform(vDyThisSegment.cbegin(), vDyThisSegment.cend(), Dy2.cbegin(), vDyThisSegment.begin(), [](const auto& dy1, const auto& dy2) {return dy1 + dy2; });

#if defined _DEBUG
               ATLASSERT(std::equal(vdy.cbegin(), vdy.cend(), vDyThisSegment.cbegin(), [](const auto& a, const auto& b) {return IsEqual(a, b); }));
#endif
            }
         }
         else
         {
            LoadCaseIDType lcidPT = GetLoadCaseID(pgsTypes::pftPostTensioning);
            IntervalIndexType stressingIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);
            if ((resultsType == rtIncremental && intervalIdx == stressingIntervalIdx) || (resultsType == rtCumulative && stressingIntervalIdx <= intervalIdx))
            {
               std::vector<Float64> vDyThisStrandType;
               GetSectionResults(stressingIntervalIdx, lcidPT, vPoiThisSegment, resultsType, &vFx, &vFy, &vMz, &vDx, &vDyThisStrandType, &vRz);
               std::transform(vDyThisStrandType.cbegin(), vDyThisStrandType.cend(), vDyThisSegment.cbegin(), vDyThisSegment.begin(), [](const auto& a, const auto& b) {return a + b; });
            }
         }

         // if this is an interval when support locations change, the deflections must be adjusted for the new support datum
         static bool bGettingSupportAdjustment = false; // this method is recursive... we don't want to apply the support datum adjustment when getting the deflections to make the support datum adjustment

         IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
         IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
         IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
         IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

         if( (intervalIdx == liftingIntervalIdx || intervalIdx == storageIntervalIdx || intervalIdx == haulingIntervalIdx || intervalIdx == haulingIntervalIdx || erectionIntervalIdx <= intervalIdx)
            && !bGettingSupportAdjustment)
         {
            const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
            // See if we have a free end (i.e. segment is freely supported by the adjacent segment at endType)
            pgsTypes::DropInType dropInType = pSegment->IsDropIn();

            // Get supports at deflection datum locations at segment erection interval. We will use these support locations as datums throughout
            IntervalIndexType supportIntervalIdx = min(intervalIdx, erectionIntervalIdx);
            std::vector<pgsPointOfInterest> vSupportPois = GetDeflectionDatumLocationsForSegment(segmentKey, supportIntervalIdx, dropInType);
            PoiList vSupports;
            MakePoiList(vSupportPois, &vSupports);

            // Perform rigid body movement on segment so deflection at both supports is zero, or matches supporting segment for dropins.
            if (vSupportPois.size() == 1)
            {
               // Only one support location. Simply translate all values to zero out that location
               bGettingSupportAdjustment = true;
               std::vector<Float64> Dsupports = GetDeflection(intervalIdx, pfType, vSupports, rtCumulative);
               bGettingSupportAdjustment = false;

               Float64 Dsupp = Dsupports.front();

               auto poiIter(vPoiThisSegment.begin());
               auto poiIterEnd(vPoiThisSegment.end());
               auto DyIter(vDyThisSegment.begin());
               for (; poiIter != poiIterEnd; poiIter++, DyIter++)
               {
                  const pgsPointOfInterest& poi(*poiIter);
                  Float64 Dy = *DyIter;

                  if(pPoi->IsOnSegment(poi))
                  {
                     Dy -= Dsupp;
                  }

                  vDy.push_back(Dy);
               }
            }
            else
            {
               // Get deflections at storage datums. This is distance we need to move deflections to get to zero at these locations
               bGettingSupportAdjustment = true;
               std::vector<Float64> Dsupports = GetDeflection(intervalIdx, pfType, vSupports, rtCumulative);
               bGettingSupportAdjustment = false;

               Float64 DyStart = Dsupports.front();
               Float64 DyEnd = Dsupports.back();

               // Check if this is a drop in. If so, we need to adjust deflections at the free end(s) of the drop in to match the supporting segment
               if (erectionIntervalIdx <= intervalIdx && pgsTypes::ditNotDropIn != dropInType)
               {
                  bool isDropInAtStart = pgsTypes::ditYesFreeBothEnds == dropInType || pgsTypes::ditYesFreeStartEnd == dropInType;
                  bool isDropInAtEnd = pgsTypes::ditYesFreeBothEnds == dropInType || pgsTypes::ditYesFreeEndEnd == dropInType;

                  if (isDropInAtStart)
                  {
                     // Start end hangs on adjacent segment. Get deflection at end of supporting segment
                     const CPrecastSegmentData* prevSeg = pSegment->GetPrevSegment();
                     ATLASSERT(prevSeg);
                     CSegmentKey prevSegKey = prevSeg->GetSegmentKey();

                     PoiList endPois;
                     pPoi->GetPointsOfInterest(prevSeg->GetSegmentKey(), POI_END_FACE, &endPois);

                     Float64 DyDropinStart = GetDeflection(intervalIdx, pfType, endPois.front(), rtCumulative);
                     DyStart -= DyDropinStart;
                  }

                  if (isDropInAtEnd)
                  {
                     const CPrecastSegmentData* nextSeg = pSegment->GetNextSegment();
                     ATLASSERT(nextSeg);

                     CSegmentKey nextSegKey = nextSeg->GetSegmentKey();

                     PoiList endPois;
                     pPoi->GetPointsOfInterest(nextSeg->GetSegmentKey(), POI_START_FACE, &endPois);

                     Float64 DyDropinEnd = GetDeflection(intervalIdx, pfType, endPois.front(), rtCumulative);
                     DyEnd -= DyDropinEnd;
                  }
               }

               // Use linear translation to move & rotate deflection into place at end supports
               Float64 XgStart = vSupportPois.front().GetDistFromStart();
               Float64 XgEnd = vSupportPois.back().GetDistFromStart();

               auto poiIter(vPoiThisSegment.begin());
               auto poiIterEnd(vPoiThisSegment.end());
               auto DyIter(vDyThisSegment.begin());
               for (; poiIter != poiIterEnd; poiIter++, DyIter++)
               {
                  const pgsPointOfInterest& poi(*poiIter);
                  Float64 Dy = *DyIter;

                  if (pPoi->IsOnSegment(poi))
                  {
                     Float64 Xg = poi.GetDistFromStart();
                     Float64 d = ::LinInterp(Xg - XgStart, DyStart, DyEnd, XgEnd - XgStart);
                     Dy -= d;
                  }

                  vDy.push_back(Dy);
               }
            }
         }
         else
         {
            // not an interval when supports locations change... the deflections don't need to be adjusted
            vDy.insert(vDy.end(),vDyThisSegment.begin(),vDyThisSegment.end());
         }
      }
   }
   else if ( pfType == pgsTypes::pftSecondaryEffects)
   {
      // post-tensioning hasn't been applied yet so the response is zero
      vDy.resize(vPoi.size(),0.0);
   }
   else
   {
      LoadCaseIDType lcid = GetLoadCaseID(pfType);
      GetSectionResults(intervalIdx, lcid, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );
   }


   return vDy;
}

std::vector<Float64> CSegmentModelManager::GetPretensionXDeflection(IntervalIndexType intervalIdx, const PoiList& vPoi, ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   // NOTE: Deflections for prestress are based on the equivalent loading. This is sufficient.
   // If we need to be more precise, we can use the method of virtual work and integrate the
   // M/EI diagram for the prestress moment based on P*e.

   std::vector<Float64> result;
   std::vector<sysSectionValue> vFx, vFy, vMz;
   std::vector<Float64> vDx, vDy, vRz;
   // For elastic analysis we assume that the deflection due to the pretension force does not
   // change with time. The only change in deflection that we account for is due to rigid body
   // displacement of the girder segment. The girder segment is on different support locations
   // at release, during storage, and after erection. However, the deflected shape of the girder
   // due to prestress does not change. We account for the rigid body displacement by deducting
   // the release deflection at the support locations from the deflections.
   GET_IFACE(ISectionProperties, pSectProps);
   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(IPointOfInterest, pPoi);
   std::list<PoiList> sPoi;
   pPoi->GroupBySegment(vPoi, &sPoi);
   for (const auto& vPoiThisSegment : sPoi)
   {
      const pgsPointOfInterest& poi(vPoiThisSegment.front());
      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      // get the Pretension deflection at release
      std::vector<Float64> vDxThisSegment;
      std::vector<Float64> vDyThisSegment;
      vDxThisSegment.resize(vPoiThisSegment.size(), 0.0);
      vDyThisSegment.resize(vPoiThisSegment.size(), 0.0);

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType tsInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
      IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);

      if (resultsType == rtCumulative || intervalIdx == releaseIntervalIdx || intervalIdx == tsRemovalIntervalIdx)
      {
         int firstStrandType = 0;
         int nStrandTypes = 2;
         if (tsInstallationIntervalIdx != INVALID_INDEX && (tsInstallationIntervalIdx <= intervalIdx && intervalIdx < tsRemovalIntervalIdx))
         {
            nStrandTypes++;
         }
         if (intervalIdx == tsRemovalIntervalIdx && resultsType == rtIncremental)
         {
            firstStrandType = (int)(pgsTypes::Temporary);
            nStrandTypes = firstStrandType + 1;
         }

         for (int i = firstStrandType; i < nStrandTypes; i++)
         {
            pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
            LoadCaseIDType lcidMx, lcidMy;
            GetLoadCaseID(strandType, &lcidMx, &lcidMy);

            std::vector<Float64> vDxThisStrandType;
            GetSectionResults(releaseIntervalIdx, lcidMy, vPoiThisSegment, resultsType, &vFx, &vFy, &vMz, &vDx, &vDxThisStrandType, &vRz);
            std::transform(vDxThisStrandType.cbegin(), vDxThisStrandType.cend(), vDxThisSegment.cbegin(), vDxThisSegment.begin(), [](const auto& a, const auto& b) {return a + b;});

            std::vector<Float64> vDyThisStrandType;
            GetSectionResults(releaseIntervalIdx, lcidMx, vPoiThisSegment, resultsType, &vFx, &vFy, &vMz, &vDx, &vDyThisStrandType, &vRz);
            std::transform(vDyThisStrandType.cbegin(), vDyThisStrandType.cend(), vDyThisSegment.cbegin(), vDyThisSegment.begin(), [](const auto& a, const auto& b) {return a + b;});
         }

         if (intervalIdx == tsRemovalIntervalIdx && resultsType == rtIncremental)
         {
            // removal of strands causes opposite deflection and Ec changes
            GET_IFACE(IMaterials, pMaterials);
            Float64 Eci = pMaterials->GetSegmentEc(segmentKey, releaseIntervalIdx);
            Float64 Ec = pMaterials->GetSegmentEc(segmentKey, intervalIdx);
            std::transform(vDyThisSegment.cbegin(), vDyThisSegment.cend(), vDyThisSegment.begin(), [Eci, Ec](const auto& D) {return -Eci*D / Ec; });
            std::transform(vDxThisSegment.cbegin(), vDxThisSegment.cend(), vDxThisSegment.begin(), [Eci, Ec](const auto& D) {return -Eci*D / Ec; });
         }

         // at this time, vDxThisSegment has deflections due to the lateral prestressing moment based on the vertical stiffness of the girder
         // we need to factor out the vertical stiffness and factor in the lateral stiffness.
         // At the same time, we need the lateral deflection due to the vertical prestressing 
         PoiList vMidSpanPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vMidSpanPoi);
         const pgsPointOfInterest& msPoi(vMidSpanPoi.front());
         Float64 Ixx = pSectProps->GetIxx(releaseIntervalIdx, msPoi);
         Float64 Iyy = pSectProps->GetIyy(releaseIntervalIdx, msPoi);
         Float64 Ixy = pSectProps->GetIxy(releaseIntervalIdx, msPoi);

         // the model was built with EI = E(Ixx*Iyy-Ixy*Ixy)/Iyy. We want to multipy vDxThisSegment deflections by this value to remove its effect
         // and then dividy by E(Ixx*Iyy-Ixy*Ixy)/Ixx to get the actual lateral deflection... if we simplify the expressions, all we need to do
         // is multiply vDxThisSegment by Ixx/Iyy

         // Also, we need to get the lateral deflection due to the vertical prestressing. This is accomplished by getting the
         // vertical deflection due to vertical prestressing and multiplying vDyThisSegment by -Ixy/Iyy

         Float64 D1 = Ixx / Iyy;
         Float64 D2 = -Ixy / Iyy;
         std::transform(vDxThisSegment.cbegin(), vDxThisSegment.cend(), vDyThisSegment.cbegin(), vDxThisSegment.begin(), [&D1,&D2](const auto& dx,const auto& dy) {return D1*dx + D2*dy;});
      }

      // if this is an interval when support locations change, the deflections must be adjusted for the new support datum
      IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      if (intervalIdx == liftingIntervalIdx || intervalIdx == storageIntervalIdx || intervalIdx == haulingIntervalIdx || intervalIdx == haulingIntervalIdx || intervalIdx == erectionIntervalIdx)
      {
         PoiAttributeType poiAttribute;

         if (liftingIntervalIdx <= intervalIdx && intervalIdx < storageIntervalIdx)
         {
            poiAttribute = POI_LIFT_SEGMENT;
         }
         else if (storageIntervalIdx <= intervalIdx && intervalIdx < haulingIntervalIdx)
         {
            poiAttribute = POI_STORAGE_SEGMENT;
         }
         else if (haulingIntervalIdx <= intervalIdx && intervalIdx < erectionIntervalIdx)
         {
            poiAttribute = POI_HAUL_SEGMENT;
         }
         else
         {
            ATLASSERT(erectionIntervalIdx <= intervalIdx);
            poiAttribute = POI_ERECTED_SEGMENT;
         }

         // the support locations have changed since release

         // get the segment support locations
         PoiList vSupports;
         pPoi->GetPointsOfInterest(segmentKey, poiAttribute | POI_0L | POI_10L, &vSupports);
         ATLASSERT(vSupports.size() == 2);

         // get the deflections at the segment support locations
         std::vector<Float64> Dsupports = GetPretensionXDeflection(releaseIntervalIdx, vSupports, rtCumulative);

         // get the values for use in the linear interpoloation
         Float64 X1 = vSupports.front().get().GetDistFromStart();
         Float64 X2 = vSupports.back().get().GetDistFromStart();
         Float64 Y1 = Dsupports.front();
         Float64 Y2 = Dsupports.back();

         // adjust the deflections by deducting the deflection at the support location
         // this makes the deflection at the support location zero, as it should be
         auto poiIter(vPoiThisSegment.begin());
         auto poiIterEnd(vPoiThisSegment.end());
         auto DxIter(vDxThisSegment.begin());
         for (; poiIter != poiIterEnd; poiIter++, DxIter++)
         {
            const pgsPointOfInterest& poi(*poiIter);
            Float64 Dx = *DxIter;

            if (pPoi->IsOnSegment(poi))
            {
               Float64 d = ::LinInterp(poi.GetDistFromStart(), Y1, Y2, X2 - X1);
               Dx -= d;
            }

            result.push_back(Dx);
         }
      }
      else
      {
         // not an interval when supports locations change... the deflections don't need to be adjusted
         result.insert(result.end(), vDxThisSegment.begin(), vDxThisSegment.end());
      }
   }

   return result;
}

std::vector<Float64> CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<sysSectionValue> vFx,vFy,vMz;
   std::vector<Float64> vDx,vDy,vRz;

   if ( pfType == pgsTypes::pftPretension)
   {
      GET_IFACE(IIntervals, pIntervals);
      GET_IFACE(IPointOfInterest, pPoi);
      std::list<PoiList> sPoi;
      pPoi->GroupBySegment(vPoi, &sPoi);
      for (const auto& vPoiThisSegment : sPoi)
      {
         const CSegmentKey& segmentKey(vPoiThisSegment.front().get().GetSegmentKey());
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
         std::vector<Float64> vRzThisSegment;
         if ((resultsType == rtIncremental && intervalIdx == releaseIntervalIdx) || (resultsType == rtCumulative && releaseIntervalIdx <= intervalIdx))
         {
            GetPrestressSectionResults(intervalIdx, vPoiThisSegment, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRzThisSegment);
            vRz.insert(vRz.end(), vRzThisSegment.begin(), vRzThisSegment.end());
         }
         else
         {
            vRz.insert(vRz.end(), vPoiThisSegment.size(), 0.0);
         }
      }
   }
   else if (pfType == pgsTypes::pftPostTensioning)
   {
      LoadCaseIDType lcidPT = GetLoadCaseID(pfType);

      GET_IFACE(IIntervals, pIntervals);
      GET_IFACE(IPointOfInterest, pPoi);
      std::list<PoiList> sPoi;
      pPoi->GroupBySegment(vPoi, &sPoi);
      for (const auto& vPoiThisSegment : sPoi)
      {
         const CSegmentKey& segmentKey(vPoiThisSegment.front().get().GetSegmentKey());
         IntervalIndexType stressingIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);
         std::vector<Float64> vRzThisSegment;
         if ((resultsType == rtIncremental && intervalIdx == stressingIntervalIdx) || (resultsType == rtCumulative && stressingIntervalIdx <= intervalIdx))
         {
            GetSectionResults(stressingIntervalIdx, lcidPT, vPoiThisSegment, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRzThisSegment);
            vRz.insert(vRz.end(), vRzThisSegment.begin(), vRzThisSegment.end());
         }
         else
         {
            vRz.insert(vRz.end(), vPoiThisSegment.size(), 0.0);
         }
      }
   }
   else if ( pfType == pgsTypes::pftSecondaryEffects )
   {
      // no secondary effects for segments
      vRz.resize(vPoi.size(),0.0);
   }
   else
   {
      LoadCaseIDType lcid = GetLoadCaseID(pfType);
      GetSectionResults(intervalIdx, lcid, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );
   }

   return vRz;
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   ATLASSERT(pfType != pgsTypes::pftPretension);
   LoadCaseIDType lcid = GetLoadCaseID(pfType);
   GetSectionStresses(intervalIdx,lcid,resultsType,vPoi,topLocation,botLocation,pfTop,pfBot);
}

Float64 CSegmentModelManager::GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> forces = GetAxial(intervalIdx,comboType,vPoi,resultsType);
   ATLASSERT(forces.size() == 1);
   return forces.front();
}

sysSectionValue CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> shears = GetShear(intervalIdx,comboType,vPoi,resultsType);
   ATLASSERT(shears.size() == 1);
   return shears.front();
}

Float64 CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> moments = GetMoment(intervalIdx,comboType,vPoi,resultsType);
   ATLASSERT(moments.size() == 1);
   return moments.front();
}

Float64 CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> deflections = GetDeflection(intervalIdx,comboType,vPoi,resultsType);
   ATLASSERT(deflections.size() == 1);
   return deflections.front();
}

Float64 CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> rotations = GetRotation(intervalIdx,comboType,vPoi,resultsType);
   ATLASSERT(rotations.size() == 1);
   return rotations.front();
}

Float64 CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,LoadingCombinationType comboType,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType) const
{
   Float64 R = 0;

   CSegmentKey segmentKey = GetSegmentKey(girderKey,pierIdx);

   CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);

   const auto& loadCombinationMap(GetLoadCombinationMap(girderKey));
   auto range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.cend() )
   {
      if ( range.second != loadCombinationMap.cend() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( auto iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         LoadCaseIDType lcid = GetLoadCaseID(pModelData,strLoadingName.c_str());

         Float64 r = GetReaction(intervalIdx,lcid,pierIdx,girderKey,resultsType);
         R += r;
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   for( const auto& pfType : pfTypes)
   {
      LoadCaseIDType lcid = GetLoadCaseID(pfType);
      R += GetReaction(intervalIdx,lcid,pierIdx,girderKey,resultsType);
   }

   return R;
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop, fBot;
   GetStress(intervalIdx,comboType,vPoi,resultsType,topLocation,botLocation,&fTop,&fBot);

   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);

   *pfTop = fTop.front();
   *pfBot = fBot.front();
}

std::vector<Float64> CSegmentModelManager::GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> vF;

   // before release, there aren't any results
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().get().GetSegmentKey());
   if ( intervalIdx < releaseIntervalIdx )
   {
      vF.resize(vPoi.size(),0.0);
      return vF;
   }

   const auto& loadCombinationMap(GetLoadCombinationMap(vPoi.front().get().GetSegmentKey()));
   auto range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.cend() )
   {
      if ( range.second != loadCombinationMap.cend() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( auto iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         std::vector<Float64> vf = GetAxial(intervalIdx,strLoadingName.c_str(),vPoi,resultsType);
         if ( vF.size() == 0 )
         {
            vF = vf;
         }
         else
         {
            std::transform(vf.cbegin(),vf.cend(),vF.cbegin(),vF.begin(),[](const auto& a, const auto& b) {return a + b;});
         }
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   for (const auto& pfType : pfTypes)
   {
      std::vector<Float64> vf = GetAxial(intervalIdx,pfType,vPoi,resultsType);
      if ( vF.size() == 0 )
      {
         vF = vf;
      }
      else
      {
         std::transform(vf.cbegin(),vf.cend(),vF.cbegin(),vF.begin(),[](const auto& a, const auto& b) {return a + b;});
      }
   }

   return vF;
}

std::vector<sysSectionValue> CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<sysSectionValue> vShear;

   // before release, there aren't any results
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().get().GetSegmentKey());
   if ( intervalIdx < releaseIntervalIdx )
   {
      vShear.resize(vPoi.size(),sysSectionValue(0,0));
      return vShear;
   }

   const auto& loadCombinationMap(GetLoadCombinationMap(vPoi.front().get().GetSegmentKey()));
   auto range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.cend() )
   {
      if ( range.second != loadCombinationMap.cend() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( auto iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         std::vector<sysSectionValue> vS = GetShear(intervalIdx,strLoadingName.c_str(),vPoi,resultsType);
         if ( vShear.size() == 0 )
         {
            vShear = vS;
         }
         else
         {
            std::transform(vS.cbegin(),vS.cend(),vShear.cbegin(),vShear.begin(),[](const auto& a, const auto& b) {return a + b;});
         }
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   for( const auto& pfType : pfTypes)
   {
      std::vector<sysSectionValue> vS = GetShear(intervalIdx,pfType,vPoi,resultsType);
      if ( vShear.size() == 0 )
      {
         vShear = vS;
      }
      else
      {
         std::transform(vS.cbegin(),vS.cend(),vShear.cbegin(),vShear.begin(),[](const auto& a, const auto& b) {return a + b;});
      }
   }

   return vShear;
}

std::vector<Float64> CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> vM;

   // before release, there aren't any results
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().get().GetSegmentKey());
   if ( intervalIdx < releaseIntervalIdx )
   {
      vM.resize(vPoi.size(),0.0);
      return vM;
   }

   const auto& loadCombinationMap(GetLoadCombinationMap(vPoi.front().get().GetSegmentKey()));
   auto range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.cend() )
   {
      if ( range.second != loadCombinationMap.cend() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( auto iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         std::vector<Float64> vm = GetMoment(intervalIdx,strLoadingName.c_str(),vPoi,resultsType);
         if ( vM.size() == 0 )
         {
            vM = vm;
         }
         else
         {
            std::transform(vm.cbegin(),vm.cend(),vM.cbegin(),vM.begin(),[](const auto& a, const auto& b) {return a + b;});
         }
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   for (const auto& pfType : pfTypes)
   {
      std::vector<Float64> vm = GetMoment(intervalIdx,pfType,vPoi,resultsType);
      if ( vM.size() == 0 )
      {
         vM = vm;
      }
      else
      {
         std::transform(vm.cbegin(),vm.cend(),vM.cbegin(),vM.begin(),[](const auto& a, const auto& b) {return a + b;});
      }
   }

   return vM;
}

std::vector<Float64> CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> vD;

   // before release, there aren't any results
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().get().GetSegmentKey());
   if ( intervalIdx < releaseIntervalIdx )
   {
      vD.resize(vPoi.size(),0.0);
      return vD;
   }

   const auto& loadCombinationMap(GetLoadCombinationMap(vPoi.front().get().GetSegmentKey()));
   auto range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.cend() )
   {
      if ( range.second != loadCombinationMap.cend() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for (auto iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         std::vector<Float64> vd = GetDeflection(intervalIdx,strLoadingName.c_str(),vPoi,resultsType);
         if ( vD.size() == 0 )
         {
            vD = vd;
         }
         else
         {
            std::transform(vd.cbegin(),vd.cend(),vD.cbegin(),vD.begin(),[](const auto& a, const auto& b) {return a + b;});
         }
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   for( const auto& pfType : pfTypes)
   {
      std::vector<Float64> vd = GetDeflection(intervalIdx,pfType,vPoi,resultsType);
      if ( vD.size() == 0 )
      {
         vD = vd;
      }
      else
      {
         std::transform(vd.cbegin(),vd.cend(),vD.cbegin(),vD.begin(),[](const auto& a, const auto& b) {return a + b;});
      }
   }

   return vD;
}

std::vector<Float64> CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> vR;

   // before release, there aren't any results
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().get().GetSegmentKey());
   if ( intervalIdx < releaseIntervalIdx )
   {
      vR.resize(vPoi.size(),0.0);
      return vR;
   }

   const auto& loadCombinationMap(GetLoadCombinationMap(vPoi.front().get().GetSegmentKey()));
   auto range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.cend() )
   {
      if ( range.second != loadCombinationMap.cend() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( auto iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         std::vector<Float64> vr = GetRotation(intervalIdx,strLoadingName.c_str(),vPoi,resultsType);
         if ( vR.size() == 0 )
         {
            vR = vr;
         }
         else
         {
            std::transform(vr.cbegin(),vr.cend(),vR.cbegin(),vR.begin(),[](const auto& a, const auto& b) {return a + b;});
         }
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   for( const auto& pfType : pfTypes)
   {
      std::vector<Float64> vr = GetRotation(intervalIdx,pfType,vPoi,resultsType);
      if ( vR.size() == 0 )
      {
         vR = vr;
      }
      else
      {
         std::transform(vr.cbegin(),vr.cend(),vR.cbegin(),vR.begin(),[](const auto& a, const auto& b) {return a + b;});
      }
   }

   return vR;
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pfTop->resize(vPoi.size(),0.0);
   pfBot->resize(vPoi.size(),0.0);

   // before release, there aren't any results
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().get().GetSegmentKey());
   if ( intervalIdx < releaseIntervalIdx )
   {
      return;
   }

   const auto& loadCombinationMap(GetLoadCombinationMap(vPoi.front().get().GetSegmentKey()));
   auto range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.cend() )
   {
      if ( range.second != loadCombinationMap.cend() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( auto iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         std::vector<Float64> ft,fb;
         GetStress(intervalIdx,strLoadingName.c_str(),vPoi,resultsType,topLocation,botLocation,&ft,&fb);
         std::transform(ft.cbegin(),ft.cend(),pfTop->cbegin(),pfTop->begin(),[](const auto& a, const auto& b) {return a + b;});
         std::transform(fb.cbegin(),fb.cend(),pfBot->cbegin(),pfBot->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   for( const auto& pfType : pfTypes)
   {
      std::vector<Float64> ft,fb;
      GetStress(intervalIdx,pfType,vPoi,resultsType,topLocation,botLocation,&ft,&fb);
      std::transform(ft.cbegin(),ft.cend(),pfTop->cbegin(),pfTop->begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(fb.cbegin(),fb.cend(),pfBot->cbegin(),pfBot->begin(),[](const auto& a, const auto& b) {return a + b;});
   }
}

////////////////////////////
void CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,sysSectionValue* pMin,sysSectionValue* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> vMin, vMax;
   GetShear(intervalIdx,ls,vPoi,&vMin,&vMax);

   ATLASSERT(vMin.size() == 1);
   ATLASSERT(vMax.size() == 1);

   *pMin = vMin.front();
   *pMax = vMax.front();
}

void CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> vMin, vMax;
   GetMoment(intervalIdx,ls,vPoi,&vMin,&vMax);

   ATLASSERT(vMin.size() == 1);
   ATLASSERT(vMax.size() == 1);

   *pMin = vMin.front();
   *pMax = vMax.front();
}

void CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,bool bIncludePrestress,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> vMin, vMax;
   GetDeflection(intervalIdx,ls,vPoi,bIncludePrestress,&vMin,&vMax);

   ATLASSERT(vMin.size() == 1);
   ATLASSERT(vMax.size() == 1);

   *pMin = vMin.front();
   *pMax = vMax.front();
}

void CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,PierIndexType pierIdx,const CGirderKey& girderKey,bool bIncludeImpact,Float64* pMin,Float64* pMax) const
{
   Float64 R = GetReaction(intervalIdx,lcDC,pierIdx,girderKey,rtCumulative);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin, gDCMax; 
   pLoadFactors->GetDC(ls, &gDCMin, &gDCMax);

   *pMin = gDCMin*R;
   *pMax = gDCMax*R;
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> vMin, vMax;
   GetStress(intervalIdx,ls,vPoi,stressLocation,bIncludePrestress,&vMin,&vMax);

   ATLASSERT(vMin.size() == 1);
   ATLASSERT(vMax.size() == 1);

   *pMin = vMin.front();
   *pMax = vMax.front();
}

void CSegmentModelManager::GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const PoiList& vPoi,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> forces = GetAxial(intervalIdx,lcDC,vPoi,rtCumulative);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin, gDCMax;
   pLoadFactors->GetDC(ls, &gDCMin, &gDCMax);

   std::transform(forces.cbegin(), forces.cend(), std::back_inserter(*pMin), [&gDCMin](const auto& value) {return gDCMin*value;});
   std::transform(forces.cbegin(), forces.cend(), std::back_inserter(*pMax), [&gDCMax](const auto& value) {return gDCMax*value;});
}

void CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const PoiList& vPoi,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<sysSectionValue> shears = GetShear(intervalIdx,lcDC,vPoi,rtCumulative);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin, gDCMax;
   pLoadFactors->GetDC(ls, &gDCMin, &gDCMax);

   std::transform(shears.cbegin(),shears.cend(),std::back_inserter(*pMin), [&gDCMin](const auto& value) {return gDCMin*value;});
   std::transform(shears.cbegin(),shears.cend(),std::back_inserter(*pMax), [&gDCMax](const auto& value) {return gDCMax*value;});
}

void CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const PoiList& vPoi,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> moments = GetMoment(intervalIdx,lcDC,vPoi,rtCumulative);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin, gDCMax;
   pLoadFactors->GetDC(ls, &gDCMin, &gDCMax);

   std::transform(moments.cbegin(),moments.cend(),std::back_inserter(*pMin),[&gDCMin](const auto& value) {return gDCMin*value;});
   std::transform(moments.cbegin(),moments.cend(),std::back_inserter(*pMax),[&gDCMax](const auto& value) {return gDCMax*value;});
}

void CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const PoiList& vPoi,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> deflection = GetDeflection(intervalIdx,lcDC,vPoi,rtCumulative);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin, gDCMax;
   pLoadFactors->GetDC(ls, &gDCMin, &gDCMax);

   std::transform(deflection.cbegin(),deflection.cend(),std::back_inserter(*pMin),[&gDCMin](const auto& value) {return gDCMin*value;});
   std::transform(deflection.cbegin(),deflection.cend(),std::back_inserter(*pMax),[&gDCMax](const auto& value) {return gDCMax*value;});

   if ( bIncludePrestress )
   {
      std::vector<Float64> ps = GetDeflection(intervalIdx,pgsTypes::pftPretension,vPoi,rtCumulative);
      std::transform(ps.cbegin(),ps.cend(),pMin->cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(ps.cbegin(),ps.cend(),pMax->cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
   }
}

void CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> rotation = GetRotation(intervalIdx,lcDC,vPoi,rtCumulative);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin, gDCMax;
   pLoadFactors->GetDC(limitState, &gDCMin, &gDCMax);

   std::transform(rotation.cbegin(),rotation.cend(),std::back_inserter(*pMin),[&gDCMin](const auto& value) {return gDCMin*value;});
   std::transform(rotation.cbegin(),rotation.cend(),std::back_inserter(*pMax),[&gDCMax](const auto& value) {return gDCMax*value;});

   if ( bIncludePrestress )
   {
      std::vector<Float64> ps = GetRotation(intervalIdx,pgsTypes::pftPretension,vPoi,rtCumulative);
      std::transform(ps.cbegin(),ps.cend(),pMin->cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(ps.cbegin(),ps.cend(),pMax->cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
   }
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pgsTypes::StressLocation topLocation = IsGirderStressLocation(stressLocation) ? pgsTypes::TopGirder    : pgsTypes::TopDeck;
   pgsTypes::StressLocation botLocation = IsGirderStressLocation(stressLocation) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck;

   // our limit state consists of only DC (self weight of segment) and Prestressing

   // Get the DC stresses
   std::vector<Float64> fTop, fBot;
   GetStress(intervalIdx,lcDC,vPoi,rtCumulative,topLocation,botLocation,&fTop,&fBot);

   // apply the DC load factor and put the results in pMin and pMax
   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin, gDCMax;
   pLoadFactors->GetDC(limitState, &gDCMin, &gDCMax);

   if ( IsTopStressLocation(stressLocation) )
   {
      std::transform(fTop.cbegin(),fTop.cend(),std::back_inserter(*pMin),[&gDCMin](const auto& value) {return gDCMin*value;});
      std::transform(fTop.cbegin(),fTop.cend(),std::back_inserter(*pMax),[&gDCMax](const auto& value) {return gDCMax*value;});
   }
   else
   {
      std::transform(fBot.cbegin(),fBot.cend(),std::back_inserter(*pMin),[&gDCMin](const auto& value) {return gDCMin*value;});
      std::transform(fBot.cbegin(),fBot.cend(),std::back_inserter(*pMax),[&gDCMax](const auto& value) {return gDCMax*value;});
   }

   if ( bIncludePrestress )
   {
      // get and add the pretension stresses to pMin and pMax
      GET_IFACE(IPretensionStresses, pPretensionStresses);
      std::vector<Float64> fPS;
      pPretensionStresses->GetStress(intervalIdx, vPoi, stressLocation, false/*no live load*/, limitState,INVALID_INDEX,&fPS);
      std::transform(pMin->cbegin(),pMin->cend(), fPS.cbegin(),pMin->begin(),[](const auto& fLimitState, const auto& fPS) {return fLimitState + fPS;});
      std::transform(pMax->cbegin(),pMax->cend(), fPS.cbegin(),pMax->begin(),[](const auto& fLimitState, const auto& fPS) {return fLimitState + fPS;});
   }
}

///////////////////////////
// IExternalLoading
bool CSegmentModelManager::CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName)
{
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   GET_IFACE(IBridge, pBridge);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderLineIdx, &vGirderKeys);
   for(const auto& girderKey : vGirderKeys)
   {
      const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(girderKey);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(girderKey,segIdx);

         for ( int i = 0; i < 4; i++ )
         {
            CSegmentModelData* pModelData;
            if ( i == 0 )
            {
               pModelData = GetReleaseModel(segmentKey);
            }
            else if ( i == 1 )
            {
               pModelData = GetLiftingModel(segmentKey);
            }
            else if ( i == 2 )
            {
               pModelData = GetStorageModel(segmentKey);
            }
            else if ( i == 3 )
            {
               pModelData = GetHaulingModel(segmentKey);
            }

            CComPtr<IFem2dLoadingCollection> loadings;
            pModelData->Model->get_Loadings(&loadings);

            LoadCaseIDType lcid = GetFirstExternalLoadCaseID() + pModelData->ExternalLoadMap.size();

            CComPtr<IFem2dLoading> loading;
            if ( FAILED(loadings->Create(lcid,&loading)) )
            {
               ATLASSERT(false);
               return false;
            }

            pModelData->ExternalLoadMap.insert(std::make_pair(std::_tstring(strLoadingName),lcid));
         }
      } // next segment
   } // next group

   return true;
}

bool CSegmentModelManager::AddLoadingToLoadCombination(GirderIndexType girderLineIdx, LPCTSTR strLoadingName, LoadingCombinationType comboType)
{
   GET_IFACE(IBridge, pBridge);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderLineIdx, &vGirderKeys);
   for (const auto& girderKey : vGirderKeys)
   {
      auto& loadCombinationMap(GetLoadCombinationMap(girderKey));
      loadCombinationMap.insert(std::make_pair(comboType,strLoadingName));
   }
   return true;
}

bool CSegmentModelManager::CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   LoadCaseIDType lcid = GetLoadCaseID(pfType);
   return CreateConcentratedLoad(intervalIdx,lcid,poi,Fx,Fy,Mz);
}

bool CSegmentModelManager::CreateConcentratedLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   CSegmentModelData* pModelData = GetSegmentModel(poi.GetSegmentKey(), intervalIdx);
   LoadCaseIDType lcid = GetLoadCaseID(pModelData,strLoadingName);
   return CreateConcentratedLoad(intervalIdx,lcid,poi,Fx,Fy,Mz);
}

bool CSegmentModelManager::CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
   LoadCaseIDType lcid = GetLoadCaseID(pfType);
   return CreateUniformLoad(intervalIdx,lcid,poi1,poi2,wx,wy);
}

bool CSegmentModelManager::CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
   ATLASSERT(poi1.GetSegmentKey().IsEqual(poi2.GetSegmentKey()));
   CSegmentModelData* pModelData = GetSegmentModel(poi1.GetSegmentKey(), intervalIdx);
   LoadCaseIDType lcid = GetLoadCaseID(pModelData,strLoadingName);
   return CreateUniformLoad(intervalIdx,lcid,poi1,poi2,wx,wy);
}

bool CSegmentModelManager::CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
   LoadCaseIDType lcid = GetLoadCaseID(pfType);
   return CreateInitialStrainLoad(intervalIdx,lcid,poi1,poi2,e,r);
}

bool CSegmentModelManager::CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
   ATLASSERT(poi1.GetSegmentKey().IsEqual(poi2.GetSegmentKey()));
   CSegmentModelData* pModelData = GetSegmentModel(poi1.GetSegmentKey(), intervalIdx);
   LoadCaseIDType lcid = GetLoadCaseID(pModelData,strLoadingName);
   return CreateInitialStrainLoad(intervalIdx,lcid,poi1,poi2,e,r);
}

std::vector<Float64> CSegmentModelManager::GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().get().GetSegmentKey(),intervalIdx);
   if (pModelData)
   {
      LoadCaseIDType lcid = GetLoadCaseID(pModelData, strLoadingName);

      std::vector<sysSectionValue> vFx, vFy, vMz;
      std::vector<Float64> vDx, vDy, vRz;
      GetSectionResults(intervalIdx, lcid, vPoi, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz);

      std::transform(std::cbegin(vPoi), std::cend(vPoi), std::cbegin(vFx), std::back_inserter(results), [](const pgsPointOfInterest& poi, const sysSectionValue& Fx) {return IsZero(poi.GetDistFromStart()) ? -Fx.Right() : Fx.Left(); });
   }
   else
   {
      results.resize(vPoi.size(), 0.0);
   }
   return results;
}

std::vector<sysSectionValue> CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   // Assuming that the loading ID sequence for all models is the same
   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().get().GetSegmentKey(),intervalIdx);
   if (pModelData)
   {
      LoadCaseIDType lcid = GetLoadCaseID(pModelData, strLoadingName);

      std::vector<sysSectionValue> vFx, vFy, vMz;
      std::vector<Float64> vDx, vDy, vRz;
      GetSectionResults(intervalIdx, lcid, vPoi, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz);

      return vFy;
   }
   else
   {
      std::vector<sysSectionValue> results;
      results.resize(vPoi.size(), sysSectionValue(0.0, 0.0));
      return results;
   }
}

std::vector<Float64> CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().get().GetSegmentKey(),intervalIdx);
   if (pModelData)
   {
      LoadCaseIDType lcid = GetLoadCaseID(pModelData, strLoadingName);

      std::vector<sysSectionValue> vFx, vFy, vMz;
      std::vector<Float64> vDx, vDy, vRz;
      GetSectionResults(intervalIdx, lcid, vPoi, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz);

      std::transform(std::cbegin(vPoi), std::cend(vPoi), std::cbegin(vMz), std::back_inserter(results), [](const pgsPointOfInterest& poi, const sysSectionValue& Mz) {return IsZero(poi.GetDistFromStart()) ? -Mz.Right() : Mz.Left(); });
   }
   else
   {
      results.resize(vPoi.size(), 0.0);
   }
   return results;
}

std::vector<Float64> CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().get().GetSegmentKey(),intervalIdx);
   if (pModelData)
   {
      LoadCaseIDType lcid = GetLoadCaseID(pModelData, strLoadingName);

      std::vector<sysSectionValue> vFx, vFy, vMz;
      std::vector<Float64> vDx, vDy, vRz;
      GetSectionResults(intervalIdx, lcid, vPoi, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz);

      return vDy;
   }
   else
   {
      std::vector<Float64> results;
      results.resize(vPoi.size(), 0.0);
      return results;
   }
}

std::vector<Float64> CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,ResultsType resultsType) const
{
   ATLASSERT(VerifyPoi(vPoi));

   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().get().GetSegmentKey(),intervalIdx);
   if (pModelData)
   {
      LoadCaseIDType lcid = GetLoadCaseID(pModelData, strLoadingName);

      std::vector<sysSectionValue> vFx, vFy, vMz;
      std::vector<Float64> vDx, vDy, vRz;
      GetSectionResults(intervalIdx, lcid, vPoi, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz);

      return vRz;
   }
   else
   {
      std::vector<Float64> results;
      results.resize(vPoi.size(), 0.0);
      return results;
   }
}

void CSegmentModelManager::GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const
{
   CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);
   if (pModelData)
   {
      CComQIPtr<IFem2dModelResults> results(pModelData->Model);
      JointIDType leftJntID, rightJntID;
      leftJntID = 0;

      CComPtr<IFem2dJointCollection> joints;
      pModelData->Model->get_Joints(&joints);
      CollectionIndexType nJoints;
      joints->get_Count(&nJoints);
      rightJntID = nJoints - 1;

      LoadCaseIDType lcid = GetLoadCaseID(pModelData, strLoadingName);

      Float64 fx, mz;
      CAnalysisResult ar(_T(__FILE__), __LINE__);
      ar = results->ComputeReactions(lcid, leftJntID, &fx, pRleft, &mz);
      ar = results->ComputeReactions(lcid, leftJntID, &fx, pRright, &mz);
   }
   else
   {
      *pRleft = 0.0;
      *pRright = 0.0;
   }
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   ATLASSERT(VerifyPoi(vPoi));

   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().get().GetSegmentKey(),intervalIdx);
   if (pModelData)
   {
      LoadCaseIDType lcid = GetLoadCaseID(pModelData, strLoadingName);
      GetSectionStresses(intervalIdx, lcid, resultsType, vPoi, topLocation, botLocation, pfTop, pfBot);
   }
   else
   {
      pfTop->resize(vPoi.size(), 0.0);
      pfBot->resize(vPoi.size(), 0.0);
   }
}

bool RemoveStrongbacksSupports(const CTemporarySupportData* pTS)
{
   return (pTS->GetSupportType() == pgsTypes::StrongBack ? true : false);
}

bool RemoveTowerSupports(const CTemporarySupportData* pTS)
{
   return (pTS->GetSupportType() == pgsTypes::ErectionTower ? true : false);
}

class RemoveTSNotInstalled
{
public:
   RemoveTSNotInstalled(IIntervals* pIntervals, IntervalIndexType interval) { m_IntervalIndex = interval; m_pIntervals = pIntervals; }
   bool operator()(const CTemporarySupportData* tsData)
   {
      // make sure support exists in current interval
      IntervalIndexType iremove = m_pIntervals->GetTemporarySupportRemovalInterval(tsData->GetIndex());
      IntervalIndexType iadd = m_pIntervals->GetTemporarySupportErectionInterval(tsData->GetIndex());

      return m_IntervalIndex < iadd || m_IntervalIndex > iremove;
   }
private:
   IIntervals* m_pIntervals;
   IntervalIndexType m_IntervalIndex;
};


std::vector<pgsPointOfInterest> CSegmentModelManager::GetDeflectionDatumLocationsForSegment(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx, pgsTypes::DropInType dropInType) const
{
   ASSERT_SEGMENT_KEY(segmentKey);

   GET_IFACE(IPointOfInterest, pPoi);
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if (intervalIdx < erectionIntervalIdx)
   {
      // Intervals prior to erection
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

      PoiAttributeType poiReference;
      if (/*releaseIntervalIdx <= intervalIdx &&*/ intervalIdx < liftingIntervalIdx)
      {
         poiReference = POI_RELEASED_SEGMENT;
      }
      else if (liftingIntervalIdx <= intervalIdx && intervalIdx < storageIntervalIdx)
      {
         poiReference = POI_LIFT_SEGMENT;
      }
      else if (storageIntervalIdx <= intervalIdx && intervalIdx < haulingIntervalIdx)
      {
         poiReference = POI_STORAGE_SEGMENT;
      }
      else
      {
         ATLASSERT(haulingIntervalIdx <= intervalIdx && intervalIdx < erectionIntervalIdx);
         poiReference = POI_HAUL_SEGMENT;
      }

      PoiList vSupportPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | poiReference, &vSupportPoi);

      std::vector<pgsPointOfInterest> vPoi;
      for (const auto& poi : vSupportPoi)
      {
         vPoi.emplace_back(poi);
      }

      return vPoi;
   }
   else
   {
      // We are in erected bridge
      // Basic rules are as follows:
      //1)	Rules are executed based on support conditions at erection time of segment in question
      //2)	If a segment is supported by 2 or more permanent piers, then both EDD locations are at the two outermost permanent piers
      //3)	Else If segment supported by 1 permanent pier :
      //    a)	If zero erection towers.Place EDD at pierand strongback at other end
      //    b)	Else If 1 erection tower, or multiple towers all on same side of pier
      //       i)	If tower(s) on side of pier with adjacent segment with free end, place EDD at outermost tower.Allow tower adjustment if free to move
      //       ii)	Else if tower on side of pier with a supporting adjacent segment, Place EDD at strongback, or tower if tower at closure
      //    c)	Else if > 1 towers, and towers on both sides of pier :
      //      Place single EDD at Pier, allow outermost towers to be adjustable in co - dependent seesaw motion.Any redundant towers go along for the ride.Segment rotation is independent of adjacent segment BC’s.
      //4)	Else If zero permanent piers :
      //    a)	If zero towers : Place EDDs at strongbacks
      //    b)	Else if 1 tower:
      //       i)	If both ends of segment dependent on adjacent segments, place EDD’s at strongbacksand adjust tower elevation per unrecoverable deflection
      //       ii)	If only one end dependent on adjacent segment, place EDD at towerand at supporting strongback
      //    c)	If 2 + towers:
      //       i)	If adjacent segments at both ends free, place EDDs at outermost towers
      //       ii)	Else if both ends dependent on adjacent segments.Place EDD’s on strongbacks, adjust tower elevations
      //       iii)	Else if one end freeand other dependent.Place EDD at strongback at dependent end, and other EDD at closest tower to free end
      //5)	Additional rules for erection deflection datums :
      //    a)	Deflections at free ends of drop in segments are made to match deflection of adjacent supporting segment
      //    b)	A mismatch in end deflections can occur when segments with fully constrained ends are placed adjacently.This mismatch must be checked if post - tensioning through joint
      GET_IFACE(IBridgeDescription, pIBridgeDesc);
      const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

      // Get POI's at outermost rigid piers
      std::vector<pgsPointOfInterest> vRigidPierPois;
      std::vector<const CPierData2*> vPiers = pSegment->GetPiers();
      IndexType nPiers = vPiers.size();
      if (0 < nPiers)
      {
         if (vPiers.front()->IsBoundaryPier() || vPiers.front()->GetClosureJoint(segmentKey.girderIndex)!=nullptr)
         {
            // if supported by a boundary pier, get the CL Brg poi
            PoiList vSupportPoi;
            const CSpanData2* pPierNextSpan = vPiers.front()->GetNextSpan();
            const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
            if (pPierNextSpan != nullptr && pPierNextSpan->GetIndex() == pStartSpan->GetIndex())
            {
               pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSupportPoi);
            }
            else
            {
               ATLASSERT(nPiers == 1); // only time a boundary pier can be at end of segment
               pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSupportPoi);
            }

            vRigidPierPois.emplace_back(vSupportPoi.front());
         }
         else
         {
            // this is an interior pier, we want the CL Pier poi
            vRigidPierPois.emplace_back(pPoi->GetPierPointOfInterest(segmentKey, vPiers.front()->GetIndex()));
         }

         if (1 < nPiers)
         {
            if (vPiers.back()->IsBoundaryPier() || vPiers.back()->GetClosureJoint(segmentKey.girderIndex) != nullptr)
            {
               // if supported by a boundary pier, we want the CL Brg poi
               PoiList vSupportPoi;
               pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSupportPoi); // in collection, back() boundary pier must be at end of segment
               vRigidPierPois.emplace_back(vSupportPoi.front());
            }
            else
            {
               vRigidPierPois.emplace_back(pPoi->GetPierPointOfInterest(segmentKey, vPiers.back()->GetIndex()));
            }
         }
      }

      // Rule 2 - if 2 or more rigid piers, return pois at those piers. 
      if (nPiers > 1)
      {
         return vRigidPierPois; // Should always happen in PGSuper
      }
      else
      {
         // Look at temporary support configuration
         // Temporary supports. Clear out those not in current interval
         std::vector<const CTemporarySupportData*> vAllTS = pSegment->GetTemporarySupports();
         vAllTS.erase(std::remove_if(vAllTS.begin(), vAllTS.end(), RemoveTSNotInstalled(pIntervals, intervalIdx)), vAllTS.end());

         // Get only towers currently erected
         std::vector<const CTemporarySupportData*> vTSTowers = vAllTS;
         vTSTowers.erase(std::remove_if(vTSTowers.begin(), vTSTowers.end(), RemoveStrongbacksSupports), vTSTowers.end());
         IndexType nTowers = vTSTowers.size();

         // Get only strongbacks currently erected
         std::vector<const CTemporarySupportData*> vTSStrongbacks = vAllTS;
         vTSStrongbacks.erase(std::remove_if(vTSStrongbacks.begin(), vTSStrongbacks.end(), RemoveTowerSupports), vTSStrongbacks.end());
         IndexType nStrongbacks = vTSStrongbacks.size();

         std::vector<pgsPointOfInterest> vPoi;

         if (nPiers == 1)  // Rule 3
         {
            const CPierData2* pPier = vPiers.front();
            Float64 PierStation = pPier->GetStation();

            if (nTowers == 0)
            {
               // Rule 3a. This is a drop in
               ATLASSERT(vTSStrongbacks.size() == 1);
               if (vTSStrongbacks.front()->GetStation() < PierStation)
               {
                  if (vTSStrongbacks.front()->GetClosureJoint(segmentKey.girderIndex) == nullptr)
                  {
                     vPoi.emplace_back(pPoi->GetTemporarySupportPointOfInterest(segmentKey, vTSStrongbacks.front()->GetIndex()));
                  }
                  else
                  {
                     PoiList vSupportPoi;
                     pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSupportPoi); 
                     vPoi.emplace_back(vSupportPoi.front());
                  }

                  vPoi.push_back(vRigidPierPois.front());
               }
               else
               {
                  vPoi.push_back(vRigidPierPois.front());
                  if (vTSStrongbacks.front()->GetClosureJoint(segmentKey.girderIndex) == nullptr)
                  {
                     vPoi.emplace_back(pPoi->GetTemporarySupportPointOfInterest(segmentKey, vTSStrongbacks.front()->GetIndex()));
                  }
                  else
                  {
                     PoiList vSupportPoi;
                     pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSupportPoi);
                     vPoi.emplace_back(vSupportPoi.front());
                  }
               }
            }
            else
            {
               // 1 or more towers under segment. 
               // Determine if all towers on same side of pier, or straddle pier
               bool bTowerLeft(false), bTowerRight(false);
               for (const auto* ptower : vTSTowers)
               {
                  if (ptower->GetStation() < PierStation)
                  {
                     bTowerLeft = true;
                  }
                  else
                  {
                     bTowerRight = true;
                  }
               }

               if (bTowerLeft && bTowerRight)
               {
                  // Rule 3c: Towers on both sides. Place single EDD only at Pier
                  vPoi.push_back(vRigidPierPois.front());
               }
               else
               {
                  const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

                  if (bTowerLeft)
                  {
                     // All towers are to left of pier. See if adjacent segment is free to move 
                     const CPrecastSegmentData* pLeftSegment = pSegment->GetPrevSegment();
                     const CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metStart);
                     if (pLeftSegment && pClosure)
                     {
                        pgsTypes::DropInType adjDropInType = pLeftSegment->IsDropIn();
                        if (adjDropInType == pgsTypes::ditYesFreeEndEnd || adjDropInType == pgsTypes::ditYesFreeBothEnds)
                        {
                           // Rule 3bi. Place EDD at outermost tower near free segment to support adjacent segment
                           // Use bearings at closure if at closure
                           if (vTSTowers.front()->GetClosureJoint(segmentKey.girderIndex) != nullptr)
                           {
                              PoiList vSupportPoi;
                              pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSupportPoi); // in collection, back() boundary pier must be at end of segment
                              vPoi.emplace_back(vSupportPoi.front());
                           }
                           else
                           {
                              vPoi.push_back(pPoi->GetTemporarySupportPointOfInterest(segmentKey, vTSTowers.front()->GetIndex()));
                           }

                           vPoi.push_back(vRigidPierPois.front());
                        }
                        else
                        {
                           // Rule 3bii
                           const CTemporarySupportData* pTempS = pClosure->GetTemporarySupport();
                           if (pTempS)
                           {
                              // Use bearings at closure if at closure
                              if (vTSTowers.front()->GetClosureJoint(segmentKey.girderIndex) != nullptr)
                              {
                                 PoiList vSupportPoi;
                                 pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSupportPoi); // in collection, back() boundary pier must be at end of segment
                                 vPoi.emplace_back(vSupportPoi.front());
                              }
                              else
                              {
                                 vPoi.push_back(pPoi->GetTemporarySupportPointOfInterest(segmentKey, vTSTowers.front()->GetIndex()));
                              }

                              vPoi.push_back(vRigidPierPois.front());
                           }
                           else
                           {
                              // otherwise use outermost tower
                              vPoi.push_back(pPoi->GetTemporarySupportPointOfInterest(segmentKey, vTSTowers.front()->GetIndex()));
                              vPoi.push_back(vRigidPierPois.front());
                           }
                        }
                     }
                     else
                     {
                        // no segment to left. just place at left tower
                        vPoi.push_back(pPoi->GetTemporarySupportPointOfInterest(segmentKey, vTSTowers.front()->GetIndex()));
                        vPoi.push_back(vRigidPierPois.front());
                     }
                  }
                  else
                  {
                     ATLASSERT(bTowerRight);
                     // All towers are to right of pier. See if adjacent segment is free to move 
                     const CPrecastSegmentData* pRightSegment = pSegment->GetNextSegment();
                     const CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);
                     if (pRightSegment && pClosure)
                     {
                        pgsTypes::DropInType adjDropInType = pRightSegment->IsDropIn();
                        if (adjDropInType == pgsTypes::ditYesFreeStartEnd || adjDropInType == pgsTypes::ditYesFreeBothEnds)
                        {
                           // Rule 3bi. Place EDD at outermost tower near free segment to support adjacent segment
                           vPoi.push_back(vRigidPierPois.back());

                           if (vTSTowers.back()->GetClosureJoint(segmentKey.girderIndex) != nullptr)
                           {
                              PoiList vSupportPoi;
                              pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSupportPoi);
                              vPoi.emplace_back(vSupportPoi.front());
                           }
                           else
                           {
                              vPoi.push_back(pPoi->GetTemporarySupportPointOfInterest(segmentKey, vTSTowers.back()->GetIndex()));
                           }
                        }
                        else
                        {
                           // Rule 3bii
                           const CTemporarySupportData* pTempS = pClosure->GetTemporarySupport();
                           if (pTempS)
                           {
                              // Use temp support at closure if at closure
                              vPoi.push_back(vRigidPierPois.back());

                              PoiList vSupportPoi;
                              pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSupportPoi);
                              vPoi.emplace_back(vSupportPoi.front());
                           }
                           else
                           {
                              // otherwise use outermost tower
                              vPoi.push_back(vRigidPierPois.back());
                              vPoi.push_back(pPoi->GetTemporarySupportPointOfInterest(segmentKey, vTSTowers.back()->GetIndex()));
                           }
                        }
                     }
                     else
                     {
                        // no segment to left. just place at left tower
                        vPoi.push_back(vRigidPierPois.back());
                        vPoi.push_back(pPoi->GetTemporarySupportPointOfInterest(segmentKey, vTSTowers.back()->GetIndex()));
                     }
                  }
               }
            }
         }
         else
         {
            // nPiers == 0
            if (nTowers == 0)
            {
               // Rule 4a
               // No piers and no towers. Segment only supported by strongbacks
               ATLASSERT(nStrongbacks == 2);
               PoiList vSupportPoi;
               pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSupportPoi);
               vPoi.emplace_back(vSupportPoi.front());
               vSupportPoi.clear();
               pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSupportPoi);
               vPoi.emplace_back(vSupportPoi.front());
            }
            else // nTowers > 0 && nPiers==0
            {
               // Rules 4b-4c are basically the same
               const CPrecastSegmentData* pLeftSegment = pSegment->GetPrevSegment();
               const CClosureJointData* pLeftClosure = pSegment->GetClosureJoint(pgsTypes::metStart);
               const CTemporarySupportData* pLeftClosureSupport = pLeftClosure ? pLeftClosure->GetTemporarySupport() : nullptr;
               const CPrecastSegmentData* pRightSegment = pSegment->GetNextSegment();
               const CClosureJointData* pRightClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);
               const CTemporarySupportData* pRightClosureSupport = pRightClosure ? pRightClosure->GetTemporarySupport() : nullptr;

               if (dropInType == pgsTypes::ditYesFreeBothEnds)
               {
                  ATLASSERT(pLeftClosureSupport&& pRightClosureSupport); 
                  PoiList vSupportPoi;
                  pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSupportPoi);
                  vPoi.emplace_back(vSupportPoi.front());
                  vSupportPoi.clear();
                  pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSupportPoi);
                  vPoi.emplace_back(vSupportPoi.front());
               }
               else if (dropInType == pgsTypes::ditYesFreeEndEnd)
               {
                  ATLASSERT(pRightClosureSupport);
                  if (vTSTowers.front()->GetClosureJoint(segmentKey.girderIndex))
                  {
                     PoiList vSupportPoi;
                     pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSupportPoi);
                     vPoi.emplace_back(vSupportPoi.front());
                  }
                  else
                  {
                     vPoi.emplace_back(pPoi->GetTemporarySupportPointOfInterest(segmentKey, vTSTowers.front()->GetIndex()));
                  }

                  PoiList vSupportPoi;
                  pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSupportPoi);
                  vPoi.emplace_back(vSupportPoi.front());
               }
               else if (dropInType == pgsTypes::ditYesFreeStartEnd)
               {
                  ATLASSERT(pLeftClosureSupport);
                  PoiList vSupportPoi;
                  pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSupportPoi);
                  vPoi.emplace_back(vSupportPoi.front());

                  if (vTSTowers.back()->GetClosureJoint(segmentKey.girderIndex))
                  {
                     PoiList vSupportPoi;
                     pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSupportPoi);
                     vPoi.emplace_back(vSupportPoi.front());
                  }
                  else
                  {
                     vPoi.push_back(pPoi->GetTemporarySupportPointOfInterest(segmentKey, vTSTowers.back()->GetIndex()));
                  }
               }
               else // fixed at both ends
               {
                  ATLASSERT(nTowers>1); // need to look if this is really a case. Make arbitrary selection
                  PoiList vSupportPoi;
                  pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSupportPoi);
                  vPoi.emplace_back(vSupportPoi.front());
                  vSupportPoi.clear();
                  pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSupportPoi);
                  vPoi.emplace_back(vSupportPoi.front());
               }
            }
         }

         return vPoi;
      }
   }
}


///////////////////////////
const CSegmentModelManager::LoadCombinationMap& CSegmentModelManager::GetLoadCombinationMap(const CGirderKey& girderKey) const
{
   auto found = m_LoadCombinationMaps.find(girderKey);
   if ( found == m_LoadCombinationMaps.cend() )
   {
      auto result = m_LoadCombinationMaps.insert(std::make_pair(girderKey,LoadCombinationMap()));
      ATLASSERT(result.second == true);
      found = result.first;
   }

   return found->second;
}

CSegmentModelManager::LoadCombinationMap& CSegmentModelManager::GetLoadCombinationMap(const CGirderKey& girderKey)
{
   auto found = m_LoadCombinationMaps.find(girderKey);
   if (found == m_LoadCombinationMaps.end())
   {
      auto result = m_LoadCombinationMaps.insert(std::make_pair(girderKey, LoadCombinationMap()));
      ATLASSERT(result.second == true);
      found = result.first;
   }

   return found->second;
}

void CSegmentModelManager::GetSectionResults(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const PoiList& vPoi,ResultsType resultsType,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz) const
{
   ATLASSERT(VerifyPoi(vPoi));

#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().get().GetSegmentKey());
   ATLASSERT(intervalIdx < erectionIntervalIdx); // should be getting results from the girder model
#endif // _DEBUG

   if ( resultsType == rtIncremental )
   {
      std::vector<sysSectionValue> fxPrev, fyPrev, mzPrev, fxThis, fyThis, mzThis;
      std::vector<Float64>         dxPrev, dyPrev, rzPrev, dxThis, dyThis, rzThis;
      if ( intervalIdx == 0 )
      {
         fxPrev.resize(vPoi.size(),sysSectionValue(0.0,0.0));
         fyPrev.resize(vPoi.size(),sysSectionValue(0.0,0.0));
         mzPrev.resize(vPoi.size(),sysSectionValue(0.0,0.0));
         dxPrev.resize(vPoi.size(),0.0);
         dyPrev.resize(vPoi.size(),0.0);
         rzPrev.resize(vPoi.size(),0.0);
      }
      else
      {
         GetSectionResults(intervalIdx-1,lcid,vPoi,&fxPrev,&fyPrev,&mzPrev,&dxPrev,&dyPrev,&rzPrev);
      }

      GetSectionResults(intervalIdx  ,lcid,vPoi,&fxThis,&fyThis,&mzThis,&dxThis,&dyThis,&rzThis);
      
      std::transform(fxThis.cbegin(),fxThis.cend(),fxPrev.cbegin(),std::back_inserter(*pvFx),[](const auto& a, const auto& b) {return a - b;});
      std::transform(fyThis.cbegin(),fyThis.cend(),fyPrev.cbegin(),std::back_inserter(*pvFy),[](const auto& a, const auto& b) {return a - b;});
      std::transform(mzThis.cbegin(),mzThis.cend(),mzPrev.cbegin(),std::back_inserter(*pvMz),[](const auto& a, const auto& b) {return a - b;});

      std::transform(dxThis.cbegin(),dxThis.cend(),dxPrev.cbegin(),std::back_inserter(*pvDx),[](const auto& a, const auto& b) {return a - b;});
      std::transform(dyThis.cbegin(),dyThis.cend(),dyPrev.cbegin(),std::back_inserter(*pvDy),[](const auto& a, const auto& b) {return a - b;});
      std::transform(rzThis.cbegin(),rzThis.cend(),rzPrev.cbegin(),std::back_inserter(*pvRz),[](const auto& a, const auto& b) {return a - b;});
   }
   else
   {
      GetSectionResults(intervalIdx,lcid,vPoi,pvFx,pvFy,pvMz,pvDx,pvDy,pvRz);
   }
}

void CSegmentModelManager::GetPrestressSectionResults(IntervalIndexType intervalIdx,const PoiList& vPoi,ResultsType resultsType,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz) const
{
   ATLASSERT(VerifyPoi(vPoi));

   // all requests for pretension results come to the segment model manager
   // however, the segment model manager needs the intervalIdx to be less then the erection interval
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().get().GetSegmentKey());
   intervalIdx = Min(intervalIdx, erectionIntervalIdx - 1);

   if ( resultsType == rtIncremental )
   {
      std::vector<sysSectionValue> fxPrev, fyPrev, mzPrev, fxThis, fyThis, mzThis;
      std::vector<Float64>         dxPrev, dyPrev, rzPrev, dxThis, dyThis, rzThis;
      if ( intervalIdx == 0 )
      {
         fxPrev.resize(vPoi.size(),sysSectionValue(0.0,0.0));
         fyPrev.resize(vPoi.size(),sysSectionValue(0.0,0.0));
         mzPrev.resize(vPoi.size(),sysSectionValue(0.0,0.0));
         dxPrev.resize(vPoi.size(),0.0);
         dyPrev.resize(vPoi.size(),0.0);
         rzPrev.resize(vPoi.size(),0.0);
      }
      else
      {
         GetPrestressSectionResults(intervalIdx-1,vPoi,&fxPrev,&fyPrev,&mzPrev,&dxPrev,&dyPrev,&rzPrev);
      }

      GetPrestressSectionResults(intervalIdx  ,vPoi,&fxThis,&fyThis,&mzThis,&dxThis,&dyThis,&rzThis);
      
      std::transform(fxThis.cbegin(),fxThis.cend(),fxPrev.cbegin(),std::back_inserter(*pvFx),[](const auto& a, const auto& b) {return a - b;});
      std::transform(fyThis.cbegin(),fyThis.cend(),fyPrev.cbegin(),std::back_inserter(*pvFy),[](const auto& a, const auto& b) {return a - b;});
      std::transform(mzThis.cbegin(),mzThis.cend(),mzPrev.cbegin(),std::back_inserter(*pvMz),[](const auto& a, const auto& b) {return a - b;});

      std::transform(dxThis.cbegin(),dxThis.cend(),dxPrev.cbegin(),std::back_inserter(*pvDx),[](const auto& a, const auto& b) {return a - b;});
      std::transform(dyThis.cbegin(),dyThis.cend(),dyPrev.cbegin(),std::back_inserter(*pvDy),[](const auto& a, const auto& b) {return a - b;});
      std::transform(rzThis.cbegin(),rzThis.cend(),rzPrev.cbegin(),std::back_inserter(*pvRz),[](const auto& a, const auto& b) {return a - b;});
   }
   else
   {
      GetPrestressSectionResults(intervalIdx,vPoi,pvFx,pvFy,pvMz,pvDx,pvDy,pvRz);
   }
}

void CSegmentModelManager::GetPrestressSectionResults(IntervalIndexType intervalIdx,const PoiList& vPoi,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz) const
{
   ATLASSERT(VerifyPoi(vPoi));

   pvFx->resize(vPoi.size(),sysSectionValue(0,0));
   pvFy->resize(vPoi.size(),sysSectionValue(0,0));
   pvMz->resize(vPoi.size(),sysSectionValue(0,0));
   pvDx->resize(vPoi.size(),0.0);
   pvDy->resize(vPoi.size(),0.0);
   pvRz->resize(vPoi.size(),0.0);

   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType =  pgsTypes::StrandType(i);
      LoadCaseIDType lcidMx, lcidMy;
      GetLoadCaseID(strandType,&lcidMx,&lcidMy);

      std::vector<sysSectionValue> fx, fy, mz;
      std::vector<Float64> dx, dy, rz;
      GetSectionResults(intervalIdx,lcidMx,vPoi,&fx,&fy,&mz,&dx,&dy,&rz);
      
      std::transform(fx.cbegin(),fx.cend(),pvFx->cbegin(),pvFx->begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(fy.cbegin(),fy.cend(),pvFy->cbegin(),pvFy->begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(mz.cbegin(),mz.cend(),pvMz->cbegin(),pvMz->begin(),[](const auto& a, const auto& b) {return a + b;});

      std::transform(dx.cbegin(),dx.cend(),pvDx->cbegin(),pvDx->begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(dy.cbegin(),dy.cend(),pvDy->cbegin(),pvDy->begin(),[](const auto& a, const auto& b) {return a + b;});
      std::transform(rz.cbegin(),rz.cend(),pvRz->cbegin(),pvRz->begin(),[](const auto& a, const auto& b) {return a + b;});
   }
}

void CSegmentModelManager::GetSectionResults(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const PoiList& vPoi,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz) const
{
   ATLASSERT(VerifyPoi(vPoi));

   GET_IFACE(IIntervals, pIntervals);

   LoadCaseIDType lcidPT = GetLoadCaseID(pgsTypes::pftPostTensioning);

   CSegmentModelData* pModelData = nullptr;
   CSegmentKey prevSegmentKey;
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType stressingIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);

      if (releaseIntervalIdx <= intervalIdx && !segmentKey.IsEqual(prevSegmentKey))
      {
         // segment key changed... get the model for the current segment key
         pModelData = GetSegmentModel(segmentKey, intervalIdx);

         // apply pretension load to the segment model if it isn't already applied
         LoadCaseIDType lcidMx[3], lcidMy[3];
         GetLoadCaseID(pgsTypes::Straight,  &lcidMx[pgsTypes::Straight],  &lcidMy[pgsTypes::Straight]);
         GetLoadCaseID(pgsTypes::Harped,    &lcidMx[pgsTypes::Harped],    &lcidMy[pgsTypes::Harped]);
         GetLoadCaseID(pgsTypes::Temporary, &lcidMx[pgsTypes::Temporary], &lcidMy[pgsTypes::Temporary]);
         if (pModelData->Loads.find(lcid) == pModelData->Loads.end() &&
            ((lcid == lcidMx[pgsTypes::Straight]  || lcid == lcidMy[pgsTypes::Straight]) ||
             (lcid == lcidMx[pgsTypes::Harped]    || lcid == lcidMy[pgsTypes::Harped]) ||
             (lcid == lcidMx[pgsTypes::Temporary] || lcid == lcidMy[pgsTypes::Temporary]))
           )
         {
            ApplyPretensionLoad(pModelData,segmentKey,intervalIdx);
         }

         if (pModelData->Loads.find(lcidPT) == pModelData->Loads.end() && lcid == lcidPT)
         {
            ApplyPostTensionLoad(pModelData, segmentKey, intervalIdx);
         }
      }

      GET_IFACE_NOCHECK(IPointOfInterest,pPoi);
      if ( intervalIdx < releaseIntervalIdx || (intervalIdx < stressingIntervalIdx && lcid == lcidPT) || pPoi->IsOffSegment(poi) )
      {
         // interval is before release (so no pretension effects)
         // or interval is before post-tension and the request is for PT results (so no PT effects)
         // or the POI is off the segment
         sysSectionValue fx(0,0), fy(0,0), mz(0,0);
         Float64 dx(0), dy(0), rz(0);

         pvFx->push_back(fx);
         pvFy->push_back(fy);
         pvMz->push_back(mz);

         pvDx->push_back(dx);
         pvDy->push_back(dy);
         pvRz->push_back(rz);
      }
      else
      {
         // Check if results have been cached for this poi.
         PoiIDPairType poi_id = pModelData->PoiMap.GetModelPoi( poi );

         if ( poi_id.first == INVALID_ID )
         {
            poi_id = AddPointOfInterest( pModelData, poi );
         }

         ATLASSERT(poi_id.first != INVALID_ID);


         CComQIPtr<IFem2dModelResults> results(pModelData->Model);

         Float64 FxRight(0), FyRight(0), MzRight(0);
         Float64 FxLeft(0),  FyLeft(0),  MzLeft(0);
         CAnalysisResult ar(_T(__FILE__),__LINE__);
         ar = results->ComputePOIForces(lcid,poi_id.first,mftLeft,lotGlobal,&FxLeft,&FyLeft,&MzLeft);

         FyLeft *= -1;

         ar = results->ComputePOIForces(lcid,poi_id.second,mftRight,lotGlobal,&FxRight,&FyRight,&MzRight);

         sysSectionValue fx(FxLeft,FxRight);
         sysSectionValue fy(FyLeft,FyRight);
         sysSectionValue mz(MzLeft,MzRight);

         pvFx->push_back(fx);
         pvFy->push_back(fy);
         pvMz->push_back(mz);

         Float64 dx(0),dy(0),rz(0);
         ar = results->ComputePOIDeflections(lcid,poi_id.first,lotGlobal,&dx,&dy,&rz);

         pvDx->push_back(dx);
         pvDy->push_back(dy);
         pvRz->push_back(rz);
      }

      prevSegmentKey = segmentKey;
   }
}

void CSegmentModelManager::GetSectionStresses(IntervalIndexType intervalIdx,LoadCaseIDType lcid,ResultsType resultsType,const PoiList& vPoi,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   ATLASSERT(VerifyPoi(vPoi));

#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().get().GetSegmentKey());
   ATLASSERT(intervalIdx < erectionIntervalIdx); // should be getting results from the girder model
#endif // _DEBUG

   GET_IFACE(IPointOfInterest,pPoi);

   for ( const pgsPointOfInterest& poi : vPoi)
   {
      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      Float64 fTop,fBot;
      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) || poi.HasAttribute(POI_BOUNDARY_PIER) || (poi.GetDistFromStart() < 0 && poi.IsTenthPoint(POI_SPAN) == 1) )
      {
         // the POI is at a closure joint... closures have not been installed yet so
         // use 0 for the results
         fTop = 0;
         fBot = 0;
      }
      else
      {
         if ( resultsType == rtIncremental )
         {
            // incremental results are the change in results from the end of the previous interval and
            // the end of this interval.
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
            if ( intervalIdx == releaseIntervalIdx )
            {
               GetSectionStress(intervalIdx,lcid,poi,topLocation,botLocation,&fTop,&fBot);
            }
            else
            {
               Float64 fTopRelease, fBotRelease;
               GetSectionStress(intervalIdx-1,lcid,poi,topLocation,botLocation,&fTopRelease,&fBotRelease);

               Float64 fTopStorage, fBotStorage;
               GetSectionStress(intervalIdx,lcid,poi,topLocation,botLocation,&fTopStorage,&fBotStorage);

               fTop = fTopStorage - fTopRelease;
               fBot = fBotStorage - fBotRelease;
            }
         }
         else
         {
            // Cumulative stress is just the stress in this interval
            GetSectionStress(intervalIdx,lcid,poi,topLocation,botLocation,&fTop,&fBot);
         }
      }

#if defined _DEBUG
      if ( IsDeckStressLocation(topLocation) )
      {
         ATLASSERT(IsZero(fTop));
      }

      if ( IsDeckStressLocation(botLocation) )
      {
         ATLASSERT(IsZero(fBot));
      }
#endif

      pfTop->push_back(fTop);
      pfBot->push_back(fBot);
   }
}

void CSegmentModelManager::GetSectionStress(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const pgsPointOfInterest& poi,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);

   ApplyPretensionLoad(pModelData,segmentKey,intervalIdx);

   PoiIDPairType poi_id = pModelData->PoiMap.GetModelPoi( poi );
   if ( poi_id.first == INVALID_ID )
   {
      poi_id = AddPointOfInterest( pModelData, poi );
   }

   CComQIPtr<IFem2dModelResults> results(pModelData->Model);
   Float64 fx,fy,mz;
   Fem2dMbrFaceType face = IsZero( poi.GetDistFromStart() ) ? mftRight : mftLeft;
   PoiIDType pid = face==mftLeft ? poi_id.first : poi_id.second;

   if ( lcid == INVALID_ID )
   {
      fx = 0;
      fy = 0;
      mz = 0;
   }
   else
   {
      CAnalysisResult ar(_T(__FILE__),__LINE__);
      ar = results->ComputePOIForces(lcid,pid,face,lotMember,&fx,&fy,&mz);
   }

   GET_IFACE(ISectionProperties, pSectProp);

   Float64 Cat, Cbtx, Cbty;
   pSectProp->GetStressCoefficients(intervalIdx, poi, topLocation, nullptr, &Cat, &Cbtx, &Cbty);

   Float64 Cab, Cbbx, Cbby;
   pSectProp->GetStressCoefficients(intervalIdx, poi, botLocation, nullptr, &Cab, &Cbbx, &Cbby);

   *pfTop = Cat*fx + Cbtx*mz;
   *pfTop = IsZero(*pfTop) ? 0 : *pfTop;

   *pfBot = Cab*fx + Cbbx*mz;
   *pfBot = IsZero(*pfBot) ? 0 : *pfBot;
}

void CSegmentModelManager::GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadCaseIDType lcid,ResultsType resultsType,Float64* pRleft,Float64* pRright) const
{
   if ( resultsType == rtIncremental )
   {
      Float64 prevRleft, prevRright;
      Float64 thisRleft, thisRright;
      GetReaction(segmentKey,intervalIdx-1,lcid,rtCumulative,&prevRleft,&prevRright);
      GetReaction(segmentKey,intervalIdx,  lcid,rtCumulative,&thisRleft,&thisRright);
      *pRleft  = thisRleft  - prevRleft;
      *pRright = thisRright - prevRright;
   }
   else
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      if ( intervalIdx < releaseIntervalIdx )
      {
         *pRleft  = 0;
         *pRright = 0;
         return;
      }

      CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);

      CComQIPtr<IFem2dModelResults> results(pModelData->Model);
      CComPtr<IFem2dJointCollection> joints;
      pModelData->Model->get_Joints(&joints);
      CollectionIndexType nJoints;
      joints->get_Count(&nJoints);
      std::vector<Float64> reactions;
      for ( IndexType jntIdx = 0; jntIdx < nJoints; jntIdx++ )
      {
         CComPtr<IFem2dJoint> joint;
         joints->get_Item(jntIdx,&joint);
         VARIANT_BOOL vbIsSupport;
         joint->IsSupport(&vbIsSupport);
         if ( vbIsSupport == VARIANT_TRUE )
         {
            JointIDType jntID;
            joint->get_ID(&jntID);

            Float64 fx,fy,mz;
            CAnalysisResult ar(_T(__FILE__),__LINE__);
            ar = results->ComputeReactions(lcid,jntID,&fx,&fy,&mz);

            reactions.push_back(fy);
         }
      }

      ATLASSERT(reactions.size() == 2);

      *pRleft  = reactions.front();
      *pRright = reactions.back();
   }
}


Float64 CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,LoadCaseIDType lcid,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType) const
{
   Float64 R;
   if ( resultsType == rtIncremental )
   {
      Float64 R1 = GetReaction(intervalIdx-1,lcid,pierIdx,girderKey);
      Float64 R2 = GetReaction(intervalIdx,  lcid,pierIdx,girderKey);
      R = R2 - R1;
   }
   else
   {
      R = GetReaction(intervalIdx,lcid,pierIdx,girderKey);
   }

   return R;
}

Float64 CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,LoadCaseIDType lcid,PierIndexType pierIdx,const CGirderKey& girderKey) const
{
   GirderIndexType gdrIdx = girderKey.girderIndex;
   CSegmentKey segmentKey = GetSegmentKey(girderKey,pierIdx);

   CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);

   CComQIPtr<IFem2dModelResults> results(pModelData->Model);
   JointIDType jointID;
   if ( pierIdx == 0 )
   {
      jointID = 0;
   }
   else
   {
      CComPtr<IFem2dJointCollection> joints;
      pModelData->Model->get_Joints(&joints);
      CollectionIndexType nJoints;
      joints->get_Count(&nJoints);
      jointID = nJoints-1;
   }

   Float64 fx,fy,mz;
   CAnalysisResult ar(_T(__FILE__),__LINE__);
   ar = results->ComputeReactions(lcid,jointID,&fx,&fy,&mz);
   return fy;
}

void CSegmentModelManager::ZeroResults(const PoiList& vPoi,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz) const
{
   IndexType size = vPoi.size();
   pvFx->resize(size,sysSectionValue(0,0));
   pvFy->resize(size,sysSectionValue(0,0));
   pvMz->resize(size,sysSectionValue(0,0));

   pvDx->resize(size,0.0);
   pvDy->resize(size,0.0);
   pvRz->resize(size,0.0);
}

PoiIDPairType CSegmentModelManager::AddPointOfInterest(CSegmentModelData* pModelData,const pgsPointOfInterest& poi) const
{
   PoiIDPairType femID = pgsGirderModelFactory::AddPointOfInterest(pModelData->Model, poi);
   pModelData->PoiMap.AddMap( poi, femID );

#if defined ENABLE_LOGGING
   Float64 Xpoi = poi.GetDistFromStart();
   LOG("Adding POI " << femID.first << " at " << ::ConvertFromSysUnits(Xpoi,unitMeasure::Feet) << " ft");
#endif

   return femID;
}

void CSegmentModelManager::BuildReleaseModel(const CSegmentKey& segmentKey) const
{
   ATLASSERT(GetModelData(m_ReleaseModels,segmentKey) == nullptr);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   // Build the model
   std::_tostringstream os;
   os << "Building prestress release model" << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   // generally, segments are supported at their ends at release, but this may not be true
   // for all segments in a spliced girder. Cantilever pier segments, with heavy PS in the top
   // flange and possibly a tapered profile, could be considered supported near the middle of the segment
   // The support location is an input for PGSplice to allow the engineer to control the release condition
   GET_IFACE(IGirder,pGirder);
   Float64 left,right;
   pGirder->GetSegmentReleaseSupportLocations(segmentKey,&left,&right);

#if defined _DEBUG
   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSuperDocument() )
   {
      // must be supported at ends for PGSuper... this should be enforced through the UI
      // as there isn't a way for the user to change the default values
      ATLASSERT(IsZero(left) && IsZero(right));
   }
#endif

   CSegmentModelData segmentModel = BuildSegmentModel(segmentKey,releaseIntervalIdx,left,right,POI_RELEASED_SEGMENT);
   std::pair<SegmentModels::iterator,bool> result = m_ReleaseModels.insert( std::make_pair(segmentKey,segmentModel) );
   ATLASSERT( result.second == true );
}

void CSegmentModelManager::BuildLiftingModel(const CSegmentKey& segmentKey) const
{
   ATLASSERT(GetModelData(m_LiftingModels, segmentKey) == nullptr);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   // Build the model
   std::_tostringstream os;
   os << "Building lifting model" << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   GET_IFACE(ISegmentLifting, pSegmentLifting);
   Float64 leftOverhang  = pSegmentLifting->GetLeftLiftingLoopLocation(segmentKey);
   Float64 rightOverhang = pSegmentLifting->GetRightLiftingLoopLocation(segmentKey);

   CSegmentModelData segmentModel = BuildSegmentModel(segmentKey,liftingIntervalIdx,leftOverhang,rightOverhang,POI_LIFT_SEGMENT);
   std::pair<SegmentModels::iterator,bool> result = m_LiftingModels.insert( std::make_pair(segmentKey,segmentModel) );
   ATLASSERT( result.second == true );
}

void CSegmentModelManager::BuildHaulingModel(const CSegmentKey& segmentKey) const
{
   ATLASSERT(GetModelData(m_HaulingModels, segmentKey) == nullptr);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   // Build the model
   std::_tostringstream os;
   os << "Building hauling model" << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   GET_IFACE(ISegmentHauling, pSegmentHauling);
   Float64 leftOverhang  = pSegmentHauling->GetTrailingOverhang(segmentKey);
   Float64 rightOverhang = pSegmentHauling->GetLeadingOverhang(segmentKey);

   CSegmentModelData segmentModel = BuildSegmentModel(segmentKey,haulingIntervalIdx,leftOverhang,rightOverhang,POI_HAUL_SEGMENT);
   std::pair<SegmentModels::iterator,bool> result = m_HaulingModels.insert( std::make_pair(segmentKey,segmentModel) );
   ATLASSERT( result.second == true );
}

void CSegmentModelManager::BuildStorageModel(const CSegmentKey& segmentKey) const
{
   ATLASSERT(GetModelData(m_StorageModels, segmentKey) == nullptr);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);

   // Build the model
   std::_tostringstream os;
   os << "Building storage model" << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   GET_IFACE(IGirder,pGirder);
   Float64 left,right;
   pGirder->GetSegmentStorageSupportLocations(segmentKey,&left,&right);

   CSegmentModelData segmentModel = BuildSegmentModel(segmentKey,storageIntervalIdx,left,right,POI_STORAGE_SEGMENT);
   std::pair<SegmentModels::iterator,bool> result = m_StorageModels.insert( std::make_pair(segmentKey,segmentModel) );
   ATLASSERT( result.second == true );
}

CSegmentModelData* CSegmentModelManager::GetSegmentModel(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

   if (intervalIdx < releaseIntervalIdx)
      return nullptr;

   if ( intervalIdx < liftingIntervalIdx )
   {
      return GetReleaseModel(segmentKey);
   }
   else if ( liftingIntervalIdx <= intervalIdx && intervalIdx < storageIntervalIdx )
   {
      return GetLiftingModel(segmentKey);
   }
   else if ( storageIntervalIdx <= intervalIdx && intervalIdx < haulingIntervalIdx )
   {
      return GetStorageModel(segmentKey);
   }
   else if ( haulingIntervalIdx <= intervalIdx && intervalIdx < erectionIntervalIdx )
   {
      return GetHaulingModel(segmentKey);
   }

   ATLASSERT(false);
   return nullptr;
}

CSegmentModelData* CSegmentModelManager::GetReleaseModel(const CSegmentKey& segmentKey) const
{
   CSegmentModelData* pModelData = GetModelData(m_ReleaseModels,segmentKey);
   if ( pModelData == nullptr )
   {
      BuildReleaseModel(segmentKey);
      pModelData = GetModelData(m_ReleaseModels,segmentKey);
      ATLASSERT(pModelData != nullptr);
   }

   return pModelData;
}

CSegmentModelData* CSegmentModelManager::GetLiftingModel(const CSegmentKey& segmentKey) const
{
   CSegmentModelData* pModelData = GetModelData(m_LiftingModels,segmentKey);
   if ( pModelData == nullptr )
   {
      BuildLiftingModel(segmentKey);
      pModelData = GetModelData(m_LiftingModels,segmentKey);
      ATLASSERT(pModelData != nullptr);
   }

   return pModelData;
}

CSegmentModelData* CSegmentModelManager::GetHaulingModel(const CSegmentKey& segmentKey) const
{
   CSegmentModelData* pModelData = GetModelData(m_HaulingModels,segmentKey);
   if ( pModelData == nullptr )
   {
      BuildHaulingModel(segmentKey);
      pModelData = GetModelData(m_HaulingModels,segmentKey);
      ATLASSERT(pModelData != nullptr);
   }

   return pModelData;
}

CSegmentModelData* CSegmentModelManager::GetStorageModel(const CSegmentKey& segmentKey) const
{
   CSegmentModelData* pModelData = GetModelData(m_StorageModels,segmentKey);
   if ( pModelData == nullptr )
   {
      BuildStorageModel(segmentKey);
      pModelData = GetModelData(m_StorageModels,segmentKey);
      ATLASSERT(pModelData != nullptr);
   }

   return pModelData;
}

CSegmentModelData* CSegmentModelManager::GetModelData(SegmentModels& models,const CSegmentKey& segmentKey) const
{
   ATLASSERT(segmentKey.segmentIndex != INVALID_INDEX);
   SegmentModels::iterator found = models.find(segmentKey);
   if ( found == models.end() )
   {
      return nullptr;
   }

   CSegmentModelData* pModelData = &(*found).second;

   return pModelData;
}

CSegmentModelData CSegmentModelManager::BuildSegmentModel(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 leftSupportDistance,Float64 rightSupportDistance,PoiAttributeType refAttribute) const
{
   // Get the interface pointers we are going to use
   GET_IFACE(IBridge,            pBridge );
   GET_IFACE(IMaterials,         pMaterial );

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   // For casting yard models, use the entire girder length as the
   // span length.
   Float64 Ls = pBridge->GetSegmentLength(segmentKey);

   // Get the material properties
   Float64 Ec = pMaterial->GetSegmentEc(segmentKey,intervalIdx,pgsTypes::Start);

   // Get points of interest
   GET_IFACE(IPointOfInterest,pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, &vPoi); // we want all POI

   // remove all POI that are not actually on the segment (like POI in closure joints)
   vPoi.erase(std::remove_if(std::begin(vPoi),std::end(vPoi), [Ls = Ls](const pgsPointOfInterest& poi) {return !InRange(0.0, poi.GetDistFromStart(), Ls);}), std::end(vPoi));

   // Build the Model
   CSegmentModelData model_data;
   model_data.SegmentKey = segmentKey;
   model_data.IntervalIdx = intervalIdx;
   model_data.Ec = Ec;
   LoadCaseIDType lcid = GetLoadCaseID(pgsTypes::pftGirder);
   model_data.Loads.insert(lcid);

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   bool bModelLeftCantilever  = true;
   bool bModelRightCantilever = true;

   // always model cantilevers for lifting and hauling
   // these models aren't used for the lifting and hauling analysis, they just provide moments, shears, etc
   // for the graph views. the WBFL::Stability engine creates the real analysis models for stability analysis

   if (intervalIdx != liftingIntervalIdx && intervalIdx != haulingIntervalIdx)
   {
      pBridge->ModelCantilevers(segmentKey, leftSupportDistance, rightSupportDistance, &bModelLeftCantilever, &bModelRightCantilever);
   }

   pgsGirderModelFactory().CreateGirderModel(m_pBroker,intervalIdx,segmentKey,leftSupportDistance,Ls-rightSupportDistance,Ls,Ec,lcid,bModelLeftCantilever,bModelRightCantilever,vPoi,&model_data.Model,&model_data.PoiMap);

   // create loadings for all product load types
   // this may seems silly because many of these loads aren't applied
   // to the segment models. however, it is easier to treat everything
   // uniformly then to check for special exceptions
   //AddLoading(model_data,pgsTypes::pftGirder); // taken care of in CreateGirderModel
   for (int i = 0; i < (int)pgsTypes::pftProductForceTypeCount; i++)
   {
      pgsTypes::ProductForceType pfType = (pgsTypes::ProductForceType)i;
      if (pfType == pgsTypes::pftGirder || pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning)
      {
         // these loadings are taken care of elsewhere
         continue;
      }
      AddLoading(model_data, pfType);
   }

   CComPtr<IFem2dLoadingCollection> loadings;
   model_data.Model->get_Loadings(&loadings);
   CComPtr<IFem2dLoading> loading;
   loadings->Create(GetGirderIncrementalLoadCaseID(), &loading);

   return model_data;
}

void CSegmentModelManager::ApplyPretensionLoad(CSegmentModelData* pModelData,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const
{
   CComPtr<IFem2dLoadingCollection> loadings;
   pModelData->Model->get_Loadings(&loadings);

   GET_IFACE_NOCHECK(IProductLoads,pProductLoads);

   for (int i = 0; i < 3; i++)
   {
      pgsTypes::StrandType strandType = pgsTypes::StrandType(i);

      LoadCaseIDType lcidMx, lcidMy; // lcidMx is for loadings that cause moment about the girder section X-axis and cause vertical, y, deflections
                                     // lcidMy is for loadings that cause moment about the girder section Y-axis and cause lateral, x, deflections
      GetLoadCaseID(strandType, &lcidMx, &lcidMy);

      if (pModelData->Loads.find(lcidMx) != pModelData->Loads.end())
      {
         // this load has been previously applied
         ATLASSERT(pModelData->Loads.find(lcidMy) != pModelData->Loads.end());
         continue;
      }

      CComPtr<IFem2dLoading> loadingX, loadingY;
      CComPtr<IFem2dPointLoadCollection> pointLoadsX, pointLoadsY;
      CComPtr<IFem2dDistributedLoadCollection> distLoadsX;

      loadings->Create(lcidMx, &loadingX);
      loadingX->get_PointLoads(&pointLoadsX);
      loadingX->get_DistributedLoads(&distLoadsX);

      loadings->Create(lcidMy, &loadingY);
      loadingY->get_PointLoads(&pointLoadsY);

      pModelData->Loads.insert(lcidMx);
      pModelData->Loads.insert(lcidMy);

      std::vector<EquivPretensionLoad> vLoads = pProductLoads->GetEquivPretensionLoads(segmentKey, strandType);

      LoadIDType ptLoadIDX;
      pointLoadsX->get_Count((CollectionIndexType*)&ptLoadIDX);

      LoadIDType ptLoadIDY;
      pointLoadsY->get_Count((CollectionIndexType*)&ptLoadIDY);

      LoadIDType distLoadIDX;
      distLoadsX->get_Count((CollectionIndexType*)&distLoadIDX);

      std::vector<EquivPretensionLoad>::iterator iter(vLoads.begin());
      std::vector<EquivPretensionLoad>::iterator iterEnd(vLoads.end());
      for ( ; iter != iterEnd; iter++ )
      {
         EquivPretensionLoad& equivLoad = *iter;

         CComPtr<IFem2dPointLoad> ptLoad;
         MemberIDType mbrIDStart, mbrIDEnd;
         Float64 Xs, Xe;
         pgsGirderModelFactory::FindMember(pModelData->Model, equivLoad.Xs, &mbrIDStart, &Xs);
         pgsGirderModelFactory::FindMember(pModelData->Model, equivLoad.Xe, &mbrIDEnd, &Xe);
         
         if (IsZero(equivLoad.N))
         {
            pointLoadsX->Create(ptLoadIDX++, mbrIDStart, Xs, equivLoad.P, equivLoad.N, equivLoad.Mx, lotGlobal, &ptLoad);
         }
         else
         {
            // if N is not zero, this is the load for the vertical component of harped strand prestress
            // P is in the data structure for reporting purposes, so we can show how N is computed
            // however, there is not a new axial load, P, applied at this location
            pointLoadsX->Create(ptLoadIDX++, mbrIDStart, Xs, 0.0/*equivLoad.P*/, equivLoad.N, equivLoad.Mx, lotGlobal, &ptLoad);
         }

         if (!IsZero(equivLoad.wy))
         {
            if (mbrIDStart == mbrIDEnd)
            {
               CComPtr<IFem2dDistributedLoad> distLoad;
               distLoadsX->Create(distLoadIDX++, mbrIDStart, loadDirFy, Xs, Xe, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
            }
            else
            {
               CComPtr<IFem2dDistributedLoad> distLoad;
               distLoadsX->Create(distLoadIDX++, mbrIDStart, loadDirFy, Xs, -1, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
               for (MemberIDType mbrID = mbrIDStart + 1; mbrID < mbrIDEnd; mbrID++)
               {
                  distLoad.Release();
                  distLoadsX->Create(distLoadIDX++, mbrID, loadDirFy, 0, -1, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
               }
               distLoad.Release();
               distLoadsX->Create(distLoadIDX++, mbrIDEnd, loadDirFy, 0, Xe, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
            }
         }

         if (!IsZero(equivLoad.My))
         {
            // even though this is a moment about the vertical Y axis, we apply it to the structure
            // as if it is a moment about the horizontal X axis. This is because we have a plane frame
            // model. For deflections, we have to take out the EI used in the model and replace it
            // with EI for transverse deflection.
            ptLoad.Release();
            pointLoadsY->Create(ptLoadIDY++, mbrIDStart, Xs, 0.0, 0.0, equivLoad.My, lotGlobal, &ptLoad);
         }
      }
   }
}

void CSegmentModelManager::ApplyPostTensionLoad(CSegmentModelData* pModelData, const CSegmentKey& segmentKey, IntervalIndexType intervalIdx) const
{
   CComPtr<IFem2dLoadingCollection> loadings;
   pModelData->Model->get_Loadings(&loadings);


   LoadCaseIDType lcidPT = GetLoadCaseID(pgsTypes::pftPostTensioning);
   if (pModelData->Loads.find(lcidPT) != pModelData->Loads.end())
   {
      // this load has been previously applied
      ATLASSERT(pModelData->Loads.find(lcidPT) != pModelData->Loads.end());
      return;
   }

   CComPtr<IFem2dLoading> loading;
   CComPtr<IFem2dPointLoadCollection> pointLoads;
   CComPtr<IFem2dDistributedLoadCollection> distLoads;

   loadings->Create(lcidPT, &loading);
   loading->get_PointLoads(&pointLoads);
   loading->get_DistributedLoads(&distLoads);

   pModelData->Loads.insert(lcidPT);

   LoadIDType ptLoadID;
   pointLoads->get_Count((CollectionIndexType*)&ptLoadID);

   LoadIDType distLoadID;
   distLoads->get_Count((CollectionIndexType*)&distLoadID);

   GET_IFACE_NOCHECK(IProductLoads, pProductLoads);
   std::vector<EquivPretensionLoad> vLoads = pProductLoads->GetEquivSegmentPostTensionLoads(segmentKey);

   std::vector<EquivPretensionLoad>::iterator iter(vLoads.begin());
   std::vector<EquivPretensionLoad>::iterator iterEnd(vLoads.end());
   for (; iter != iterEnd; iter++)
   {
      EquivPretensionLoad& equivLoad = *iter;

      CComPtr<IFem2dPointLoad> ptLoad;
      MemberIDType mbrIDStart, mbrIDEnd;
      Float64 Xs, Xe;
      pgsGirderModelFactory::FindMember(pModelData->Model, equivLoad.Xs, &mbrIDStart, &Xs);
      pgsGirderModelFactory::FindMember(pModelData->Model, equivLoad.Xe, &mbrIDEnd, &Xe);

      pointLoads->Create(ptLoadID++, mbrIDStart, Xs, equivLoad.P, equivLoad.N, equivLoad.Mx, lotGlobal, &ptLoad);

      if (!IsZero(equivLoad.wy))
      {
         if (mbrIDStart == mbrIDEnd)
         {
            CComPtr<IFem2dDistributedLoad> distLoad;
            distLoads->Create(distLoadID++, mbrIDStart, loadDirFy, Xs, Xe, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
         }
         else
         {
            CComPtr<IFem2dDistributedLoad> distLoad;
            distLoads->Create(distLoadID++, mbrIDStart, loadDirFy, Xs, -1, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
            for (MemberIDType mbrID = mbrIDStart + 1; mbrID < mbrIDEnd; mbrID++)
            {
               distLoad.Release();
               distLoads->Create(distLoadID++, mbrID, loadDirFy, 0, -1, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
            }
            distLoad.Release();
            distLoads->Create(distLoadID++, mbrIDEnd, loadDirFy, 0, Xe, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
         }
      }
   }
}

LoadCaseIDType CSegmentModelManager::GetLoadCaseID(pgsTypes::ProductForceType pfType) const
{
   if (pfType == pgsTypes::pftPostTensioning)
   {
      return m_ProductLoadMap.GetMaxLoadCaseID() + 1;
   }
   else
   {
      return m_ProductLoadMap.GetLoadCaseID(pfType);
   }
}

void CSegmentModelManager::GetLoadCaseID(pgsTypes::StrandType strandType,LoadCaseIDType* plcidMx,LoadCaseIDType* plcidMy) const
{
   ATLASSERT(strandType != pgsTypes::Permanent);
   //                                               +----- this +2 is for the PT load case ID
   //                                               |
   //                                               V
   *plcidMx = m_ProductLoadMap.GetMaxLoadCaseID() + 2 + 2 * LoadCaseIDType(strandType);
   *plcidMy = m_ProductLoadMap.GetMaxLoadCaseID() + 2 + 2 * LoadCaseIDType(strandType) + 1;
}

LoadCaseIDType CSegmentModelManager::GetGirderIncrementalLoadCaseID() const
{
   return GetFirstExternalLoadCaseID() - 1;
}

LoadCaseIDType CSegmentModelManager::GetFirstExternalLoadCaseID() const
{
   // 9 = 1 for PT + 2*(3 for each strand type) + 1 for Girder_Incremental
   return m_ProductLoadMap.GetMaxLoadCaseID() + 9; // this gets the IDs past the strand IDs
}

LoadCaseIDType CSegmentModelManager::GetLoadCaseID(CSegmentModelData* pModelData,LPCTSTR strLoadingName) const
{
   if (std::_tstring(strLoadingName) == _T("Girder_Incremental"))
   {
      return GetGirderIncrementalLoadCaseID();
   }

   auto found(pModelData->ExternalLoadMap.find(strLoadingName));
   if ( found == pModelData->ExternalLoadMap.end() )
   {
      // the loading wasn't found... did you forget to create it?
      ATLASSERT(false);
      return INVALID_ID;
   }

   LoadCaseIDType lcid = found->second;
   return lcid;
}

void CSegmentModelManager::GetMemberLocation(const pgsPointOfInterest& poi,CSegmentModelData* pModelData,MemberIDType* pMbrID,Float64* pLocation)
{
   GET_IFACE(IPointOfInterest,pPoi);
   CClosureKey closureKey;
   if ( pPoi->IsInClosureJoint(poi,&closureKey) )
   {
      if ( poi.GetSegmentKey().IsEqual(closureKey) )
      {
         // poi is at the end of the model
         CComPtr<IFem2dMemberCollection> members;
         pModelData->Model->get_Members(&members);
         IndexType nMembers;
         members->get_Count(&nMembers);
         CComPtr<IFem2dMember> member;
         members->get_Item(nMembers-1,&member);
         member->get_ID(pMbrID);
         member->get_Length(pLocation);
      }
      else
      {
         // poi is at the start of the mode
         ATLASSERT(poi.GetSegmentKey().segmentIndex == closureKey.segmentIndex+1);
         CComPtr<IFem2dMemberCollection> members;
         pModelData->Model->get_Members(&members);
         CComPtr<IFem2dMember> member;
         members->get_Item(0,&member);
         member->get_ID(pMbrID);
         *pLocation = 0;
      }
   }
   else
   {
      PoiIDPairType poiID = pModelData->PoiMap.GetModelPoi(poi);
      if ( poiID.first == INVALID_ID )
      {
         poiID = AddPointOfInterest(pModelData,poi);
         ATLASSERT(poiID.first != INVALID_ID);
      }

      CComPtr<IFem2dPOICollection> pois;
      pModelData->Model->get_POIs(&pois);

      CComPtr<IFem2dPOI> femPoi;
      pois->Find(poiID.first, &femPoi); // opt to left poi for historical reasons

      femPoi->get_MemberID(pMbrID);
      femPoi->get_Location(pLocation);
   }
}

CSegmentKey CSegmentModelManager::GetSegmentKey(const CGirderKey& girderKey,PierIndexType pierIdx) const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(girderKey);
   
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   CSegmentKey segmentKey;
   if ( pGirder->GetPierIndex(pgsTypes::metStart) == pierIdx )
   {
      segmentKey = pGirder->GetSegment(0)->GetSegmentKey();
   }
   else if ( pGirder->GetPierIndex(pgsTypes::metEnd) == pierIdx )
   {
      segmentKey = pGirder->GetSegment(nSegments-1)->GetSegmentKey();
   }
   else
   {
      for ( SegmentIndexType segIdx = 1; segIdx < nSegments-1; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         SpanIndexType spanIdx = pSegment->GetSpanIndex(pgsTypes::metStart);
         if ( spanIdx == pierIdx )
         {
            segmentKey = pSegment->GetSegmentKey();
            break;
         }
      }
   }

   return segmentKey;
}

void CSegmentModelManager::AddLoading(CSegmentModelData& model_data,pgsTypes::ProductForceType pfType) const
{
   CComPtr<IFem2dLoadingCollection> loadings;
   model_data.Model->get_Loadings(&loadings);
   CComPtr<IFem2dLoading> loading;
   loadings->Create(GetLoadCaseID(pfType),&loading);
}


bool CSegmentModelManager::CreateConcentratedLoad(IntervalIndexType intervalIdx, LoadCaseIDType lcid,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   CSegmentModelData* pModelData = GetSegmentModel(poi.GetSegmentKey(),intervalIdx);

   CComPtr<IFem2dLoadingCollection> loadings;
   pModelData->Model->get_Loadings(&loadings);

   CComPtr<IFem2dLoading> loading;
   if ( FAILED(loadings->Find(lcid,&loading)) )
   {
      ATLASSERT(false);
      return false;
   }

   CComPtr<IFem2dPointLoadCollection> pointLoads;
   loading->get_PointLoads(&pointLoads);

   MemberIDType mbrID;
   Float64 location;
   GetMemberLocation(poi,pModelData,&mbrID,&location);

   // each load gets its own individual identifier... just use it's index
   IndexType nLoads;
   pointLoads->get_Count(&nLoads);
   LoadIDType loadID = (LoadIDType)(nLoads);

   CComPtr<IFem2dPointLoad> ptLoad;
   if ( FAILED(pointLoads->Create(loadID,mbrID,location,Fx,Fy,Mz,lotGlobal,&ptLoad)) )
   {
      ATLASSERT(false);
      return false;
   }

   return true;
}

bool CSegmentModelManager::CreateUniformLoad(IntervalIndexType intervalIdx, LoadCaseIDType lcid,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
   // must loop over the range of segments that this loading is applied to
   const CSegmentKey& segmentKey1(poi1.GetSegmentKey());
   const CSegmentKey& segmentKey2(poi2.GetSegmentKey());

   GET_IFACE(IPointOfInterest,pPoi);
   Float64 Xg1 = pPoi->ConvertPoiToGirderCoordinate(poi1);
   Float64 Xg2 = pPoi->ConvertPoiToGirderCoordinate(poi2);

   SegmentIndexType firstSegmentIdx = segmentKey1.segmentIndex;
   SegmentIndexType lastSegmentIdx  = segmentKey2.segmentIndex;

   for ( SegmentIndexType segIdx = firstSegmentIdx; segIdx <= lastSegmentIdx; segIdx++ )
   {
      CSegmentKey segmentKey(segmentKey1);
      segmentKey.segmentIndex = segIdx;

      // get the model for this segment
      CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);

      CComPtr<IFem2dLoadingCollection> loadings;
      pModelData->Model->get_Loadings(&loadings);

      CComPtr<IFem2dLoading> loading;
      loadings->Find(lcid,&loading);

      CComPtr<IFem2dDistributedLoadCollection> distributedLoads;
      loading->get_DistributedLoads(&distributedLoads);

      // use the loading index as its ID
      IndexType nLoads;
      distributedLoads->get_Count(&nLoads);
      LoadIDType loadID = (LoadIDType)nLoads;

      // determien the location where load starts/stops for this model
      MemberIDType mbrID1, mbrID2; 
      Float64 location1, location2;
      Float64 length1, length2; // length of mbrID1 and mbrID2

      if ( segIdx == firstSegmentIdx )
      {
         // loading starts in this fem model
         GetMemberLocation(poi1,pModelData,&mbrID1,&location1);
         CComPtr<IFem2dMemberCollection> members;
         pModelData->Model->get_Members(&members);
         CComPtr<IFem2dMember> member;
         members->Find(mbrID1,&member);
         member->get_Length(&length1);
      }
      else if ( segIdx < firstSegmentIdx )
      {
         // loading starts before this segment/fem model
         // so apply the load to the entire model
         CComPtr<IFem2dMemberCollection> members;
         pModelData->Model->get_Members(&members);
         CComPtr<IFem2dMember> member;
         members->get_Item(0,&member);
         member->get_ID(&mbrID1);
         member->get_Length(&length1);
         location1 = 0;
      }
      else
      {
         // loading starts after this segment
         continue;
      }

      if ( segIdx == lastSegmentIdx )
      {
         // loading ends in this fem model
         GetMemberLocation(poi2,pModelData,&mbrID2,&location2);
         CComPtr<IFem2dMemberCollection> members;
         pModelData->Model->get_Members(&members);
         CComPtr<IFem2dMember> member;
         members->Find(mbrID2,&member);
         member->get_Length(&length2);
      }
      else if ( segIdx < lastSegmentIdx )
      {
         // loading ends after this segment/fem model
         // so apply the load to the entire model
         CComPtr<IFem2dMemberCollection> members;
         pModelData->Model->get_Members(&members);
         IndexType nMembers;
         members->get_Count(&nMembers);
         CComPtr<IFem2dMember> member;
         members->get_Item(nMembers-1,&member);
         member->get_ID(&mbrID2);
         member->get_Length(&length2);
         location2 = length2;
      }
      else
      {
         // loading ends before this segment
         continue;
      }

      if ( mbrID1 != mbrID2 )
      {
         // load is applied over multiple FEM members
         for ( MemberIDType id = mbrID1; id <= mbrID2; id++ )
         {
            if ( id == mbrID1 )
            {
               if ( !IsEqual(location1,length1) )
               {
                  // model the load if it doesn't start and the end of member
                  if ( !IsZero(wx) )
                  {
                     CComPtr<IFem2dDistributedLoad> distLoad;
                     distributedLoads->Create(loadID++,mbrID1,loadDirFx,location1,length1,wx,wx,lotGlobal,&distLoad);
                  }

                  if ( !IsZero(wy) )
                  {
                     CComPtr<IFem2dDistributedLoad> distLoad;
                     distributedLoads->Create(loadID++,mbrID1,loadDirFy,location1,length1,wy,wy,lotGlobal,&distLoad);
                  }
               }
            }
            else if ( id == mbrID2 )
            {
               if ( !IsZero(location2) )
               {
                  // model the load if it doesn't end at the start of this member
                  if ( !IsZero(wx) )
                  {
                     CComPtr<IFem2dDistributedLoad> distLoad;
                     distributedLoads->Create(loadID++,mbrID2,loadDirFx,0.0,location2,wx,wx,lotGlobal,&distLoad);
                  }

                  if ( !IsZero(wy) )
                  {
                     CComPtr<IFem2dDistributedLoad> distLoad;
                     distributedLoads->Create(loadID++,mbrID2,loadDirFx,0.0,location2,wy,wy,lotGlobal,&distLoad);
                  }
               }
            }
            else
            {
               // this is an intermediate member between mbrID1 && mbrID2
               // the load goes over the entire length of the member
               ATLASSERT(mbrID1 < id && id < mbrID2);
               if ( !IsZero(wx) )
               {
                  CComPtr<IFem2dDistributedLoad> distLoad;
                  distributedLoads->Create(loadID++,mbrID2,loadDirFx,0.0,-1,wx,wx,lotGlobal,&distLoad);
               }

               if ( !IsZero(wy) )
               {
                  CComPtr<IFem2dDistributedLoad> distLoad;
                  distributedLoads->Create(loadID++,mbrID2,loadDirFx,0.0,-1,wy,wy,lotGlobal,&distLoad);
               }
            }
         } // next fem member
      } 
      else
      {
         // load is applied to a single member
         if ( !IsZero(wx) )
         {
            CComPtr<IFem2dDistributedLoad> distLoad;
            distributedLoads->Create(loadID++,mbrID1,loadDirFx,location1,location2,wx,wx,lotGlobal,&distLoad);
         }

         if ( !IsZero(wy) )
         {
            CComPtr<IFem2dDistributedLoad> distLoad;
            distributedLoads->Create(loadID++,mbrID1,loadDirFy,location1,location2,wy,wy,lotGlobal,&distLoad);
         }
      }
   } // next segment

   return true;
}

bool CSegmentModelManager::CreateInitialStrainLoad(IntervalIndexType intervalIdx, LoadCaseIDType lcid,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
   if ( poi1.AtSamePlace(poi2) )
   {
      // POI's are at the same point, there is no need to add a loading that has no effect
      return true;
   }

   // must loop over the range of segments that this loading is applied to
   const CSegmentKey& segmentKey1(poi1.GetSegmentKey());
   const CSegmentKey& segmentKey2(poi2.GetSegmentKey());

   SegmentIndexType firstSegmentIdx = segmentKey1.segmentIndex;
   SegmentIndexType lastSegmentIdx  = segmentKey2.segmentIndex;

   for ( SegmentIndexType segIdx = firstSegmentIdx; segIdx <= lastSegmentIdx; segIdx++ )
   {
      CSegmentKey segmentKey(segmentKey1);
      segmentKey.segmentIndex = segIdx;

      // get the model for this segment
      CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);

      CComPtr<IFem2dLoadingCollection> loadings;
      pModelData->Model->get_Loadings(&loadings);

      CComPtr<IFem2dLoading> loading;
      loadings->Find(lcid,&loading);

      CComPtr<IFem2dMemberStrainCollection> memberStrains;
      loading->get_MemberStrains(&memberStrains);

      // use the loading index as its ID
      IndexType nLoads;
      memberStrains->get_Count(&nLoads);
      LoadIDType loadID = (LoadIDType)nLoads;

      // determine the location where load starts/stops for this model
      MemberIDType mbrID1, mbrID2; 
      Float64 location1, location2;
      Float64 lengthMbr1, lengthMbr2; // length of mbrID1 and mbrID2

      if ( segIdx == firstSegmentIdx )
      {
         // loading starts in this fem model
         GetMemberLocation(poi1,pModelData,&mbrID1,&location1);
         CComPtr<IFem2dMemberCollection> members;
         pModelData->Model->get_Members(&members);
         CComPtr<IFem2dMember> member;
         members->Find(mbrID1,&member);
         member->get_Length(&lengthMbr1);
      }
      else if ( segIdx < firstSegmentIdx )
      {
         // loading starts before this segment/fem model
         // so apply the load to the entire model
         CComPtr<IFem2dMemberCollection> members;
         pModelData->Model->get_Members(&members);
         CComPtr<IFem2dMember> member;
         members->get_Item(0,&member);
         member->get_ID(&mbrID1);
         member->get_Length(&lengthMbr1);
         location1 = 0;
      }
      else
      {
         // loading starts after this segment
         continue;
      }

      if ( segIdx == lastSegmentIdx )
      {
         // loading ends in this fem model
         GetMemberLocation(poi2,pModelData,&mbrID2,&location2);
         CComPtr<IFem2dMemberCollection> members;
         pModelData->Model->get_Members(&members);
         CComPtr<IFem2dMember> member;
         members->Find(mbrID2,&member);
         member->get_Length(&lengthMbr2);
      }
      else if ( segIdx < lastSegmentIdx )
      {
         // loading ends after this segment/fem model
         // so apply the load to the entire model
         CComPtr<IFem2dMemberCollection> members;
         pModelData->Model->get_Members(&members);
         IndexType nMembers;
         members->get_Count(&nMembers);
         CComPtr<IFem2dMember> member;
         members->get_Item(nMembers-1,&member);
         member->get_ID(&mbrID2);
         member->get_Length(&lengthMbr2);
         location2 = lengthMbr2;
      }
      else
      {
         // loading ends before this segment
         continue;
      }

      if ( mbrID1 != mbrID2 )
      {
         // load is applied over multiple FEM members
         for ( MemberIDType id = mbrID1; id <= mbrID2; id++ )
         {
            if ( id == mbrID1 )
            {
               if ( !IsEqual(location1,lengthMbr1) && location1 != -1 )
               {
                  // model the load if it doesn't start and the end of member
                  CComPtr<IFem2dMemberStrain> strainLoad;
                  memberStrains->Create(loadID++,mbrID1,location1,lengthMbr1,e,r,&strainLoad);
               }
            }
            else if ( id == mbrID2 )
            {
               if ( !IsZero(location2) )
               {
                  // model the load if it doesn't end at the start of this member
                  CComPtr<IFem2dMemberStrain> strainLoad;
                  memberStrains->Create(loadID++,mbrID2,0.0,location2,e,r,&strainLoad);
               }
            }
            else
            {
               // this is an intermediate member between mbrID1 && mbrID2
               // the load goes over the entire length of the member
               ATLASSERT(mbrID1 < id && id < mbrID2);
               CComPtr<IFem2dMemberStrain> strainLoad;
               memberStrains->Create(loadID++,id,0,-1,e,r,&strainLoad);
            }
         } // next fem member
      } 
      else
      {
         // load is applied to a single member
         CComPtr<IFem2dMemberStrain> strainLoad;
         memberStrains->Create(loadID++,mbrID1,location1,location2,e,r,&strainLoad);
      }
   } // next segment

   return true;
}

bool CSegmentModelManager::VerifyPoi(const PoiList& vPoi) const
{
   GET_IFACE(IPointOfInterest, pPoi);
   std::vector<CSegmentKey> segmentKeys;
   pPoi->GetSegmentKeys(vPoi, &segmentKeys);
   return (segmentKeys.size() == 1) ? true : false;
}

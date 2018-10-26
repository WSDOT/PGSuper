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
#include "SegmentModelManager.h"
#include "PrestressTool.h"

#include <EAF\EAFAutoProgress.h>
#include <PgsExt\GirderModelFactory.h>
#include <PgsExt\LoadFactors.h>

#include <IFace\Intervals.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>

#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\PrecastSegmentData.h>

const LoadCaseIDType g_lcidStraight            = 0;
const LoadCaseIDType g_lcidHarped              = 1;
const LoadCaseIDType g_lcidTemporary           = 2;
const LoadCaseIDType g_lcidGirder              = 3;    // FEM Loading ID for girder self weight load
const LoadCaseIDType g_lcidUnitLoadBase        = -1000; // Starting FEM loading ID for unit loads add -1 for each poi loaded

CSegmentModelManager::CSegmentModelManager(SHARED_LOGFILE lf,IBroker* pBroker) :
LOGFILE(lf),m_pBroker(pBroker)
{
}

void CSegmentModelManager::Clear()
{
   m_ReleaseModels.clear();
   m_StorageModels.clear();
}

std::vector<EquivPretensionLoad> CSegmentModelManager::GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType)
{
   std::vector<EquivPretensionLoad> equivLoads;

   Float64 Ms;    // Concentrated moments at straight strand debond location
   Float64 Msl;   // Concentrated moments at straight strand bond locations (left)
   Float64 Msr;   // Concentrated moments at straight strand bond locations (right)
   Float64 Mhl;   // Concentrated moments at ends of beam for eccentric prestress forces from harped strands (left)
   Float64 Mhr;   // Concentrated moments at ends of beam for eccentric prestress forces from harped strands (right)
   Float64 Mtl;   // Concentrated moments at temporary straight strand bond locations (left)
   Float64 Mtr;   // Concentrated moments at temporary straight strand bond locations (right)
   Float64 Nl;    // Vertical loads at left harping point
   Float64 Nr;    // Vertical loads at right harping point
   Float64 Ps;    // Force in straight strands (varies with location due to debonding)
   Float64 Ph;    // Force in harped strands
   Float64 Pt;    // Force in temporary strands
   Float64 ecc_harped_start; // Eccentricity of harped strands at end of girder
   Float64 ecc_harped_end;  // Eccentricity of harped strands at end of girder
   Float64 ecc_harped_hp1;  // Eccentricity of harped strand at harping point (left)
   Float64 ecc_harped_hp2;  // Eccentricity of harped strand at harping point (right)
   Float64 ecc_straight_start;  // Eccentricity of straight strands (left)
   Float64 ecc_straight_end;    // Eccentricity of straight strands (right)
   Float64 ecc_straight_debond; // Eccentricity of straight strands (location varies)
   Float64 ecc_temporary_start; // Eccentricity of temporary strands (left)
   Float64 ecc_temporary_end;   // Eccentricity of temporary strands (right)
   Float64 hp1; // Location of left harping point
   Float64 hp2; // Location of right harping point
   Float64 Ls;  // Length of segment

   // These are the interfaces we will be using
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   Float64 E = pMaterial->GetSegmentEc(segmentKey,releaseIntervalIdx);

   Ls = pBridge->GetSegmentLength(segmentKey);


   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(segmentKey,POI_RELEASED_SEGMENT | POI_5L) );
   ATLASSERT( vPoi.size() == 1 );
   pgsPointOfInterest mid_span_poi( vPoi.front() );

#if defined _DEBUG
   ATLASSERT( mid_span_poi.IsMidSpan(POI_RELEASED_SEGMENT) == true );
#endif

   pgsPointOfInterest poiStart(segmentKey,0.0);
   pgsPointOfInterest poiEnd(segmentKey,Ls);

   if ( strandType == pgsTypes::Harped && 0 < pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped) )
   {
      hp1 = 0;
      hp2 = 0;
      Nl  = 0;
      Nr  = 0;
      Mhl = 0;
      Mhr = 0;

      // Determine the prestress force
      GET_IFACE(IPretensionForce,pPrestressForce);
      Ph = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Harped,releaseIntervalIdx,pgsTypes::Start);

      // get harping point locations
      vPoi.clear(); // recycle the vector
      vPoi = pIPoi->GetPointsOfInterest(segmentKey,POI_HARPINGPOINT);
      ATLASSERT( 0 <= vPoi.size() && vPoi.size() < 3 );
      pgsPointOfInterest hp1_poi;
      pgsPointOfInterest hp2_poi;
      if ( vPoi.size() == 0 )
      {
         hp1_poi.SetSegmentKey(segmentKey);
         hp1_poi.SetDistFromStart(0.0);
         hp2_poi.SetSegmentKey(segmentKey);
         hp2_poi.SetDistFromStart(0.0);
         hp1 = hp1_poi.GetDistFromStart();
         hp2 = hp2_poi.GetDistFromStart();
      }
      else if ( vPoi.size() == 1 )
      { 
         std::vector<pgsPointOfInterest>::const_iterator iter( vPoi.begin() );
         hp1_poi = *iter++;
         hp2_poi = hp1_poi;
         hp1 = hp1_poi.GetDistFromStart();
         hp2 = hp2_poi.GetDistFromStart();
      }
      else
      {
         std::vector<pgsPointOfInterest>::const_iterator iter( vPoi.begin() );
         hp1_poi = *iter++;
         hp2_poi = *iter++;
         hp1 = hp1_poi.GetDistFromStart();
         hp2 = hp2_poi.GetDistFromStart();
      }

      // Determine eccentricity of harped strands at end and harp point
      // (assumes eccentricities are the same at each harp point - which they are because
      // of the way the input is defined)
      Float64 nHs_effective;

      ecc_harped_start = pStrandGeom->GetHsEccentricity(releaseIntervalIdx, poiStart, &nHs_effective);
      ecc_harped_hp1   = pStrandGeom->GetHsEccentricity(releaseIntervalIdx, hp1_poi,  &nHs_effective);
      ecc_harped_hp2   = pStrandGeom->GetHsEccentricity(releaseIntervalIdx, hp2_poi,  &nHs_effective);
      ecc_harped_end   = pStrandGeom->GetHsEccentricity(releaseIntervalIdx, poiEnd,   &nHs_effective);

      // Determine equivalent loads

      // moment
      Mhl = Ph*ecc_harped_start;
      Mhr = Ph*ecc_harped_end;

      // upward force
      Float64 e_prime_start, e_prime_end;
      e_prime_start = ecc_harped_hp1 - ecc_harped_start;
      e_prime_start = IsZero(e_prime_start) ? 0 : e_prime_start;

      e_prime_end = ecc_harped_hp2 - ecc_harped_end;
      e_prime_end = IsZero(e_prime_end) ? 0 : e_prime_end;

      Nl = IsZero(hp1)    ? 0 : Ph*e_prime_start/hp1;
      Nr = IsZero(Ls-hp2) ? 0 : Ph*e_prime_end/(Ls-hp2);

      EquivPretensionLoad startMoment;
      startMoment.Xs = 0;
      startMoment.P = 0;
      startMoment.M = Mhl;

      EquivPretensionLoad leftHpLoad;
      leftHpLoad.Xs = hp1;
      leftHpLoad.P  = Nl;
      leftHpLoad.M  = 0;

      EquivPretensionLoad rightHpLoad;
      rightHpLoad.Xs = hp2;
      rightHpLoad.P  = Nr;
      rightHpLoad.M  = 0;

      EquivPretensionLoad endMoment;
      endMoment.Xs = Ls;
      endMoment.P = 0;
      endMoment.M = -Mhr;

      equivLoads.push_back(startMoment);
      equivLoads.push_back(leftHpLoad);
      equivLoads.push_back(rightHpLoad);
      equivLoads.push_back(endMoment);
   }
   else if ( strandType == pgsTypes::Straight && 0 < pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight))
   {
      GET_IFACE(IPretensionForce,pPrestressForce);

      Float64 nSsEffective;
      ecc_straight_start = pStrandGeom->GetSsEccentricity(releaseIntervalIdx, poiStart, &nSsEffective);
      ecc_straight_end   = pStrandGeom->GetSsEccentricity(releaseIntervalIdx, poiEnd,   &nSsEffective);
      Ps = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::Start);

      Msl = Ps*ecc_straight_start;
      Msr = Ps*ecc_straight_end;

      EquivPretensionLoad startMoment;
      startMoment.Xs = 0;
      startMoment.P  = 0;
      startMoment.M  = Msl;

      equivLoads.push_back(startMoment);

      EquivPretensionLoad endMoment;
      endMoment.Xs = Ls;
      endMoment.P  = 0;
      endMoment.M  = -Msr;

      equivLoads.push_back(endMoment);

      // debonding
      for (int end = 0; end < 2; end++)
      {
         // left end first, right second
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)end;
         Float64 sign = (end == 0 ?  1 : -1);
         IndexType nSections = pStrandGeom->GetNumDebondSections(segmentKey,endType,pgsTypes::Straight);
         for ( IndexType sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
         {
            Float64 location = pStrandGeom->GetDebondSection(segmentKey,endType,sectionIdx,pgsTypes::Straight);
            if ( location < 0 || Ls < location )
            {
               continue; // bond occurs after the end of the segment... skip this one
            }

            StrandIndexType nDebondedAtSection = pStrandGeom->GetNumDebondedStrandsAtSection(segmentKey,endType,sectionIdx,pgsTypes::Straight);

            // nDebonded is to be interperted as the number of strands that become bonded at this section
            // (ok, not at this section but lt past this section)
            Float64 nSsEffective;

            Ps = nDebondedAtSection*pPrestressForce->GetPrestressForcePerStrand(mid_span_poi,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::Start);
            ecc_straight_debond = pStrandGeom->GetSsEccentricity(releaseIntervalIdx, pgsPointOfInterest(segmentKey,location), &nSsEffective);

            Ms = sign*Ps*ecc_straight_debond;

            EquivPretensionLoad debondLocationLoad;
            debondLocationLoad.Xs = location;
            debondLocationLoad.P  = 0;
            debondLocationLoad.M  = Ms;

            equivLoads.push_back(debondLocationLoad);
         }
      }
   }
   else if ( strandType == pgsTypes::Temporary && 0 < pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary) )
   {
      GET_IFACE(IPretensionForce,pPrestressForce);

      Float64 nTsEffective;
      Pt = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::Start);
      ecc_temporary_start = pStrandGeom->GetTempEccentricity(releaseIntervalIdx,poiStart,&nTsEffective);
      ecc_temporary_end   = pStrandGeom->GetTempEccentricity(releaseIntervalIdx,poiEnd,  &nTsEffective);

      Mtl = Pt*ecc_temporary_start;
      Mtr = Pt*ecc_temporary_end;

      EquivPretensionLoad startMoment;
      startMoment.Xs = 0;
      startMoment.P  = 0;
      startMoment.M  = Mtl;

      EquivPretensionLoad endMoment;
      endMoment.Xs = Ls;
      endMoment.P  = 0;
      endMoment.M  = -Mtr;

      equivLoads.push_back(endMoment);
   }
   else
   {
      ATLASSERT(strandType != pgsTypes::Permanent); // can't be permanent
   }

   return equivLoads;
}

sysSectionValue CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> shears = GetShear(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(shears.size() == 1);

   return shears.front();
}

Float64 CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> moments = GetMoment(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(moments.size() == 1);

   return moments.front();
}

Float64 CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> deflections = GetDeflection(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(deflections.size() == 1);

   return deflections.front();
}

Float64 CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> rotations = GetRotation(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(rotations.size() == 1);

   return rotations.front();
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop, fBot;
   GetStress(intervalIdx,pfType,vPoi,resultsType,topLocation,botLocation,&fTop,&fBot);

   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);

   *pfTop = fTop.front();
   *pfBot = fBot.front();
}

void CSegmentModelManager::GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,ProductForceType pfType,ResultsType resultsType,Float64* pRleft,Float64* pRright)
{
   if ( pfType != pftGirder )
   {
      // only girder load creates reaction
      *pRleft = 0;
      *pRright = 0;
      return;
   }

   if ( resultsType == rtIncremental )
   {
      Float64 prevRleft, prevRright;
      Float64 thisRleft, thisRright;
      GetReaction(segmentKey,intervalIdx-1,pfType,rtCumulative,&prevRleft,&prevRright);
      GetReaction(segmentKey,intervalIdx,  pfType,rtCumulative,&thisRleft,&thisRright);
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
            HRESULT hr = results->ComputeReactions(g_lcidGirder,jntID,&fx,&fy,&mz);
            ATLASSERT(SUCCEEDED(hr));

            reactions.push_back(fy);
         }
      }

      ATLASSERT(reactions.size() == 2);

      *pRleft  = reactions.front();
      *pRright = reactions.back();
   }
}

void CSegmentModelManager::GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,ResultsType resultsType,Float64* pRleft,Float64* pRright)
{
   if ( comboType == lcDC )
   {
      // segment models only have girder self-weight load which is a DC load
      GetReaction(segmentKey,intervalIdx,pftGirder,resultsType,pRleft,pRright);
   }
   else
   {
      *pRleft  = 0;
      *pRright = 0;
   }
}

void CSegmentModelManager::GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,Float64* pRleftMin,Float64* pRleftMax,Float64* pRrightMin,Float64* pRrightMax)
{
   Float64 Rleft, Rright;
   GetReaction(segmentKey,intervalIdx,lcDC,rtCumulative,&Rleft,&Rright);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin = pLoadFactors->DCmin[limitState];
   Float64 gDCMax = pLoadFactors->DCmax[limitState];

   *pRleftMin = gDCMin*Rleft;
   *pRleftMax = gDCMax*Rleft;

   *pRrightMin = gDCMin*Rright;
   *pRrightMax = gDCMax*Rright;
}

Float64 CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,ProductForceType pfType,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType)
{
   if ( pfType != pftGirder )
   {
      // only girder load creates reaction
      return 0;
   }

   GirderIndexType gdrIdx = girderKey.girderIndex;

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
   HRESULT hr = results->ComputeReactions(g_lcidGirder,jointID,&fx,&fy,&mz);
   ATLASSERT(SUCCEEDED(hr));
   return fy;
}

std::vector<sysSectionValue> CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   // NOTE: shear due to prestressing is a little funny. It is sufficient to use the shear from the
   // equivalent loading.
   CSegmentKey segmentKey = vPoi.front().GetSegmentKey();

   std::vector<sysSectionValue> vFx,vFy,vMz;
   std::vector<Float64> vDx,vDy,vRz;
   GetSectionResults(intervalIdx, pfType, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );

   return vFy;
}

std::vector<Float64> CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   CSegmentKey segmentKey = vPoi.front().GetSegmentKey();

   std::vector<sysSectionValue> vFx,vFy,vMz;
   std::vector<Float64> vDx,vDy,vRz;
   GetSectionResults(intervalIdx, pfType, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );

   std::vector<Float64> moments;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   std::vector<sysSectionValue>::iterator momentIter(vMz.begin());
   for ( ; iter != end; iter++, momentIter++ )
   {
      sysSectionValue& mz = *momentIter;
      const pgsPointOfInterest& poi(*iter);
      if ( IsZero(poi.GetDistFromStart()) )
      {
         moments.push_back(-mz.Right());
      }
      else
      {
         moments.push_back(mz.Left());
      }
   }

   return moments;
}

std::vector<Float64> CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   // NOTE: Deflections for prestress are based on the equivalent loading. This is sufficient.
   // If we need to be more precise, we can use the method of virtual work and integrate the
   // M/EI diagram for the prestress moment based on P*e.

   std::vector<sysSectionValue> vFx,vFy,vMz;
   std::vector<Float64> vDx,vDy,vRz;
   if ( pfType == pftPretension )
   {
      // For elastic analysis we assume that the deflection due to the pretension force does not
      // change with time. The only change in deflection that we account for is due to rigid body
      // displacement of the girder segment. The girder segment is on different support locations
      // at release, during storage, and after erection. However, the deflected shape of the girder
      // due to prestress does not change. We account for the rigid body displacement by deducting
      // the release deflection at the support locations from the deflections.
      GET_IFACE(IIntervals,pIntervals);
      GET_IFACE(IPointOfInterest,pPoi);
      std::list<std::vector<pgsPointOfInterest>> sPoi( pPoi->GroupBySegment(vPoi) );
      std::list<std::vector<pgsPointOfInterest>>::iterator iter(sPoi.begin());
      std::list<std::vector<pgsPointOfInterest>>::iterator end(sPoi.end());
      for ( ; iter != end; iter++ )
      {
         std::vector<pgsPointOfInterest>& vPoiThisSegment(*iter);

         CSegmentKey segmentKey = vPoiThisSegment.front().GetSegmentKey();
      
         // get the Pretension deflection at release
         std::vector<Float64> vDyThisSegment;
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
         GetSectionResults(releaseIntervalIdx, pfType, vPoiThisSegment, resultsType, &vFx, &vFy, &vMz, &vDx, &vDyThisSegment, &vRz );

         IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
         if ( storageIntervalIdx <= intervalIdx )
         {
            // the current interval is at or after the segment is placed into storage so the
            // support locations have changed
            
            // get the segment support locations
            std::vector<pgsPointOfInterest> vStoragePoi(pPoi->GetPointsOfInterest(segmentKey,POI_STORAGE_SEGMENT));
            std::vector<pgsPointOfInterest> vStorageSupports;
            vStorageSupports.push_back(vStoragePoi.front());
            vStorageSupports.push_back(vStoragePoi.back());

            // get the deflections at the segment support locations
            std::vector<Float64> DStorageSupports = GetDeflection(releaseIntervalIdx,pftPretension,vStorageSupports,rtCumulative);

            // get the values for use in the linear interpoloation
            Float64 X1 = vStorageSupports.front().GetDistFromStart();
            Float64 X2 = vStorageSupports.back().GetDistFromStart();
            Float64 Y1 = DStorageSupports.front();
            Float64 Y2 = DStorageSupports.back();

            // adjust the deflections by deducting the deflection at the support location
            // this makes the deflection at the support location zero, as it should be
            std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoiThisSegment.begin());
            std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoiThisSegment.end());
            std::vector<Float64>::iterator DyIter(vDyThisSegment.begin());
            for (; poiIter != poiIterEnd; poiIter++, DyIter++ )
            {
               const pgsPointOfInterest& poi(*poiIter);
               Float64 d = ::LinInterp( poi.GetDistFromStart(), Y1, Y2, X2-X1);

               Float64 Dy = *DyIter;
               Dy -= d;
               
               vDy.push_back(Dy);
            }
         }
         else
         {
            // before storage... the deflections don't need to be adjusted
            vDy.insert(vDy.end(),vDyThisSegment.begin(),vDyThisSegment.end());
         }
      }
   }
   else
   {
      GetSectionResults(intervalIdx, pfType, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );
   }


   return vDy;
}

std::vector<Float64> CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   CSegmentKey segmentKey = vPoi.front().GetSegmentKey();

   std::vector<sysSectionValue> vFx,vFy,vMz;
   std::vector<Float64> vDx,vDy,vRz;
   GetSectionResults(intervalIdx, pfType, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );

   return vRz;
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   // NOTE: We want streses based on P/A + P*e/S due to prestressing where P accounts for development and losses.
   // The FEM model can't account for this so we have to use a special function to get
   // stress in the girder segment due to prestressing (there are moments in the FEM model based on the equivalent prestress loading,
   // however the FEM model doesn't have equivalent axial forces modeled)
   if ( pfType == pftPretension )
   {
      GetPrestressSectionStresses(intervalIdx,resultsType,vPoi,topLocation,botLocation,pfTop,pfBot);
   }
   else
   {
      GetSectionStresses(intervalIdx,pfType,resultsType,vPoi,topLocation,botLocation,pfTop,pfBot);
   }
}

std::vector<Float64> CSegmentModelManager::GetUnitLoadMoment(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,const pgsPointOfInterest& unitLoadPOI)
{
#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
   ATLASSERT(intervalIdx < erectionIntervalIdx); // should be getting results from the girder model
#endif // _DEBUG

   std::vector<Float64> moments;
   moments.reserve(vPoi.size());

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi(*iter);
      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);
      std::map<IDType,IDType>::iterator found(pModelData->UnitLoadIDMap.find(unitLoadPOI.GetID()));
      if ( found == pModelData->UnitLoadIDMap.end() )
      {
         // unit load isn't on this segment... there isn't any moment in this segment due to the unit load
         moments.push_back(0.0);
      }
      else
      {
         std::vector<pgsPointOfInterest> thePoi;
         thePoi.push_back(poi);

         LoadCaseIDType lcid = found->second;

         std::vector<sysSectionValue> vFx,vFy,vMz;
         std::vector<Float64> vDx,vDy,vRz;
         GetSectionResults(intervalIdx, lcid, thePoi, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );

         sysSectionValue& mz = vMz.front();
         ATLASSERT(IsEqual(mz.Left(),-mz.Right(),0.0001));

         moments.push_back(mz.Left());
      }
   }

   return moments;
}

std::vector<sysSectionValue> CSegmentModelManager::GetUnitCoupleMoment(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,const pgsPointOfInterest& unitMomentPOI)
{
#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
   ATLASSERT(intervalIdx < erectionIntervalIdx); // should be getting results from the girder model
#endif // _DEBUG

   std::vector<sysSectionValue> moments;
   moments.reserve(vPoi.size());

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi(*iter);
      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);
      std::map<IDType,IDType>::iterator found(pModelData->UnitMomentIDMap.find(unitMomentPOI.GetID()));
      if ( found == pModelData->UnitMomentIDMap.end() )
      {
         // unit load isn't on this segment... there isn't any moment in this segment due to the unit load
         moments.push_back(0.0);
      }
      else
      {
         std::vector<pgsPointOfInterest> thePoi;
         thePoi.push_back(poi);

         LoadCaseIDType lcid = found->second;

         std::vector<sysSectionValue> vFx,vFy,vMz;
         std::vector<Float64> vDx,vDy,vRz;
         GetSectionResults(intervalIdx, lcid, thePoi, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );

         sysSectionValue& mz = vMz.front();
         moments.push_back(sysSectionValue(mz.Left(),-mz.Right()));
      }
   }

   return moments;
}

sysSectionValue CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> shears = GetShear(intervalIdx,combo,vPoi,resultsType);
   ATLASSERT(shears.size() == 1);
   return shears.front();
}

Float64 CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> moments = GetMoment(intervalIdx,combo,vPoi,resultsType);
   ATLASSERT(moments.size() == 1);
   return moments.front();
}

Float64 CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> deflections = GetDeflection(intervalIdx,combo,vPoi,resultsType);
   ATLASSERT(deflections.size() == 1);
   return deflections.front();
}

Float64 CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> rotations = GetRotation(intervalIdx,combo,vPoi,resultsType);
   ATLASSERT(rotations.size() == 1);
   return rotations.front();
}

Float64 CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,LoadingCombinationType combo,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType)
{
   if ( combo == lcDC )
   {
      // segment models only have girder self-weight load which is a DC load
      return GetReaction(intervalIdx,pftGirder,pierIdx,girderKey,resultsType);
   }
   else
   {
      return 0;
   }
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const pgsPointOfInterest& poi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop, fBot;
   GetStress(intervalIdx,combo,vPoi,resultsType,topLocation,botLocation,&fTop,&fBot);

   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);

   *pfTop = fTop.front();
   *pfBot = fBot.front();
}

std::vector<sysSectionValue> CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   if ( combo == lcDC )
   {
      return GetShear(intervalIdx,pftGirder,vPoi,resultsType);
   }
   else
   {
      std::vector<sysSectionValue> vResults;
      vResults.resize(vPoi.size(),sysSectionValue(0,0));
      return vResults;
   }
}

std::vector<Float64> CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   if ( combo == lcDC )
   {
      return GetMoment(intervalIdx,pftGirder,vPoi,resultsType);
   }
   else
   {
      std::vector<Float64> vResults;
      vResults.resize(vPoi.size(),0.0);
      return vResults;
   }
}

std::vector<Float64> CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   if ( combo == lcDC )
   {
      return GetDeflection(intervalIdx,pftGirder,vPoi,resultsType);
   }
   else
   {
      std::vector<Float64> vResults;
      vResults.resize(vPoi.size(),0.0);
      return vResults;
   }
}

std::vector<Float64> CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   if ( combo == lcDC )
   {
      return GetRotation(intervalIdx,pftGirder,vPoi,resultsType);
   }
   else
   {
      std::vector<Float64> vResults;
      vResults.resize(vPoi.size(),0.0);
      return vResults;
   }
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,LoadingCombinationType combo,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   if ( combo == lcDC )
   {
      GetStress(intervalIdx,pftGirder,vPoi,resultsType,topLocation,botLocation,pfTop,pfBot);
   }
   else
   {
      pfTop->resize(vPoi.size(),0.0);
      pfBot->resize(vPoi.size(),0.0);
   }
}

////////////////////////////
void CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,sysSectionValue* pMin,sysSectionValue* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> vMin, vMax;
   GetShear(intervalIdx,ls,vPoi,&vMin,&vMax);

   ATLASSERT(vMin.size() == 1);
   ATLASSERT(vMax.size() == 1);

   *pMin = vMin.front();
   *pMax = vMax.front();
}

void CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> vMin, vMax;
   GetMoment(intervalIdx,ls,vPoi,&vMin,&vMax);

   ATLASSERT(vMin.size() == 1);
   ATLASSERT(vMax.size() == 1);

   *pMin = vMin.front();
   *pMax = vMax.front();
}

void CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,bool bIncludePrestress,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> vMin, vMax;
   GetDeflection(intervalIdx,ls,vPoi,bIncludePrestress,&vMin,&vMax);

   ATLASSERT(vMin.size() == 1);
   ATLASSERT(vMax.size() == 1);

   *pMin = vMin.front();
   *pMax = vMax.front();
}

void CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,PierIndexType pierIdx,const CGirderKey& girderKey,bool bIncludeImpact,Float64* pMin,Float64* pMax)
{
   Float64 R = GetReaction(intervalIdx,lcDC,pierIdx,girderKey,rtCumulative);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin = pLoadFactors->DCmin[ls];
   Float64 gDCMax = pLoadFactors->DCmax[ls];

   *pMin = gDCMin*R;
   *pMax = gDCMax*R;
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> vMin, vMax;
   GetStress(intervalIdx,ls,vPoi,stressLocation,bIncludePrestress,&vMin,&vMax);

   ATLASSERT(vMin.size() == 1);
   ATLASSERT(vMax.size() == 1);

   *pMin = vMin.front();
   *pMax = vMax.front();
}

void CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const std::vector<pgsPointOfInterest>& vPoi,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax)
{
   std::vector<sysSectionValue> shears = GetShear(intervalIdx,lcDC,vPoi,rtCumulative);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin = pLoadFactors->DCmin[ls];
   Float64 gDCMax = pLoadFactors->DCmax[ls];

   std::transform(shears.begin(),shears.end(),std::back_inserter(*pMin),FactorElements<sysSectionValue>(gDCMin));
   std::transform(shears.begin(),shears.end(),std::back_inserter(*pMax),FactorElements<sysSectionValue>(gDCMax));
}

void CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const std::vector<pgsPointOfInterest>& vPoi,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   std::vector<Float64> moments = GetMoment(intervalIdx,lcDC,vPoi,rtCumulative);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin = pLoadFactors->DCmin[ls];
   Float64 gDCMax = pLoadFactors->DCmax[ls];

   std::transform(moments.begin(),moments.end(),std::back_inserter(*pMin),FactorElements<Float64>(gDCMin));
   std::transform(moments.begin(),moments.end(),std::back_inserter(*pMax),FactorElements<Float64>(gDCMax));
}

void CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const std::vector<pgsPointOfInterest>& vPoi,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   std::vector<Float64> deflection = GetDeflection(intervalIdx,lcDC,vPoi,rtCumulative);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin = pLoadFactors->DCmin[ls];
   Float64 gDCMax = pLoadFactors->DCmax[ls];

   std::transform(deflection.begin(),deflection.end(),std::back_inserter(*pMin),FactorElements<Float64>(gDCMin));
   std::transform(deflection.begin(),deflection.end(),std::back_inserter(*pMax),FactorElements<Float64>(gDCMax));

   if ( bIncludePrestress )
   {
      std::vector<Float64> ps = GetDeflection(intervalIdx,pftPretension,vPoi,rtCumulative);
      std::transform(ps.begin(),ps.end(),pMin->begin(),pMin->begin(),std::plus<Float64>());
      std::transform(ps.begin(),ps.end(),pMax->begin(),pMax->begin(),std::plus<Float64>());
   }
}

void CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   std::vector<Float64> rotation = GetRotation(intervalIdx,lcDC,vPoi,rtCumulative);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin = pLoadFactors->DCmin[limitState];
   Float64 gDCMax = pLoadFactors->DCmax[limitState];

   std::transform(rotation.begin(),rotation.end(),std::back_inserter(*pMin),FactorElements<Float64>(gDCMin));
   std::transform(rotation.begin(),rotation.end(),std::back_inserter(*pMax),FactorElements<Float64>(gDCMax));

   if ( bIncludePrestress )
   {
      std::vector<Float64> ps = GetRotation(intervalIdx,pftPretension,vPoi,rtCumulative);
      std::transform(ps.begin(),ps.end(),pMin->begin(),pMin->begin(),std::plus<Float64>());
      std::transform(ps.begin(),ps.end(),pMax->begin(),pMax->begin(),std::plus<Float64>());
   }
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation stressLocation,bool bIncludePrestress,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   pgsTypes::StressLocation topLocation = IsGirderStressLocation(stressLocation) ? pgsTypes::TopGirder    : pgsTypes::TopDeck;
   pgsTypes::StressLocation botLocation = IsGirderStressLocation(stressLocation) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck;
   std::vector<Float64> fTop, fBot;
   GetStress(intervalIdx,lcDC,vPoi,rtCumulative,topLocation,botLocation,&fTop,&fBot);

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gDCMin = pLoadFactors->DCmin[ls];
   Float64 gDCMax = pLoadFactors->DCmax[ls];

   if ( IsTopStressLocation(stressLocation) )
   {
      std::transform(fTop.begin(),fTop.end(),std::back_inserter(*pMin),FactorElements<Float64>(gDCMin));
      std::transform(fTop.begin(),fTop.end(),std::back_inserter(*pMax),FactorElements<Float64>(gDCMax));
   }
   else
   {
      std::transform(fBot.begin(),fBot.end(),std::back_inserter(*pMin),FactorElements<Float64>(gDCMin));
      std::transform(fBot.begin(),fBot.end(),std::back_inserter(*pMax),FactorElements<Float64>(gDCMax));
   }

   if ( bIncludePrestress )
   {
      std::vector<Float64> fTopPS, fBotPS;
      GetStress(intervalIdx,pftPretension,vPoi,rtCumulative,topLocation,botLocation,&fTopPS,&fBotPS);
      if ( IsTopStressLocation(stressLocation) )
      {
         std::transform(pMin->begin(),pMin->end(),fTopPS.begin(),pMin->begin(),std::plus<Float64>());
         std::transform(pMax->begin(),pMax->end(),fTopPS.begin(),pMax->begin(),std::plus<Float64>());
      }
      else
      {
         std::transform(pMin->begin(),pMin->end(),fBotPS.begin(),pMin->begin(),std::plus<Float64>());
         std::transform(pMax->begin(),pMax->end(),fBotPS.begin(),pMax->begin(),std::plus<Float64>());
      }
   }
}

///////////////////////////
// IExternalLoading
bool CSegmentModelManager::CreateLoadGroup(LPCTSTR strLoadGroupName)
{
#pragma Reminder("IMPLEMENT")
   // not really using this right now.... implement when this feature is fleshed out
   return true;
}

bool CSegmentModelManager::AddLoadGroupToLoadCombinationType(LPCTSTR strLoadGroupName,LoadingCombinationType lcCombo)
{
#pragma Reminder("IMPLEMENT")
   // not really using this right now.... implement when this feature is fleshed out
   return true;
}

bool CSegmentModelManager::CreateLoad(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,LoadType loadType,Float64 P,LPCTSTR strLoadGroupName)
{
#pragma Reminder("IMPLEMENT")
   // not really using this right now.... implement when this feature is fleshed out
   return true;
}

bool CSegmentModelManager::CreateInitialStrainLoad(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r,LPCTSTR strLoadGroupName)
{
#pragma Reminder("IMPLEMENT")
   // not really using this right now.... implement when this feature is fleshed out
   return true;
}

std::vector<Float64> CSegmentModelManager::GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
#pragma Reminder("IMPLEMENT")
   // not really using this right now.... implement when this feature is fleshed out
   return std::vector<Float64>(vPoi.size(),0);
}

std::vector<sysSectionValue> CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
#pragma Reminder("IMPLEMENT")
   // not really using this right now.... implement when this feature is fleshed out
   return std::vector<sysSectionValue>(vPoi.size(),sysSectionValue(0,0));
}

std::vector<Float64> CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
#pragma Reminder("IMPLEMENT")
   // not really using this right now.... implement when this feature is fleshed out
   return std::vector<Float64>(vPoi.size(),0);
}

std::vector<Float64> CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
#pragma Reminder("IMPLEMENT")
   // not really using this right now.... implement when this feature is fleshed out
   return std::vector<Float64>(vPoi.size(),0);
}

std::vector<Float64> CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
#pragma Reminder("IMPLEMENT")
   // not really using this right now.... implement when this feature is fleshed out
   return std::vector<Float64>(vPoi.size(),0);
}

Float64 CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,LPCTSTR strLoadGroupName,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType)
{
#pragma Reminder("IMPLEMENT")
   // not really using this right now.... implement when this feature is fleshed out
   return 0;
}


///////////////////////////
void CSegmentModelManager::GetPrestressSectionStresses(IntervalIndexType intervalIdx,ResultsType resultsType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   CPrestressTool prestressTool(m_pBroker);
   prestressTool.GetPretensionStress(intervalIdx,resultsType,vPoi,topLocation,botLocation,pfTop,pfBot);
}

void CSegmentModelManager::GetSectionResults(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz)
{
   GET_IFACE(IIntervals,pIntervals);
#if defined _DEBUG
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
   ATLASSERT(intervalIdx < erectionIntervalIdx); // should be getting results from the girder model
#endif // _DEBUG

   if ( resultsType == rtIncremental )
   {
      std::vector<sysSectionValue> fxPrev, fyPrev, mzPrev, fxThis, fyThis, mzThis;
      std::vector<Float64>         dxPrev, dyPrev, rzPrev, dxThis, dyThis, rzThis;
      GetSectionResults(intervalIdx-1,pfType,vPoi,&fxPrev,&fyPrev,&mzPrev,&dxPrev,&dyPrev,&rzPrev);
      GetSectionResults(intervalIdx  ,pfType,vPoi,&fxThis,&fyThis,&mzThis,&dxThis,&dyThis,&rzThis);
      
      std::transform(fxThis.begin(),fxThis.end(),fxPrev.begin(),std::back_inserter(*pvFx),std::minus<sysSectionValue>());
      std::transform(fyThis.begin(),fyThis.end(),fyPrev.begin(),std::back_inserter(*pvFy),std::minus<sysSectionValue>());
      std::transform(mzThis.begin(),mzThis.end(),mzPrev.begin(),std::back_inserter(*pvMz),std::minus<sysSectionValue>());

      std::transform(dxThis.begin(),dxThis.end(),dxPrev.begin(),std::back_inserter(*pvDx),std::minus<Float64>());
      std::transform(dyThis.begin(),dyThis.end(),dyPrev.begin(),std::back_inserter(*pvDy),std::minus<Float64>());
      std::transform(rzThis.begin(),rzThis.end(),rzPrev.begin(),std::back_inserter(*pvRz),std::minus<Float64>());
   }
   else
   {
      GetSectionResults(intervalIdx,pfType,vPoi,pvFx,pvFy,pvMz,pvDx,pvDy,pvRz);
   }
}

void CSegmentModelManager::GetSectionResults(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz)
{
   if ( pfType == pftGirder )
   {
      LoadCaseIDType lcid = GetLoadCaseID(pfType);
      GetSectionResults(intervalIdx,lcid,vPoi,pvFx,pvFy,pvMz,pvDx,pvDy,pvRz);
   }
   else if ( pfType == pftPretension )
   {
      pvFx->resize(vPoi.size(),sysSectionValue(0,0));
      pvFy->resize(vPoi.size(),sysSectionValue(0,0));
      pvMz->resize(vPoi.size(),sysSectionValue(0,0));
      pvDx->resize(vPoi.size(),0.0);
      pvDy->resize(vPoi.size(),0.0);
      pvRz->resize(vPoi.size(),0.0);

      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::StrandType strandType =  pgsTypes::StrandType(i);
         LoadCaseIDType lcid = GetLoadCaseID(strandType);
         std::vector<sysSectionValue> fx, fy, mz;
         std::vector<Float64> dx, dy, rz;
         GetSectionResults(intervalIdx,lcid,vPoi,&fx,&fy,&mz,&dx,&dy,&rz);
         
         std::transform(fx.begin(),fx.end(),pvFx->begin(),pvFx->begin(),std::plus<sysSectionValue>());
         std::transform(fy.begin(),fy.end(),pvFy->begin(),pvFy->begin(),std::plus<sysSectionValue>());
         std::transform(mz.begin(),mz.end(),pvMz->begin(),pvMz->begin(),std::plus<sysSectionValue>());

         std::transform(dx.begin(),dx.end(),pvDx->begin(),pvDx->begin(),std::plus<Float64>());
         std::transform(dy.begin(),dy.end(),pvDy->begin(),pvDy->begin(),std::plus<Float64>());
         std::transform(rz.begin(),rz.end(),pvRz->begin(),pvRz->begin(),std::plus<Float64>());
      }
   }
   else
   {
      ZeroResults(vPoi,pvFx,pvFy,pvMz,pvDx,pvDy,pvRz);
   }
}

void CSegmentModelManager::GetSectionResults(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const std::vector<pgsPointOfInterest>& vPoi,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz)
{
   GET_IFACE(IIntervals,pIntervals);

   CSegmentModelData* pModelData = NULL;
   CSegmentKey lastSegmentKey;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);

      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      if ( releaseIntervalIdx <= intervalIdx && !segmentKey.IsEqual(lastSegmentKey) )
      {
         lastSegmentKey = segmentKey;
         pModelData = GetSegmentModel(segmentKey,intervalIdx);

         if ( pModelData->Loads.find(lcid) == pModelData->Loads.end() && (lcid == g_lcidStraight || lcid == g_lcidHarped || lcid == g_lcidTemporary) )
         {
            ApplyPretensionLoad(pModelData,segmentKey);
         }
      }

      GET_IFACE_NOCHECK(IPointOfInterest,pPoi);
      if ( intervalIdx < releaseIntervalIdx || pPoi->IsInClosureJoint(poi) || poi.HasAttribute(POI_BOUNDARY_PIER) )
      {
         // interval is before release or the POI is at a closure joint... closures have not been installed yet so
         // use 0 for the results
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
         PoiIDType poi_id = pModelData->PoiMap.GetModelPoi( poi );

         if ( poi_id == INVALID_ID )
         {
            poi_id = AddPointOfInterest( pModelData, poi );
         }

         ATLASSERT(poi_id != INVALID_ID);


         CComQIPtr<IFem2dModelResults> results(pModelData->Model);

         Float64 FxRight(0), FyRight(0), MzRight(0);
         Float64 FxLeft(0),  FyLeft(0),  MzLeft(0);
         HRESULT hr = results->ComputePOIForces(lcid,poi_id,mftLeft,lotGlobal,&FxLeft,&FyLeft,&MzLeft);
         ATLASSERT(SUCCEEDED(hr));

         FyLeft *= -1;

         hr = results->ComputePOIForces(lcid,poi_id,mftRight,lotGlobal,&FxRight,&FyRight,&MzRight);
         ATLASSERT(SUCCEEDED(hr));

         sysSectionValue fx(FxLeft,FxRight);
         sysSectionValue fy(FyLeft,FyRight);
         sysSectionValue mz(MzLeft,MzRight);

         pvFx->push_back(fx);
         pvFy->push_back(fy);
         pvMz->push_back(mz);

         Float64 dx,dy,rz;
         hr = results->ComputePOIDeflections(lcid,poi_id,lotGlobal,&dx,&dy,&rz);
         ATLASSERT(SUCCEEDED(hr));

         pvDx->push_back(dx);
         pvDy->push_back(dy);
         pvRz->push_back(rz);
      }
   }
}

void CSegmentModelManager::GetSectionStresses(IntervalIndexType intervalIdx,ProductForceType pfType,ResultsType resultsType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
   ATLASSERT(intervalIdx < erectionIntervalIdx); // should be getting results from the girder model
#endif // _DEBUG

   GET_IFACE(IPointOfInterest,pPoi);

   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);
      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      Float64 fTop,fBot;
      if ( pPoi->IsInClosureJoint(poi) || poi.HasAttribute(POI_BOUNDARY_PIER) )
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
            IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
            if ( intervalIdx == releaseIntervalIdx )
            {
               GetSectionStress(intervalIdx,pfType,poi,topLocation,botLocation,&fTop,&fBot);
            }
            else if ( releaseIntervalIdx < intervalIdx && intervalIdx < storageIntervalIdx )
            {
               // there is no change in loading during this time. incremental results are all zero
               fTop = 0;
               fBot = 0;
            }
            else if ( intervalIdx == storageIntervalIdx )
            {
               // the change in results when the segment is placed into storage is the final results
               // in storage minus the final results after release

               Float64 fTopRelease, fBotRelease;
               GetSectionStress(releaseIntervalIdx,pfType,poi,topLocation,botLocation,&fTopRelease,&fBotRelease);

               Float64 fTopStorage, fBotStorage;
               GetSectionStress(storageIntervalIdx,pfType,poi,topLocation,botLocation,&fTopStorage,&fBotStorage);

               fTop = fTopStorage - fTopRelease;
               fBot = fBotStorage - fBotRelease;
            }
            else
            {
               // there is no change in loading after storage. well, there is, but it is modeled
               // in the LBAM girder models. there is no new loading between storage and erection
               // incremental results are all zero
               fTop = 0;
               fBot = 0;
            }
         }
         else
         {
            // Cumulative stress is just the stress in this interval
            GetSectionStress(intervalIdx,pfType,poi,topLocation,botLocation,&fTop,&fBot);
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

void CSegmentModelManager::GetSectionStress(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);

   ApplyPretensionLoad(pModelData,segmentKey);

   PoiIDType poi_id = pModelData->PoiMap.GetModelPoi( poi );
   if ( poi_id == INVALID_ID )
   {
      poi_id = AddPointOfInterest( pModelData, poi );
   }

   CComQIPtr<IFem2dModelResults> results(pModelData->Model);
   Float64 fx,fy,mz;
   Fem2dMbrFaceType face = IsZero( poi.GetDistFromStart() ) ? mftRight : mftLeft;

   LoadCaseIDType lcid = GetLoadCaseID(pfType);
   if ( lcid == INVALID_ID )
   {
      fx = 0;
      fy = 0;
      mz = 0;
   }
   else
   {
      HRESULT hr = results->ComputePOIForces(lcid,poi_id,face,lotMember,&fx,&fy,&mz);
      ATLASSERT(SUCCEEDED(hr));
   }

   GET_IFACE(ISectionProperties, pSectProp);
   Float64 A = pSectProp->GetAg(intervalIdx,poi);
   Float64 St = pSectProp->GetS(intervalIdx,poi,topLocation);
   Float64 Sb = pSectProp->GetS(intervalIdx,poi,botLocation);

   *pfTop = (IsZero(A) || IsZero(St) ? 0 : fx/A + mz/St);
   *pfTop = IsZero(*pfTop) ? 0 : *pfTop;

   *pfBot = (IsZero(A) || IsZero(Sb) ? 0 : fx/A + mz/Sb);
   *pfBot = IsZero(*pfBot) ? 0 : *pfBot;
}

void CSegmentModelManager::ZeroResults(const std::vector<pgsPointOfInterest>& vPoi,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz)
{
   IndexType size = vPoi.size();
   pvFx->resize(size,sysSectionValue(0,0));
   pvFy->resize(size,sysSectionValue(0,0));
   pvMz->resize(size,sysSectionValue(0,0));

   pvDx->resize(size,0.0);
   pvDy->resize(size,0.0);
   pvRz->resize(size,0.0);
}

PoiIDType CSegmentModelManager::AddPointOfInterest(CSegmentModelData* pModelData,const pgsPointOfInterest& poi)
{
   PoiIDType femID = pgsGirderModelFactory::AddPointOfInterest(pModelData->Model, poi);
   pModelData->PoiMap.AddMap( poi, femID );

#if defined ENABLE_LOGGING
   Float64 Xpoi = poi.GetDistFromStart();
   LOG("Adding POI " << femID << " at " << ::ConvertFromSysUnits(Xpoi,unitMeasure::Feet) << " ft");
#endif

   return femID;
}

void CSegmentModelManager::BuildReleaseModel(const CSegmentKey& segmentKey)
{
   CSegmentModelData* pModelData = GetModelData(m_ReleaseModels,segmentKey);
   if ( pModelData == 0 )
   {
      GET_IFACE(IProgress,pProgress);
      CEAFAutoProgress ap(pProgress);

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      // Build the model
      std::_tostringstream os;
      os << "Building prestress release model" << std::ends;
      pProgress->UpdateMessage( os.str().c_str() );

      CSegmentModelData segmentModel = BuildSegmentModel(segmentKey,releaseIntervalIdx,0.0,0.0);
      std::pair<SegmentModels::iterator,bool> result = m_ReleaseModels.insert( std::make_pair(segmentKey,segmentModel) );
      ATLASSERT( result.second == true );
   }

}

CSegmentModelData* CSegmentModelManager::GetSegmentModel(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   ATLASSERT(releaseIntervalIdx <= intervalIdx && intervalIdx <= erectionIntervalIdx);

   return (intervalIdx < storageIntervalIdx ? GetReleaseModel(segmentKey) : GetStorageModel(segmentKey));
}

void CSegmentModelManager::BuildStorageModel(const CSegmentKey& segmentKey)
{
   CSegmentModelData* pModelData = GetModelData(m_StorageModels,segmentKey);
   if ( pModelData == 0 )
   {
      GET_IFACE(IProgress,pProgress);
      CEAFAutoProgress ap(pProgress);

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);

      // Build the model
      std::_tostringstream os;
      os << "Building storage model" << std::ends;
      pProgress->UpdateMessage( os.str().c_str() );

      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
      Float64 left  = pSegment->HandlingData.LeftStoragePoint;
      Float64 right = pSegment->HandlingData.RightStoragePoint;


      CSegmentModelData segmentModel = BuildSegmentModel(segmentKey,storageIntervalIdx,left,right);
      std::pair<SegmentModels::iterator,bool> result = m_StorageModels.insert( std::make_pair(segmentKey,segmentModel) );
      ATLASSERT( result.second == true );
   }
}

CSegmentModelData* CSegmentModelManager::GetReleaseModel(const CSegmentKey& segmentKey)
{
   CSegmentModelData* pModelData = GetModelData(m_ReleaseModels,segmentKey);
   if ( pModelData == NULL )
   {
      BuildReleaseModel(segmentKey);
      pModelData = GetModelData(m_ReleaseModels,segmentKey);
      ATLASSERT(pModelData != NULL);
   }

   return pModelData;
}

CSegmentModelData* CSegmentModelManager::GetStorageModel(const CSegmentKey& segmentKey)
{
   CSegmentModelData* pModelData = GetModelData(m_StorageModels,segmentKey);
   if ( pModelData == NULL )
   {
      BuildStorageModel(segmentKey);
      pModelData = GetModelData(m_StorageModels,segmentKey);
      ATLASSERT(pModelData != NULL);
   }

   return pModelData;
}

CSegmentModelData* CSegmentModelManager::GetModelData(SegmentModels& models,const CSegmentKey& segmentKey)
{
   ATLASSERT(segmentKey.segmentIndex != INVALID_INDEX);
   SegmentModels::iterator found = models.find(segmentKey);
   if ( found == models.end() )
   {
      return NULL;
   }

   CSegmentModelData* pModelData = &(*found).second;

   return pModelData;
}

Float64 g_L;
bool RemovePOI(const pgsPointOfInterest& poi)
{
   return g_L < poi.GetDistFromStart();
}

CSegmentModelData CSegmentModelManager::BuildSegmentModel(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 leftSupportDistance,Float64 rightSupportDistance)
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
   Float64 Ec = pMaterial->GetSegmentEc(segmentKey,intervalIdx);

   // Get points of interest
   GET_IFACE(IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPOI( pIPoi->GetPointsOfInterest(segmentKey) );
   pIPoi->RemovePointsOfInterest(vPOI,POI_CLOSURE);
   pIPoi->RemovePointsOfInterest(vPOI,POI_BOUNDARY_PIER);
   g_L = Ls;
   vPOI.erase(std::remove_if(vPOI.begin(),vPOI.end(),RemovePOI),vPOI.end());

   // Build the Model
   CSegmentModelData model_data;
   model_data.Interval = intervalIdx;
   LoadCaseIDType lcid = GetLoadCaseID(pftGirder);
   model_data.Loads.insert(lcid);
   pgsGirderModelFactory().CreateGirderModel(m_pBroker,intervalIdx,segmentKey,leftSupportDistance,Ls-rightSupportDistance,Ec,lcid,true,vPOI,&model_data.Model,&model_data.PoiMap);

   // Generate unit loads
   CComPtr<IFem2dPOICollection> POIs;
   model_data.Model->get_POIs(&POIs);
   LoadCaseIDType lcidUnitLoad = g_lcidUnitLoadBase;
   LoadCaseIDType lcidUnitMoment = lcidUnitLoad-1;
   CComPtr<IFem2dLoadingCollection> loadings;
   model_data.Model->get_Loadings(&loadings);
   std::vector<pgsPointOfInterest>::iterator iter(vPOI.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPOI.end());
   for ( ; iter != end; iter++, lcidUnitLoad -= 2, lcidUnitMoment -= 2 )
   {
      pgsPointOfInterest& poi = *iter;

      CComPtr<IFem2dLoading> unitLoadLoading;
      loadings->Create(lcidUnitLoad,&unitLoadLoading);

      CComPtr<IFem2dLoading> unitMomentLoading;
      loadings->Create(lcidUnitMoment,&unitMomentLoading);

      CComPtr<IFem2dPointLoadCollection> unitLoadLoadingPointLoads;
      unitLoadLoading->get_PointLoads(&unitLoadLoadingPointLoads);

      CComPtr<IFem2dPointLoadCollection> unitMomentLoadingPointLoads;
      unitMomentLoading->get_PointLoads(&unitMomentLoadingPointLoads);

      PoiIDType modelPoiID = model_data.PoiMap.GetModelPoi(poi);

      CComPtr<IFem2dPOI> femPoi;
      POIs->Find(modelPoiID,&femPoi);

      MemberIDType mbrID;
      Float64 x;
      femPoi->get_MemberID(&mbrID);
      femPoi->get_Location(&x);

      CComPtr<IFem2dPointLoad> pointLoad;
      Float64 Q = 1.0; // magnitude 1.0
      unitLoadLoadingPointLoads->Create(0,mbrID,x,0.0,Q,0.0,lotMember,&pointLoad);
      pointLoad.Release();
      unitMomentLoadingPointLoads->Create(0,mbrID,x,0.0,0.0,Q,lotMember,&pointLoad);

      model_data.UnitLoadIDMap.insert(std::make_pair(poi.GetID(),lcidUnitLoad));
      model_data.UnitMomentIDMap.insert(std::make_pair(poi.GetID(),lcidUnitMoment));
   }

   return model_data;
}

void CSegmentModelManager::ApplyPretensionLoad(CSegmentModelData* pModelData,const CSegmentKey& segmentKey)
{
   CComPtr<IFem2dLoadingCollection> loadings;
   pModelData->Model->get_Loadings(&loadings);

   for ( int i = 0; i < 3; i++ )
   {
      pgsTypes::StrandType strandType = pgsTypes::StrandType(i);

      LoadCaseIDType lcid = GetLoadCaseID(strandType);

      if ( pModelData->Loads.find(lcid) != pModelData->Loads.end() )
      {
         // this load has been previously applied
         continue;
      }

      std::vector<EquivPretensionLoad> vLoads = GetEquivPretensionLoads(segmentKey,strandType);

      CComPtr<IFem2dLoading> loading;
      CComPtr<IFem2dPointLoadCollection> pointLoads;

      loadings->Create(lcid,&loading);
      loading->get_PointLoads(&pointLoads);
      LoadIDType ptLoadID;
      pointLoads->get_Count((CollectionIndexType*)&ptLoadID);
      pModelData->Loads.insert(lcid);

      std::vector<EquivPretensionLoad>::iterator iter(vLoads.begin());
      std::vector<EquivPretensionLoad>::iterator iterEnd(vLoads.end());
      for ( ; iter != iterEnd; iter++ )
      {
         EquivPretensionLoad& equivLoad = *iter;

         CComPtr<IFem2dPointLoad> ptLoad;
         MemberIDType mbrID;
         Float64 x;
         pgsGirderModelFactory::FindMember(pModelData->Model,equivLoad.Xs,&mbrID,&x);
         pointLoads->Create(ptLoadID++,mbrID,x,0.00,equivLoad.P,equivLoad.M,lotGlobal,&ptLoad);
      }
   }
}

LoadCaseIDType CSegmentModelManager::GetLoadCaseID(ProductForceType pfType)
{
   LoadCaseIDType lcid;
   switch(pfType)
   {
   case pftGirder:
      lcid = g_lcidGirder;
      break;

   default:
      lcid = INVALID_ID;
   }

   return lcid;
}

LoadCaseIDType CSegmentModelManager::GetLoadCaseID(pgsTypes::StrandType strandType)
{
   return LoadCaseIDType(strandType);
}


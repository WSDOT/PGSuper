///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include <PgsExt\BridgeDescription2.h>

#include <IFace\Intervals.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>

#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\PrecastSegmentData.h>

CSegmentModelManager::CSegmentModelManager(SHARED_LOGFILE lf,IBroker* pBroker) :
LOGFILE(lf),m_pBroker(pBroker)
{
}

void CSegmentModelManager::Clear()
{
   m_ReleaseModels.clear();
   m_StorageModels.clear();
   m_LoadCombinationMaps.clear();
}

void CSegmentModelManager::DumpAnalysisModels(GirderIndexType gdrIdx)
{
   USES_CONVERSION;
   SegmentModels::iterator iter(m_StorageModels.begin());
   SegmentModels::iterator end(m_StorageModels.end());
   for ( ; iter != end; iter++ )
   {
      CSegmentKey segmentKey(iter->first);
      CSegmentModelData& segmentModelData(iter->second);

      CString strFilename;
      strFilename.Format(_T("Segment_%d_Storage_Fem2d.xml"),segmentKey.segmentIndex+1);

      CComPtr<IStructuredSave2> save;
      save.CoCreateInstance(CLSID_StructuredSave2);
      save->Open(T2BSTR(strFilename));

      CComQIPtr<IStructuredStorage2> storage(segmentModelData.Model);
      storage->Save(save);

      save->Close();
   }

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
      Ph = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Harped,releaseIntervalIdx,pgsTypes::End);

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

      ecc_harped_start = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiStart, pgsTypes::Harped, &nHs_effective);
      ecc_harped_hp1   = pStrandGeom->GetEccentricity(releaseIntervalIdx, hp1_poi,  pgsTypes::Harped, &nHs_effective);
      ecc_harped_hp2   = pStrandGeom->GetEccentricity(releaseIntervalIdx, hp2_poi,  pgsTypes::Harped, &nHs_effective);
      ecc_harped_end   = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiEnd,   pgsTypes::Harped, &nHs_effective);

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

      Nl = IsZero(hp1)    || IsZero(hp2-hp1) ? 0 : Ph*(e_prime_start/hp1 + (e_prime_end - e_prime_start)/(hp2-hp1));
      Nr = IsZero(Ls-hp2) || IsZero(hp2-hp1) ? 0 : Ph*((e_prime_end - e_prime_start)/(hp2-hp1) + e_prime_end/(Ls-hp2));

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
      ecc_straight_start = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiStart, pgsTypes::Straight, &nSsEffective);
      ecc_straight_end   = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiEnd,   pgsTypes::Straight, &nSsEffective);
      Ps = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::End);

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

            Ps = nDebondedAtSection*pPrestressForce->GetPrestressForcePerStrand(mid_span_poi,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::End);
            ecc_straight_debond = pStrandGeom->GetEccentricity(releaseIntervalIdx, pgsPointOfInterest(segmentKey,location), pgsTypes::Straight, &nSsEffective);

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
      Pt = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::End);
      ecc_temporary_start = pStrandGeom->GetEccentricity(releaseIntervalIdx,poiStart,pgsTypes::Temporary, &nTsEffective);
      ecc_temporary_end   = pStrandGeom->GetEccentricity(releaseIntervalIdx,poiEnd,  pgsTypes::Temporary, &nTsEffective);

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

Float64 CSegmentModelManager::GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> axial = GetAxial(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(axial.size() == 1);

   return axial.front();
}

sysSectionValue CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> shears = GetShear(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(shears.size() == 1);

   return shears.front();
}

Float64 CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> moments = GetMoment(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(moments.size() == 1);

   return moments.front();
}

Float64 CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> deflections = GetDeflection(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(deflections.size() == 1);

   return deflections.front();
}

Float64 CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> rotations = GetRotation(intervalIdx,pfType,vPoi,resultsType);
   ATLASSERT(rotations.size() == 1);

   return rotations.front();
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
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

void CSegmentModelManager::GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,ResultsType resultsType,Float64* pRleft,Float64* pRright)
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

void CSegmentModelManager::GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,ResultsType resultsType,Float64* pRleft,Float64* pRright)
{
   *pRleft = 0;
   *pRright = 0;

   CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);

   LoadCombinationMap& loadCombinationMap(GetLoadCombinationMap(segmentKey));
   std::pair<LoadCombinationMap::iterator,LoadCombinationMap::iterator> range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.end() )
   {
      if ( range.second != loadCombinationMap.end() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( LoadCombinationMap::iterator iter = range.first; iter != range.second; iter++ )
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
   BOOST_FOREACH(pgsTypes::ProductForceType pfType,pfTypes)
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

Float64 CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType)
{
   if ( pfType != pgsTypes::pftGirder )
   {
      // only girder load creates reaction
      return 0;
   }

   LoadCaseIDType lcid = GetLoadCaseID(pfType);
   return GetReaction(intervalIdx,lcid,pierIdx,girderKey,resultsType);
}

std::vector<Float64> CSegmentModelManager::GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
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
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   std::vector<sysSectionValue>::iterator axialIter(vFx.begin());
   for ( ; iter != end; iter++, axialIter++ )
   {
      sysSectionValue& fx = *axialIter;
      const pgsPointOfInterest& poi(*iter);
      if ( IsZero(poi.GetDistFromStart()) )
      {
         axial.push_back(-fx.Right());
      }
      else
      {
         axial.push_back(fx.Left());
      }
   }

   return axial;
}

std::vector<Float64> CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
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

std::vector<sysSectionValue> CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
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

std::vector<Float64> CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   // NOTE: Deflections for prestress are based on the equivalent loading. This is sufficient.
   // If we need to be more precise, we can use the method of virtual work and integrate the
   // M/EI diagram for the prestress moment based on P*e.

   std::vector<sysSectionValue> vFx,vFy,vMz;
   std::vector<Float64> vDx,vDy,vRz;
   if ( pfType == pgsTypes::pftPretension )
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
         vDyThisSegment.resize(vPoiThisSegment.size());
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

         for ( int i = 0; i < 3; i++ )
         {
            pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
            LoadCaseIDType lcid = GetLoadCaseID(strandType);
            std::vector<Float64> vDyThisStrandType;
            GetSectionResults(releaseIntervalIdx, lcid, vPoiThisSegment, resultsType, &vFx, &vFy, &vMz, &vDx, &vDyThisStrandType, &vRz );
            std::transform(vDyThisStrandType.begin(),vDyThisStrandType.end(),vDyThisSegment.begin(),vDyThisSegment.begin(),std::plus<Float64>());
         }

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
            std::vector<Float64> DStorageSupports = GetDeflection(releaseIntervalIdx,pgsTypes::pftPretension,vStorageSupports,rtCumulative);

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
   else if ( pfType == pgsTypes::pftSecondaryEffects || pfType == pgsTypes::pftPostTensioning )
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

std::vector<Float64> CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   std::vector<sysSectionValue> vFx,vFy,vMz;
   std::vector<Float64> vDx,vDy,vRz;

   if ( pfType == pgsTypes::pftPretension )
   {
      GetPrestressSectionResults(intervalIdx, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );
   }
   else if ( pfType == pgsTypes::pftSecondaryEffects || pfType == pgsTypes::pftPostTensioning )
   {
      // post-tensioning hasn't been applied yet so the response is zero
      vRz.resize(vPoi.size(),0.0);
   }
   else
   {
      LoadCaseIDType lcid = GetLoadCaseID(pfType);
      GetSectionResults(intervalIdx, lcid, vPoi, resultsType, &vFx, &vFy, &vMz, &vDx, &vDy, &vRz );
   }

   return vRz;
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   // NOTE: We want streses based on P/A + P*e/S due to prestressing where P accounts for development and losses.
   // The FEM model can't account for this so we have to use a special function to get
   // stress in the girder segment due to prestressing (there are moments in the FEM model based on the equivalent prestress loading,
   // however the FEM model doesn't have equivalent axial forces modeled)
   if ( pfType == pgsTypes::pftPretension )
   {
      GetPrestressSectionStresses(intervalIdx,vPoi,resultsType,topLocation,botLocation,pfTop,pfBot);
   }
   else
   {
      LoadCaseIDType lcid = GetLoadCaseID(pfType);
      GetSectionStresses(intervalIdx,lcid,resultsType,vPoi,topLocation,botLocation,pfTop,pfBot);
   }
}

sysSectionValue CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> shears = GetShear(intervalIdx,comboType,vPoi,resultsType);
   ATLASSERT(shears.size() == 1);
   return shears.front();
}

Float64 CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> moments = GetMoment(intervalIdx,comboType,vPoi,resultsType);
   ATLASSERT(moments.size() == 1);
   return moments.front();
}

Float64 CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> deflections = GetDeflection(intervalIdx,comboType,vPoi,resultsType);
   ATLASSERT(deflections.size() == 1);
   return deflections.front();
}

Float64 CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> rotations = GetRotation(intervalIdx,comboType,vPoi,resultsType);
   ATLASSERT(rotations.size() == 1);
   return rotations.front();
}

Float64 CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,LoadingCombinationType comboType,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType)
{
   Float64 R = 0;

   CSegmentKey segmentKey = GetSegmentKey(girderKey,pierIdx);

   CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);

   LoadCombinationMap& loadCombinationMap(GetLoadCombinationMap(girderKey));
   std::pair<LoadCombinationMap::iterator,LoadCombinationMap::iterator> range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.end() )
   {
      if ( range.second != loadCombinationMap.end() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( LoadCombinationMap::iterator iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         LoadCaseIDType lcid = GetLoadCaseID(pModelData,strLoadingName.c_str());

         Float64 r = GetReaction(intervalIdx,lcid,pierIdx,girderKey,resultsType);
         R += r;
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   BOOST_FOREACH(pgsTypes::ProductForceType pfType,pfTypes)
   {
      LoadCaseIDType lcid = GetLoadCaseID(pfType);
      R += GetReaction(intervalIdx,lcid,pierIdx,girderKey,resultsType);
   }

   return R;
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop, fBot;
   GetStress(intervalIdx,comboType,vPoi,resultsType,topLocation,botLocation,&fTop,&fBot);

   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);

   *pfTop = fTop.front();
   *pfBot = fBot.front();
}

std::vector<sysSectionValue> CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   std::vector<sysSectionValue> vShear;

   // before release, there aren't any results
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
   if ( intervalIdx < releaseIntervalIdx )
   {
      vShear.resize(vPoi.size(),sysSectionValue(0,0));
      return vShear;
   }

   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().GetSegmentKey(),intervalIdx);

   LoadCombinationMap& loadCombinationMap(GetLoadCombinationMap(vPoi.front().GetSegmentKey()));
   std::pair<LoadCombinationMap::iterator,LoadCombinationMap::iterator> range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.end() )
   {
      if ( range.second != loadCombinationMap.end() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( LoadCombinationMap::iterator iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         std::vector<sysSectionValue> vS = GetShear(intervalIdx,strLoadingName.c_str(),vPoi,resultsType);
         if ( vShear.size() == 0 )
         {
            vShear = vS;
         }
         else
         {
            std::transform(vS.begin(),vS.end(),vShear.begin(),vShear.begin(),std::plus<sysSectionValue>());
         }
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   BOOST_FOREACH(pgsTypes::ProductForceType pfType,pfTypes)
   {
      std::vector<sysSectionValue> vS = GetShear(intervalIdx,pfType,vPoi,resultsType);
      if ( vShear.size() == 0 )
      {
         vShear = vS;
      }
      else
      {
         std::transform(vS.begin(),vS.end(),vShear.begin(),vShear.begin(),std::plus<sysSectionValue>());
      }
   }

   return vShear;
}

std::vector<Float64> CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   std::vector<Float64> vM;

   // before release, there aren't any results
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
   if ( intervalIdx < releaseIntervalIdx )
   {
      vM.resize(vPoi.size(),0.0);
      return vM;
   }

   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().GetSegmentKey(),intervalIdx);

   LoadCombinationMap& loadCombinationMap(GetLoadCombinationMap(vPoi.front().GetSegmentKey()));
   std::pair<LoadCombinationMap::iterator,LoadCombinationMap::iterator> range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.end() )
   {
      if ( range.second != loadCombinationMap.end() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( LoadCombinationMap::iterator iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         std::vector<Float64> vm = GetMoment(intervalIdx,strLoadingName.c_str(),vPoi,resultsType);
         if ( vM.size() == 0 )
         {
            vM = vm;
         }
         else
         {
            std::transform(vm.begin(),vm.end(),vM.begin(),vM.begin(),std::plus<Float64>());
         }
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   BOOST_FOREACH(pgsTypes::ProductForceType pfType,pfTypes)
   {
      std::vector<Float64> vm = GetMoment(intervalIdx,pfType,vPoi,resultsType);
      if ( vM.size() == 0 )
      {
         vM = vm;
      }
      else
      {
         std::transform(vm.begin(),vm.end(),vM.begin(),vM.begin(),std::plus<Float64>());
      }
   }

   return vM;
}

std::vector<Float64> CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   std::vector<Float64> vD;

   // before release, there aren't any results
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
   if ( intervalIdx < releaseIntervalIdx )
   {
      vD.resize(vPoi.size(),0.0);
      return vD;
   }

   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().GetSegmentKey(),intervalIdx);

   LoadCombinationMap& loadCombinationMap(GetLoadCombinationMap(vPoi.front().GetSegmentKey()));
   std::pair<LoadCombinationMap::iterator,LoadCombinationMap::iterator> range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.end() )
   {
      if ( range.second != loadCombinationMap.end() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( LoadCombinationMap::iterator iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         std::vector<Float64> vd = GetDeflection(intervalIdx,strLoadingName.c_str(),vPoi,resultsType);
         if ( vD.size() == 0 )
         {
            vD = vd;
         }
         else
         {
            std::transform(vd.begin(),vd.end(),vD.begin(),vD.begin(),std::plus<Float64>());
         }
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   BOOST_FOREACH(pgsTypes::ProductForceType pfType,pfTypes)
   {
      std::vector<Float64> vd = GetDeflection(intervalIdx,pfType,vPoi,resultsType);
      if ( vD.size() == 0 )
      {
         vD = vd;
      }
      else
      {
         std::transform(vd.begin(),vd.end(),vD.begin(),vD.begin(),std::plus<Float64>());
      }
   }

   return vD;
}

std::vector<Float64> CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   std::vector<Float64> vR;

   // before release, there aren't any results
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
   if ( intervalIdx < releaseIntervalIdx )
   {
      vR.resize(vPoi.size(),0.0);
      return vR;
   }

   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().GetSegmentKey(),intervalIdx);

   LoadCombinationMap& loadCombinationMap(GetLoadCombinationMap(vPoi.front().GetSegmentKey()));
   std::pair<LoadCombinationMap::iterator,LoadCombinationMap::iterator> range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.end() )
   {
      if ( range.second != loadCombinationMap.end() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( LoadCombinationMap::iterator iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         std::vector<Float64> vr = GetRotation(intervalIdx,strLoadingName.c_str(),vPoi,resultsType);
         if ( vR.size() == 0 )
         {
            vR = vr;
         }
         else
         {
            std::transform(vr.begin(),vr.end(),vR.begin(),vR.begin(),std::plus<Float64>());
         }
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   BOOST_FOREACH(pgsTypes::ProductForceType pfType,pfTypes)
   {
      std::vector<Float64> vr = GetRotation(intervalIdx,pfType,vPoi,resultsType);
      if ( vR.size() == 0 )
      {
         vR = vr;
      }
      else
      {
         std::transform(vr.begin(),vr.end(),vR.begin(),vR.begin(),std::plus<Float64>());
      }
   }

   return vR;
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   pfTop->resize(vPoi.size(),0.0);
   pfBot->resize(vPoi.size(),0.0);

   // before release, there aren't any results
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().GetSegmentKey());
   if ( intervalIdx < releaseIntervalIdx )
   {
      return;
   }

   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().GetSegmentKey(),intervalIdx);

   LoadCombinationMap& loadCombinationMap(GetLoadCombinationMap(vPoi.front().GetSegmentKey()));
   std::pair<LoadCombinationMap::iterator,LoadCombinationMap::iterator> range = loadCombinationMap.equal_range(comboType);
   if ( range.first != loadCombinationMap.end() )
   {
      if ( range.second != loadCombinationMap.end() )
      {
         range.second++; // makes it one past where we want to stop... makes the loop work correctly
      }

      for ( LoadCombinationMap::iterator iter = range.first; iter != range.second; iter++ )
      {
         const std::_tstring& strLoadingName(iter->second);
         std::vector<Float64> ft,fb;
         GetStress(intervalIdx,strLoadingName.c_str(),vPoi,resultsType,topLocation,botLocation,&ft,&fb);
         std::transform(ft.begin(),ft.end(),pfTop->begin(),pfTop->begin(),std::plus<Float64>());
         std::transform(fb.begin(),fb.end(),pfBot->begin(),pfBot->begin(),std::plus<Float64>());
      }
   }

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
   BOOST_FOREACH(pgsTypes::ProductForceType pfType,pfTypes)
   {
      std::vector<Float64> ft,fb;
      GetStress(intervalIdx,pfType,vPoi,resultsType,topLocation,botLocation,&ft,&fb);
      std::transform(ft.begin(),ft.end(),pfTop->begin(),pfTop->begin(),std::plus<Float64>());
      std::transform(fb.begin(),fb.end(),pfBot->begin(),pfBot->begin(),std::plus<Float64>());
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
      std::vector<Float64> ps = GetDeflection(intervalIdx,pgsTypes::pftPretension,vPoi,rtCumulative);
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
      std::vector<Float64> ps = GetRotation(intervalIdx,pgsTypes::pftPretension,vPoi,rtCumulative);
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
      GetStress(intervalIdx,pgsTypes::pftPretension,vPoi,rtCumulative,topLocation,botLocation,&fTopPS,&fBotPS);
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
bool CSegmentModelManager::CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GroupIndexType nGroups = pIBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirdersThisGroup = pIBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
      GirderIndexType gdrIdx = Min(girderLineIdx,nGirdersThisGroup-1);

      CGirderKey girderKey(grpIdx,gdrIdx);

      const CSplicedGirderData* pGirder = pIBridgeDesc->GetGirder(girderKey);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(girderKey,segIdx);

         for ( int i = 0; i < 2; i++ )
         {
            CSegmentModelData* pModelData;
            if ( i == 0 )
            {
               pModelData = GetReleaseModel(segmentKey);
            }
            else
            {
               pModelData = GetStorageModel(segmentKey);
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

bool CSegmentModelManager::AddLoadingToLoadCombination(GirderIndexType girderLineIdx,LPCTSTR strLoadingName,LoadingCombinationType comboType)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GroupIndexType nGroups = pIBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirdersThisGroup = pIBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
      GirderIndexType gdrIdx = Min(girderLineIdx,nGirdersThisGroup-1);

      CGirderKey girderKey(grpIdx,gdrIdx);
      LoadCombinationMap& loadCombinationMap(GetLoadCombinationMap(girderKey));
      loadCombinationMap.insert(std::make_pair(comboType,strLoadingName));
   }
   return true;
}

bool CSegmentModelManager::CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   return CreateConcentratedLoad(intervalIdx,pfType,NULL,poi,Fx,Fy,Mz);
}

bool CSegmentModelManager::CreateConcentratedLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   return CreateConcentratedLoad(intervalIdx,pgsTypes::pftProductForceTypeCount,strLoadingName,poi,Fx,Fy,Mz);
}

bool CSegmentModelManager::CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
   return CreateUniformLoad(intervalIdx,pfType,NULL,poi1,poi2,wx,wy);
}

bool CSegmentModelManager::CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
   return CreateUniformLoad(intervalIdx,pgsTypes::pftProductForceTypeCount,strLoadingName,poi1,poi2,wx,wy);
}

bool CSegmentModelManager::CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
   return CreateInitialStrainLoad(intervalIdx,pfType,NULL,poi1,poi2,e,r);
}

bool CSegmentModelManager::CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
   return CreateInitialStrainLoad(intervalIdx,pgsTypes::pftProductForceTypeCount,strLoadingName,poi1,poi2,e,r);
}

std::vector<Float64> CSegmentModelManager::GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   // Assuming that the loading ID sequence for all models is the same
   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().GetSegmentKey(),intervalIdx);
   LoadCaseIDType lcid = GetLoadCaseID(pModelData,strLoadingName);

   std::vector<sysSectionValue> vFx, vFy, vMz;
   std::vector<Float64> vDx, vDy, vRz;
   GetSectionResults(intervalIdx,lcid,vPoi,&vFx,&vFy,&vMz,&vDx,&vDy,&vRz);

   std::vector<Float64> results;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   std::vector<sysSectionValue>::const_iterator fxIter(vFx.begin());
   for ( ; poiIter != poiIterEnd; poiIter++, fxIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);
      const sysSectionValue& Fx(*fxIter);

      if ( IsZero(poi.GetDistFromStart()) )
      {
         results.push_back(-Fx.Right());
      }
      else
      {
         results.push_back(Fx.Left());
      }
   }
   return results;
}

std::vector<sysSectionValue> CSegmentModelManager::GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   // Assuming that the loading ID sequence for all models is the same
   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().GetSegmentKey(),intervalIdx);
   LoadCaseIDType lcid = GetLoadCaseID(pModelData,strLoadingName);

   std::vector<sysSectionValue> vFx, vFy, vMz;
   std::vector<Float64> vDx, vDy, vRz;
   GetSectionResults(intervalIdx,lcid,vPoi,&vFx,&vFy,&vMz,&vDx,&vDy,&vRz);

   return vFy;
}

std::vector<Float64> CSegmentModelManager::GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   // Assuming that the loading ID sequence for all models is the same
   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().GetSegmentKey(),intervalIdx);
   LoadCaseIDType lcid = GetLoadCaseID(pModelData,strLoadingName);

   std::vector<sysSectionValue> vFx, vFy, vMz;
   std::vector<Float64> vDx, vDy, vRz;
   GetSectionResults(intervalIdx,lcid,vPoi,&vFx,&vFy,&vMz,&vDx,&vDy,&vRz);

   std::vector<Float64> results;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   std::vector<sysSectionValue>::const_iterator mzIter(vMz.begin());
   for ( ; poiIter != poiIterEnd; poiIter++, mzIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);
      const sysSectionValue& Mz(*mzIter);

      if ( IsZero(poi.GetDistFromStart()) )
      {
         results.push_back(-Mz.Right());
      }
      else
      {
         results.push_back(Mz.Left());
      }
   }
   return results;
}

std::vector<Float64> CSegmentModelManager::GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   // Assuming that the loading ID sequence for all models is the same
   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().GetSegmentKey(),intervalIdx);
   LoadCaseIDType lcid = GetLoadCaseID(pModelData,strLoadingName);

   std::vector<sysSectionValue> vFx, vFy, vMz;
   std::vector<Float64> vDx, vDy, vRz;
   GetSectionResults(intervalIdx,lcid,vPoi,&vFx,&vFy,&vMz,&vDx,&vDy,&vRz);

   return vDy;
}

std::vector<Float64> CSegmentModelManager::GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType)
{
   // Assuming that the loading ID sequence for all models is the same
   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().GetSegmentKey(),intervalIdx);
   LoadCaseIDType lcid = GetLoadCaseID(pModelData,strLoadingName);

   std::vector<sysSectionValue> vFx, vFy, vMz;
   std::vector<Float64> vDx, vDy, vRz;
   GetSectionResults(intervalIdx,lcid,vPoi,&vFx,&vFy,&vMz,&vDx,&vDy,&vRz);

   return vRz;
}

Float64 CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType)
{
   CSegmentKey segmentKey = GetSegmentKey(girderKey,pierIdx);
   CSegmentModelData* pModelData = GetSegmentModel(segmentKey,intervalIdx);

   LoadCaseIDType lcid = GetLoadCaseID(pModelData,strLoadingName);

   return GetReaction(intervalIdx,lcid,pierIdx,girderKey,resultsType);
}

void CSegmentModelManager::GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   CSegmentModelData* pModelData = GetSegmentModel(vPoi.front().GetSegmentKey(),intervalIdx);
   LoadCaseIDType lcid = GetLoadCaseID(pModelData,strLoadingName);

   GetSectionStresses(intervalIdx,lcid,resultsType,vPoi,topLocation,botLocation,pfTop,pfBot);
}


///////////////////////////
CSegmentModelManager::LoadCombinationMap& CSegmentModelManager::GetLoadCombinationMap(const CGirderKey& girderKey)
{
   std::map<CGirderKey,LoadCombinationMap>::iterator found = m_LoadCombinationMaps.find(girderKey);
   if ( found == m_LoadCombinationMaps.end() )
   {
      std::pair<std::map<CGirderKey,LoadCombinationMap>::iterator,bool> result = m_LoadCombinationMaps.insert(std::make_pair(girderKey,LoadCombinationMap()));
      ATLASSERT(result.second == true);
      found = result.first;
   }

   return found->second;
}

void CSegmentModelManager::GetPrestressSectionStresses(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   CPrestressTool prestressTool(m_pBroker);
   prestressTool.GetPretensionStress(intervalIdx,resultsType,vPoi,topLocation,botLocation,pfTop,pfBot);
}

void CSegmentModelManager::GetSectionResults(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz)
{
#if defined _DEBUG
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
   ATLASSERT(intervalIdx < erectionIntervalIdx); // should be getting results from the girder model
#endif // _DEBUG

   if ( resultsType == rtIncremental )
   {
      std::vector<sysSectionValue> fxPrev, fyPrev, mzPrev, fxThis, fyThis, mzThis;
      std::vector<Float64>         dxPrev, dyPrev, rzPrev, dxThis, dyThis, rzThis;
      GetSectionResults(intervalIdx-1,lcid,vPoi,&fxPrev,&fyPrev,&mzPrev,&dxPrev,&dyPrev,&rzPrev);
      GetSectionResults(intervalIdx  ,lcid,vPoi,&fxThis,&fyThis,&mzThis,&dxThis,&dyThis,&rzThis);
      
      std::transform(fxThis.begin(),fxThis.end(),fxPrev.begin(),std::back_inserter(*pvFx),std::minus<sysSectionValue>());
      std::transform(fyThis.begin(),fyThis.end(),fyPrev.begin(),std::back_inserter(*pvFy),std::minus<sysSectionValue>());
      std::transform(mzThis.begin(),mzThis.end(),mzPrev.begin(),std::back_inserter(*pvMz),std::minus<sysSectionValue>());

      std::transform(dxThis.begin(),dxThis.end(),dxPrev.begin(),std::back_inserter(*pvDx),std::minus<Float64>());
      std::transform(dyThis.begin(),dyThis.end(),dyPrev.begin(),std::back_inserter(*pvDy),std::minus<Float64>());
      std::transform(rzThis.begin(),rzThis.end(),rzPrev.begin(),std::back_inserter(*pvRz),std::minus<Float64>());
   }
   else
   {
      GetSectionResults(intervalIdx,lcid,vPoi,pvFx,pvFy,pvMz,pvDx,pvDy,pvRz);
   }
}

void CSegmentModelManager::GetPrestressSectionResults(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,ResultsType resultsType,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz)
{
   if ( resultsType == rtIncremental )
   {
      std::vector<sysSectionValue> fxPrev, fyPrev, mzPrev, fxThis, fyThis, mzThis;
      std::vector<Float64>         dxPrev, dyPrev, rzPrev, dxThis, dyThis, rzThis;
      GetPrestressSectionResults(intervalIdx-1,vPoi,&fxPrev,&fyPrev,&mzPrev,&dxPrev,&dyPrev,&rzPrev);
      GetPrestressSectionResults(intervalIdx  ,vPoi,&fxThis,&fyThis,&mzThis,&dxThis,&dyThis,&rzThis);
      
      std::transform(fxThis.begin(),fxThis.end(),fxPrev.begin(),std::back_inserter(*pvFx),std::minus<sysSectionValue>());
      std::transform(fyThis.begin(),fyThis.end(),fyPrev.begin(),std::back_inserter(*pvFy),std::minus<sysSectionValue>());
      std::transform(mzThis.begin(),mzThis.end(),mzPrev.begin(),std::back_inserter(*pvMz),std::minus<sysSectionValue>());

      std::transform(dxThis.begin(),dxThis.end(),dxPrev.begin(),std::back_inserter(*pvDx),std::minus<Float64>());
      std::transform(dyThis.begin(),dyThis.end(),dyPrev.begin(),std::back_inserter(*pvDy),std::minus<Float64>());
      std::transform(rzThis.begin(),rzThis.end(),rzPrev.begin(),std::back_inserter(*pvRz),std::minus<Float64>());
   }
   else
   {
      GetPrestressSectionResults(intervalIdx,vPoi,pvFx,pvFy,pvMz,pvDx,pvDy,pvRz);
   }
}

void CSegmentModelManager::GetPrestressSectionResults(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz)
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

void CSegmentModelManager::GetSectionResults(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const std::vector<pgsPointOfInterest>& vPoi,std::vector<sysSectionValue>* pvFx,std::vector<sysSectionValue>* pvFy,std::vector<sysSectionValue>* pvMz,std::vector<Float64>* pvDx,std::vector<Float64>* pvDy,std::vector<Float64>* pvRz)
{
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IBridge,pBridge);

   CSegmentModelData* pModelData = NULL;
   CSegmentKey lastSegmentKey;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);

      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      Float64 segmentLength = pBridge->GetSegmentLength(segmentKey);

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      if ( releaseIntervalIdx <= intervalIdx && !segmentKey.IsEqual(lastSegmentKey) )
      {
         lastSegmentKey = segmentKey;
         pModelData = GetSegmentModel(segmentKey,intervalIdx);

         if ( pModelData->Loads.find(lcid) == pModelData->Loads.end() && (lcid == GetLoadCaseID(pgsTypes::Straight) || lcid == GetLoadCaseID(pgsTypes::Harped) || lcid == GetLoadCaseID(pgsTypes::Temporary) ) )
         {
            ApplyPretensionLoad(pModelData,segmentKey);
         }
      }

      GET_IFACE_NOCHECK(IPointOfInterest,pPoi);
      if ( intervalIdx < releaseIntervalIdx || pPoi->IsOffSegment(poi) )
      {
         // interval is before release or the POI is off the segment
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

         Float64 dx(0),dy(0),rz(0);
         hr = results->ComputePOIDeflections(lcid,poi_id,lotGlobal,&dx,&dy,&rz);
         ATLASSERT(SUCCEEDED(hr));

         pvDx->push_back(dx);
         pvDy->push_back(dy);
         pvRz->push_back(rz);
      }
   }
}

void CSegmentModelManager::GetSectionStresses(IntervalIndexType intervalIdx,LoadCaseIDType lcid,ResultsType resultsType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
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
            IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
            if ( intervalIdx == releaseIntervalIdx )
            {
               GetSectionStress(intervalIdx,lcid,poi,topLocation,botLocation,&fTop,&fBot);
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
               GetSectionStress(releaseIntervalIdx,lcid,poi,topLocation,botLocation,&fTopRelease,&fBotRelease);

               Float64 fTopStorage, fBotStorage;
               GetSectionStress(storageIntervalIdx,lcid,poi,topLocation,botLocation,&fTopStorage,&fBotStorage);

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

void CSegmentModelManager::GetSectionStress(IntervalIndexType intervalIdx,LoadCaseIDType lcid,const pgsPointOfInterest& poi,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
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

void CSegmentModelManager::GetReaction(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadCaseIDType lcid,ResultsType resultsType,Float64* pRleft,Float64* pRright)
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
            HRESULT hr = results->ComputeReactions(lcid,jntID,&fx,&fy,&mz);
            ATLASSERT(SUCCEEDED(hr));

            reactions.push_back(fy);
         }
      }

      ATLASSERT(reactions.size() == 2);

      *pRleft  = reactions.front();
      *pRright = reactions.back();
   }
}


Float64 CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,LoadCaseIDType lcid,PierIndexType pierIdx,const CGirderKey& girderKey,ResultsType resultsType)
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

Float64 CSegmentModelManager::GetReaction(IntervalIndexType intervalIdx,LoadCaseIDType lcid,PierIndexType pierIdx,const CGirderKey& girderKey)
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
   HRESULT hr = results->ComputeReactions(lcid,jointID,&fx,&fy,&mz);
   ATLASSERT(SUCCEEDED(hr));
   return fy;
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

      CSegmentModelData segmentModel = BuildSegmentModel(segmentKey,releaseIntervalIdx,0.0,0.0,POI_RELEASED_SEGMENT);
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


      CSegmentModelData segmentModel = BuildSegmentModel(segmentKey,storageIntervalIdx,left,right,POI_STORAGE_SEGMENT);
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
   return !::InRange(0.0,poi.GetDistFromStart(),g_L);
}

CSegmentModelData CSegmentModelManager::BuildSegmentModel(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,Float64 leftSupportDistance,Float64 rightSupportDistance,PoiAttributeType refAttribute)
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
   std::vector<pgsPointOfInterest> vPOI( pIPoi->GetPointsOfInterest(segmentKey) ); // we want all POI
   g_L = Ls;
   vPOI.erase(std::remove_if(vPOI.begin(),vPOI.end(),RemovePOI),vPOI.end());

   // Build the Model
   CSegmentModelData model_data;
   model_data.Interval = intervalIdx;
   LoadCaseIDType lcid = GetLoadCaseID(pgsTypes::pftGirder);
   model_data.Loads.insert(lcid);
   pgsGirderModelFactory().CreateGirderModel(m_pBroker,intervalIdx,segmentKey,leftSupportDistance,Ls-rightSupportDistance,Ec,lcid,true,vPOI,&model_data.Model,&model_data.PoiMap);

   // create loadings for all product load types
   // this may seems silly because may of these loads aren't applied
   // to the segment models. however, it is easier to treat everything
   // uniformly then to check for special exceptions
   //AddLoading(model_data,pgsTypes::pftGirder); // taken care of in CreateGirderModel
   AddLoading(model_data,pgsTypes::pftConstruction);
   AddLoading(model_data,pgsTypes::pftSlab);
   AddLoading(model_data,pgsTypes::pftSlabPad);
   AddLoading(model_data,pgsTypes::pftSlabPanel);
   AddLoading(model_data,pgsTypes::pftDiaphragm);
   AddLoading(model_data,pgsTypes::pftSidewalk);
   AddLoading(model_data,pgsTypes::pftTrafficBarrier);
   AddLoading(model_data,pgsTypes::pftShearKey);
   AddLoading(model_data,pgsTypes::pftSecondaryEffects);
   AddLoading(model_data,pgsTypes::pftOverlay);
   AddLoading(model_data,pgsTypes::pftOverlayRating);
   AddLoading(model_data,pgsTypes::pftUserDC);
   AddLoading(model_data,pgsTypes::pftUserDW);
   AddLoading(model_data,pgsTypes::pftUserLLIM);
   AddLoading(model_data,pgsTypes::pftCreep);
   AddLoading(model_data,pgsTypes::pftShrinkage);
   AddLoading(model_data,pgsTypes::pftRelaxation);

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

LoadCaseIDType CSegmentModelManager::GetLoadCaseID(pgsTypes::ProductForceType pfType)
{
   return m_ProductLoadMap.GetLoadCaseID(pfType);
}

LoadCaseIDType CSegmentModelManager::GetLoadCaseID(pgsTypes::StrandType strandType)
{
   return m_ProductLoadMap.GetMaxLoadCaseID() + LoadCaseIDType(strandType);
}

LoadCaseIDType CSegmentModelManager::GetFirstExternalLoadCaseID()
{
   return m_ProductLoadMap.GetMaxLoadCaseID() + 4; // this gets the IDs past the strand IDs
}

LoadCaseIDType CSegmentModelManager::GetLoadCaseID(CSegmentModelData* pModelData,LPCTSTR strLoadingName)
{
   std::map<std::_tstring,LoadCaseIDType>::iterator found(pModelData->ExternalLoadMap.find(strLoadingName));
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
      PoiIDType poiID = pModelData->PoiMap.GetModelPoi(poi);
      if ( poiID == INVALID_ID )
      {
         poiID = AddPointOfInterest(pModelData,poi);
         ATLASSERT(poiID != INVALID_ID);
      }

      CComPtr<IFem2dPOICollection> pois;
      pModelData->Model->get_POIs(&pois);

      CComPtr<IFem2dPOI> femPoi;
      pois->Find(poiID,&femPoi);

      femPoi->get_MemberID(pMbrID);
      femPoi->get_Location(pLocation);
   }
}

CSegmentKey CSegmentModelManager::GetSegmentKey(const CGirderKey& girderKey,PierIndexType pierIdx)
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

void CSegmentModelManager::AddLoading(CSegmentModelData& model_data,pgsTypes::ProductForceType pfType)
{
   CComPtr<IFem2dLoadingCollection> loadings;
   model_data.Model->get_Loadings(&loadings);
   CComPtr<IFem2dLoading> loading;
   loadings->Create(GetLoadCaseID(pfType),&loading);
}


bool CSegmentModelManager::CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   CSegmentModelData* pModelData = GetSegmentModel(poi.GetSegmentKey(),intervalIdx);

   LoadCaseIDType lcid;
   if ( strLoadingName )
   {
      lcid = GetLoadCaseID(pModelData,strLoadingName);
   }
   else
   {
      lcid = GetLoadCaseID(pfType);
   }

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

bool CSegmentModelManager::CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
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

      // get the member strain loading collection for this model
      LoadCaseIDType lcid;
      if ( strLoadingName )
      {
         lcid = GetLoadCaseID(pModelData,strLoadingName);
      }
      else
      {
         lcid = GetLoadCaseID(pfType);
      }

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

bool CSegmentModelManager::CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
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

      // get the member strain loading collection for this model
      LoadCaseIDType lcid;
      if ( strLoadingName )
      {
         lcid = GetLoadCaseID(pModelData,strLoadingName);
      }
      else
      {
         lcid = GetLoadCaseID(pfType);
      }

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
                  CComPtr<IFem2dMemberStrain> strainLoad;
                  memberStrains->Create(loadID++,mbrID1,location1,length1,e,r,&strainLoad);
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
